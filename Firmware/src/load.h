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
    : dac(dac), adc(adc), fan(fan), pwrEnPin(pwrEnPin),
      enabled(false), current(0.0) {

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

  /** Enable / Disable the Load */
  bool setEnabled(bool enabled) {
    if (this->enabled == enabled) {
      // no state change
      return true;
    }

    if (enabled) {
      // TODO: move power enable into a separate method
      digitalWrite(this->pwrEnPin, HIGH);
      // TODO: make delay time configurable
      delay(50);

    } else {
      // TODO: move power disable into a separate method
      digitalWrite(this->pwrEnPin, LOW);
      // TODO: make delay time configurable
      delay(50);
    }

    // save state
    this->enabled = enabled;

    return true;
  }

  /** Set the Load Current (in amps) */
  bool setCurrent(float current) {
    if ((current < 0.0) || (current > HardwareValues::MAX_TOTAL_CURRENT)) {
      // invalid set current value
      return false;
    }

    if (current > 0.0) {
      // auto-enable load when set current is >= 0.0A (TODO: make this configurable)
      this->setEnabled(true);
    }

    // set the DAC value
    uint16_t dacValue = current * HardwareValues::currentSetDacMultiplier;
    this->dac.set(dacValue);

    if (current == 0.0) {
      // auto-disable load when current is set to 0.0A (TODO: make this configurable)
      this->setEnabled(false);
    }

    // save state
    this->current = current;

    return true;
  }

  /** Set the Fan Speed (0.0 to 1.0) */
  bool setFanSpeed(float speed) {
    if (speed < 0.0 || speed > 1.0) {
      // invalid fan speed
      return false;
    }

    // set the fan speed
    this->fan.set(speed);

    // save state
    this->fanSpeed = speed;

    return true;
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

  /** Get Enabled state */
  bool isEnabled() {
    return this->enabled;
  }

  /** Get set current */
  float getSetCurrent() {
    return this->current;
  }

  /** Get fan speed */
  float getFanSpeed() {
    return this->fanSpeed;
  }

private:
  DAC &dac;
  ADC &adc;
  Fan &fan;
  const uint8_t pwrEnPin;

  /* State: */

  /** Load state (enabled/disabled) */
  bool enabled;

  /** Set current */
  float current;

  /** Fan speed */
  float fanSpeed;

};

#endif