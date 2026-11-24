#include "BatteryMonitor.h"

#include <Arduino.h>

BatteryMonitor::BatteryMonitor(uint8_t adcPin, float dividerRatio, float calibration)
    : pin_(adcPin), ratio_(dividerRatio), calibration_(calibration) {}

void BatteryMonitor::begin() {
  pinMode(pin_, INPUT);
  analogSetPinAttenuation(pin_, ADC_11db);
  filteredMv_ = sampleMv();
  primed_ = true;
}

float BatteryMonitor::sampleMv() const {
  // Average a few calibrated readings to reduce motor noise.
  uint32_t sum = 0;
  constexpr int kSamples = 8;
  for (int i = 0; i < kSamples; ++i) sum += analogReadMilliVolts(pin_);
  return (static_cast<float>(sum) / kSamples) * ratio_ * calibration_;
}

void BatteryMonitor::update() {
  const float mv = sampleMv();
  if (!primed_) {
    filteredMv_ = mv;
    primed_ = true;
    return;
  }
  // Exponential filter: rejects short sags when motors start.
  filteredMv_ += 0.1f * (mv - filteredMv_);
}
