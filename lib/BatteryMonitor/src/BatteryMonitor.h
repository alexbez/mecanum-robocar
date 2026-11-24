#pragma once
// Battery voltage measurement through a resistor divider on an ADC1 pin.

#include <stdint.h>

class BatteryMonitor {
 public:
  // dividerRatio = (Rtop + Rbottom) / Rbottom, e.g. 100k/22k -> 5.545
  BatteryMonitor(uint8_t adcPin, float dividerRatio, float calibration = 1.0f);

  void begin();

  // Takes a sample and updates the filtered value. Call every ~20-100 ms.
  void update();

  uint16_t millivolts() const { return static_cast<uint16_t>(filteredMv_ + 0.5f); }

  // A reading below this is treated as "no battery sense wired".
  bool present(uint16_t minPlausibleMv = 3000) const { return millivolts() >= minPlausibleMv; }

 private:
  float sampleMv() const;

  uint8_t pin_;
  float ratio_;
  float calibration_;
  float filteredMv_ = 0.0f;
  bool primed_ = false;
};
