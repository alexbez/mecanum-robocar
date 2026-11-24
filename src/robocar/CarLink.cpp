#include "CarLink.h"

#include <Arduino.h>
#include <EspNowTransport.h>
#include <string.h>

#include "CarConfig.h"

namespace {
portMUX_TYPE gLinkMux = portMUX_INITIALIZER_UNLOCKED;

float axis(int16_t v) {
  float f = v / 1000.0f;
  return f > 1.0f ? 1.0f : (f < -1.0f ? -1.0f : f);
}
}  // namespace

bool CarLink::begin() {
  EspNowTransport &radio = EspNowTransport::instance();
  if (!radio.begin(proto::kWifiChannel)) return false;
  radio.onReceive(&CarLink::onReceive, this);
  return true;
}

void CarLink::onReceive(const uint8_t mac[6], const uint8_t *data, size_t len, void *ctx) {
  static_cast<CarLink *>(ctx)->handle(mac, data, len);
}

void CarLink::handle(const uint8_t mac[6], const uint8_t *data, size_t len) {
  if (!proto::validate(data, len, proto::MsgType::Control, sizeof(proto::ControlPacket))) return;

  proto::ControlPacket pkt;
  memcpy(&pkt, data, sizeof(pkt));
  const uint32_t now = millis();

  portENTER_CRITICAL(&gLinkMux);
  const bool sameRemote = paired_ && memcmp(mac, remoteMac_, 6) == 0;
  const bool remoteSilent = !paired_ || (now - cmd_.receivedAtMs) > cfg::kRepairAfterMs;
  if (sameRemote || remoteSilent) {
    if (!sameRemote) {
      memcpy(remoteMac_, mac, 6);
      paired_ = true;
      peerPending_ = true;  // registered in update(), outside the Wi-Fi task
    }
    cmd_.vx = axis(pkt.vx);
    cmd_.vy = axis(pkt.vy);
    cmd_.wz = axis(pkt.wz);
    cmd_.speedLimit = (pkt.speedLimitPct > 100 ? 100 : pkt.speedLimitPct) / 100.0f;
    cmd_.armed = (pkt.flags & proto::kCtrlArmed) != 0;
    cmd_.seq = pkt.hdr.seq;
    cmd_.receivedAtMs = now;
    hasCmd_ = true;
    ++packetCount_;
  }
  portEXIT_CRITICAL(&gLinkMux);
}

void CarLink::update(uint32_t nowMs) {
  uint8_t mac[6];
  portENTER_CRITICAL(&gLinkMux);
  const bool pending = peerPending_;
  peerPending_ = false;
  memcpy(mac, remoteMac_, 6);
  portEXIT_CRITICAL(&gLinkMux);
  if (pending) {
    peerReady_ = EspNowTransport::instance().addPeer(mac);
  }

  if (nowMs - rateWindowStartMs_ >= 1000) {
    portENTER_CRITICAL(&gLinkMux);
    const uint32_t count = packetCount_;
    portEXIT_CRITICAL(&gLinkMux);
    const uint32_t n = count - rateWindowCount_;
    rateHz_ = n > 255 ? 255 : static_cast<uint8_t>(n);
    rateWindowCount_ = count;
    rateWindowStartMs_ = nowMs;
  }
}

bool CarLink::latest(RemoteCommand &out) const {
  portENTER_CRITICAL(&gLinkMux);
  out = cmd_;
  const bool has = hasCmd_;
  portEXIT_CRITICAL(&gLinkMux);
  return has;
}

bool CarLink::paired() const {
  portENTER_CRITICAL(&gLinkMux);
  const bool p = paired_;
  portEXIT_CRITICAL(&gLinkMux);
  return p;
}

bool CarLink::sendTelemetry(proto::TelemetryPacket &pkt) {
  uint8_t mac[6];
  portENTER_CRITICAL(&gLinkMux);
  const bool p = paired_ && !peerPending_;
  memcpy(mac, remoteMac_, 6);
  portEXIT_CRITICAL(&gLinkMux);
  if (!p || !peerReady_) return false;

  proto::initHeader(pkt.hdr, proto::MsgType::Telemetry, txSeq_++);
  proto::seal(&pkt, sizeof(pkt));
  return EspNowTransport::instance().send(mac, &pkt, sizeof(pkt));
}
