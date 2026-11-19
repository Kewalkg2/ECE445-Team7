#include "drivers/photodiodes.h"
#include "config/glasses_config.h"

namespace Photodiodes {
static const char *error = "none";
static bool ready = false;
static bool waitForSet(volatile uint32_t *reg, uint32_t mask) {
  uint32_t start = micros();
  while (!(*reg & mask)) if ((uint32_t)(micros()-start) > 100000) return false;
  return true;
}
static bool waitForClear(volatile uint32_t *reg, uint32_t mask) {
  uint32_t start = micros();
  while (*reg & mask) if ((uint32_t)(micros()-start) > 100000) return false;
  return true;
}
bool begin() {
  ready = false;
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_ADCEN;
  // PA0=ADC1_IN5, PA1=ADC1_IN6, PA4=ADC1_IN9 on STM32L412CB.
  for (uint8_t pin : {0,1,4}) {
    GPIOA->MODER |= (3UL << (pin*2));
    GPIOA->PUPDR &= ~(3UL << (pin*2));
  }
  if (ADC1->CR & ADC_CR_ADSTART) {
    ADC1->CR |= ADC_CR_ADSTP;
    if (!waitForClear(&ADC1->CR, ADC_CR_ADSTP)) { error="ADC stop timeout"; return false; }
  }
  if (ADC1->CR & ADC_CR_ADEN) {
    ADC1->CR |= ADC_CR_ADDIS;
    if (!waitForClear(&ADC1->CR, ADC_CR_ADEN)) { error="ADC disable timeout"; return false; }
  }
  ADC1->CR &= ~ADC_CR_DEEPPWD;
  ADC1->CR |= ADC_CR_ADVREGEN;
  delayMicroseconds(20);
  ADC1->CR &= ~ADC_CR_ADCALDIF;
  ADC1->CR |= ADC_CR_ADCAL;
  if (!waitForClear(&ADC1->CR, ADC_CR_ADCAL)) { error="ADC calibration timeout"; return false; }
  // Preserve tested synchronous clock setting; check actual PCLK against MCU limits.
  ADC12_COMMON->CCR = (ADC12_COMMON->CCR & ~ADC_CCR_CKMODE) | ADC_CCR_CKMODE_0;
  ADC1->CFGR = 0; // 12-bit, single software-triggered conversion
  // 16x hardware oversampling: ratio bits=3, shift=4.
  ADC1->CFGR2 = ADC_CFGR2_ROVSE | (3UL << ADC_CFGR2_OVSR_Pos) | (4UL << ADC_CFGR2_OVSS_Pos);
  // 24.5-cycle sample time for each selected channel.
  for (uint8_t ch : {5,6,9}) {
    if (ch < 10) {
      uint32_t shift = 3UL*ch;
      ADC1->SMPR1 = (ADC1->SMPR1 & ~(7UL << shift)) | (3UL << shift);
    }
  }
  ADC1->SQR1 = 5UL << ADC_SQR1_SQ1_Pos;
  ADC1->ISR = ADC_ISR_ADRDY;
  ADC1->CR |= ADC_CR_ADEN;
  if (!waitForSet(&ADC1->ISR, ADC_ISR_ADRDY)) { error="ADC ready timeout"; return false; }
  ready = true;
  return true;
}
static bool readChannel(uint8_t ch, uint16_t &value) {
  ADC1->SQR1 = (uint32_t)ch << ADC_SQR1_SQ1_Pos;
  ADC1->ISR = ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR;
  ADC1->CR |= ADC_CR_ADSTART;
  // Bounded polling works inside a timer ISR; micros() timeouts do not.
  uint32_t spins = 0;
  while (!(ADC1->ISR & ADC_ISR_EOC)) {
    if (++spins >= 100000UL) {
      error="ADC conversion timeout"; ready=false; return false;
    }
  }
  value = (uint16_t)ADC1->DR;
  return true;
}
bool read(Sample &out) {
  if (!ready) return false;
  out.timestampUs = micros();
  return readChannel(5,out.wide) && readChannel(6,out.narrow) && readChannel(9,out.feedback);
}
const char* lastError() { return error; }
}
