/*
 * Copyright (c) 2025 by Attila Tőkés.
 *
 * Licence: MIT
 */
#include <Arduino.h>

#ifndef FAN_H
#define FAN_H

/** Digital to Analog Converter (DAC) implemented on hardware in R-2R configuration. */
class Fan {

public:

  const uint8_t pin;
  const uint16_t max;

  /**
   * Instantiates the Fan PWN control on a given pin.
   *
   */
  Fan(const uint8_t pin, const uint16_t max)
    : pin(pin), max(max) {
  }

  /** Set the output to an analog value. */
  void set(float value) {
    analogWrite(this->pin, (1.0 - value) * this->max);
  }

private:
};

#endif