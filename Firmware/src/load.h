/*
 * Copyright (c) 2025 by Attila Tőkés.
 *
 * Licence: MIT
 */
#ifndef LOAD_H
#define LOAD_H

#include <Arduino.h>
#include "dac.h"
#include "adc.h"

/** Digital to Analog Converter (DAC) implemented on hardware in R-2R configuration. */
class Load {

public:

  /**
   * Instantiates the Electronic Load.
   */
  Load(DAC &dac, ADC &adc)
    : dac(dac), adc(adc) {
  }

  void handle() {
    this->adc.handle();
  }

  uint16_t getLoadVoltage() {
    return this->adc.get(0);
  }

  uint16_t getLoadCurrent() {
    return this->getLoadCurrent1() + this->getLoadCurrent2();
  }

  uint16_t getLoadCurrent1() {
    return this->adc.get(2);
  }

  uint16_t getLoadCurrent2() {
    return this->adc.get(1);
  }

  float getTemperature() {
    uint16_t tempMilliVolts = this->adc.get(3);

    // RT = R1 / (Vin / Vout - 1) = 10 kOhm / (3.3V / Vin - 1)
    float rTherm = 10000.0 / (3300.0 / tempMilliVolts - 1.0);
    float rThermLog = log(rTherm);

    float tempK = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * rThermLog * rThermLog)) * rThermLog);
    float tempC = tempK - 273.15;

    return tempC;
    //return (float) tempMilliVolts;
  }

  void setCurrent(uint16_t value) {
    this->dac.set(value);
  }

private:
  DAC &dac;
  ADC &adc;
};

#endif