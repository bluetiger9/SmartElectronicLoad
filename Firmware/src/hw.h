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
  static constexpr float V_LOAD_DIVIDER_R_UP = 100000.0;    // R_up = 100 kOhm
  static constexpr float V_LOAD_DIVIDER_R_MIDDLE = 20000.0;    // R_middle = 20 kOhm
#if defined ESP32_S3                              // TODO: Make this a config value
  static constexpr float V_LOAD_DIVIDER_R_DOWN = 2200.0;   // R_down = 2.2 kOhm
  #elif defined ESP32_S2
  static constexpr float V_LOAD_DIVIDER_R_DOWN = 4700.0;   // R_down = 4.7 kOhm
#endif

  /** Load Voltage ADC Multiplier (lower range) **/
  static float loadVoltageAdcMultiplier1;

  /** Load Voltage ADC Multiplier (upper range) **/
  static float loadVoltageAdcMultiplier2;

  /**
   * Init the Load Voltage ADC Multiplier.
   *
   * V_adc = V_load * R_down / (R_down + R_up)
   * V_load = V_adc * (R_down + R_up) / R_down
   *
   **/
  static void initLoadVoltageAdcMultiplier() {
    //TODO: 1x (direct voltage measurement, capped to 3.3V)
    loadVoltageAdcMultiplier1 = (V_LOAD_DIVIDER_R_UP + V_LOAD_DIVIDER_R_MIDDLE + V_LOAD_DIVIDER_R_DOWN)
                            / (V_LOAD_DIVIDER_R_MIDDLE + V_LOAD_DIVIDER_R_DOWN) * MILLIVOLTS_TO_VOLTS;
    loadVoltageAdcMultiplier2 = (V_LOAD_DIVIDER_R_UP + V_LOAD_DIVIDER_R_MIDDLE + V_LOAD_DIVIDER_R_DOWN)
                            / (V_LOAD_DIVIDER_R_DOWN) * MILLIVOLTS_TO_VOLTS;
  }

  /** Current Sense OpAmp Multiplier */
  static constexpr float C_SENSE_MULTIPLIER = 10.0; // TODO: Make this configurable

  /** Load Current Sense ADC voltage divider resistors */
  static constexpr float C_SENSE_DIVIDER_R_UP = 22.0;   // R_up = 22 Ohm
  static constexpr float C_SENSE_DIVIDER_R_DOWN = -1.0; // R_down = not populated

  /** Load Current Sense Resistor */
#if defined ESP32_S3                              // TODO: Make this a config value
  static constexpr float C_SENSE_RESISTOR = 0.010;
#elif defined ESP32_S2
  static constexpr float C_SENSE_RESISTOR = 0.010;
#endif

  /** Number of Channels */
#if defined ESP32_S3                              // TODO: Make this a config value
  static constexpr float NR_CHANNELS = 2.0;
#elif defined ESP32_S2
  static constexpr float NR_CHANNELS = 1.0;
#endif

  /**
   * Max Current Allowed per Channel (in Amps)
   *
   * Set is based on either the MOSFET current limits
   * or Current Sense resistor values.
   *
   * I_max = V_ref / (R_sense * Multiplier)
   * V_ref = 3.3V - minus margin (ex 2.5V)
   *
   */
  static constexpr float MAX_CURRENT_PER_CHANNEL = 25.0;

  static constexpr float MAX_TOTAL_CURRENT = MAX_CURRENT_PER_CHANNEL * NR_CHANNELS;

  /** Current Sense ADC Multiplier */
  static float currentSenseAdcMultiplier;

  /**
   * Current Sense ADC Multiplier
   *
   * R_sense = 0.100 Ohm / 0.010 Ohm
   * Multiplier (C sense) = x 20
   * R1 (upper) = 33 kOhm, R2 (lower) = 10 kOhm
   *
   * C_load = V_adc * (R2 + R1) / (R2 * Multiplier * R_sense)
   */
  static void initCurrentSenseAdcMultiplier() {
    if (C_SENSE_DIVIDER_R_DOWN <= 0.0) {
      // no voltage divider
      currentSenseAdcMultiplier = 1 / (C_SENSE_MULTIPLIER * C_SENSE_RESISTOR) * MILLIVOLTS_TO_VOLTS;

    } else {
      // with voltage divider
      currentSenseAdcMultiplier =  (C_SENSE_DIVIDER_R_DOWN + C_SENSE_DIVIDER_R_UP)
                                 / (C_SENSE_DIVIDER_R_DOWN * C_SENSE_MULTIPLIER * C_SENSE_RESISTOR) * MILLIVOLTS_TO_VOLTS;
    }

  }


  /** DAC Max Value */
  static constexpr uint16_t DAC_MAX_VALUE = ((1 << 14) - 1);

  /** DAC Supply Voltage */
  static constexpr float DAC_SUPPLY_VOLTAGE = 3.3;

  /** DAC OpAmp Multiplier (3.3V to 10V) */
  static constexpr float DAC_MULTIPLIER = 1.0;

  /** Volts to DAC Multiplier */
  static constexpr float VOLTS_TO_DAC = ((float) DAC_MAX_VALUE) / (3.3);


  /**
   * Current Set DAC Multiplier
   *
   * R_sense = 0.100 Ohm / 0.010 Ohm
   * Multiplier (DAC) = x 1.0
   * Multiplier (sense) = x 10
   * Channels = 1 / 2
   *
   * V_dac = Mult_sense * C_set * R_sense / Channels / Mult_dac
   */

  static float currentSetDacMultiplier;

  /**
   * Current Set DAC Multiplier
   *
   * V_dac = C_set * (Mult_sense * R_sense / Mult_dac)
   */
  static void initCurrentSetDacMultiplier() {
    currentSetDacMultiplier = (C_SENSE_MULTIPLIER * C_SENSE_RESISTOR / NR_CHANNELS / DAC_MULTIPLIER) * VOLTS_TO_DAC;
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