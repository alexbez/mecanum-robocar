#include "RoboCar.h"

#include <Arduino.h>
#include <EspNowTransport.h>
#include <string.h>

#include "CarConfig.h"

using proto::CarState;

namespace {
int16_t toI16(float v) {
  if (v > 32767.0f) return 32767;
  if (v < -32768.0f) return -32768;
  return static_cast<int16_t>(lroundf(v));
}
}  // namespace

RoboCar::RoboCar()
    : led_(cfg::kLedPin, cfg::kLedBrightness),
      battery_(cfg::kBatteryPin, cfg::kBatteryDivider, cfg::kBatteryCalibration) {}

void RoboCar::begin() {
  Serial.begin(115200);
  led_.begin();
  led_.setState(CarState::Booting);
  led_.update(millis());

  if (!drive_.begin()) Serial.println("[car] encoder (PCNT) init failed");
  drive_.setEnabled(false);
  battery_.begin();

  if (!link_.begin()) {
    Serial.println("[car] ESP-NOW init failed");
  }

  uint8_t mac[6];
  EspNowTransport::instance().localMac(mac);
  Serial.printf("[car] ready, MAC %02X:%02X:%02X:%02X:%02X:%02X, channel %u\n", mac[0], mac[1],
                mac[2], mac[3], mac[4], mac[5], proto::kWifiChannel);

  state_ = CarState::WaitingForRemote;
  lastControlUs_ = micros();
}

void RoboCar::loop() {
  const uint32_t nowUs = micros();
  if (nowUs - lastControlUs_ >= cfg::kControlPeriodMs * 1000u) {
    const float dt = (nowUs - lastControlUs_) * 1e-6f;
    lastControlUs_ = nowUs;
    controlStep(dt);
  }

  const uint32_t now = millis();
  if (now - lastBatteryMs_ >= cfg::kBatteryPeriodMs) {
    lastBatteryMs_ = now;
    battery_.update();
  }
  if (now - lastTelemetryMs_ >= cfg::kTelemetryPeriodMs) {
    lastTelemetryMs_ = now;
    sendTelemetry(now);
  }

  link_.update(now);
  led_.setState(state_);
  led_.setBatteryWarning(battery_.present() && battery_.millivolts() < cfg::kBatteryWarnMv);
  led_.update(now);
  log(now);
}

CarState RoboCar::evaluateState(uint32_t now, const RemoteCommand &cmd, bool hasCmd) {
  const bool battPresent = battery_.present();
  const uint16_t mv = battery_.millivolts();
  if (battPresent && mv < cfg::kBatteryCriticalMv) lowBatteryLatched_ = true;
  // Hysteresis: voltage recovers when motors stop, so require the warn level.
  if (lowBatteryLatched_ && (!battPresent || mv >= cfg::kBatteryWarnMv)) lowBatteryLatched_ = false;

  const bool linkAlive = hasCmd && (now - cmd.receivedAtMs) <= cfg::kFailsafeMs;

  // A "disarmed" packet acknowledges any previous trip and clears faults.
  if (linkAlive && !cmd.armed) {
    needDisarm_ = false;
    drive_.clearFaults();
  }

  if (lowBatteryLatched_) {
    needDisarm_ = true;
    return CarState::LowBattery;
  }
  if (drive_.faultMask() != 0) {
    needDisarm_ = true;
    return CarState::Fault;
  }
  if (!hasCmd) return CarState::WaitingForRemote;
  if (!linkAlive) {
    needDisarm_ = true;
    return CarState::LinkLost;
  }
  if (!cmd.armed || needDisarm_) return CarState::Disarmed;
  return drive_.isMoving() ? CarState::Driving : CarState::Armed;
}

void RoboCar::controlStep(float dt) {
  RemoteCommand cmd;
  const bool hasCmd = link_.latest(cmd);
  const uint32_t now = millis();  // read after latest() so receivedAtMs <= now

  state_ = evaluateState(now, cmd, hasCmd);
  const bool enable = state_ == CarState::Armed || state_ == CarState::Driving;

  drive_.setEnabled(enable);
  if (enable) {
    const float k = cmd.speedLimit;
    drive_.command(cmd.vx * k, cmd.vy * k, cmd.wz * k);
  } else {
    drive_.stop();
  }
  drive_.update(dt);
}

void RoboCar::sendTelemetry(uint32_t now) {
  proto::TelemetryPacket t;
  memset(&t, 0, sizeof(t));
  t.state = static_cast<uint8_t>(state_);
  if (battery_.present() && battery_.millivolts() < cfg::kBatteryWarnMv) t.flags |= proto::kTelBatteryWarn;
  if (cfg::kClosedLoop) t.flags |= proto::kTelClosedLoop;
  if (needDisarm_) t.flags |= proto::kTelNeedDisarm;
  t.faultMask = drive_.faultMask();
  t.controlRateHz = link_.controlRateHz();
  t.batteryMv = battery_.millivolts();
  for (uint8_t i = 0; i < proto::kWheelCount; ++i) {
    t.wheelRpmX10[i] = toI16(drive_.wheelRpm(i) * 10.0f);
    t.wheelTargetX10[i] = toI16(drive_.wheelTargetRpm(i) * 10.0f);
    t.wheelDutyPct[i] = static_cast<int8_t>(lroundf(drive_.wheelDuty(i) * 100.0f));
  }
  const BodyVelocity v = drive_.bodyVelocity();
  t.vxMmS = toI16(v.vx);
  t.vyMmS = toI16(v.vy);
  t.wzMradS = toI16(v.wz * 1000.0f);
  t.odometerMm = drive_.odometerMm();
  t.uptimeS = now / 1000;
  link_.sendTelemetry(t);
}

void RoboCar::log(uint32_t now) {
  if (now - lastLogMs_ < cfg::kLogPeriodMs) return;
  lastLogMs_ = now;
  Serial.printf("[car] %-9s batt=%5umV rx=%2uHz rpm FL=%6.1f FR=%6.1f RL=%6.1f RR=%6.1f faults=0x%X\n",
                proto::carStateName(state_), battery_.millivolts(), link_.controlRateHz(),
                drive_.wheelRpm(0), drive_.wheelRpm(1), drive_.wheelRpm(2), drive_.wheelRpm(3),
                drive_.faultMask());
}
