
#include <Arduino.h>

const PinName leds[] = {
    PB_12, PB_13, PB_14, PB_15
};

void blinkerSetup() {
    for (int i = 0; i < 4; i++) {
        pinMode(leds[i], OUTPUT);
        digitalWrite(leds[i], LOW);
    }
}

void blinkerLoop() {
    for (int i = 0; i < 4; i++) {
        digitalWrite(leds[i], HIGH);
        delay(500);
        digitalWrite(leds[i], LOW);
    }
}

