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

#include "hal/gpio_hal.h"

/** Main Electronic Load */
class Load {

public:

  enum Mode {
    CONSTANT_CURRENT,
    CONSTANT_POWER,
    CONSTANT_RESISTANCE
  };

  enum ProtectState {
    OK,
    OK_DISABLED, // DISABLED (conflict with the #DISABLED macro)
    TRIPPED_OVER_TEMPERATURE,
    TRIPPED_OVER_VOLTAGE,
    TRIPPED_OVER_CURRENT,
    TRIPPED_OVER_POWER
  };

  /**
   * Instantiates the Electronic Load.
   */
  Load(DAC &dac, ADC &adc, Fan &fan, uint8_t pwrEnPin)
    : dac(dac), adc(adc), fan(fan), pwrEnPin(pwrEnPin),
      enabled(false), mode(CONSTANT_CURRENT), current(0.0), power(0.0), resistance(10000000.0), fanSpeed(0.0) {

      digitalWrite(this->pwrEnPin, LOW);
  }

  void handle() {
    this->adc.handle();

    if (this->adc.lastReadTimeMicros > this->lastAdcTimestamp) {
      // new ADC data available (this will run at ~4kHz rate)

      // adjust load current based on the operating mode
      if (this->mode == CONSTANT_POWER) {
        this->adjustLoadCurrentForPower();

      } else if (this->mode == CONSTANT_RESISTANCE) {
        this->adjustLoadCurrentForResistance();
      }

      // check protections
      this->checkProtections();

      // save the last processed ADC timestamp
      this->lastAdcTimestamp = this->adc.lastReadTimeMicros;
    }
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

    if ((enabled) && (this->protectionState > OK_DISABLED)) {
      // cannot enable when in tripped state
      return false;
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

  /** Set operating mode */
  bool setMode(Mode mode) {
    if (this->mode == mode) {
      // no state change
      return true;
    }

    // set default values
    this->power = 0.0;
    this->resistance = 10000000.0;
    this->setCurrent(0.0);

    switch (mode)
    {
    case CONSTANT_CURRENT:
      this->mode = CONSTANT_CURRENT;
      this->setCurrent(0.0);
      break;

    case CONSTANT_POWER:
      this->mode = CONSTANT_POWER;
      break;

    case CONSTANT_RESISTANCE:
      this->mode = CONSTANT_RESISTANCE;
      break;

    default:
      break;
    }

    return true;
  }

  /** Set the Load Current (in amps) */
  bool setCurrent(float current, bool checkMode = true) {
    if ((current < 0.0) || (current > HardwareValues::MAX_TOTAL_CURRENT)) {
      // invalid set current value
      return false;
    }

    if ((checkMode == true) && (this->mode != CONSTANT_CURRENT)) {
      // invalid mode for setting current
      return false;
    }

    if ((current > 0.0) && (this->protectionState > OK_DISABLED)) {
      // cannot set current when in tripped state
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

  /** Set the Load Power (in watts) */
  bool setPower(float power) {
    if ((power < 0.0) || (power > HardwareValues::MAX_TOTAL_POWER)) {
      // invalid set power value
      return false;
    }

    if ((power > 0.0) && (this->protectionState > OK_DISABLED)) {
      // cannot set power when in tripped state
      return false;
    }

    if ((this->mode != CONSTANT_POWER)) {
      // invalid mode for setting power
      return false;
    }

    // save state
    this->power = power;

    // adjust the load current to maintain the set power
    this->adjustLoadCurrentForPower();

    return true;
  }

  /** Set the Resistance (in ohms) */
  bool setResistance(float resistance) {
    if ((resistance < HardwareValues::MIN_TOTAL_RESISTANCE)) {
      // invalid set resistance value
      return false;
    }

    if ((this->mode != CONSTANT_RESISTANCE)) {
      // invalid mode for setting resistance
      return false;
    }

    // save state
    this->resistance = resistance;

    // adjust the load current to maintain the set resistance
    this->adjustLoadCurrentForResistance();

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

  /** Get Operating Mode */
  Mode getMode() {
    return this->mode;
  }

  /** Get set current */
  float getSetCurrent() {
    return this->current;
  }

  /** Get set power */
  float getSetPower() {
    return this->power;
  }

  /** Get set resistance */
  float getSetResistance() {
    return this->resistance;
  }

  /** Get fan speed */
  float getFanSpeed() {
    return this->fanSpeed;
  }

  /** Get Protection State */
  ProtectState getProtectState() {
    return this->protectionState;
  }

  /** Set over temperature limit */
  bool setOverTemperatureLimit(float temp) {
    if (temp < 0.0) {
      // invalid over temperature value
      return false;
    }

    this->overTempC = temp;
    return true;
  }

  /** Set over current limit */
  bool setOverCurrentLimit(float current) {
    if (current < 0.0) {
      // invalid over current value
      return false;
    }

    this->overCurrentA = current;
    return true;
  }

  /** Set over voltage limit */
  bool setOverVoltageLimit(float voltage) {
    if (voltage < 0.0) {
      // invalid over voltage value
      return false;
    }

    this->overVoltageV = voltage;
    return true;
  }

  /** Set over power limit */
  bool setOverPowerLimit(float power) {
    if (power < 0.0) {
      // invalid over power value
      return false;
    }

    this->overPowerW = power;
    return true;
  }

  /** Get over temperature limit */
  float getOverTemperatureLimit() {
    return this->overTempC;
  }

  /** Get over current limit */
  float getOverCurrentLimit() {
    return this->overCurrentA;
  }

  /** Get over voltage limit */
  float getOverVoltageLimit() {
    return this->overVoltageV;
  }

  /** Get over power limit */
  float getOverPowerLimit() {
    return this->overPowerW;
  }

  /** Reset tripped protections */
  bool resetProtections() {
    this->protectionState = OK;
    return true;
  }

  /** Enable protections */
  bool enableProtections(bool enable = true) {
    if (this->protectionState > OK_DISABLED) {
      // tripped protections should be reset first
      return false;
    }

    this->protectionState = enable ? OK : OK_DISABLED;

    return true;
  }

private:
  DAC &dac;
  ADC &adc;
  Fan &fan;
  const uint8_t pwrEnPin;

  /* State: */

  /** Load state (enabled/disabled) */
  bool enabled;

  /** Operating mode */
  Mode mode;

  /** Set current */
  float current;

  /** Set power */
  float power;

  /** Set resistance */
  float resistance;

  /** Fan speed */
  float fanSpeed;

  /** Over temperature protection */
  float overTempC = 80.0;

  /** Over current protection */
  float overCurrentA = 1.2 * HardwareValues::MAX_TOTAL_CURRENT;

  /** Over voltage protection */
  float overVoltageV = 100.0;

  /** Over power protection */
  float overPowerW = 400.0;

  /** Protection state */
  ProtectState protectionState = OK;

  /** Last ADC timestamp processed */
  uint64_t lastAdcTimestamp = 0;

  /** Adjust Load Current to maintain the set Power */
  void adjustLoadCurrentForPower() {
    float voltage = this->getLoadVoltage();
    if (voltage > 0.0) {
      float setCurrent = this->power / voltage;
      this->setCurrent(setCurrent, false);
    }
  }

  /** Adjust Load Current to maintain the set Resistance */
  void adjustLoadCurrentForResistance() {
    float voltage = this->getLoadVoltage();
    float setCurrent = voltage / this->resistance;
    this->setCurrent(setCurrent, false);
  }

  /** Check protections */
  void checkProtections() {
    if (this->protectionState != OK) {
      // already tripped or disabled
      return;
    }

    // over temperature
    if (this->overTempC > 0.0) {
      float temperature = this->getTemperature();
      if (temperature >= this->overTempC) {
        tripped(TRIPPED_OVER_TEMPERATURE);
        return;
      }
    }

    // over voltage
    if (this->overVoltageV > 0.0) {
      float voltage = this->getLoadVoltage();
      if (voltage >= this->overVoltageV) {
        tripped(TRIPPED_OVER_VOLTAGE);
        return;
      }
    }

    // over current
    if (this->overCurrentA > 0.0) {
      float current = this->getLoadCurrent();
      if (current >= this->overCurrentA) {
        tripped(TRIPPED_OVER_CURRENT);
        return;
      }
    }

    // over power
    if (this->overPowerW > 0.0) {
      float voltage = this->getLoadVoltage();
      float current = this->getLoadCurrent();
      float power = voltage * current;
      if (power >= this->overPowerW) {
        tripped(TRIPPED_OVER_POWER);
        return;
      }
    }
  }

  void tripped(ProtectState state) {
    // set current to 0A
    this->setCurrent(0.0, false);
    this->power = 0.0;
    this->resistance = 10000000.0;

    // disable load
    this->setEnabled(false);

    // set state
    this->protectionState = state;
  }

};

#endif