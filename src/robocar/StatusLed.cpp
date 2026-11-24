#include "StatusLed.h"

#include <math.h>

using proto::CarState;

StatusLed::StatusLed(uint8_t pin, uint8_t brightness)
    : pixel_(1, pin, NEO_GRB + NEO_KHZ800), brightness_(brightness) {}

void StatusLed::begin() {
  pixel_.begin();
  pixel_.setBrightness(brightness_);
  pixel_.clear();
  pixel_.show();
}

StatusLed::Rgb StatusLed::scale(Rgb c, float k) {
  return {static_cast<uint8_t>(c.r * k), static_cast<uint8_t>(c.g * k), static_cast<uint8_t>(c.b * k)};
}

StatusLed::Rgb StatusLed::colourFor(uint32_t now) const {
  const Rgb off = {0, 0, 0};
  const Rgb white = {255, 255, 255};
  const Rgb blue = {0, 0, 255};
  const Rgb red = {255, 0, 0};
  const Rgb yellow = {255, 160, 0};
  const Rgb green = {0, 255, 0};
  const Rgb cyan = {0, 220, 255};
  const Rgb orange = {255, 70, 0};
  const Rgb magenta = {255, 0, 200};

  // Battery warning overlay: 150 ms orange flash every 2 s.
  if (batteryWarn_ && (now % 2000) < 150 &&
      (state_ == CarState::Armed || state_ == CarState::Driving || state_ == CarState::Disarmed)) {
    return orange;
  }

  switch (state_) {
    case CarState::Booting:
      return white;
    case CarState::WaitingForRemote: {
      const float phase = (now % 2000) / 2000.0f;
      const float k = 0.08f + 0.92f * (0.5f - 0.5f * cosf(phase * 2.0f * static_cast<float>(M_PI)));
      return scale(blue, k);
    }
    case CarState::LinkLost:
      return (now % 250) < 125 ? red : off;
    case CarState::Disarmed: {
      const float phase = (now % 3000) / 3000.0f;
      const float k = 0.3f + 0.7f * (0.5f - 0.5f * cosf(phase * 2.0f * static_cast<float>(M_PI)));
      return scale(yellow, k);
    }
    case CarState::Armed:
      return green;
    case CarState::Driving:
      return cyan;
    case CarState::LowBattery:
      return (now % 1000) < 500 ? orange : off;
    case CarState::Fault: {
      const uint32_t t = now % 1200;
      return (t < 120 || (t >= 240 && t < 360)) ? magenta : off;
    }
  }
  return off;
}

void StatusLed::update(uint32_t nowMs) {
  const Rgb c = colourFor(nowMs);
  const uint32_t packed = Adafruit_NeoPixel::Color(c.r, c.g, c.b);
  if (packed == lastColour_) return;
  lastColour_ = packed;
  pixel_.setPixelColor(0, packed);
  pixel_.show();
}
