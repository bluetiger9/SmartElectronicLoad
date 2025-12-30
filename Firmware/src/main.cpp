/*
 * Copyright (c) 2025 by Attila Tőkés.
 *
 * Licence: MIT
 */
#include <Arduino.h>
#include <EEPROM.h>
#include <Adafruit_TinyUSB.h>

#include "hal/gpio_hal.h"

#include "dac.h"
#include "adc.h"
#include "fan.h"
#include "load.h"
#include "web.h"
#include "wifi.h"
#include "ota.h"
#include "srv.h"
#include "hw.h"

/* Pin Configuration */

const uint8_t PROG_PIN = 0;
const uint8_t LED_PIN = 21;

#if defined ESP32_S3
const uint8_t BTN_PIN = 47;
#elif defined ESP32_S2
const uint8_t BTN_PIN = 33;
#endif

const uint8_t VOLTAGE_SENSE_PIN_1 = 6;
const uint8_t VOLTAGE_SENSE_PIN_2 = 7;
const uint8_t CURRENT_SENSE_PIN_1 = 10;
const uint8_t CURRENT_SENSE_PIN_2 = 9;
const uint8_t TEMP_SENSE_PIN = 5;

const uint8_t FAN_PIN = 3;

const uint8_t LOAD_PWR_EN_PIN = 8;

#if defined ESP32_S3
const uint8_t DAC_PIN_0 = 48;
#elif defined ESP32_S2
const uint8_t DAC_PIN_0 = 34;
#endif

const uint8_t DAC_PIN_1 = 14;
const uint8_t DAC_PIN_2 = 35;
const uint8_t DAC_PIN_3 = 36;

const uint8_t DAC_PIN_4 = 37;
const uint8_t DAC_PIN_5 = 38;
const uint8_t DAC_PIN_6 = 39;
const uint8_t DAC_PIN_7 = 1;

const uint8_t DAC_PIN_8 = 2;
const uint8_t DAC_PIN_9 = 42;
const uint8_t DAC_PIN_10 = 12;
const uint8_t DAC_PIN_11 = 13;

const uint8_t DAC_PIN_12 = 41;
const uint8_t DAC_PIN_13 = 40;

const uint8_t NR_DAC_PINS = 14;

const uint8_t DAC_PINS[] = {
  DAC_PIN_0, DAC_PIN_1, DAC_PIN_2, DAC_PIN_3,
  DAC_PIN_4, DAC_PIN_5, DAC_PIN_6, DAC_PIN_7,
  DAC_PIN_8, DAC_PIN_9, DAC_PIN_10, DAC_PIN_11,
  DAC_PIN_12, DAC_PIN_13,
};

//const uint8_t NR_ADC_PINS = 5;
const uint8_t NR_ADC_PINS = 5;

const uint8_t ADC_PINS[] = {
  //VOLTAGE_SENSE_PIN_1, VOLTAGE_SENSE_PIN_2, CURRENT_SENSE_PIN_1, CURRENT_SENSE_PIN_2, TEMP_SENSE_PIN
  VOLTAGE_SENSE_PIN_1, CURRENT_SENSE_PIN_1, CURRENT_SENSE_PIN_2, TEMP_SENSE_PIN, VOLTAGE_SENSE_PIN_2
};

DAC dac(NR_DAC_PINS, DAC_PINS, 8);

ADC adc(NR_ADC_PINS, ADC_PINS);

Fan fan(FAN_PIN, 255);

Load load(dac, adc, fan, LOAD_PWR_EN_PIN);

Wireless wifi;

Service srv(dac, adc);

WebServer webServer(80, load, srv);

OTA ota;

#define EEPROM_SIZE 4

volatile uint8_t restartRequest = 0;
bool progMode = false;

// Global mutex
portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED;

