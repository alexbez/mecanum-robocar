#pragma once
// Remote controller configuration (LilyGo T-Display-S3).
//
// Free header pins on the T-Display-S3: 1, 2, 3, 10, 11, 12, 13 and
// 16, 17, 18, 21, 43, 44. The joystick axes must be on ADC1 (GPIO 1..10)
// because ADC2 is unavailable while Wi-Fi/ESP-NOW is running.
// Power the joystick from 3V3, NOT 5 V.

#include <stdint.h>

namespace rcfg {

// ------------------------------------------------------------- Joystick ----
constexpr uint8_t kJoyXPin = 1;   // VRx
constexpr uint8_t kJoyYPin = 2;   // VRy
constexpr uint8_t kJoySwPin = 21; // SW (active low, internal pull-up)
constexpr bool kJoyInvertX = false;  // set so that stick right = +X
constexpr bool kJoyInvertY = true;   // set so that stick forward = +Y
constexpr float kJoyDeadzone = 0.08f;  // fraction of full travel
constexpr float kJoyExpo = 0.35f;      // 0 = linear, 1 = fully cubic

// -------------------------------------------------------- Board buttons ----
constexpr uint8_t kArmButtonPin = 0;    // BOOT button (left of USB-C): arm / disarm
constexpr uint8_t kGearButtonPin = 14;  // KEY button (right of USB-C): speed gear

// --------------------------------------------------------------- Board -----
constexpr uint8_t kLcdPowerPin = 15;  // must be HIGH to power the LCD on battery
constexpr uint8_t kBatteryPin = 4;    // onboard 1:2 divider
constexpr float kBatteryDivider = 2.0f;

// ---------------------------------------------------------------- Radio ----
constexpr uint32_t kControlPeriodMs = 20;      // 50 Hz control packets
constexpr uint32_t kTelemetryTimeoutMs = 600;  // car considered offline after this
constexpr uint32_t kCarForgetMs = 3000;        // fall back to broadcast after this
constexpr uint32_t kArmGraceMs = 500;          // ignore stale telemetry right after arming

// --------------------------------------------------------------- Display ---
constexpr uint32_t kDisplayPeriodMs = 50;  // 20 fps
constexpr uint32_t kMessageMs = 1500;

// ------------------------------------------------------------ Speed gears --
constexpr uint8_t kGears[] = {30, 60, 100};  // percent of max wheel speed
constexpr uint8_t kGearCount = sizeof(kGears) / sizeof(kGears[0]);
constexpr uint8_t kDefaultGear = 0;

}  // namespace rcfg
