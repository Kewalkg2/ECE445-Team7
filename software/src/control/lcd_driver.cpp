#include "control/lcd_driver.h"
#include "config/glasses_config.h"
#include "drivers/dac60502.h"
#include <Arduino.h>
#include <Wire.h>
#include <math.h>
namespace LCDDriver {
static bool running=false, phase=false, good=false;
static uint32_t nextToggle=0;
static float amplitude=0;
static constexpr float CENTER_V=2.5f;
static constexpr uint32_t HALF_PERIOD_US=1000000UL/(2*Config::LCD_DRIVE_HZ);
static bool writeLens(uint8_t address, float a, float b) {
  return DAC60502::setVoltage(address,DAC60502::CHANNEL_A,a) &&
         DAC60502::setVoltage(address,DAC60502::CHANNEL_B,b);
}
static bool writePhase() {
  const float h=0.5f*amplitude;
  const float a=CENTER_V+(phase?h:-h);
  const float b=CENTER_V+(phase?-h:h);
  // Both lenses operate at the same amplitude until independent calibration.
  return writeLens(DAC60502::U3,a,b);
}
bool begin() {
  running=false;good=false;
  if (!Config::ENABLE_LENS_ACTUATION || !Config::OPTICAL_CALIBRATION_COMPLETE)
    return false; // no accidental I2C or shutter initialization
  Wire.setSDA(PA_10);
  Wire.setSCL(PA_9);
  Wire.begin();
  Wire.setClock(400000);
  good=DAC60502::init(DAC60502::U3);
  if (!good) return false;
  amplitude=0;phase=false;
  if (!writePhase()) {good=false;return false;}
  nextToggle=micros()+HALF_PERIOD_US;
  running=true;
  return true;
}
bool setAmplitude(float volts) {
  if (!running || !isfinite(volts)) return false;
  amplitude=fmaxf(0.0f,fminf(Config::MAX_DIFFERENTIAL_V,volts));
  return true;
}
void service(uint32_t now) {
  if (!running) return;
  if ((int32_t)(now-nextToggle)<0) return;
  // Do not attempt catch-up I2C bursts after a missed deadline. A late phase
  // introduces DC imbalance; flag it as a fault rather than pretending 60 Hz.
  if ((uint32_t)(now-nextToggle)>HALF_PERIOD_US/2) {stop();return;}
  phase=!phase;
  if (!writePhase()) {stop();return;}
  nextToggle+=HALF_PERIOD_US;
}
void stop() {
  running=false;good=false;
  // Best-effort zero differential (center-center), NOT a validated safe state.
  // If I2C has failed we cannot guarantee what voltage remains at the LCD.
  amplitude=0;
  writeLens(DAC60502::U3,CENTER_V,CENTER_V);
}
bool healthy(){return running && good;}
}
