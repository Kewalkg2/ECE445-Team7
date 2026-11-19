
#include <Arduino.h>

// ==========================================
// Feedback photodiode ADC test
// ==========================================

constexpr uint32_t FB_PIN = PA4;
constexpr float ADC_REFERENCE_V = 3.3f;
constexpr uint32_t ADC_MAX = 4095;

static uint32_t lastFeedbackPrint = 0;

void feedbackSetup() {
    Serial.begin(115200);
    delay(3000);

    analogReadResolution(12);
    pinMode(FB_PIN, INPUT_ANALOG);

    Serial.println("=== Feedback ADC Test ===");
    Serial.println("Reading FbOut on PA4");
    Serial.println("ADC resolution: 12 bits");
}

void feedbackLoop() {
    if (millis() - lastFeedbackPrint >= 100) {
        lastFeedbackPrint = millis();

        uint16_t raw = analogRead(FB_PIN);

        float voltage =
            (static_cast<float>(raw) / ADC_MAX)
            * ADC_REFERENCE_V;

        Serial.print("FbOut ADC: ");
        Serial.print(raw);

        Serial.print(" | Voltage: ");
        Serial.print(voltage, 3);
        Serial.println(" V");
    }
}
