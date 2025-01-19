/*
 * Copyright (c) 2025 by Attila Tőkés.
 *
 * Licence: MIT
 */
#include <Arduino.h>

#ifndef ADC_H
#define ADC_H

/** Analog to Digital Converter (ADC) implemented on the ESP32-S3 chip. */
class ADC {

public:

  const uint8_t nrChannels;
  const uint8_t *pins;
  uint16_t *values;

  ADC(const uint8_t nrChannels, const uint8_t *pins)
    : nrChannels(nrChannels), pins(pins) {

      this->values = new uint16_t[nrChannels];

      analogReadResolution(12);
      analogSetAttenuation(ADC_11db);
  }

  void handle() {
    for (uint8_t chan = 0; chan < this->nrChannels; chan++) {
      //this->values[chan] = analogRead(this->pins[chan]);
      //this->values[chan] = analogReadRaw(this->pins[chan]);
      this->values[chan] = analogReadMilliVolts(this->pins[chan]);
    }
  }

  uint16_t getRaw(uint8_t chan) {
    return this->values[chan];
  }

  uint16_t getMilliVolts(uint8_t chan) {
    // note: raw currently are current in millivolts
    return this->values[chan];
  }

private:
};

#endif