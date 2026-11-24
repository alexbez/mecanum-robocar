#pragma once
// Top-level robocar application: owns all subsystems and runs the state machine.
//
// Safety rules:
//  - Motors are enabled only in ARMED / DRIVING.
//  - No control packet for cfg::kFailsafeMs -> LINK LOST, motors braked.
//  - After boot, link loss, a fault or low battery the remote must send
//    "disarmed" at least once before the car accepts "armed" again, so the car
//    never jumps off because a stick was held when the link came back.
//  - Wheel stall / encoder fault -> FAULT, cleared by disarming on the remote.

#include <BatteryMonitor.h>
#include <RoboProtocol.h>
#include <stdint.h>

#include "CarLink.h"
#include "MecanumDrive.h"
#include "StatusLed.h"

class RoboCar {
 public:
  RoboCar();

  void begin();
  void loop();

 private:
  void controlStep(float dt);
  proto::CarState evaluateState(uint32_t nowMs, const RemoteCommand &cmd, bool hasCmd);
  void sendTelemetry(uint32_t nowMs);
  void log(uint32_t nowMs);

  MecanumDrive drive_;
  CarLink link_;
  StatusLed led_;
  BatteryMonitor battery_;

  proto::CarState state_ = proto::CarState::Booting;
  bool needDisarm_ = true;
  bool lowBatteryLatched_ = false;

  uint32_t lastControlUs_ = 0;
  uint32_t lastBatteryMs_ = 0;
  uint32_t lastTelemetryMs_ = 0;
  uint32_t lastLogMs_ = 0;
};
