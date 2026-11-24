#include "MecanumDrive.h"

#include <Arduino.h>

#include "CarConfig.h"

MecanumDrive::MecanumDrive()
    : wheels_{WheelController(cfg::kWheels[0]), WheelController(cfg::kWheels[1]),
              WheelController(cfg::kWheels[2]), WheelController(cfg::kWheels[3])},
      kinematics_(cfg::kWheelDiameterMm, cfg::kHalfWheelbaseMm, cfg::kHalfTrackMm) {}

bool MecanumDrive::begin() {
  pinMode(cfg::kStbyPin, OUTPUT);
  digitalWrite(cfg::kStbyPin, LOW);
  bool ok = true;
  for (auto &w : wheels_) ok &= w.begin();
  return ok;
}

void MecanumDrive::setEnabled(bool enabled) {
  if (enabled == enabled_) return;
  enabled_ = enabled;
  if (!enabled_) stop();
  digitalWrite(cfg::kStbyPin, enabled_ ? HIGH : LOW);
}

void MecanumDrive::command(float vx, float vy, float wz) {
  float mix[4];
  MecanumKinematics::mixNormalized(vx, vy, wz * cfg::kTurnGain, mix);
  for (int i = 0; i < 4; ++i) wheels_[i].setTargetRpm(mix[i] * cfg::kMaxWheelRpm);
}

void MecanumDrive::update(float dt) {
  float rpm[4];
  for (int i = 0; i < 4; ++i) {
    wheels_[i].update(dt, enabled_);
    rpm[i] = wheels_[i].measuredRpm();
  }
  body_ = kinematics_.forward(rpm);
  odometerMm_ += sqrt(body_.vx * body_.vx + body_.vy * body_.vy) * dt;
}

uint8_t MecanumDrive::faultMask() const {
  uint8_t mask = 0;
  for (int i = 0; i < 4; ++i) {
    if (wheels_[i].fault()) mask |= static_cast<uint8_t>(1u << i);
  }
  return mask;
}

void MecanumDrive::clearFaults() {
  for (auto &w : wheels_) w.resetFault();
}

bool MecanumDrive::isMoving() const {
  for (const auto &w : wheels_) {
    if (fabsf(w.targetRpm()) > 0.5f || fabsf(w.measuredRpm()) > 2.0f) return true;
  }
  return false;
}
