
#include <Arduino.h>
#include <Wire.h>
#include "dac60502.h"

// ==========================================
// LCD channel mapping
// VERIFY AGAINST YOUR PCB SCHEMATIC
// ==========================================

constexpr uint8_t LCDG_DAC = DAC60502::U3;
constexpr uint8_t LCDG_CHANNEL = DAC60502::CHANNEL_A;

constexpr uint8_t LCDK_DAC = DAC60502::U3;
constexpr uint8_t LCDK_CHANNEL = DAC60502::CHANNEL_B;

// ==========================================
// Test configuration
// ==========================================

constexpr uint32_t WAVE_FREQ_HZ = 60;
constexpr uint32_t HALF_PERIOD_US =
    1000000UL / (2 * WAVE_FREQ_HZ);

// 10 seconds up, 10 seconds down
constexpr uint32_t RAMP_TIME_MS = 10000;
constexpr uint32_t SWEEP_PERIOD_MS = 2 * RAMP_TIME_MS;

// Differential voltage amplitude: 1 V to 4 V
constexpr float MIN_AMPLITUDE_V = 1.0f;
constexpr float MAX_AMPLITUDE_V = 4.0f;

// Both DAC outputs centered around 2.5 V
constexpr float CENTER_VOLTAGE = 2.5f;

// ==========================================
// State
// ==========================================

static bool enabled = false;
static bool phase = false;

static uint32_t lastTransitionUs = 0;
static uint32_t sweepStartMs = 0;
static uint32_t lastPrintMs = 0;

static float currentAmplitude = MIN_AMPLITUDE_V;

// ==========================================
// Calculate linear triangular amplitude
// ==========================================

float calculateAmplitude(uint32_t elapsedMs) {
    uint32_t t = elapsedMs % SWEEP_PERIOD_MS;

    constexpr float RANGE =
        MAX_AMPLITUDE_V - MIN_AMPLITUDE_V;

    if (t < RAMP_TIME_MS) {
        return MIN_AMPLITUDE_V +
               RANGE * static_cast<float>(t) / RAMP_TIME_MS;
    }

    return MIN_AMPLITUDE_V +
           RANGE * static_cast<float>(
               SWEEP_PERIOD_MS - t
           ) / RAMP_TIME_MS;
}

// ==========================================
// Apply complementary LCD voltages
// ==========================================

bool applyLCDOutputs(float amplitude, bool currentPhase) {
    float halfAmplitude = amplitude / 2.0f;

    float voltageG = currentPhase
        ? CENTER_VOLTAGE + halfAmplitude
        : CENTER_VOLTAGE - halfAmplitude;

    float voltageK = currentPhase
        ? CENTER_VOLTAGE - halfAmplitude
        : CENTER_VOLTAGE + halfAmplitude;

    bool okG = DAC60502::setVoltage(
        LCDG_DAC, LCDG_CHANNEL, voltageG
    );

    bool okK = DAC60502::setVoltage(
        LCDK_DAC, LCDK_CHANNEL, voltageK
    );

    return okG && okK;
}

// ==========================================
// Setup
// ==========================================

void lcdSweepSetup() {
    Serial.begin(115200);
    delay(3000);

    Wire.setSDA(PA_10);
    Wire.setSCL(PA_9);
    Wire.begin();
    Wire.setClock(100000);

    Serial.println("=== LCD Amplitude Sweep Test ===");

    bool ok = DAC60502::init(LCDG_DAC);

    if (LCDK_DAC != LCDG_DAC) {
        ok &= DAC60502::init(LCDK_DAC);
    }

    if (!ok) {
        Serial.println("DAC initialization FAILED");
        return;
    }

    phase = false;
    currentAmplitude = MIN_AMPLITUDE_V;

    if (!applyLCDOutputs(currentAmplitude, phase)) {
        Serial.println("Initial DAC write FAILED");
        return;
    }

    sweepStartMs = millis();
    lastTransitionUs = micros();
    lastPrintMs = millis();

    enabled = true;

    Serial.println("60 Hz complementary square wave");
    Serial.println("Amplitude: 1 -> 4 -> 1 V");
    Serial.println("Sweep period: 20 seconds");
}

// ==========================================
// Main loop
// ==========================================

void lcdSweepLoop() {
    if (!enabled)
        return;

    uint32_t nowUs = micros();
    uint32_t nowMs = millis();

    currentAmplitude = calculateAmplitude(
        nowMs - sweepStartMs
    );

    // Change polarity every half-cycle
    if ((uint32_t)(nowUs - lastTransitionUs) >=
        HALF_PERIOD_US) {

        phase = !phase;

        if (!applyLCDOutputs(currentAmplitude, phase)) {
            enabled = false;
            Serial.println("DAC write FAILED");

            // Attempt to remove differential voltage
            DAC60502::setVoltage(
                LCDG_DAC,
                LCDG_CHANNEL,
                CENTER_VOLTAGE
            );

            DAC60502::setVoltage(
                LCDK_DAC,
                LCDK_CHANNEL,
                CENTER_VOLTAGE
            );

            return;
        }

        lastTransitionUs += HALF_PERIOD_US;
    }

    // Print amplitude every 500 ms
    if (nowMs - lastPrintMs >= 500) {
        lastPrintMs = nowMs;

        Serial.print("Amplitude: ");
        Serial.print(currentAmplitude, 3);
        Serial.println(" V");
    }
}
