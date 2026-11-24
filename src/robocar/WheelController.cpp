#include "WheelController.h"

#include <Arduino.h>

WheelController::WheelController(const cfg::WheelHardware &hw)
    : motor_(hw.motor, hw.ledcChannel, hw.motorInverted, cfg::kPwmFrequencyHz,
             cfg::kPwmResolutionBits),
      encoder_(hw.encA, hw.encB, hw.pcntUnit, hw.encoderInverted),
      pid_({cfg::kPidKp, cfg::kPidKi, cfg::kPidKd}, -1.0f, 1.0f) {}

bool WheelController::begin() {
  motor_.begin();
  const bool ok = encoder_.begin();
  count_ = encoder_.read();
  return ok;
}

float WheelController::feedForward(float rpm) const {
  if (fabsf(rpm) < 0.5f) return 0.0f;
  const float ff = rpm / cfg::kRpmAtFullDuty;
  return ff + (rpm > 0.0f ? cfg::kMinDuty : -cfg::kMinDuty);
}

void WheelController::resetFault() {
  fault_ = false;
  stallMs_ = 0;
}

void WheelController::update(float dt, bool enabled) {
  // --- measure ---
  const int32_t now = encoder_.read();
  const int32_t delta = now - count_;
  count_ = now;
  const float rawRpm = (dt > 0.0f) ? (delta / cfg::kCountsPerWheelRev) / dt * 60.0f : 0.0f;
  rpm_ += cfg::kRpmFilterAlpha * (rawRpm - rpm_);

  if (!enabled || fault_) {
    ramped_ = 0.0f;
    pid_.reset();
    motor_.brake();
    return;
  }

  // --- ramp the target (acceleration limit) ---
  const float maxStep = cfg::kMaxAccelRpmPerS * dt;
  const float diff = target_ - ramped_;
  ramped_ += (diff > maxStep) ? maxStep : (diff < -maxStep ? -maxStep : diff);

  // --- control ---
  float duty = feedForward(ramped_);
  if (cfg::kClosedLoop) {
    duty += pid_.update(ramped_, rpm_, dt);
  }
  if (fabsf(ramped_) < 0.5f && fabsf(target_) < 0.5f) {
    // Standing still: hold with brake, avoid PID hunting around zero.
    duty = 0.0f;
    pid_.reset();
  }
  motor_.setDuty(duty);

  // --- stall / encoder fault detection ---
  if (cfg::kClosedLoop && fabsf(motor_.duty()) > cfg::kStallDuty && fabsf(rpm_) < cfg::kStallRpm) {
    stallMs_ += static_cast<uint32_t>(dt * 1000.0f);
    if (stallMs_ >= cfg::kStallTimeMs) {
      fault_ = true;
      motor_.coast();
    }
  } else {
    stallMs_ = 0;
  }
}
