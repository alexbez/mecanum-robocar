#pragma once
// One TB6612FNG channel (PWMx, xIN1, xIN2) driving a DC motor.

#include <stdint.h>

struct MotorPins {
  uint8_t pwm;
  uint8_t in1;
  uint8_t in2;
};

class Motor {
 public:
  Motor(const MotorPins &pins, uint8_t ledcChannel, bool inverted, uint32_t pwmFreqHz,
        uint8_t resolutionBits);

  void begin();

  // duty -1..1. Sign selects direction; PWM off-time is short-brake (slow decay).
  void setDuty(float duty);

  // Both inputs high: motor terminals shorted, wheel stops quickly.
  void brake();

  // Both inputs low: outputs high-Z, wheel freewheels.
  void coast();

  float duty() const { return duty_; }

 private:
  void writePwm(uint32_t value);

  MotorPins pins_;
  uint8_t channel_;
  bool inverted_;
  uint32_t freqHz_;
  uint8_t bits_;
  uint32_t maxValue_;
  float duty_ = 0.0f;
};
