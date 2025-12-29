/*
 * Copyright (c) 2025 by Attila Tőkés.
 *
 * Licence: MIT
 */
#ifndef SRV_H
#define SRV_H

#include <Arduino.h>
#include <EEPROM.h>
#include "dac.h"

extern volatile uint8_t restartRequest;

/** Test / service functionality */
class Service {

public:

  /**
   * Instantiates the Web Server.
   */
  Service(DAC &dac, ADC &adc)
    : dac(dac), adc(adc) {
  }

  void dacSet(uint16_t value) {
    dac.set(value);
  }

  void dacSwipe() {
    int16_t maxValue = (1 << dac.nrPins) - 1;

    // swipe up
    for (int16_t value = 0; value <= maxValue; value++) {
      dac.set(value);
    }

    // swipe down
    for (int16_t value = maxValue; value >= 0; value--) {
      dac.set(value);
    }
  }

  void progModeRestart() {
    Serial.println("Requesting programming mode restart...");
    restartRequest = 2;
  }

  void restart() {
    Serial.println("Requesting normal restart...");
    restartRequest = 1;
  }

private:
  DAC& dac;
  ADC& adc;
};

#endif