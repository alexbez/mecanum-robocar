#pragma once
// Mecanum wheel kinematics (rollers in "X" pattern seen from above,
// i.e. the classic O-configuration when viewed from below).
//
// Body frame: x forward, y left, z up (counter-clockwise positive).
// Wheel order: FL, FR, RL, RR.

struct BodyVelocity {
  float vx;  // mm/s
  float vy;  // mm/s
  float wz;  // rad/s
};

class MecanumKinematics {
 public:
  MecanumKinematics(float wheelDiameterMm, float halfWheelbaseMm, float halfTrackMm);

  // Normalised command (-1..1 each axis) -> normalised wheel speeds (-1..1).
  // Wheel speeds are scaled down together if any exceeds 1.
  static void mixNormalized(float vx, float vy, float wz, float out[4]);

  // Wheel speeds (rpm) -> body velocity (forward kinematics, for telemetry).
  BodyVelocity forward(const float wheelRpm[4]) const;

  float wheelCircumferenceMm() const;

 private:
  float radiusMm_;
  float lxPlusLy_;
};
