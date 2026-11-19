
#include <Arduino.h>
#include <Wire.h>
#include "dac60502.h"

// ===========================================
// LCD output mapping - VERIFY WITH SCHEMATIC
// ===========================================

// Placeholder: LCDG = U3 channel A
constexpr uint8_t LCDG_DAC = DAC60502::U3;
constexpr uint8_t LCDG_CHANNEL = DAC60502::CHANNEL_A;

// Placeholder: LCDK = U3 channel B
constexpr uint8_t LCDK_DAC = DAC60502::U3;
constexpr uint8_t LCDK_CHANNEL = DAC60502::CHANNEL_B;

// ===========================================
// Waveform configuration
// ===========================================

constexpr uint32_t FREQUENCY_HZ = 60;
constexpr uint32_t HALF_PERIOD_US =
    1000000UL / (2 * FREQUENCY_HZ);

constexpr float HIGH_VOLTAGE = 5.0f;
constexpr float LOW_VOLTAGE = 0.0f;

static bool waveEnabled = false;
static bool phase = false;
static uint32_t lastTransition = 0;
static uint32_t transitionCount = 0;

// Set both output voltages for one phase.
bool applyPhase(bool newPhase) {
    float voltageG = newPhase ? HIGH_VOLTAGE : LOW_VOLTAGE;
    float voltageK = newPhase ? LOW_VOLTAGE : HIGH_VOLTAGE;

    bool okG = DAC60502::setVoltage(
        LCDG_DAC, LCDG_CHANNEL, voltageG
    );

    bool okK = DAC60502::setVoltage(
        LCDK_DAC, LCDK_CHANNEL, voltageK
    );

    return okG && okK;
}

void lcdWaveSetup() {
    Serial.begin(115200);
    delay(3000);

    Wire.setSDA(PA_10);
    Wire.setSCL(PA_9);
    Wire.begin();
    Wire.setClock(100000);

    Serial.println("=== LCD Square Wave Test ===");

    // Initialize the DACs used by the LCD outputs.
    bool ok = DAC60502::init(LCDG_DAC);

    if (LCDK_DAC != LCDG_DAC) {
        ok &= DAC60502::init(LCDK_DAC);
    }

    if (!ok) {
        Serial.println("DAC initialization FAILED");
        return;
    }

    // Start in the LOW/HIGH state.
    phase = false;

    if (!applyPhase(phase)) {
        Serial.println("Initial DAC writes FAILED");
        return;
    }

    lastTransition = micros();
    waveEnabled = true;

    Serial.println("Waveform enabled");
    Serial.println("Frequency target: 60 Hz");
    Serial.println("Duty cycle target: 50%");
    Serial.println("Differential target: +/-5 V");
}

void lcdWaveLoop() {
    if (!waveEnabled)
        return;

    uint32_t now = micros();

    if ((uint32_t)(now - lastTransition) >= HALF_PERIOD_US) {
        phase = !phase;

        if (!applyPhase(phase)) {
            waveEnabled = false;
            Serial.println("DAC write FAILED - waveform stopped");

            // Attempt to bring both outputs to zero.
            DAC60502::setVoltage(
                LCDG_DAC, LCDG_CHANNEL, 0.0f
            );
            DAC60502::setVoltage(
                LCDK_DAC, LCDK_CHANNEL, 0.0f
            );
            return;
        }

        lastTransition += HALF_PERIOD_US;
        transitionCount++;
    }

    // Avoid printing on every transition.
    static uint32_t lastPrint = 0;

    if (millis() - lastPrint >= 2000) {
        lastPrint = millis();

        Serial.print("Transitions: ");
        Serial.println(transitionCount);
    }
}
