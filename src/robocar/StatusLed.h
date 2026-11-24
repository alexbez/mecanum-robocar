#pragma once
// Onboard WS2812 RGB LED showing the car state.
//
//   BOOTING      white, solid
//   WAITING      blue, breathing          (no remote seen yet)
//   LINK LOST    red, fast blink          (failsafe, motors braked)
//   DISARMED     yellow, slow pulse
//   ARMED        green, solid
//   DRIVING      cyan, solid
//   LOW BATT     orange, 1 Hz blink       (motors disabled)
//   FAULT        magenta, double blink    (wheel stall / encoder fault)
//   + battery warning: short orange flash every 2 s on top of ARMED/DRIVING/DISARMED

#include <Adafruit_NeoPixel.h>
#include <RoboProtocol.h>
#include <stdint.h>

class StatusLed {
 public:
  StatusLed(uint8_t pin, uint8_t brightness);

  void begin();
  void setState(proto::CarState state) { state_ = state; }
  void setBatteryWarning(bool warn) { batteryWarn_ = warn; }

  // Call often (every loop); only writes the LED when the colour changes.
  void update(uint32_t nowMs);

 private:
  struct Rgb {
    uint8_t r, g, b;
  };
  Rgb colourFor(uint32_t nowMs) const;
  static Rgb scale(Rgb c, float k);

  Adafruit_NeoPixel pixel_;
  uint8_t brightness_;
  proto::CarState state_ = proto::CarState::Booting;
  bool batteryWarn_ = false;
  uint32_t lastColour_ = 0xFFFFFFFF;
};
