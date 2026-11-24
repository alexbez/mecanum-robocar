#include "Button.h"

#include <Arduino.h>

Button::Button(uint8_t pin, uint32_t debounceMs) : pin_(pin), debounceMs_(debounceMs) {}

void Button::begin() {
  pinMode(pin_, INPUT_PULLUP);
  stable_ = lastRaw_ = digitalRead(pin_) == LOW;
  changedAtMs_ = millis();
}

void Button::update(uint32_t nowMs) {
  const bool raw = digitalRead(pin_) == LOW;
  if (raw != lastRaw_) {
    lastRaw_ = raw;
    changedAtMs_ = nowMs;
  }
  if (raw != stable_ && nowMs - changedAtMs_ >= debounceMs_) {
    stable_ = raw;
    if (stable_) pressedEvent_ = true;
  }
}

bool Button::wasPressed() {
  const bool e = pressedEvent_;
  pressedEvent_ = false;
  return e;
}
