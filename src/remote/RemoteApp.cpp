#include "RemoteApp.h"

#include <Arduino.h>
#include <EspNowTransport.h>

#include "RemoteConfig.h"

using proto::CarState;

RemoteApp::RemoteApp()
    : joystick_(rcfg::kJoyXPin, rcfg::kJoyYPin, rcfg::kJoyInvertX, rcfg::kJoyInvertY,
                rcfg::kJoyDeadzone, rcfg::kJoyExpo),
      joyButton_(rcfg::kJoySwPin),
      armButton_(rcfg::kArmButtonPin),
      gearButton_(rcfg::kGearButtonPin),
      battery_(rcfg::kBatteryPin, rcfg::kBatteryDivider),
      gear_(rcfg::kDefaultGear) {}

void RemoteApp::begin() {
  Serial.begin(115200);
  display_.begin(rcfg::kLcdPowerPin);
  display_.splash("MECANUM REMOTE", "release the joystick...");

  joystick_.begin();  // calibrates the centre position
  joyButton_.begin();
  armButton_.begin();
  gearButton_.begin();
  battery_.begin();

  if (!link_.begin()) {
    display_.splash("RADIO ERROR", "ESP-NOW init failed");
    Serial.println("[remote] ESP-NOW init failed");
    delay(3000);
  }

  uint8_t mac[6];
  EspNowTransport::instance().localMac(mac);
  Serial.printf("[remote] ready, MAC %02X:%02X:%02X:%02X:%02X:%02X, channel %u\n", mac[0], mac[1],
                mac[2], mac[3], mac[4], mac[5], proto::kWifiChannel);
}

bool RemoteApp::telemetryFresh(uint32_t now) const {
  return haveTel_ && telAtMs_ <= now && (now - telAtMs_) <= rcfg::kTelemetryTimeoutMs;
}

void RemoteApp::showMessage(const char *msg, uint32_t now) {
  message_ = msg;
  messageAtMs_ = now;
}

void RemoteApp::handleButtons(uint32_t now) {
  joyButton_.update(now);
  armButton_.update(now);
  gearButton_.update(now);

  if (joyButton_.wasPressed()) {
    mode_ = (mode_ == DriveMode::Drive) ? DriveMode::Strafe : DriveMode::Drive;
    showMessage(mode_ == DriveMode::Drive ? "DRIVE: stick X turns" : "STRAFE: stick X slides", now);
  }

  if (gearButton_.wasPressed()) {
    gear_ = (gear_ + 1) % rcfg::kGearCount;
  }

  if (armButton_.wasPressed()) {
    if (armed_) {
      armed_ = false;
      showMessage("DISARMED", now);
    } else if (!telemetryFresh(now)) {
      showMessage("NO LINK TO CAR", now);
    } else if (!joystick_.centered()) {
      showMessage("CENTRE THE STICK", now);
    } else {
      const auto s = static_cast<CarState>(tel_.state);
      if (s == CarState::LowBattery) {
        showMessage("CAR BATTERY LOW", now);
      } else {
        armed_ = true;
        armedAtMs_ = now;
        showMessage("ARMED", now);
      }
    }
  }
}

void RemoteApp::supervise(uint32_t now) {
  // Pull latest telemetry.
  uint32_t at = 0;
  if (link_.latestTelemetry(tel_, at)) {
    haveTel_ = true;
    telAtMs_ = at;
  }

  if (!armed_) return;

  // Disarm on link loss, and when the car tripped and waits for a disarm.
  if (!telemetryFresh(now)) {
    armed_ = false;
    showMessage("LINK LOST - DISARMED", now);
    return;
  }
  if (now - armedAtMs_ < rcfg::kArmGraceMs || telAtMs_ < armedAtMs_) return;

  const auto s = static_cast<CarState>(tel_.state);
  if (s == CarState::Fault) {
    armed_ = false;
    showMessage("WHEEL STALL - DISARMED", now);
  } else if (s == CarState::LowBattery) {
    armed_ = false;
    showMessage("LOW BATTERY - DISARMED", now);
  } else if (s == CarState::Disarmed && (tel_.flags & proto::kTelNeedDisarm)) {
    armed_ = false;
    showMessage("CAR NOT READY - REARM", now);
  }
}

void RemoteApp::sendControl() {
  const float jx = joystick_.x();
  const float jy = joystick_.y();
  float vx = jy, vy = 0.0f, wz = 0.0f;
  if (mode_ == DriveMode::Drive) {
    wz = -jx;  // stick right -> turn clockwise (negative yaw)
  } else {
    vy = -jx;  // stick right -> move right (negative y)
  }
  if (!armed_) vx = vy = wz = 0.0f;
  link_.sendControl(vx, vy, wz, rcfg::kGears[gear_], armed_);
}

void RemoteApp::render(uint32_t now) {
  DisplayModel m;
  m.haveTelemetry = haveTel_;
  m.telemetryFresh = telemetryFresh(now);
  m.tel = tel_;
  m.telemetryRateHz = link_.telemetryRateHz();
  m.armed = armed_;
  m.mode = mode_;
  m.speedPct = rcfg::kGears[gear_];
  m.joyX = joystick_.x();
  m.joyY = joystick_.y();
  m.remoteBatteryMv = battery_.millivolts();
  if (message_ && now - messageAtMs_ < rcfg::kMessageMs) {
    m.message = message_;
  } else {
    message_ = nullptr;
  }
  display_.render(m);
}

void RemoteApp::loop() {
  const uint32_t now = millis();

  joystick_.update();
  handleButtons(now);
  link_.update(now);
  supervise(now);

  if (now - lastControlMs_ >= rcfg::kControlPeriodMs) {
    lastControlMs_ = now;
    sendControl();
  }
  if (now - lastBatteryMs_ >= 200) {
    lastBatteryMs_ = now;
    battery_.update();
  }
  if (now - lastDisplayMs_ >= rcfg::kDisplayPeriodMs) {
    lastDisplayMs_ = now;
    render(now);
  }
}
