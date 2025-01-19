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
#include "fan.h"
#include "hw.h"

/** Digital to Analog Converter (DAC) implemented on hardware in R-2R configuration. */
class Load {

public:

  /**
   * Instantiates the Electronic Load.
   */
  Load(DAC &dac, ADC &adc, Fan &fan)
    : dac(dac), adc(adc), fan(fan) {
  }

  void handle() {
    this->adc.handle();
  }

  /** Get the Load Voltage (in volts). */
  float getLoadVoltage() {
     uint16_t loadVoltageRaw = this->getLoadVoltageRaw();
     return loadVoltageRaw * HardwareValues::loadVoltageAdcMultiplier;
  }

  /** Get the full Load Current (in amps). */
  float getLoadCurrent() {
    return this->getLoadCurrent1() + this->getLoadCurrent2();
  }

  /** Get the Load Current on channel one (in amps). */
  float getLoadCurrent1() {
    uint16_t loadCurrentRaw1 = this->adc.getMilliVolts(1);
    return loadCurrentRaw1 * HardwareValues::currentSenseAdcMultiplier;
  }

  /** Get the Load Current on channel two (in amps). */
  float getLoadCurrent2() {
    uint16_t loadCurrentRaw2 = this->adc.getMilliVolts(2);
    return loadCurrentRaw2 * HardwareValues::currentSenseAdcMultiplier;
  }

  /** Set the Load Current (in amps) */
  void setCurrent(float current) {
    if (current < 0.0) return;

    uint16_t dacValue = current * HardwareValues::currentSetDacMultiplier / NR_CHANNELS;
    this->dac.set(dacValue);
  }

  /** Set the Fan Speed (0.0 to 1.0) */
  void setFanSpeed(float speed) {
    if (speed < 0.0 || speed > 1.0) return;

    this->fan.set(speed);
  }

  /** Get Temperature (in Celsius). */
  float getTemperature() {
    uint16_t tempMilliVolts = this->getTemperatureRaw();

    // RT = R1 / (Vin / Vout - 1) = 10 kOhm / (3.3V / Vin - 1)
    float rTherm = 10000.0 / (3300.0 / tempMilliVolts - 1.0);
    float rThermLog = log(rTherm);

    float tempK = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * rThermLog * rThermLog)) * rThermLog);
    float tempC = tempK - 273.15;

    return tempC;
    //return (float) tempMilliVolts;
  }

  /** Get the raw Load Voltage reading (in millivolts) */
  uint16_t getLoadVoltageRaw() {
    return this->adc.getMilliVolts(0);
  }

  uint16_t getLoadCurrentRaw1() {
    return this->adc.getMilliVolts(1);
  }

  uint16_t getLoadCurrentRaw2() {
    return this->adc.getMilliVolts(2);
  }

  /** Get the raw Temperature reading (in millivolts) */
  uint16_t getTemperatureRaw() {
    return this->adc.getMilliVolts(3);
  }

private:
  DAC &dac;
  ADC &adc;
  Fan &fan;

  /** Nr channels */
  static constexpr float NR_CHANNELS = 2.0;
};

#endif