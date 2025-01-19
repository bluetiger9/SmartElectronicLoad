/*
 * Copyright (c) 2025 by Attila Tőkés.
 *
 * Licence: MIT
 */
#ifndef SRV_H
#define SRV_H

#include <Arduino.h>
#include "dac.h"

/** Test / service functionality */
class Service {

public:

  /**
   * Instantiates the Web Server.
   */
  Service(DAC &dac)
    : dac(dac) {
  }

  void dacSet(uint16_t value) {
    dac.set(value);
  }

  void dacSwipe() {
    // swipe up
    for (int16_t value = 0; value < 4096; value++) {
      dac.set(value);
    }

    // swipe down
    for (int16_t value = 4096; value >= 0; value--) {
      dac.set(value);
    }
  }

private:
  DAC& dac;



};

#endif