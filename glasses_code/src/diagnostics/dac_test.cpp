
#include <Arduino.h>
#include <Wire.h>
#include "dac60502.h"

namespace {

void printRegister(uint8_t address, uint8_t reg,
                   const char* name) {
    uint16_t value = 0;

    Serial.print(name);
    Serial.print(": ");

    if (DAC60502::readRegister(address, reg, value)) {
        Serial.print("0x");
        if (value < 0x1000) Serial.print("0");
        if (value < 0x0100) Serial.print("0");
        if (value < 0x0010) Serial.print("0");
        Serial.println(value, HEX);
    } else {
        Serial.println("READ FAILED");
    }
}

void testChannel(uint8_t address, uint8_t channel,
                 float voltage) {
    Serial.print("Channel ");
    Serial.print(channel == DAC60502::CHANNEL_A ? "A" : "B");
    Serial.print(" -> ");
    Serial.print(voltage, 3);
    Serial.print(" V: ");

    bool ok = DAC60502::setVoltage(
        address, channel, voltage
    );

    Serial.println(ok ? "WRITE OK" : "WRITE FAILED");

    if (ok) {
        uint8_t reg = channel == DAC60502::CHANNEL_A
                    ? DAC60502::REG_DACA
                    : DAC60502::REG_DACB;

        printRegister(address, reg, "  DAC register");
    }
}

void testDAC(uint8_t address, const char* name,
             float voltageA, float voltageB) {
    Serial.println();
    Serial.print("=== Testing ");
    Serial.print(name);
    Serial.println(" ===");

    if (!DAC60502::init(address)) {
        Serial.println("INITIALIZATION FAILED");
        return;
    }

    Serial.println("Initialization OK");

    printRegister(address, DAC60502::REG_CONFIG, "CONFIG");
    printRegister(address, DAC60502::REG_GAIN, "GAIN");

    testChannel(address, DAC60502::CHANNEL_A, voltageA);
    testChannel(address, DAC60502::CHANNEL_B, voltageB);
}

} // anonymous namespace

void dacTestSetup() {
    Serial.begin(115200);
    delay(3000);

    // Known working I2C pins from our scanner.
    Wire.setSDA(PA_10);
    Wire.setSCL(PA_9);
    Wire.begin();
    Wire.setClock(10000);

    Serial.println();
    Serial.println("=== DAC60502 Dual DAC Test ===");

    // U3: A = 1.25 V, B = 3.53 V
    testDAC(DAC60502::U3, "U3 (0x48)", 1.25f, 3.53f);

    // U6: A = 4.64 V, B = 1.25 V
    testDAC(DAC60502::U6, "U6 (0x49)", 4.64f, 1.25f);

    Serial.println();
    Serial.println("=== DAC Test Complete ===");
}

void dacTestLoop() {
    static uint32_t lastPrint = 0;

    if (millis() - lastPrint >= 2000) {
        lastPrint = millis();
        Serial.println("Both DACs test running");
    }
}
