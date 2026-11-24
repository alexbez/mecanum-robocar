# Mecanum robocar + ESP-NOW remote

One PlatformIO project, two firmwares:

| Environment | Board | Role |
|---|---|---|
| `robocar` | YD-ESP32-S3, marked "YD-ESP32-23" (ESP32-S3-WROOM-1, onboard WS2812 RGB LED) | 4× 25GA-370 motors via 2× TB6612FNG, mecanum kinematics, closed-loop wheel speed |
| `remote`  | LilyGo T-Display-S3 | analog joystick, 1.9" ST7789 screen with live telemetry |

```bash
pio run                       # build both
pio run -e robocar -t upload  # flash the car
pio run -e remote -t upload   # flash the remote
pio device monitor -e robocar # 115200 baud log
```

## Layout

Interfaces (`.h`) for both firmwares live in `include/`, implementations
(`.cpp`) in `src/robocar/` or `src/remote/`. PlatformIO adds `include/` to the
search path, so the code includes headers by name (`#include "Motor.h"`).

```
lib/RoboProtocol/     packet formats, CRC, car states        (shared)
lib/EspNowTransport/  ESP-NOW wrapper (Arduino-ESP32 2.x/3.x) (shared)
lib/BatteryMonitor/   ADC + divider battery voltage          (shared)

Robocar                  include/ (.h)  +  src/robocar/ (.cpp)
  CarConfig.h            pins, geometry, PID, safety limits  <- edit this
  Motor                  one TB6612FNG channel (LEDC PWM 20 kHz)
  Encoder                quadrature decoding in the PCNT peripheral (x4)
  Pid                    PID with anti-windup
  WheelController        feed-forward + PID, accel ramp, stall detection
  MecanumKinematics      mixing and forward kinematics
  MecanumDrive           4 wheels + STBY + odometry
  CarLink                ESP-NOW: control in, telemetry out, pairing
  StatusLed              RGB LED patterns
  RoboCar                state machine, scheduling, telemetry
  main.cpp

Remote                   include/ (.h)  +  src/remote/ (.cpp)
  RemoteConfig.h         pins, joystick tuning, gears        <- edit this
  Joystick               centre calibration, dead zone, expo
  Button                 debounced buttons
  RemoteLink             ESP-NOW: control out, telemetry in
  Display                sprite-based UI
  RemoteApp              controls, arming logic, scheduling
  main.cpp
```

The shared libraries keep their headers in `lib/<name>/src/`. PlatformIO only
compiles a library when it finds the library's own headers there.

## Car wiring (YD-ESP32-S3)

| Function | GPIO | | Function | GPIO |
|---|---|---|---|---|
| Driver 1 PWMA (FL) | 8 | | Driver 2 PWMA (RL) | 14 |
| Driver 1 AIN1 / AIN2 | 9 / 10 | | Driver 2 AIN1 / AIN2 | 21 / 47 |
| Driver 1 PWMB (FR) | 11 | | Driver 2 PWMB (RR) | 2 |
| Driver 1 BIN1 / BIN2 | 12 / 13 | | Driver 2 BIN1 / BIN2 | 39 / 40 |
| STBY (both drivers) | 41 | | Battery sense (ADC1) | 1 |
| Encoder FL A / B | 4 / 5 | | Encoder RL A / B | 15 / 16 |
| Encoder FR A / B | 6 / 7 | | Encoder RR A / B | 17 / 18 |
| RGB LED (onboard) | 48 | | | |

