#pragma once
// Car side of the ESP-NOW link.
//
// Pairing: the car locks onto the first remote that sends a valid control
// packet (same pairing key) and replies to it with unicast telemetry. If that
// remote stays silent for cfg::kRepairAfterMs, another remote may take over.

#include <RoboProtocol.h>
#include <stdint.h>

struct RemoteCommand {
  float vx = 0.0f;  // -1..1
  float vy = 0.0f;
  float wz = 0.0f;
  float speedLimit = 0.0f;  // 0..1
  bool armed = false;
  uint16_t seq = 0;
  uint32_t receivedAtMs = 0;
};

class CarLink {
 public:
  bool begin();

  // Updates the packet-rate counter. Call every loop.
  void update(uint32_t nowMs);

  // Copies the most recent command. Returns false if none was ever received.
  bool latest(RemoteCommand &out) const;

  bool paired() const;
  uint8_t controlRateHz() const { return rateHz_; }

  // Fills the header (seq) and CRC, then sends to the paired remote.
  bool sendTelemetry(proto::TelemetryPacket &pkt);

 private:
  static void onReceive(const uint8_t mac[6], const uint8_t *data, size_t len, void *ctx);
  void handle(const uint8_t mac[6], const uint8_t *data, size_t len);

  // Shared with the Wi-Fi task, guarded by a spinlock in the .cpp.
  RemoteCommand cmd_;
  bool hasCmd_ = false;
  uint8_t remoteMac_[6] = {0};
  bool paired_ = false;
  bool peerPending_ = false;
  uint32_t packetCount_ = 0;

  // Loop-side state.
  bool peerReady_ = false;
  uint32_t rateWindowStartMs_ = 0;
  uint32_t rateWindowCount_ = 0;
  uint8_t rateHz_ = 0;
  uint16_t txSeq_ = 0;
};
