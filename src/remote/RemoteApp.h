#pragma once
// Top-level remote controller application.
//
// Controls:
//   Joystick Y       forward / backward
//   Joystick X       turn (DRIVE mode) or strafe sideways (STRAFE mode)
//   Joystick press   toggle DRIVE / STRAFE
//   BOOT button      arm / disarm (arming needs a live link and a centred stick)
//   KEY button       cycle speed gear (30 / 60 / 100 %)

#include <BatteryMonitor.h>
#include <RoboProtocol.h>
#include <stdint.h>

#include "Button.h"
#include "Display.h"
#include "Joystick.h"
#include "RemoteLink.h"

class RemoteApp {
 public:
  RemoteApp();

  void begin();
  void loop();

 private:
  void handleButtons(uint32_t now);
  void supervise(uint32_t now);
  void sendControl();
  void render(uint32_t now);
  void showMessage(const char *msg, uint32_t now);
  bool telemetryFresh(uint32_t now) const;

  Joystick joystick_;
  Button joyButton_;
  Button armButton_;
  Button gearButton_;
  RemoteLink link_;
  Display display_;
  BatteryMonitor battery_;

  bool armed_ = false;
  uint32_t armedAtMs_ = 0;
  DriveMode mode_ = DriveMode::Drive;
  uint8_t gear_;

  proto::TelemetryPacket tel_ = {};
  uint32_t telAtMs_ = 0;
  bool haveTel_ = false;

  const char *message_ = nullptr;
  uint32_t messageAtMs_ = 0;

  uint32_t lastControlMs_ = 0;
  uint32_t lastDisplayMs_ = 0;
  uint32_t lastBatteryMs_ = 0;
};