- YD-ESP32-S3 board:
  - The onboard RGB LED is connected to GPIO48 only when the solder jumper marked **RGB** is bridged. If the LED stays dark, bridge it with a blob of solder.
  - For flashing and the serial log, see [Flashing the robocar](#flashing-the-robocar-com-or-usb-port).
  - When powering the board from the 5 V pin (e.g. a 5 V buck converter from the battery), check the **IN-OUT** jumper. Bridged, the 5 V pin and USB 5 V are connected directly, so don't plug USB into a powered car unless your converter can take it.
- TB6612FNG: `VM` = battery (max 13.5 V, so a 3S pack is fine), `VCC` = 3.3 V, all GNDs common.
- Battery divider: battery+ → 100 kΩ → GPIO1 → 22 kΩ → GND (ratio set in `CarConfig.h`).
- Encoders: power the Hall sensors from **3.3 V**. If they run on 5 V, their outputs will be 5 V, which can damage the ESP32-S3.
- The TB6612FNG is rated 1.2 A continuous / 3.2 A peak per channel. A stalled 25GA-370 can draw more than that, so stall detection cuts the motor after 1.2 s.

## Flashing the robocar: COM or USB port

The YD-ESP32-S3 has two USB-C connectors:

| Port | What it is | Use it for |
|---|---|---|
| **COM** | CH343 USB-to-serial chip wired to UART0 (GPIO43/44) | **Use this one** for flashing and the serial log |
| **USB** | the ESP32-S3's own (native) USB on GPIO19/20 | only if you switch to the `robocar_usb` environment |

**Using COM (recommended, no configuration needed).** The `robocar` environment already sends `Serial` to UART0, which is what the COM port shows. Plug a cable into **COM** and run:

```bash
pio run -e robocar -t upload
pio device monitor -e robocar
```

- The CH343 resets the board into the bootloader automatically, so you don't need to press BOOT or RST.
- On macOS the port shows up as `/dev/cu.usbmodem…` or `/dev/cu.wchusbserial…`. If it doesn't appear at all, install WCH's CH34x driver.
- If the remote is plugged in at the same time, set `upload_port` / `monitor_port` in `[env:robocar]` (commented examples are in `platformio.ini`) so PlatformIO doesn't pick the wrong board.

**Using USB (optional).** Use the `robocar_usb` environment, which adds `-DARDUINO_USB_CDC_ON_BOOT=1` so `Serial` goes to the native USB port:

```bash
pio run -e robocar_usb -t upload
pio device monitor -e robocar_usb
```

- If the very first upload over this port fails, hold **BOOT**, press and release **RST**, release **BOOT**, then upload again. After that, uploads work without the buttons.
- With `robocar_usb`, nothing is printed on the COM port, and the other way round. Pick one port and stick to it.

## Remote wiring (T-Display-S3)

| Joystick | GPIO |
|---|---|
| VRx | 1 |
| VRy | 2 |
| SW | 21 |
| +5V pin of module | **3V3** |
| GND | GND |

The joystick and LCD are already wired on the board. GPIO15 (LCD power) is switched on in firmware.

## Controls

| Input | Action |
|---|---|
| Stick forward/back | drive forward/back |
| Stick left/right | turn (DRIVE mode) or strafe sideways (STRAFE mode) |
| Stick press | toggle DRIVE / STRAFE |
| BOOT button (GPIO0) | arm / disarm. Arming needs a live link and a centred stick |
| KEY button (GPIO14) | speed gear 30 → 60 → 100 % |

## Remote display

The screen is 320 × 170 pixels in landscape (USB-C on the right) and refreshes 20 times a second. The car sends telemetry 10 times a second.

```
+----------------------------------------------------------------+
| [ DRIVING ]                            RX 10Hz    REM 4.05V    |  <- header
|                                                                 |
| CAR BATTERY                     | FL           55 | FR       55 |
| 11.84 V                         |      52.3       |     52.1    |
|                                 |  [    |=====  ] |  [  |===== ]|
| SPEED        ODOMETER           +-----------------+-------------+
| 0.18 m/s     12.4 m             | RL           55 | RR       55 |
|                                 |      51.9       |     52.4    |
| [ DRIVE    60% ]  (o)           |  [    |=====  ] |  [  |===== ]|
| [     ARMED     ]               |                 |             |
+----------------------------------------------------------------+
```

### Header (top bar)

| Item | Meaning |
|---|---|
| State box (left) | The car's state, in the same colours as the car's RGB LED: `WAITING` blue, `DISARMED` amber, `ARMED` green, `DRIVING` cyan, `LINK LOST` red, `LOW BATT` orange, `FAULT` magenta. Before the first telemetry arrives it shows `SEARCHING CAR` (blue). If telemetry stops for more than 600 ms it shows `NO SIGNAL` (blinking red). |
| `RX nnHz` | Telemetry packets received from the car per second. About 10 is normal. Green while the link is alive, red when it is not. |
| `REM x.xxV` | The remote's own battery voltage. Turns orange below 3.5 V. On USB power it shows the charger voltage instead. |

### Left panel

| Item | Meaning |
|---|---|
| `CAR BATTERY` | Battery voltage measured on the car. Green when OK, orange below the warning level (10.5 V), red when the car has cut the motors for low battery (below 9.9 V). Shows `--.-- V` if there is no reading (no telemetry yet, or battery sensing not wired). |
| `SPEED` | How fast the car actually moves, in m/s, calculated from the wheel encoders (includes sideways motion). |
| `ODOMETER` | Total distance driven since the car was switched on, in metres. |
| Mode box | `DRIVE` (cyan): stick left/right turns the car. `STRAFE` (yellow): stick left/right slides it sideways. The percentage is the speed gear. |
| Circle | The joystick position as the remote reads it, after dead zone and expo. The dot is red while armed, white otherwise. Useful to check that the stick is centred before arming. |
| Arm box | `ARMED` (red) when the motors can run. `SAFE` (grey) when disarmed. `SAFE - press ARM` means the car reports it needs a disarm before it will accept arming again. It shows briefly after the link comes back, and stays on while the car is in `LOW BATT`. |

### Wheel panels (right side)

Four panels arranged as the wheels sit on the car (FL, FR / RL, RR, seen from above).

| Item | Meaning |
|---|---|
| Name (top left) | `FL`, `FR`, `RL`, `RR`. Replaced by `STALL` with a magenta border when that wheel tripped the stall/encoder check. |
| Small number (top right) | Target wheel speed in rpm that the speed controller is aiming for. |
| Big number | Measured wheel speed in rpm, from the encoder. |
| Bar | Motor power (PWM duty). Grows right from the centre line for forward, left for reverse. Green normally, orange above 90 %: that motor is at its limit (heavy load or a low battery). |

### Greyed-out values

When the link is lost, the panels keep the last values received but draw them in grey, so it's clear they are not live.

### Pop-up messages

A message appears in the middle of the screen for 1.5 s:

| Message | When |
|---|---|
| `ARMED` / `DISARMED` | after pressing the BOOT button |
| `DRIVE: stick X turns` / `STRAFE: stick X slides` | after pressing the joystick |
| `NO LINK TO CAR` | arming refused: no telemetry from the car |
| `CENTRE THE STICK` | arming refused: the joystick is not centred |
| `CAR BATTERY LOW` | arming refused: the car is in low-battery state |
| `LINK LOST - DISARMED` | the remote disarmed itself because telemetry stopped |
| `WHEEL STALL - DISARMED` | the car reported a stalled wheel or encoder fault |
| `LOW BATTERY - DISARMED` | the car's battery fell below the critical level |
| `CAR NOT READY - REARM` | the car rejected arming (e.g. after a reboot); press ARM again |

### At power-up

The remote shows `MECANUM REMOTE / release the joystick...` for a moment while it measures the joystick's centre position, so don't touch the stick while switching it on. `RADIO ERROR` means ESP-NOW failed to start; restart the remote.

## Car RGB LED

| Colour | State |
|---|---|
| white | booting |
| blue, breathing | waiting for a remote |
| yellow, slow pulse | disarmed (link OK) |
| green | armed, standing |
| cyan | driving |
| red, fast blink | link lost: no control packet for 300 ms, motors braked |
| orange, 1 Hz blink | battery critical (< 9.9 V), motors disabled |
| magenta, double blink | wheel stall / encoder fault, motors disabled |
| short orange flash every 2 s | battery warning (< 10.5 V) |

## Safety behaviour

- The car moves only when the remote says *armed* and packets arrive at least every 300 ms.
- After boot, link loss, a fault or low battery, the car waits for one *disarmed* packet before it will accept *armed* again. The remote disarms itself in these cases, so press ARM again.
- Pairing: the car locks onto the first remote with the same `ROBO_PAIR_KEY` (in `platformio.ini`). Another remote can take over only after 3 s of silence. Change the key if other people run the same firmware nearby.

## First bring-up (wheels off the ground!)

1. Set `kClosedLoop = false` in `CarConfig.h`, arm, and push the stick forward. Every wheel must spin "forward". Flip `motorInverted` for any wheel that spins the wrong way.
2. Watch the serial log (`rpm FL=…`): forward rotation must give **positive** rpm. Flip `encoderInverted` per wheel if not.
3. Check the mecanum pattern: in STRAFE mode, pushing the stick right must make the car slide right. If it rotates or goes diagonally, the wheels are swapped (rollers must form an **X** when seen from above).
4. Set `kClosedLoop = true`. If wheels oscillate, lower `kPidKp`. If they reach the target slowly under load, raise `kPidKi`.
5. Measure the battery with a multimeter and set `kBatteryCalibration` so the screen matches.
6. Set `kWheelDiameterMm`, `kHalfWheelbaseMm` and `kHalfTrackMm` to your chassis so the speed and odometer are right.

## Notes

- The T-Display-S3 has an IPS **LCD** (ST7789), not an OLED. TFT_eSPI is configured entirely through `build_flags`, so its own `User_Setup.h` is ignored.
- Both boards use Wi-Fi channel 1 (`ROBO_WIFI_CHANNEL`). Change it on both if that channel is busy.
- Encoder resolution: 11 PPR × 4 edges × 103 = 4532 counts per wheel revolution.
