/*
 * Copyright (c) 2025 by Attila Tőkés.
 *
 * Licence: MIT
 */
#include <Arduino.h>

#ifndef DAC_H
#define DAC_H

/** Digital to Analog Converter (DAC) implemented on hardware in R-2R configuration. */
class DAC {

public:

  const uint8_t nrPins;
  const uint8_t *pins;
  const uint16_t nrPresets;
  uint32_t *presetRawValues;

  /**
   * Instantiates the DAC on a given set of pins.
   *
   * The number of pins drives the resolution of the DAC.
   *
   * Optionally it can store a number of precomputed presets for fast writes.
   */
  DAC(const int nrPins, const uint8_t *pins, const uint16_t nrPresets)
    : nrPins(nrPins), pins(pins), nrPresets(nrPresets) {

      if (nrPresets > 0) {
        this->presetRawValues = new uint32_t[4 * nrPresets];
      }
  }

  /** Set the output to an analog value. */
  void set(uint16_t value) {
    uint32_t p1_set, p1_clear, p2_set, p2_clear;

    // calculate the raw GPIO pin values
    this->prepareRaw(value, p1_set, p1_clear, p2_set, p2_clear);

    // write the raw values to the GPIO pins
    this->setRaw(p1_set, p1_clear, p2_set, p2_clear);
  }

  /** Prepare a preset for an analog value. */
  void preparePreset(uint16_t preset, uint16_t value) {
    if (preset >= nrPresets) return;

    // precompute rae values for the given value
    this->prepareRaw(value,
        this->presetRawValues[4 * preset], this->presetRawValues[4 * preset +1],
        this->presetRawValues[4 * preset + 2], this->presetRawValues[4 * preset +3]);
  }

  /** Apply a preset. */
  void setPreset(uint16_t preset) {
    if (preset >= nrPresets) return;

    // write the precomputed raw values to the GPIO pins
    this->setRaw(this->presetRawValues[4 * preset], this->presetRawValues[4 * preset +1],
        this->presetRawValues[4 * preset + 2], this->presetRawValues[4 * preset +3]);
  }

private:

  /** Convert analog value to raw clear & set actions for the two GPIO ports. */
  void prepareRaw(uint16_t value, uint32_t &p1_set, uint32_t &p1_clear, uint32_t &p2_set, uint32_t &p2_clear) {
    p1_set = 0;
    p1_clear = 0;
    p2_set = 0;
    p2_clear = 0;

    for (auto nr = 0; nr < nrPins; nr++) {
      uint8_t pin = this->pins[nr];
      bool bitVal = value % 2;
      value = value >> 1;
      if (pin <= 31) {
        if (bitVal == 1) {
          p1_set |= ((uint32_t) 1 << pin);
        } else {
          p1_clear |= ((uint32_t) 1 << pin);
        }
      } else {
        if (bitVal == 1) {
          p2_set |= ((uint32_t) 1 << (pin - 32));
        } else {
          p2_clear |= ((uint32_t) 1 << (pin - 32));
        }
      }
    }
  }

  /** Set raw clear & set flags for the two GPIO ports */
  void setRaw(uint32_t p1_set, uint32_t p1_clear, uint32_t p2_set, uint32_t p2_clear) {
    if (p1_clear > 0) GPIO.out_w1tc = p1_clear;
    if (p2_clear > 0) GPIO.out1_w1tc.val = p2_clear;
    if (p1_set > 0) GPIO.out_w1ts = p1_set;
    if (p2_set > 0) GPIO.out1_w1ts.val = p2_set;
  }
};

#endif