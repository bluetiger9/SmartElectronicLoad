#include "adc.h"
#include "hal/gpio_hal.h"

ADC* ADC::instance;

const uint8_t LED_PIN = 21;

//#define DEBUG_CONTINUOUS_ADC_LED_PIN 1

/** Continuous ADC result interrupt handler */
void ARDUINO_ISR_ATTR adcComplete() {

#ifdef DEBUG_CONTINUOUS_ADC_LED_PIN
  GPIO.out_w1tc = ((uint32_t) 1 << LED_PIN);
#endif

  ADC::instance->readContinuousValues();

#ifdef DEBUG_CONTINUOUS_ADC_LED_PIN
  GPIO.out_w1ts = ((uint32_t) 1 << LED_PIN);
#endif

}