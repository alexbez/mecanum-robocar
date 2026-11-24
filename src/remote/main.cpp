// Mecanum robocar remote controller firmware (LilyGo T-Display-S3).
// All logic lives in RemoteApp; see RemoteConfig.h for pins and tuning.

#include <Arduino.h>

#include "RemoteApp.h"

static RemoteApp remote;

void setup() { remote.begin(); }

void loop() { remote.loop(); }
