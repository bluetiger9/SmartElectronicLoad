/*
 * Copyright (c) 2025 by Attila Tőkés.
 *
 * Licence: MIT
 */
#include <Arduino.h>

#ifndef ADC_H
#define ADC_H

/**
 * Continuous ADC parameters:
 *   - frequency: 83.333 kHz (max, overall for all pins)
 *   - conversions per pin: 4
 *   - effective per pin frequency: 83.333 kHz / 4 / 5 ~= 4.1 kHz
 */
const uint8_t ADC_CONTINUOUS_CONVERSIONS_PER_PIN = 4;
const uint32_t ADC_CONTINUOUS_FREQ = 83333; // max freq

/** Continuous ADC callback */
void ARDUINO_ISR_ATTR adcComplete();

/** Analog to Digital Converter (ADC) implemented on the ESP32-S3 chip. */
class ADC {

public:

  const uint8_t nrChannels;
  const uint8_t *pins;
  uint16_t *values;
  uint64_t lastReadTimeMicros = 0;

  adc_continuous_data_t *result = NULL;

  static ADC *instance;

  ADC(const uint8_t nrChannels, const uint8_t *pins)
    : nrChannels(nrChannels), pins(pins) {

    instance = this;

    this->values = new uint16_t[nrChannels];

    // Synchonous read mode (disabled)
    // analogReadResolution(12);
    // analogSetAttenuation(ADC_11db);
  }

  /** Initialize and start ADC reads */
  void begin() {
    // init continuous ADC reads
    Serial.println("Setting up continuous ADC reads...");
    analogContinuousSetWidth(12);
    analogContinuousSetAtten(ADC_11db);
    if (!analogContinuous(pins, nrChannels, ADC_CONTINUOUS_CONVERSIONS_PER_PIN, ADC_CONTINUOUS_FREQ, &adcComplete)) {
      Serial.println("Continuous ADC setup ERROR!");
      return;
    };

    // start continuous ADC reads
    Serial.println("Starting continuous ADC reads...");
    if (!analogContinuousStart()) {
      Serial.println("Continuous ADC start ERROR!");
      return;
    }
  }

  void pause() {
    Serial.print("P!");
    if (!analogContinuousStop()) {
      Serial.println("Continuous ADC stop (pause) ERROR!");
    }
  }

  void resume() {
    Serial.print("R!");
    if (!analogContinuousStart()) {
      Serial.println("Continuous ADC restart ERROR!");
    }
  }

  /** Consumes the continuous ADC read values (called by the continuous ADC callback) */
  void readContinuousValues() {
    // note: analogContinuousRead() takes ~10us, which is bit slow
    if (!analogContinuousRead(&result, 0)) {
      Serial.println("Continuous ADC read ERROR!");
      return;
    }

    // save the avg ADC value
    for (int chan = 0; chan < this->nrChannels; chan++) {
      #ifdef ESP32_S2
      // why the 2x multiplier is needed?
      this->values[chan] = 2 * result[chan].avg_read_mvolts;
      #else // S3
      this->values[chan] = result[chan].avg_read_mvolts;
      #endif
    }

    this->lastReadTimeMicros = micros();
  }

  void handle() {
    // Note: nothing to do with async / continuous reads

    // Synchonous read mode (disabled)
    // for (uint8_t chan = 0; chan < this->nrChannels; chan++) {
    //   this->values[chan] = analogRead(this->pins[chan]);
    //   this->values[chan] = analogReadRaw(this->pins[chan]);
    //   this->values[chan] = result[chan].avg_read_mvolts;
    // }
  }

  /** Get the ADC value for a given channel (in millivolts) */
  uint16_t getMilliVolts(uint8_t chan) {
    // note: values currently are in millivolts
    return this->values[chan];
  }

private:
};

#endif