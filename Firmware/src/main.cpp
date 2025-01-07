/*
 * Copyright (c) 2025 by Attila Tőkés.
 *
 * Licence: MIT
 */
#include <Arduino.h>

#include "dac.h"
#include "adc.h"
#include "load.h"
#include "web.h"
#include "wifi.h"
#include "ota.h"

/* Pin Configuration */

const uint8_t PROG_PIN = 0;
const uint8_t LED_PIN = 21;
const uint8_t BTN_PIN = 47; // pin 33 on ESP32-S2

const uint8_t VOLTAGE_SENSE_PIN = 6;
const uint8_t CURRENT_SENSE_PIN_1 = 10;
const uint8_t CURRENT_SENSE_PIN_2 = 9;
const uint8_t TEMP_SENSE_PIN = 5;

const uint8_t DAC_PIN_0 = 48; // pin 34 on ESP32-S2
const uint8_t DAC_PIN_1 = 14;
const uint8_t DAC_PIN_2 = 35;
const uint8_t DAC_PIN_3 = 36;

const uint8_t DAC_PIN_4 = 37;
const uint8_t DAC_PIN_5 = 38;
const uint8_t DAC_PIN_6 = 39;
const uint8_t DAC_PIN_7 = 40;

const uint8_t DAC_PIN_8 = 41;
const uint8_t DAC_PIN_9 = 42;
const uint8_t DAC_PIN_10 = 44;
const uint8_t DAC_PIN_11 = 43;

const uint8_t DAC_PIN_12 = 2;
const uint8_t DAC_PIN_13 = 1;

const uint8_t NR_DAC_PINS = 12; // normally 14, pins 10 and 11 not used

const uint8_t DAC_PINS[] = {
  DAC_PIN_0, DAC_PIN_1, DAC_PIN_2, DAC_PIN_3,
  DAC_PIN_4, DAC_PIN_5, DAC_PIN_6, DAC_PIN_7,
  DAC_PIN_8, DAC_PIN_9, /*DAC_PIN_10, DAC_PIN_11,*/
  DAC_PIN_12, DAC_PIN_13,
};

const uint8_t NR_ADC_PINS = 4;

const uint8_t ADC_PINS[] = {
  VOLTAGE_SENSE_PIN, CURRENT_SENSE_PIN_1, CURRENT_SENSE_PIN_2, TEMP_SENSE_PIN
};

DAC dac(NR_DAC_PINS, DAC_PINS, 8);

ADC adc(NR_ADC_PINS, ADC_PINS);

Load load(dac, adc);

Wireless wifi;

WebServer webServer(80, load);

OTA ota;

bool progMode = false;

void setup() {
  Serial.begin(115200);

  pinMode(PROG_PIN, INPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  for (auto nr = 0; nr < NR_DAC_PINS; nr++) {
    pinMode(DAC_PINS[nr], OUTPUT);
  }

  for (auto nr = 0; nr < NR_ADC_PINS; nr++) {
    pinMode(ADC_PINS[nr], INPUT);
    adcAttachPin(ADC_PINS[nr]);
  }

  if (digitalRead(BTN_PIN) == 0) {
    // enable prog mode
    progMode = true;
    Serial.println("Programming mode...");
  }

  wifi.begin();

  webServer.begin();

  if (progMode) {
    ota.begin();
  }
}

void loop() {
  if (progMode) {
    ota.handle();

  } else {
    load.handle();
  }
}