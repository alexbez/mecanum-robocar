// Mecanum robocar firmware (YD-ESP32-S3 / ESP32-S3-WROOM-1).
// All logic lives in RoboCar; see CarConfig.h for pins and tuning.

#include <Arduino.h>

#include "RoboCar.h"

static RoboCar car;

void setup() { car.begin(); }

void loop() { car.loop(); }
