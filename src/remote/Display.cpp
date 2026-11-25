#include "Display.h"

#include <Arduino.h>
#include <math.h>

using proto::CarState;

namespace {
constexpr int kW = 320;
constexpr int kH = 170;
constexpr int kHeaderH = 24;

constexpr uint16_t kBg = TFT_BLACK;
constexpr uint16_t kPanel = 0x18E3;  // dark grey
constexpr uint16_t kLabel = 0x8410;  // mid grey
constexpr uint16_t kText = TFT_WHITE;
constexpr uint16_t kDim = 0x4208;

// Fonts built into TFT_eSPI: 1 = 8 px, 2 = 16 px, 4 = 26 px.
constexpr uint8_t kSmall = 1;
constexpr uint8_t kMedium = 2;
constexpr uint8_t kLarge = 4;
}  // namespace

Display::Display() : tft_(), canvas_(&tft_) {}

void Display::begin(uint8_t lcdPowerPin) {
  pinMode(lcdPowerPin, OUTPUT);
  digitalWrite(lcdPowerPin, HIGH);

  tft_.init();
  // Landscape with USB-C on the LEFT, as the board sits in the printed case
  // (hardware/remote-case). Use 1 for USB-C on the right.
  tft_.setRotation(3);
  tft_.fillScreen(kBg);
#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
#endif

  canvas_.setColorDepth(16);
  spriteOk_ = canvas_.createSprite(kW, kH) != nullptr;
}

uint16_t Display::stateColour(CarState s) {
  switch (s) {
    case CarState::Booting: return TFT_LIGHTGREY;
    case CarState::WaitingForRemote: return TFT_BLUE;
    case CarState::LinkLost: return TFT_RED;
    case CarState::Disarmed: return 0xFD20;  // amber
    case CarState::Armed: return TFT_DARKGREEN;
    case CarState::Driving: return 0x04FF;  // cyan-blue
    case CarState::LowBattery: return TFT_ORANGE;
    case CarState::Fault: return TFT_MAGENTA;
  }
  return TFT_DARKGREY;
}

void Display::splash(const char *line1, const char *line2) {
  TFT_eSPI &g = spriteOk_ ? static_cast<TFT_eSPI &>(canvas_) : tft_;
  g.fillScreen(kBg);
  g.setTextDatum(MC_DATUM);
  g.setTextColor(kText, kBg);
  g.drawString(line1, kW / 2, kH / 2 - 16, kLarge);
  g.setTextColor(kLabel, kBg);
  g.drawString(line2, kW / 2, kH / 2 + 18, kMedium);
  if (spriteOk_) canvas_.pushSprite(0, 0);
}

void Display::drawHeader(const DisplayModel &m) {
  TFT_eSPI &g = canvas_;
  g.fillRect(0, 0, kW, kHeaderH, kPanel);

  const char *stateText;
  uint16_t colour;
  if (!m.haveTelemetry) {
    stateText = "SEARCHING CAR";
    colour = TFT_BLUE;
  } else if (!m.telemetryFresh) {
    stateText = "NO SIGNAL";
    colour = TFT_RED;
  } else {
    const auto s = static_cast<CarState>(m.tel.state);
    stateText = proto::carStateName(s);
    colour = stateColour(s);
  }
  // Blink the pill when there is no signal.
  const bool blinkOff = m.haveTelemetry && !m.telemetryFresh && (millis() % 600) < 300;
  g.fillRoundRect(3, 3, 130, kHeaderH - 6, 5, blinkOff ? kPanel : colour);
  g.setTextDatum(MC_DATUM);
  g.setTextColor(kText);
  g.drawString(stateText, 3 + 65, kHeaderH / 2, kMedium);

  char buf[24];
  g.setTextDatum(MR_DATUM);
  const float remV = m.remoteBatteryMv / 1000.0f;
  snprintf(buf, sizeof(buf), "REM %.2fV", remV);
  g.setTextColor(remV < 3.5f && remV > 2.5f ? TFT_ORANGE : kLabel);
  g.drawString(buf, kW - 4, kHeaderH / 2, kMedium);

  snprintf(buf, sizeof(buf), "RX %uHz", m.telemetryRateHz);
  g.setTextColor(m.telemetryFresh ? TFT_GREEN : TFT_RED);
  g.drawString(buf, kW - 92, kHeaderH / 2, kMedium);
}

