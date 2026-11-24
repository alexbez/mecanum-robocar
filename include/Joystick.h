#pragma once
// Two-axis analog joystick (potentiometers) with centre calibration,
// dead zone and expo curve.

#include <stdint.h>

class Joystick {
 public:
  Joystick(uint8_t pinX, uint8_t pinY, bool invertX, bool invertY, float deadzone, float expo);

  // Measures the resting position. Keep the stick released during boot.
  void begin();

  // Samples both axes. Call every loop.
  void update();

  float x() const { return x_; }  // -1 (left) .. 1 (right)
  float y() const { return y_; }  // -1 (back) .. 1 (forward)

  bool centered(float tolerance = 0.05f) const;

  uint16_t rawX() const { return rawX_; }
  uint16_t rawY() const { return rawY_; }

 private:
  static uint16_t readAveraged(uint8_t pin);
  float shape(uint16_t raw, uint16_t centre, bool invert) const;

  uint8_t pinX_, pinY_;
  bool invertX_, invertY_;
  float deadzone_, expo_;
  uint16_t centreX_ = 2048, centreY_ = 2048;
  uint16_t rawX_ = 2048, rawY_ = 2048;
  float x_ = 0.0f, y_ = 0.0f;
};
