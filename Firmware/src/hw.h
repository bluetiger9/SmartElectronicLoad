/*
 * Copyright (c) 2025 by Attila Tőkés.
 *
 * Licence: MIT
 */
#ifndef HW_H
#define HW_H

#include <Arduino.h>

/** Hardware Value */
class HardwareValues {

public:

  /** Millivolts to Volts */
  static constexpr float MILLIVOLTS_TO_VOLTS = (1.0) / (1000.0);

  /** Load Voltage ADC voltage divider resistors */
  static constexpr float V_LOAD_DIVIDER_R_UP = 47000.0;    // R_up = 47 kOhm
  static constexpr float V_LOAD_DIVIDER_R_DOWN = 1000.0;   // R_down = 1 kOhm

  /** Load Voltage ADC Multiplier **/
  static float loadVoltageAdcMultiplier;

  /**
   * Init the Load Voltage ADC Multiplier.
   *
   * V_adc = V_load * R_down / (R_down + R_up)
   * V_load = V_adc * (R_down + R_up) / R_down
   *
   **/
  static void initLoadVoltageAdcMultiplier() {
    loadVoltageAdcMultiplier = (V_LOAD_DIVIDER_R_UP + V_LOAD_DIVIDER_R_DOWN)
                             / (V_LOAD_DIVIDER_R_DOWN) * MILLIVOLTS_TO_VOLTS;
  }

  /** Current Sense OpAmp Multiplier */
  static constexpr float C_SENSE_MULTIPLIER = 20.0; // TODO: Make this configurable

  /** Load Current Sense ADC voltage divider resistors */
  static constexpr float C_SENSE_DIVIDER_R_UP = 33000.0;   // R_up = 33 kOhm
  static constexpr float C_SENSE_DIVIDER_R_DOWN = 10000.0; // R_down = 10 kOhm

  /** Load Current Sense Resistor */
#if defined ESP32_S3                              // TODO: Make this a config value
  static constexpr float C_SENSE_RESISTOR = 0.010;
#elif defined ESP32_S2
  static constexpr float C_SENSE_RESISTOR = 0.100;
#endif

  /** Current Sense ADC Multiplier */
  static float currentSenseAdcMultiplier;

  /**
   * Current Sense ADC Multiplier
   *
   * R_sense = 0.100 Ohm
   * Multiplier (C sense) = x 20
   * R1 (upper) = 33 kOhm, R2 (lower) = 10 kOhm
   *
   * C_load = V_adc * (R2 + R1) / (R2 * Multiplier * R_sense)
   */
  static void initCurrentSenseAdcMultiplier() {
    currentSenseAdcMultiplier =  (C_SENSE_DIVIDER_R_DOWN + C_SENSE_DIVIDER_R_UP)
                               / (C_SENSE_DIVIDER_R_DOWN * C_SENSE_MULTIPLIER * C_SENSE_RESISTOR) * MILLIVOLTS_TO_VOLTS;
  }


  /** DAC Max Value */
  static constexpr uint16_t DAC_MAX_VALUE = (4096 - 1);

  /** DAC Supply Voltage */
  static constexpr float DAC_SUPPLY_VOLTAGE = 3.3;

  /** DAC OpAmp Multiplier (3.3V to 10V) */
  static constexpr float DAC_MULTIPLIER = 3.0;

  /** Volts to DAC Multiplier */
  static constexpr float VOLTS_TO_DAC = ((float) DAC_MAX_VALUE) / (3.3);


  /**
   * Current Set DAC Multiplier
   *
   * R_sense = 0.100 Ohm
   * Multiplier (DAC) = x 3.0
   * Multiplier (sense) = x 20
   *
   *
   * V_dac = Mult_sense * C_set * R_sense / Mult_dac
   */
  static constexpr float C_SET_DAC_MULTIPLIER = (20.0 * 0.100) / DAC_MULTIPLIER * VOLTS_TO_DAC;

  /** Current Set DAC Multiplier */
  static float currentSetDacMultiplier;

  /**
   * Current Set DAC Multiplier
   *
   * V_dac = C_set * (Mult_sense * R_sense / Mult_dac)
   */
  static void initCurrentSetDacMultiplier() {
    currentSetDacMultiplier = (C_SENSE_MULTIPLIER * C_SENSE_RESISTOR / DAC_MULTIPLIER) * VOLTS_TO_DAC;
  }

  static void init() {
    initLoadVoltageAdcMultiplier();
    initCurrentSenseAdcMultiplier();
    initCurrentSetDacMultiplier();
  }

private:
  HardwareValues() {};
};

#endif