#pragma once
// Debounced push button (active low, internal pull-up).

#include <stdint.h>

class Button {
 public:
  explicit Button(uint8_t pin, uint32_t debounceMs = 25);

  void begin();
  void update(uint32_t nowMs);

  bool isDown() const { return stable_; }

  // True once per press (on the press edge).
  bool wasPressed();

 private:
  uint8_t pin_;
  uint32_t debounceMs_;
  bool stable_ = false;
  bool lastRaw_ = false;
  uint32_t changedAtMs_ = 0;
  bool pressedEvent_ = false;
};
