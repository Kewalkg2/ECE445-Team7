#include <Arduino.h>

void usbSerialSetup() {
    Serial.begin(115200);
}

void usbSerialLoop() {
    static unsigned long lastPrint = 0;

    if (millis() - lastPrint >= 1000) {
        lastPrint = millis();
        Serial.println("Hello from STM32L412!");
    }
}