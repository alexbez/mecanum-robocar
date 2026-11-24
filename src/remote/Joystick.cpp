#include "Joystick.h"

#include <Arduino.h>
#include <math.h>

namespace {
constexpr float kAdcMax = 4095.0f;
constexpr float kEndGain = 1.08f;  // pots rarely reach the rails: reach 1.0 a bit early
}  // namespace

Joystick::Joystick(uint8_t pinX, uint8_t pinY, bool invertX, bool invertY, float deadzone, float expo)
    : pinX_(pinX), pinY_(pinY), invertX_(invertX), invertY_(invertY), deadzone_(deadzone), expo_(expo) {}

uint16_t Joystick::readAveraged(uint8_t pin) {
  uint32_t sum = 0;
  for (int i = 0; i < 4; ++i) sum += analogRead(pin);
  return static_cast<uint16_t>(sum / 4);
}

void Joystick::begin() {
  analogReadResolution(12);
  analogSetPinAttenuation(pinX_, ADC_11db);
  analogSetPinAttenuation(pinY_, ADC_11db);

  uint32_t sx = 0, sy = 0;
  constexpr int kSamples = 64;
  for (int i = 0; i < kSamples; ++i) {
    sx += analogRead(pinX_);
    sy += analogRead(pinY_);
    delay(2);
  }
  centreX_ = static_cast<uint16_t>(sx / kSamples);
  centreY_ = static_cast<uint16_t>(sy / kSamples);

  // A centre far from mid-scale means the stick was held or is miswired.
  if (centreX_ < 1000 || centreX_ > 3100) centreX_ = 2048;
  if (centreY_ < 1000 || centreY_ > 3100) centreY_ = 2048;
}

float Joystick::shape(uint16_t raw, uint16_t centre, bool invert) const {
  float v;
  if (raw >= centre) {
    v = (raw - centre) / (kAdcMax - centre);
  } else {
    v = -static_cast<float>(centre - raw) / static_cast<float>(centre);
  }
  v *= kEndGain;
  if (v > 1.0f) v = 1.0f;
  if (v < -1.0f) v = -1.0f;

  // Dead zone, then rescale so output starts at 0 just outside it.
  const float a = fabsf(v);
  if (a < deadzone_) return 0.0f;
  float s = (a - deadzone_) / (1.0f - deadzone_);
  s = (1.0f - expo_) * s + expo_ * s * s * s;
  v = v > 0.0f ? s : -s;
  return invert ? -v : v;
}

void Joystick::update() {
  rawX_ = readAveraged(pinX_);
  rawY_ = readAveraged(pinY_);
  x_ = shape(rawX_, centreX_, invertX_);
  y_ = shape(rawY_, centreY_, invertY_);
}

bool Joystick::centered(float tolerance) const { return fabsf(x_) <= tolerance && fabsf(y_) <= tolerance; }
