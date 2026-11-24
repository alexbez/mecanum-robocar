#include "Motor.h"

#include <Arduino.h>

Motor::Motor(const MotorPins &pins, uint8_t ledcChannel, bool inverted, uint32_t pwmFreqHz,
             uint8_t resolutionBits)
    : pins_(pins),
      channel_(ledcChannel),
      inverted_(inverted),
      freqHz_(pwmFreqHz),
      bits_(resolutionBits),
      maxValue_((1u << resolutionBits) - 1u) {}

void Motor::begin() {
  pinMode(pins_.in1, OUTPUT);
  pinMode(pins_.in2, OUTPUT);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttachChannel(pins_.pwm, freqHz_, bits_, channel_);
#else
  ledcSetup(channel_, freqHz_, bits_);
  ledcAttachPin(pins_.pwm, channel_);
#endif
  coast();
}

void Motor::writePwm(uint32_t value) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pins_.pwm, value);
#else
  ledcWrite(channel_, value);
#endif
}

void Motor::setDuty(float duty) {
  if (duty > 1.0f) duty = 1.0f;
  if (duty < -1.0f) duty = -1.0f;
  duty_ = duty;

  float d = inverted_ ? -duty : duty;
  if (d == 0.0f) {
    brake();
    duty_ = 0.0f;
    return;
  }
  const bool forward = d > 0.0f;
  digitalWrite(pins_.in1, forward ? HIGH : LOW);
  digitalWrite(pins_.in2, forward ? LOW : HIGH);
  writePwm(static_cast<uint32_t>(fabsf(d) * maxValue_ + 0.5f));
}

void Motor::brake() {
  duty_ = 0.0f;
  digitalWrite(pins_.in1, HIGH);
  digitalWrite(pins_.in2, HIGH);
  writePwm(maxValue_);
}

void Motor::coast() {
  duty_ = 0.0f;
  digitalWrite(pins_.in1, LOW);
  digitalWrite(pins_.in2, LOW);
  writePwm(0);
}
