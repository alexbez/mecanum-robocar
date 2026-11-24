#pragma once
// Thin ESP-NOW wrapper shared by both firmwares.
//
// - Puts Wi-Fi in STA mode on a fixed channel (no access point needed).
// - Hides the receive-callback signature difference between
//   Arduino-ESP32 2.x (ESP-IDF 4.4) and 3.x (ESP-IDF 5.x).
// - Only one instance can exist (ESP-NOW has a single global callback).

#include <stddef.h>
#include <stdint.h>

class EspNowTransport {
 public:
  // Called from the Wi-Fi task. Keep it short and copy data out.
  using ReceiveHandler = void (*)(const uint8_t mac[6], const uint8_t *data, size_t len, void *ctx);

  static EspNowTransport &instance();

  bool begin(uint8_t channel);
  void onReceive(ReceiveHandler handler, void *ctx);

  // Adds a unicast peer (no-op if it already exists).
  bool addPeer(const uint8_t mac[6]);
  bool send(const uint8_t mac[6], const void *data, size_t len);
  bool broadcast(const void *data, size_t len);

  void localMac(uint8_t out[6]) const;

  static constexpr uint8_t kBroadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  // Internal: forwards a received frame to the registered handler.
  static void dispatch(const uint8_t *mac, const uint8_t *data, int len);

 private:
  EspNowTransport() = default;

  uint8_t channel_ = 1;
  ReceiveHandler handler_ = nullptr;
  void *ctx_ = nullptr;
};
