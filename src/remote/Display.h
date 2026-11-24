#pragma once
// T-Display-S3 screen (ST7789, 320x170 landscape), drawn into an off-screen
// sprite and pushed in one go to avoid flicker.
//
//  +--------------------------------------------------------------+
//  | [ CAR STATE ]                         RX 10Hz   REM 4.05V    |
//  | BATTERY            |  FL  12.3     |  FR  12.1               |
//  | 11.84 V            |  [===   ]     |  [===   ]               |
//  | SPEED              |  RL  12.0     |  RR  12.4               |
//  | 0.32 m/s  12.4 m   |  [===   ]     |  [===   ]               |
//  | DRIVE  60%   (o)   |                                         |
//  | [ ARMED ]          |                                         |
//  +--------------------------------------------------------------+

#include <RoboProtocol.h>
#include <TFT_eSPI.h>
#include <stdint.h>

enum class DriveMode : uint8_t {
  Drive,   // stick X turns the car
  Strafe,  // stick X moves the car sideways
};

struct DisplayModel {
  bool haveTelemetry = false;
  bool telemetryFresh = false;
  proto::TelemetryPacket tel = {};
  uint8_t telemetryRateHz = 0;

  bool armed = false;
  DriveMode mode = DriveMode::Drive;
  uint8_t speedPct = 0;
  float joyX = 0.0f;
  float joyY = 0.0f;
  uint16_t remoteBatteryMv = 0;

  const char *message = nullptr;  // transient banner, nullptr = none
};

class Display {
 public:
  Display();

  void begin(uint8_t lcdPowerPin);
  void splash(const char *line1, const char *line2);
  void render(const DisplayModel &m);

 private:
  void drawHeader(const DisplayModel &m);
  void drawLeftPanel(const DisplayModel &m);
  void drawWheel(int x, int y, const char *name, const DisplayModel &m, uint8_t i);
  void drawJoystick(int cx, int cy, int r, const DisplayModel &m);
  void drawMessage(const char *msg);
  static uint16_t stateColour(proto::CarState s);

  TFT_eSPI tft_;
  TFT_eSprite canvas_;
  bool spriteOk_ = false;
};
