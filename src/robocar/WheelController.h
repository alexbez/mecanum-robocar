#pragma once
// Closed-loop speed control of one wheel: motor + encoder + PID.
//
// duty = feed-forward(target) + PID(target - measured), with a ramped target
// (acceleration limit) and stall detection.

#include <stdint.h>

#include "CarConfig.h"
#include "Encoder.h"
#include "Motor.h"
#include "Pid.h"

class WheelController {
 public:
  explicit WheelController(const cfg::WheelHardware &hw);

  bool begin();

  void setTargetRpm(float rpm) { target_ = rpm; }

  // Runs one control step. When `enabled` is false the motor is braked and
  // the controller state is reset (encoder keeps counting).
  void update(float dt, bool enabled);

  void resetFault();

  float targetRpm() const { return target_; }
  float rampedTargetRpm() const { return ramped_; }
  float measuredRpm() const { return rpm_; }
  float duty() const { return motor_.duty(); }
  int32_t count() const { return count_; }
  bool fault() const { return fault_; }

 private:
  float feedForward(float rpm) const;

  Motor motor_;
  Encoder encoder_;
  Pid pid_;

  float target_ = 0.0f;
  float ramped_ = 0.0f;
  float rpm_ = 0.0f;
  int32_t count_ = 0;
  uint32_t stallMs_ = 0;
  bool fault_ = false;
};