/** Control loop task (priority=2) */
void controlLoopTask(void *pvParameters) {
  Serial.println("Control loop task started.");

  // begin ADC readings
  adc.begin();

  while (true) {
    for (uint64_t idx = 0; idx < 10000; idx++) {
      load.handle();

      // GPIO.out_w1tc = ((uint32_t) 1 << LED_PIN);
      // GPIO.out_w1tc = ((uint32_t) 1 << LED_PIN);
      // GPIO.out_w1ts = ((uint32_t) 1 << LED_PIN);
    }

    if (restartRequest > 0) {
      taskENTER_CRITICAL(&mutex);

      if (restartRequest == 2) {
        // OTA restart
        EEPROM.begin(4);

        Serial.println("Setting EEPROM programming mode...");
        EEPROM.write(0, 1);
        EEPROM.commit();
      }

      Serial.println("Restarting ESP32...");
      Serial.flush();

      esp_restart();

      taskEXIT_CRITICAL(&mutex);
      return;
    }

    // TODO: why was a 1s delay here?
    //vTaskDelay(1000 / portTICK_PERIOD_MS);

    // let other tasks run
    taskYIELD();
  }
}

/** Critical control loop task (priority=10) */
void criticalControlLoopTask(void *pvParameters) {
  Serial.println("Critical control loop task started.");

  while (true) {
    // note: this will only run on request

    // sleep for now
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

/** Control loop task handle */
TaskHandle_t controlLoopTaskHandle;

/** Critical control loop task handle */
TaskHandle_t criticalControlLoopTaskHandle;

void setup() {
  HardwareValues::init();

  Serial.begin(115200);

  pinMode(PROG_PIN, INPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  pinMode(FAN_PIN, OUTPUT);
  pinMode(LOAD_PWR_EN_PIN, OUTPUT);
  //analogWriteFrequency(FAN_PIN, 25000);

  fan.set(0.00);

  for (auto nr = 0; nr < NR_DAC_PINS; nr++) {
    pinMode(DAC_PINS[nr], OUTPUT);
  }

  for (auto nr = 0; nr < NR_ADC_PINS; nr++) {
    pinMode(ADC_PINS[nr], INPUT);
    //adcAttachPin(ADC_PINS[nr]); todo
  }

  EEPROM.begin(EEPROM_SIZE);

  int persProgMode = EEPROM.read(0);

  Serial.printf("EEPROM prog mode flag: %d\n", persProgMode);
  if (persProgMode > 0) {
    // prog mode requested
    progMode = true;
    EEPROM.write(0, 0); // Reset flag
    EEPROM.commit();
    Serial.println("Programming mode requested...");
  }

  if (progMode || digitalRead(BTN_PIN) == 0) {
    // enable prog mode
    progMode = true;
    Serial.println("Programming mode...");
  }

  // start WiFi
  wifi.begin();

  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
    return;
  }

  Serial.println("Content:");
  File dir = LittleFS.open("/");
  File file = dir.openNextFile();
  char buf[1024];
  while (file) {
     Serial.println(file.name());
     file = dir.openNextFile();
  }

  // start the web server
  webServer.begin();

  if (progMode) {
    ota.begin();
    return;
  }

  // create control loop task
  Serial.println("Creating control loop and critical control loop tasks...");

  // wrap task creation in a critical section
  taskENTER_CRITICAL(&mutex);

  // create control loop task
  auto retval = xTaskCreate(
      controlLoopTask,        // Task function
      "ControlLoopTask",      // Name of the task
      2048,                   // Stack size
      NULL,                   // Task parameter
      1,                      // Priority (higher than loop()'s priority 1)
      &controlLoopTaskHandle  // Task handle
  );

  if (retval != pdPASS) {
    Serial.println("Creation of control loop task FAILED!");
    return;
  }

  // create critical control loop task
  retval = xTaskCreate(
      criticalControlLoopTask,        // Task function
      "CriticalControlLoopTask",      // Name of the task
      2048,                           // Stack size
      NULL,                           // Task parameter
      5,                              // Priority (higher than loop()'s priority 1)
      &criticalControlLoopTaskHandle  // Task handle
  );

  if (retval != pdPASS) {
    Serial.println("Creation of critical control loop task FAILED!");
    return;
  }

  // end of critical section
  taskEXIT_CRITICAL(&mutex);

  if (!TinyUSBDevice.isInitialized()) {
    TinyUSBDevice.begin(0);
  }

}

void loop() {
  if (progMode) {
    ota.handle();
  }

}