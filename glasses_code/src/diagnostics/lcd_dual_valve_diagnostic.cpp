// ECE 445: independent dual-LCD bench diagnostic.
// Install as src/diagnostics/lcd_dual_valve_diagnostic.cpp
// Use the [env:lcd_dual_test] build_src_filter shown separately.
// BENCH ONLY: do not wear as protective eyewear.
#include <Arduino.h>
#include <Wire.h>
#include "drivers/dac60502.h"

namespace {
constexpr uint32_t PHASE_MS = 3000;
constexpr uint32_t HALF_PERIOD_US = 8333; // 60 Hz full AC cycle
constexpr float DARK_DIFF_V = 4.0f;       // +/- 4 V differential; no DC bias
constexpr float MID_V = 2.5f;

bool ready = false;
bool dark = false;
bool positive = false;
uint32_t phaseStartedMs = 0;
uint32_t nextEdgeUs = 0;
uint32_t missedEdges = 0;
uint32_t writes = 0;

bool writePair(uint8_t addr, float diffV) {
  const float a = MID_V + diffV * 0.5f;
  const float b = MID_V - diffV * 0.5f;
  // Execute both writes, even if one fails, to report overall success.
  const bool okA = DAC60502::setVoltage(addr, DAC60502::CHANNEL_A, a);
  const bool okB = DAC60502::setVoltage(addr, DAC60502::CHANNEL_B, b);
  return okA && okB;
}

bool writeBoth(float diffV) {
  const bool u3 = writePair(DAC60502::U3, diffV);
  const bool u6 = writePair(DAC60502::U6, diffV);
  if (!u3 || !u6) {
    Serial.print("I2C WRITE FAILURE U3="); Serial.print(u3 ? "OK" : "FAIL");
    Serial.print(" U6="); Serial.println(u6 ? "OK" : "FAIL");
    return false;
  }
  ++writes;
  return true;
}

void stopOnError() {
  ready = false;
  // Best-effort safe, zero-differential command to both chips.
  writeBoth(0.0f);
  Serial.println("STOPPED: I2C failure. Check power/wiring before restarting.");
}

void enterPhase(bool newDark) {
  dark = newDark;
  phaseStartedMs = millis();
  positive = false;
  nextEdgeUs = micros() + HALF_PERIOD_US;
  const float diff = dark ? -DARK_DIFF_V : 0.0f;
  if (!writeBoth(diff)) { stopOnError(); return; }
  Serial.print("PHASE: ");
  Serial.print(dark ? "DARK command (+/-4 V, 60 Hz AC)" : "CLEAR command (0 V differential)");
  Serial.print(" | writes="); Serial.print(writes);
  Serial.print(" | missedEdges="); Serial.println(missedEdges);
}
}

void setup() {
  Serial.begin(115200);
  delay(1200);
  Serial.println("\nECE445 DUAL LCD TEST - U3 0x48 + U6 0x49");
  Serial.println("Bench only. Remove glasses from face. 3s clear / 3s dark command.");
  Wire.setSDA(PA10);
  Wire.setSCL(PA9);
  Wire.begin();
  Wire.setClock(400000); // four channel writes per half-cycle

  const bool u3 = DAC60502::init(DAC60502::U3);
  const bool u6 = DAC60502::init(DAC60502::U6);
  Serial.print("U3 init: "); Serial.println(u3 ? "OK" : "FAIL");
  Serial.print("U6 init: "); Serial.println(u6 ? "OK" : "FAIL");
  ready = u3 && u6;
  if (!ready) {
    Serial.println("STOPPED: both DACs must initialize successfully.");
    return;
  }
  enterPhase(false);
}

void loop() {
  if (!ready) { delay(100); return; }
  if (static_cast<uint32_t>(millis() - phaseStartedMs) >= PHASE_MS) {
    enterPhase(!dark);
    return;
  }
  if (!dark) { delay(1); return; }

  const uint32_t now = micros();
  if (static_cast<int32_t>(now - nextEdgeUs) < 0) return;
  const uint32_t elapsed = 1U + (now - nextEdgeUs) / HALF_PERIOD_US;
  if (elapsed > 1U) missedEdges += elapsed - 1U;
  nextEdgeUs += elapsed * HALF_PERIOD_US;
  if (elapsed & 1U) positive = !positive;
  if (!writeBoth(positive ? DARK_DIFF_V : -DARK_DIFF_V)) stopOnError();
}
