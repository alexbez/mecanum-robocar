#include "RoboProtocol.h"

#include <string.h>

namespace proto {

const char *carStateName(CarState s) {
  switch (s) {
    case CarState::Booting: return "BOOTING";
    case CarState::WaitingForRemote: return "WAITING";
    case CarState::LinkLost: return "LINK LOST";
    case CarState::Disarmed: return "DISARMED";
    case CarState::Armed: return "ARMED";
    case CarState::Driving: return "DRIVING";
    case CarState::LowBattery: return "LOW BATT";
    case CarState::Fault: return "FAULT";
  }
  return "?";
}

// CRC-8/ATM, polynomial 0x07.
uint8_t crc8(const uint8_t *data, size_t len) {
  uint8_t crc = 0;
  for (size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; ++b) {
      crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x07) : static_cast<uint8_t>(crc << 1);
    }
  }
  return crc;
}

void initHeader(Header &hdr, MsgType type, uint16_t seq) {
  hdr.magic = kMagic;
  hdr.version = kVersion;
  hdr.type = static_cast<uint8_t>(type);
  hdr.pairKey = kPairKey;
  hdr.seq = seq;
}

void seal(void *packet, size_t size) {
  uint8_t *bytes = static_cast<uint8_t *>(packet);
  bytes[size - 1] = crc8(bytes, size - 1);
}

bool validate(const uint8_t *data, size_t len, MsgType expected, size_t expectedSize) {
  if (data == nullptr || len != expectedSize || len < sizeof(Header) + 1) return false;
  Header hdr;
  memcpy(&hdr, data, sizeof(hdr));
  if (hdr.magic != kMagic || hdr.version != kVersion || hdr.pairKey != kPairKey) return false;
  if (hdr.type != static_cast<uint8_t>(expected)) return false;
  return crc8(data, len - 1) == data[len - 1];
}

}  // namespace proto
