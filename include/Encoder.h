#pragma once
// Quadrature encoder read by the ESP32-S3 hardware pulse counter (PCNT).
//
// Counts all four edges of A and B (x4 decoding), so no CPU interrupts are
// used. The 16-bit hardware counter is extended to 32 bits in software;
// call read() at least every ~5 s (it is called at 100 Hz).

#include <stdint.h>

class Encoder {
 public:
  Encoder(uint8_t pinA, uint8_t pinB, uint8_t pcntUnit, bool inverted);

  bool begin();

  // Total signed count since begin().
  int32_t read();

 private:
  uint8_t pinA_;
  uint8_t pinB_;
  uint8_t unit_;
  bool inverted_;
  int16_t lastRaw_ = 0;
  int32_t total_ = 0;
};
