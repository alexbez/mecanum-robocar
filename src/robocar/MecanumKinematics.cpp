#include "MecanumKinematics.h"

#include <math.h>

MecanumKinematics::MecanumKinematics(float wheelDiameterMm, float halfWheelbaseMm, float halfTrackMm)
    : radiusMm_(wheelDiameterMm * 0.5f), lxPlusLy_(halfWheelbaseMm + halfTrackMm) {}

void MecanumKinematics::mixNormalized(float vx, float vy, float wz, float out[4]) {
  out[0] = vx - vy - wz;  // FL
  out[1] = vx + vy + wz;  // FR
  out[2] = vx + vy - wz;  // RL
  out[3] = vx - vy + wz;  // RR

  float maxAbs = 1.0f;
  for (int i = 0; i < 4; ++i) maxAbs = fmaxf(maxAbs, fabsf(out[i]));
  for (int i = 0; i < 4; ++i) out[i] /= maxAbs;
}

BodyVelocity MecanumKinematics::forward(const float wheelRpm[4]) const {
  // rpm -> linear rim speed (mm/s)
  const float k = 2.0f * static_cast<float>(M_PI) * radiusMm_ / 60.0f;
  const float fl = wheelRpm[0] * k;
  const float fr = wheelRpm[1] * k;
  const float rl = wheelRpm[2] * k;
  const float rr = wheelRpm[3] * k;

  BodyVelocity v;
  v.vx = (fl + fr + rl + rr) * 0.25f;
  v.vy = (-fl + fr + rl - rr) * 0.25f;
  v.wz = (-fl + fr - rl + rr) / (4.0f * lxPlusLy_);
  return v;
}

float MecanumKinematics::wheelCircumferenceMm() const {
  return 2.0f * static_cast<float>(M_PI) * radiusMm_;
}
