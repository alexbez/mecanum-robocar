#pragma once
// Robocar hardware configuration: pins, geometry, motor and control constants.
//
// Board: YD-ESP32-S3 ("YD-ESP32-23"), ESP32-S3-WROOM-1 module, DevKitC-1
// pinout. Pins avoid strapping pins (0, 3, 45, 46), native USB (19, 20),
// flash/PSRAM (26-37; 35-37 are taken by octal PSRAM on N16R8/N8R8 modules)
// and UART0 (43, 44, used by the CH343 USB-serial port).

#include <stdint.h>

#include "Motor.h"

namespace cfg {

// ---------------------------------------------------------------- RGB LED --
// Onboard WS2812 on GPIO48. On the YD-ESP32-S3 the LED is connected only when
// the solder jumper marked "RGB" on the board is bridged.
constexpr uint8_t kLedPin = 48;
constexpr uint8_t kLedBrightness = 40;  // 0..255, the LED is very bright

// ---------------------------------------------------------- Battery sense --
// 3S Li-ion/LiPo through a divider: 100k (top) / 22k (bottom) -> 12.6 V = 2.27 V
// Must be an ADC1 pin (ADC2 does not work while Wi-Fi is on).
constexpr uint8_t kBatteryPin = 1;
constexpr float kBatteryDivider = (100.0f + 22.0f) / 22.0f;
constexpr float kBatteryCalibration = 1.0f;  // adjust after measuring with a multimeter
constexpr uint16_t kBatteryWarnMv = 10500;   // 3.5 V/cell
constexpr uint16_t kBatteryCriticalMv = 9900;  // 3.3 V/cell -> motors disabled

// -------------------------------------------------- TB6612FNG drivers ------
// Driver #1 = front axle (A = front-left, B = front-right)
// Driver #2 = rear axle  (A = rear-left,  B = rear-right)
// STBY of both drivers is tied to one GPIO.
constexpr uint8_t kStbyPin = 41;

struct WheelHardware {
  MotorPins motor;
  uint8_t ledcChannel;
  bool motorInverted;  // flip if the wheel spins backwards on "forward"
  uint8_t encA;
  uint8_t encB;
  uint8_t pcntUnit;
  bool encoderInverted;  // flip if rpm sign is opposite to the drive direction
};

// Order: FL, FR, RL, RR (see proto::Wheel).
// Right-side motors are mounted mirrored, so they are inverted by default.
constexpr WheelHardware kWheels[4] = {
    // motor {pwm, in1, in2}, ledc, inv,   encA, encB, pcnt, encInv
    {{8, 9, 10}, 0, false, 4, 5, 0, false},    // FL  (driver 1, channel A)
    {{11, 12, 13}, 1, true, 6, 7, 1, true},    // FR  (driver 1, channel B)
    {{14, 21, 47}, 2, false, 15, 16, 2, false},  // RL  (driver 2, channel A)
    {{2, 39, 40}, 3, true, 17, 18, 3, true},   // RR  (driver 2, channel B)
};

constexpr uint32_t kPwmFrequencyHz = 20000;  // above audible range
constexpr uint8_t kPwmResolutionBits = 10;

// ----------------------------------------------------- Motor / encoder -----
// 25GA-370, 1:103, 60 rpm @ 12 V, 11-pulse Hall encoder on the motor shaft.
constexpr float kEncoderPpr = 11.0f;
constexpr float kGearRatio = 103.0f;
constexpr float kQuadrature = 4.0f;  // PCNT counts both edges of A and B
constexpr float kCountsPerWheelRev = kEncoderPpr * kQuadrature * kGearRatio;  // 4532

constexpr float kNominalRpm = 60.0f;   // no-load wheel speed at 12 V
constexpr float kMaxWheelRpm = 55.0f;  // command ceiling, leaves PID headroom
constexpr float kRpmAtFullDuty = 62.0f;  // feed-forward slope, tune on your battery
constexpr float kMinDuty = 0.08f;        // duty needed to overcome static friction

// ----------------------------------------------------- Chassis geometry ----
constexpr float kWheelDiameterMm = 65.0f;
constexpr float kHalfWheelbaseMm = 80.0f;  // centre to front/rear axle (lx)
constexpr float kHalfTrackMm = 95.0f;      // centre to left/right wheel (ly)

// --------------------------------------------------------- Control loop ----
constexpr bool kClosedLoop = true;  // false = feed-forward only (no encoders)
constexpr uint32_t kControlPeriodMs = 10;  // 100 Hz
constexpr float kPidKp = 0.010f;  // duty per rpm of error
constexpr float kPidKi = 0.060f;  // duty per rpm*s
constexpr float kPidKd = 0.0002f;
constexpr float kMaxAccelRpmPerS = 150.0f;  // target ramp, limits wheel slip
constexpr float kRpmFilterAlpha = 0.35f;    // low-pass on measured rpm
constexpr float kTurnGain = 0.6f;           // rotation share of the wheel speed

// Stall / encoder fault: high duty but (almost) no rotation for too long.
constexpr float kStallDuty = 0.55f;
constexpr float kStallRpm = 2.0f;
constexpr uint32_t kStallTimeMs = 1200;

// ------------------------------------------------------------- Radio -------
constexpr uint32_t kFailsafeMs = 300;        // stop if no control packet
constexpr uint32_t kRepairAfterMs = 3000;    // accept a different remote after this
constexpr uint32_t kTelemetryPeriodMs = 100; // 10 Hz
constexpr uint32_t kBatteryPeriodMs = 50;
constexpr uint32_t kLogPeriodMs = 1000;

}  // namespace cfg
