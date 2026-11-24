#pragma once
// Four-wheel mecanum drivetrain: kinematics + four speed-controlled wheels
// + TB6612FNG standby control + odometry.

#include <stdint.h>

#include "MecanumKinematics.h"
#include "WheelController.h"

class MecanumDrive {
 public:
  MecanumDrive();

  bool begin();

  // Enables/disables both TB6612FNG drivers (STBY pin). Disabled = coasting.
  void setEnabled(bool enabled);
  bool enabled() const { return enabled_; }

  // Normalised command, each axis -1..1 (already multiplied by speed limit).
  void command(float vx, float vy, float wz);
  void stop() { command(0.0f, 0.0f, 0.0f); }

  // Runs one control step for all wheels. dt in seconds.
  void update(float dt);

  uint8_t faultMask() const;
  void clearFaults();

  bool isMoving() const;

  float wheelRpm(uint8_t i) const { return wheels_[i].measuredRpm(); }
  float wheelTargetRpm(uint8_t i) const { return wheels_[i].rampedTargetRpm(); }
  float wheelDuty(uint8_t i) const { return wheels_[i].duty(); }

  BodyVelocity bodyVelocity() const { return body_; }
  uint32_t odometerMm() const { return static_cast<uint32_t>(odometerMm_); }

 private:
  WheelController wheels_[4];
  MecanumKinematics kinematics_;
  bool enabled_ = false;
  BodyVelocity body_ = {0.0f, 0.0f, 0.0f};
  double odometerMm_ = 0.0;
};
