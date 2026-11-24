#include "EspNowTransport.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>

constexpr uint8_t EspNowTransport::kBroadcast[6];

EspNowTransport &EspNowTransport::instance() {
  static EspNowTransport inst;
  return inst;
}

void EspNowTransport::dispatch(const uint8_t *mac, const uint8_t *data, int len) {
  EspNowTransport &self = instance();
  if (self.handler_ && mac && data && len > 0) {
    self.handler_(mac, data, static_cast<size_t>(len), self.ctx_);
  }
}

#if ESP_ARDUINO_VERSION_MAJOR >= 3
static void onRecvThunk(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  EspNowTransport::dispatch(info->src_addr, data, len);
}
#else
static void onRecvThunk(const uint8_t *mac, const uint8_t *data, int len) {
  EspNowTransport::dispatch(mac, data, len);
}
#endif

bool EspNowTransport::begin(uint8_t channel) {
  channel_ = channel;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_ps(WIFI_PS_NONE);  // lowest latency
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel_, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) return false;
  if (esp_now_register_recv_cb(onRecvThunk) != ESP_OK) return false;
  return addPeer(kBroadcast);
}

void EspNowTransport::onReceive(ReceiveHandler handler, void *ctx) {
  ctx_ = ctx;
  handler_ = handler;
}

bool EspNowTransport::addPeer(const uint8_t mac[6]) {
  if (esp_now_is_peer_exist(mac)) return true;
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = channel_;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  return esp_now_add_peer(&peer) == ESP_OK;
}

bool EspNowTransport::send(const uint8_t mac[6], const void *data, size_t len) {
  return esp_now_send(mac, static_cast<const uint8_t *>(data), len) == ESP_OK;
}

bool EspNowTransport::broadcast(const void *data, size_t len) { return send(kBroadcast, data, len); }

void EspNowTransport::localMac(uint8_t out[6]) const { esp_wifi_get_mac(WIFI_IF_STA, out); }
