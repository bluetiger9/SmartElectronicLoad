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
  Load(DAC &dac, ADC &adc, Fan &fan, uint8_t pwrEnPin)
    : dac(dac), adc(adc), fan(fan), pwrEnPin(pwrEnPin) {

      digitalWrite(this->pwrEnPin, LOW);
  }

  void handle() {
    this->adc.handle();
  }

  /** Get the Load Voltage (in volts). */
  float getLoadVoltage() {
    // get the load voltage from the 1st division stage
    uint16_t loadVoltageRaw1 = this->getLoadVoltage1Raw();
    if (loadVoltageRaw1 <= 2500) {
      // bellow ~2.5V threshold => use the 1st division stage
      return loadVoltageRaw1 * HardwareValues::loadVoltageAdcMultiplier1;

    } else {
      // above ~2.5V threshold the 1st division stage may be saturated => use the 2nd division stage
      uint16_t loadVoltageRaw2 = this->getLoadVoltage2Raw();
      return loadVoltageRaw2 * HardwareValues::loadVoltageAdcMultiplier2;
    }
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
    if (HardwareValues::NR_CHANNELS <= 1.0) {
      // ignore the 2nd channel (reduses noise when only 1 channel is used)
      return 0.0;
    }

    uint16_t loadCurrentRaw2 = this->adc.getMilliVolts(2);
    return loadCurrentRaw2 * HardwareValues::currentSenseAdcMultiplier;
  }

  /** Set the Load Current (in amps) */
  void setCurrent(float current) {
    if (current < 0.0) return;

    if (current > HardwareValues::MAX_TOTAL_CURRENT) {
      return;
    }

    if (current > 0.0) {
      // TODO: move power enable into a separate method
      digitalWrite(this->pwrEnPin, HIGH);
      // TODO: make delay time configurable
      delay(50);
    }

    uint16_t dacValue = current * HardwareValues::currentSetDacMultiplier;
    this->dac.set(dacValue);

    if (current == 0.0) {
      // TODO: move power disable into a separate method
      digitalWrite(this->pwrEnPin, LOW);
      // TODO: make delay time configurable
      delay(50);
    }
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

  /** Get the raw Load Voltage reading at the 1st division stage (in millivolts) */
  uint16_t getLoadVoltage1Raw() {
    return this->adc.getMilliVolts(0);
  }

  /** Get the raw Load Voltage reading at the 2nd division stage (in millivolts) */
  uint16_t getLoadVoltage2Raw() {
    return this->adc.getMilliVolts(4);
  }

  /** Get the raw Load Current reading (in millivolts) */
  uint16_t getLoadCurrentRaw1() {
    return this->adc.getMilliVolts(1);
  }

  /** Get the raw Load Current reading (in millivolts) */
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
  const uint8_t pwrEnPin;
};

#endif