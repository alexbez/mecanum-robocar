#include "RemoteLink.h"

#include <Arduino.h>
#include <EspNowTransport.h>
#include <string.h>

#include "RemoteConfig.h"

namespace {
portMUX_TYPE gLinkMux = portMUX_INITIALIZER_UNLOCKED;

int16_t axis(float v) {
  if (v > 1.0f) v = 1.0f;
  if (v < -1.0f) v = -1.0f;
  return static_cast<int16_t>(lroundf(v * 1000.0f));
}
}  // namespace

bool RemoteLink::begin() {
  EspNowTransport &radio = EspNowTransport::instance();
  if (!radio.begin(proto::kWifiChannel)) return false;
  radio.onReceive(&RemoteLink::onReceive, this);
  return true;
}

void RemoteLink::onReceive(const uint8_t mac[6], const uint8_t *data, size_t len, void *ctx) {
  static_cast<RemoteLink *>(ctx)->handle(mac, data, len);
}

void RemoteLink::handle(const uint8_t mac[6], const uint8_t *data, size_t len) {
  if (!proto::validate(data, len, proto::MsgType::Telemetry, sizeof(proto::TelemetryPacket))) return;

  const uint32_t now = millis();
  portENTER_CRITICAL(&gLinkMux);
  // Lock onto the first car that answers; ignore others while it is alive.
  if (!carKnown_ || memcmp(mac, carMac_, 6) == 0) {
    if (!carKnown_) {
      memcpy(carMac_, mac, 6);
      carKnown_ = true;
    }
    memcpy(&tel_, data, sizeof(tel_));
    telAtMs_ = now;
    hasTel_ = true;
    ++telCount_;
  }
  portEXIT_CRITICAL(&gLinkMux);
}

void RemoteLink::update(uint32_t nowMs) {
  portENTER_CRITICAL(&gLinkMux);
  const uint32_t count = telCount_;
  if (carKnown_ && nowMs - telAtMs_ > rcfg::kCarForgetMs && telAtMs_ <= nowMs) {
    carKnown_ = false;
  }
  const bool known = carKnown_;
  uint8_t mac[6];
  memcpy(mac, carMac_, 6);
  portEXIT_CRITICAL(&gLinkMux);

  // Register the car as a unicast peer outside the Wi-Fi callback.
  if (known && !carIsPeer_) {
    carIsPeer_ = EspNowTransport::instance().addPeer(mac);
  } else if (!known) {
    carIsPeer_ = false;
  }

  if (nowMs - rateWindowStartMs_ >= 1000) {
    const uint32_t n = count - rateWindowCount_;
    rateHz_ = n > 255 ? 255 : static_cast<uint8_t>(n);
    rateWindowCount_ = count;
    rateWindowStartMs_ = nowMs;
  }
}

bool RemoteLink::sendControl(float vx, float vy, float wz, uint8_t speedPct, bool armed) {
  proto::ControlPacket pkt = {};
  proto::initHeader(pkt.hdr, proto::MsgType::Control, txSeq_++);
  pkt.vx = axis(vx);
  pkt.vy = axis(vy);
  pkt.wz = axis(wz);
  pkt.speedLimitPct = speedPct > 100 ? 100 : speedPct;
  pkt.flags = armed ? proto::kCtrlArmed : 0;
  proto::seal(&pkt, sizeof(pkt));

  EspNowTransport &radio = EspNowTransport::instance();
  if (carIsPeer_) {
    uint8_t mac[6];
    portENTER_CRITICAL(&gLinkMux);
    memcpy(mac, carMac_, 6);
    portEXIT_CRITICAL(&gLinkMux);
    return radio.send(mac, &pkt, sizeof(pkt));
  }
  return radio.broadcast(&pkt, sizeof(pkt));
}

bool RemoteLink::latestTelemetry(proto::TelemetryPacket &out, uint32_t &receivedAtMs) const {
  portENTER_CRITICAL(&gLinkMux);
  out = tel_;
  receivedAtMs = telAtMs_;
  const bool has = hasTel_;
  portEXIT_CRITICAL(&gLinkMux);
  return has;
}

bool RemoteLink::carKnown() const {
  portENTER_CRITICAL(&gLinkMux);
  const bool k = carKnown_;
  portEXIT_CRITICAL(&gLinkMux);
  return k;
}
