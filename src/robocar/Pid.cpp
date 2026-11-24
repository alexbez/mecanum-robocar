#include "Pid.h"

namespace {
float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
}  // namespace

Pid::Pid(const Gains &gains, float outMin, float outMax)
    : gains_(gains), outMin_(outMin), outMax_(outMax) {}

void Pid::reset() {
  integral_ = 0.0f;
  lastMeasured_ = 0.0f;
  first_ = true;
}

float Pid::update(float setpoint, float measured, float dt) {
  if (dt <= 0.0f) return 0.0f;

  const float error = setpoint - measured;
  const float derivative = first_ ? 0.0f : -(measured - lastMeasured_) / dt;
  lastMeasured_ = measured;
  first_ = false;

  const float p = gains_.kp * error;
  const float d = gains_.kd * derivative;

  // Integrate, then clamp so P + I + D stays inside the output range.
  integral_ += gains_.ki * error * dt;
  integral_ = clampf(integral_, outMin_ - p - d, outMax_ - p - d);
  integral_ = clampf(integral_, outMin_, outMax_);

  return clampf(p + integral_ + d, outMin_, outMax_);
}
