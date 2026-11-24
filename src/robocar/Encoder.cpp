#include "Encoder.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/pcnt.h>

namespace {
// The PCNT counter resets to 0 when it reaches either limit, so the raw value
// is always the true count modulo kLimit. Deltas are unwrapped in read().
constexpr int16_t kLimit = 30000;
constexpr uint16_t kGlitchFilterApbCycles = 800;  // 10 us at 80 MHz
}  // namespace

Encoder::Encoder(uint8_t pinA, uint8_t pinB, uint8_t pcntUnit, bool inverted)
    : pinA_(pinA), pinB_(pinB), unit_(pcntUnit), inverted_(inverted) {}

bool Encoder::begin() {
  const auto unit = static_cast<pcnt_unit_t>(unit_);

  // Channel 0: count edges on A, direction from B.
  pcnt_config_t c0 = {};
  c0.pulse_gpio_num = pinA_;
  c0.ctrl_gpio_num = pinB_;
  c0.channel = PCNT_CHANNEL_0;
  c0.unit = unit;
  c0.pos_mode = PCNT_COUNT_DEC;
  c0.neg_mode = PCNT_COUNT_INC;
  c0.lctrl_mode = PCNT_MODE_REVERSE;
  c0.hctrl_mode = PCNT_MODE_KEEP;
  c0.counter_h_lim = kLimit;
  c0.counter_l_lim = -kLimit;

  // Channel 1: count edges on B, direction from A.
  pcnt_config_t c1 = c0;
  c1.pulse_gpio_num = pinB_;
  c1.ctrl_gpio_num = pinA_;
  c1.channel = PCNT_CHANNEL_1;
  c1.pos_mode = PCNT_COUNT_INC;
  c1.neg_mode = PCNT_COUNT_DEC;

  if (pcnt_unit_config(&c0) != ESP_OK) return false;
  if (pcnt_unit_config(&c1) != ESP_OK) return false;

  // Hall sensor outputs are usually open-collector: enable pull-ups.
  gpio_pullup_en(static_cast<gpio_num_t>(pinA_));
  gpio_pullup_en(static_cast<gpio_num_t>(pinB_));

  pcnt_set_filter_value(unit, kGlitchFilterApbCycles);
  pcnt_filter_enable(unit);
  pcnt_counter_pause(unit);
  pcnt_counter_clear(unit);
  pcnt_counter_resume(unit);

  lastRaw_ = 0;
  total_ = 0;
  return true;
}

int32_t Encoder::read() {
  int16_t raw = 0;
  pcnt_get_counter_value(static_cast<pcnt_unit_t>(unit_), &raw);

  int32_t delta = static_cast<int32_t>(raw) - lastRaw_;
  if (delta > kLimit / 2) delta -= kLimit;
  if (delta < -kLimit / 2) delta += kLimit;
  lastRaw_ = raw;

  total_ += inverted_ ? -delta : delta;
  return total_;
}