void Display::drawLeftPanel(const DisplayModel &m) {
  TFT_eSPI &g = canvas_;
  const int x = 6;
  const bool live = m.haveTelemetry && m.telemetryFresh;
  char buf[32];

  // Car battery
  g.setTextDatum(TL_DATUM);
  g.setTextColor(kLabel);
  g.drawString("CAR BATTERY", x, 30, kSmall);
  uint16_t battColour = kDim;
  if (live) {
    battColour = TFT_GREEN;
    if (m.tel.flags & proto::kTelBatteryWarn) battColour = TFT_ORANGE;
    if (static_cast<CarState>(m.tel.state) == CarState::LowBattery) battColour = TFT_RED;
  }
  if (m.haveTelemetry && m.tel.batteryMv >= 1000) {
    snprintf(buf, sizeof(buf), "%.2f V", m.tel.batteryMv / 1000.0f);
  } else {
    snprintf(buf, sizeof(buf), "--.-- V");
  }
  g.setTextColor(battColour);
  g.drawString(buf, x, 40, kLarge);

  // Speed and odometer
  g.setTextColor(kLabel);
  g.drawString("SPEED", x, 70, kSmall);
  g.drawString("ODOMETER", x + 90, 70, kSmall);
  g.setTextColor(live ? kText : kDim);
  const float speed = sqrtf(static_cast<float>(m.tel.vxMmS) * m.tel.vxMmS +
                            static_cast<float>(m.tel.vyMmS) * m.tel.vyMmS) / 1000.0f;
  snprintf(buf, sizeof(buf), "%.2f m/s", m.haveTelemetry ? speed : 0.0f);
  g.drawString(buf, x, 80, kMedium);
  snprintf(buf, sizeof(buf), "%.1f m", m.tel.odometerMm / 1000.0f);
  g.drawString(buf, x + 90, 80, kMedium);

  // Mode + gear
  g.fillRoundRect(x, 102, 96, 22, 4, kPanel);
  g.setTextDatum(ML_DATUM);
  g.setTextColor(m.mode == DriveMode::Drive ? TFT_CYAN : TFT_YELLOW);
  g.drawString(m.mode == DriveMode::Drive ? "DRIVE" : "STRAFE", x + 6, 113, kMedium);
  g.setTextDatum(MR_DATUM);
  g.setTextColor(kText);
  snprintf(buf, sizeof(buf), "%u%%", m.speedPct);
  g.drawString(buf, x + 92, 113, kMedium);

  // Arm state
  const bool needDisarm = m.haveTelemetry && (m.tel.flags & proto::kTelNeedDisarm);
  uint16_t armColour = m.armed ? TFT_RED : kPanel;
  const char *armText = m.armed ? "ARMED" : (needDisarm ? "SAFE - press ARM" : "SAFE");
  g.fillRoundRect(x, 130, 140, 32, 6, armColour);
  g.setTextDatum(MC_DATUM);
  g.setTextColor(kText);
  g.drawString(armText, x + 70, 146, m.armed ? kLarge : kMedium);

  drawJoystick(128, 113, 15, m);
}

void Display::drawJoystick(int cx, int cy, int r, const DisplayModel &m) {
  TFT_eSPI &g = canvas_;
  g.drawCircle(cx, cy, r, kLabel);
  g.drawFastHLine(cx - r, cy, 2 * r, kDim);
  g.drawFastVLine(cx, cy - r, 2 * r, kDim);
  const int px = cx + static_cast<int>(m.joyX * (r - 3));
  const int py = cy - static_cast<int>(m.joyY * (r - 3));
  g.fillCircle(px, py, 4, m.armed ? TFT_RED : TFT_WHITE);
}

void Display::drawWheel(int x, int y, const char *name, const DisplayModel &m, uint8_t i) {
  TFT_eSPI &g = canvas_;
  constexpr int w = 76;
  constexpr int h = 66;
  const bool live = m.haveTelemetry && m.telemetryFresh;
  const bool fault = m.haveTelemetry && (m.tel.faultMask & (1u << i));

  g.fillRoundRect(x, y, w, h, 5, kPanel);
  if (fault) g.drawRoundRect(x, y, w, h, 5, TFT_MAGENTA);

  g.setTextDatum(TL_DATUM);
  g.setTextColor(fault ? TFT_MAGENTA : kLabel);
  g.drawString(fault ? "STALL" : name, x + 5, y + 4, kSmall);

  char buf[16];
  const float rpm = m.tel.wheelRpmX10[i] / 10.0f;
  const float target = m.tel.wheelTargetX10[i] / 10.0f;
  g.setTextDatum(TR_DATUM);
  g.setTextColor(kLabel);
  snprintf(buf, sizeof(buf), "%.0f", target);
  g.drawString(buf, x + w - 5, y + 4, kSmall);

  g.setTextDatum(MC_DATUM);
  g.setTextColor(live ? kText : kDim);
  snprintf(buf, sizeof(buf), "%.1f", m.haveTelemetry ? rpm : 0.0f);
  g.drawString(buf, x + w / 2, y + 28, kLarge);

  // Duty bar, centred: right = forward, left = reverse.
  const int bx = x + 6;
  const int bw = w - 12;
  const int by = y + h - 14;
  g.fillRect(bx, by, bw, 8, kBg);
  const int half = bw / 2;
  const int duty = m.tel.wheelDutyPct[i];
  const int len = (abs(duty) * half) / 100;
  const uint16_t barColour = abs(duty) > 90 ? TFT_ORANGE : TFT_GREEN;
  if (live && len > 0) {
    if (duty > 0) {
      g.fillRect(bx + half, by, len, 8, barColour);
    } else {
      g.fillRect(bx + half - len, by, len, 8, barColour);
    }
  }
  g.drawFastVLine(bx + half, by - 1, 10, kLabel);
}

void Display::drawMessage(const char *msg) {
  TFT_eSPI &g = canvas_;
  constexpr int w = 240;
  constexpr int h = 40;
  const int x = (kW - w) / 2;
  const int y = (kH - h) / 2 + 10;
  g.fillRoundRect(x, y, w, h, 8, TFT_NAVY);
  g.drawRoundRect(x, y, w, h, 8, TFT_WHITE);
  g.setTextDatum(MC_DATUM);
  g.setTextColor(TFT_WHITE);
  g.drawString(msg, kW / 2, y + h / 2, kMedium);
}

void Display::render(const DisplayModel &m) {
  if (!spriteOk_) return;  // not enough RAM for the frame buffer
  canvas_.fillSprite(kBg);
  drawHeader(m);
  drawLeftPanel(m);
  drawWheel(160, 30, "FL", m, proto::kFL);
  drawWheel(240, 30, "FR", m, proto::kFR);
  drawWheel(160, 100, "RL", m, proto::kRL);
  drawWheel(240, 100, "RR", m, proto::kRR);
  if (m.message) drawMessage(m.message);
  canvas_.pushSprite(0, 0);
}
