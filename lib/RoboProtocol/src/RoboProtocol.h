#pragma once
// Wire protocol shared by the robocar and the remote controller (ESP-NOW).
//
// Every packet starts with a Header and ends with a CRC-8 over all preceding
// bytes. Packets with a wrong magic, version, pairing key, size or CRC are
// dropped by the receiver, so stray ESP-NOW traffic is ignored.

#include <stddef.h>
#include <stdint.h>

#ifndef ROBO_PAIR_KEY
#define ROBO_PAIR_KEY 0x4D45434Bu
#endif

#ifndef ROBO_WIFI_CHANNEL
#define ROBO_WIFI_CHANNEL 1
#endif

namespace proto {

constexpr uint16_t kMagic = 0xB0C4;
constexpr uint8_t kVersion = 1;
constexpr uint32_t kPairKey = ROBO_PAIR_KEY;
constexpr uint8_t kWifiChannel = ROBO_WIFI_CHANNEL;
constexpr uint8_t kWheelCount = 4;

// Wheel order used everywhere: front-left, front-right, rear-left, rear-right.
enum Wheel : uint8_t { kFL = 0, kFR = 1, kRL = 2, kRR = 3 };

enum class MsgType : uint8_t {
  Control = 1,    // remote -> car, ~50 Hz
  Telemetry = 2,  // car -> remote, ~10 Hz
};

// Car state, reported in telemetry and shown on the car's RGB LED.
enum class CarState : uint8_t {
  Booting = 0,
  WaitingForRemote = 1,  // never received a valid control packet
  LinkLost = 2,          // control packets stopped (failsafe, motors braked)
  Disarmed = 3,          // link OK, motors disabled
  Armed = 4,             // link OK, motors enabled, standing still
  Driving = 5,           // armed and moving
  LowBattery = 6,        // battery below critical level, motors disabled
  Fault = 7,             // wheel stall / encoder fault, motors disabled
};

const char *carStateName(CarState s);

// Control flags (ControlPacket::flags)
constexpr uint8_t kCtrlArmed = 1 << 0;

// Telemetry flags (TelemetryPacket::flags)
constexpr uint8_t kTelBatteryWarn = 1 << 0;
constexpr uint8_t kTelClosedLoop = 1 << 1;
constexpr uint8_t kTelNeedDisarm = 1 << 2;  // car waits for a disarm before re-arming

struct __attribute__((packed)) Header {
  uint16_t magic;
  uint8_t version;
  uint8_t type;  // MsgType
  uint32_t pairKey;
  uint16_t seq;
};

// Normalised motion command. All axes are -1000..1000.
//   vx : forward (+) / backward (-)
//   vy : strafe left (+) / right (-)
//   wz : rotate counter-clockwise (+) / clockwise (-)
struct __attribute__((packed)) ControlPacket {
  Header hdr;
  int16_t vx;
  int16_t vy;
  int16_t wz;
  uint8_t speedLimitPct;  // 0..100, scales the whole command on the car
  uint8_t flags;          // kCtrl*
  uint8_t crc;
};

struct __attribute__((packed)) TelemetryPacket {
  Header hdr;
  uint8_t state;      // CarState
  uint8_t flags;      // kTel*
  uint8_t faultMask;  // bit n = wheel n stalled / encoder fault
  uint8_t controlRateHz;
  uint16_t batteryMv;
  int16_t wheelRpmX10[kWheelCount];     // measured wheel speed, rpm * 10
  int16_t wheelTargetX10[kWheelCount];  // target wheel speed, rpm * 10
  int8_t wheelDutyPct[kWheelCount];     // applied PWM duty, -100..100
  int16_t vxMmS;                        // body velocity from encoders
  int16_t vyMmS;
  int16_t wzMradS;
  uint32_t odometerMm;  // total distance travelled
  uint32_t uptimeS;
  uint8_t crc;
};

static_assert(sizeof(ControlPacket) <= 250, "ESP-NOW payload limit");
static_assert(sizeof(TelemetryPacket) <= 250, "ESP-NOW payload limit");

uint8_t crc8(const uint8_t *data, size_t len);

// Fills magic/version/type/key/seq.
void initHeader(Header &hdr, MsgType type, uint16_t seq);

// Computes and stores the trailing CRC. `size` is sizeof(packet).
void seal(void *packet, size_t size);

// Validates size, magic, version, key, type and CRC.
bool validate(const uint8_t *data, size_t len, MsgType expected, size_t expectedSize);

}  // namespace proto
