
#include <Arduino.h>
#include <Wire.h>

const PinName i2cLeds[] = {
    PB_12, PB_13, PB_14, PB_15
};

void i2cScanSetup() {
    for (int i = 0; i < 4; i++) {
        pinMode(i2cLeds[i], OUTPUT);
        digitalWrite(i2cLeds[i], LOW);
    }

    Serial.begin(115200);
    delay(3000);

    // Request internal pull-ups before I2C initialization
    pinMode(PA_10, INPUT_PULLUP);
    pinMode(PA_9, INPUT_PULLUP);

    Wire.setSDA(PA_10);
    Wire.setSCL(PA_9);

    Wire.begin();
    Wire.setClock(10000);

    Serial.println("I2C initialization attempted");
}

void i2cScanLoop() {
    int count = 0;

    Serial.println();
    Serial.println("Starting I2C scan...");

    for (uint8_t address = 0x08; address <= 0x77; address++) {
        Wire.beginTransmission(address);

        uint8_t error = Wire.endTransmission();

        if (error == 0) {
            Serial.print("DEVICE FOUND at 0x");
            if (address < 16) Serial.print("0");
            Serial.println(address, HEX);
            count++;
        }
        else if (error != 2) {
            Serial.print("I2C error at 0x");
            Serial.print(address, HEX);
            Serial.print(" : ");
            Serial.println(error);
        }
    }

    Serial.print("Scan complete. Addresses found: ");
    Serial.println(count);

    // Display count on LEDs
    for (int i = 0; i < 4; i++) {
        digitalWrite(i2cLeds[i], (count >> i) & 1);
    }

    delay(2000);
}
