#include "adc.h"
#include "hal/gpio_hal.h"

ADC* ADC::instance;

volatile bool adc_conversion_done = false;

const uint8_t LED_PIN = 21;

void ARDUINO_ISR_ATTR adcComplete() {
  GPIO.out_w1tc = ((uint32_t) 1 << LED_PIN);

  ADC::instance->readContinuousValues();

  GPIO.out_w1ts = ((uint32_t) 1 << LED_PIN);
}