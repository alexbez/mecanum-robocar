#pragma once
// Remote side of the ESP-NOW link.
//
// Control packets are broadcast until the car answers with telemetry; then
// they are sent unicast to that car (unicast gets MAC-level ACK + retries).
// If the car is silent for rcfg::kCarForgetMs, the remote goes back to broadcast.

#include <RoboProtocol.h>
#include <stdint.h>

class RemoteLink {
 public:
  bool begin();

  // Updates rate counters and the car-forget timeout. Call every loop.
  void update(uint32_t nowMs);

  // vx, vy, wz in -1..1.
  bool sendControl(float vx, float vy, float wz, uint8_t speedPct, bool armed);

  // Copies the latest telemetry. Returns false if none ever received.
  bool latestTelemetry(proto::TelemetryPacket &out, uint32_t &receivedAtMs) const;

  bool carKnown() const;
  uint8_t telemetryRateHz() const { return rateHz_; }

 private:
  static void onReceive(const uint8_t mac[6], const uint8_t *data, size_t len, void *ctx);
  void handle(const uint8_t mac[6], const uint8_t *data, size_t len);

  // Shared with the Wi-Fi task, guarded by a spinlock in the .cpp.
  proto::TelemetryPacket tel_ = {};
  uint32_t telAtMs_ = 0;
  bool hasTel_ = false;
  uint8_t carMac_[6] = {0};
  bool carKnown_ = false;
  bool carIsPeer_ = false;
  uint32_t telCount_ = 0;

  uint32_t rateWindowStartMs_ = 0;
  uint32_t rateWindowCount_ = 0;
  uint8_t rateHz_ = 0;
  uint16_t txSeq_ = 0;
};
