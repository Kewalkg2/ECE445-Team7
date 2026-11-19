
#include <Arduino.h>
#include <arduinoFFT.h>

// ==========================================
// SELECT PHOTODIODE HERE
// ==========================================

// PA0 = WideOut
// PA1 = NarrowOut
// PA4 = FbOut

#define ADC_PIN PA1

// ==========================================
// FFT CONFIGURATION
// ==========================================

constexpr uint16_t SAMPLES = 1024;
constexpr float SAMPLING_FREQUENCY = 1024.0f;

constexpr uint8_t MIN_FREQ = 3;
constexpr uint8_t MAX_FREQ = 60;
constexpr uint8_t TOP_COUNT = 5;

float vReal[SAMPLES];
float vImag[SAMPLES];

ArduinoFFT<float> FFT(
    vReal,
    vImag,
    SAMPLES,
    SAMPLING_FREQUENCY
);

struct Peak {
    uint16_t bin;
    float magnitude;
};

Peak topPeaks[TOP_COUNT];

// ==========================================
// SETUP
// ==========================================

void fftTestSetup() {
    Serial.begin(115200);
    delay(3000);

    pinMode(ADC_PIN, INPUT_ANALOG);
    analogReadResolution(12);

    Serial.println("=== PHOTODIODE FFT TEST ===");
    Serial.println("1024 samples @ 1024 Hz");
    Serial.println("Frequency range: 3-60 Hz");

#if ADC_PIN == PA0
    Serial.println("Sensor: WideOut (PA0)");
#elif ADC_PIN == PA1
    Serial.println("Sensor: NarrowOut (PA1)");
#elif ADC_PIN == PA4
    Serial.println("Sensor: FbOut (PA4)");
#endif
}

// ==========================================
// LOOP
// ==========================================

void fftTestLoop() {

    // STEP 1: Collect 1024 samples
    uint32_t startTime = micros();

    for (uint16_t i = 0; i < SAMPLES; i++) {

        uint32_t targetTime =
            startTime +
            (uint32_t)(
                (uint64_t)i * 1000000ULL / 1024
            );

        while ((int32_t)(micros() - targetTime) < 0) {
            // Wait
        }

        vReal[i] = analogRead(ADC_PIN);
        vImag[i] = 0.0f;
    }

    // STEP 2: Remove DC offset
    float mean = 0.0f;

    for (uint16_t i = 0; i < SAMPLES; i++) {
        mean += vReal[i];
    }

    mean /= SAMPLES;

    for (uint16_t i = 0; i < SAMPLES; i++) {
        vReal[i] -= mean;
    }

    // STEP 3: Apply Hamming window
    FFT.windowing(
        FFTWindow::Hamming,
        FFTDirection::Forward
    );

    // STEP 4: Compute FFT
    FFT.compute(FFTDirection::Forward);

    // STEP 5: Convert to magnitude
    FFT.complexToMagnitude();

    // STEP 6: Find top 5 bins
    for (uint8_t i = 0; i < TOP_COUNT; i++) {
        topPeaks[i] = {0, -1.0f};
    }

    for (uint16_t bin = MIN_FREQ;
         bin <= MAX_FREQ;
         bin++) {

        float magnitude = vReal[bin];

        for (uint8_t rank = 0;
             rank < TOP_COUNT;
             rank++) {

            if (magnitude > topPeaks[rank].magnitude) {

                for (int j = TOP_COUNT - 1;
                     j > rank;
                     j--) {
                    topPeaks[j] = topPeaks[j - 1];
                }

                topPeaks[rank] = {bin, magnitude};
                break;
            }
        }
    }

    // STEP 7: Print top 5
    Serial.println();
    Serial.println("===== TOP 5 FREQUENCIES =====");

    for (uint8_t i = 0; i < TOP_COUNT; i++) {

        float frequency =
            topPeaks[i].bin *
            SAMPLING_FREQUENCY / SAMPLES;

        Serial.print("#");
        Serial.print(i + 1);
        Serial.print(" | ");
        Serial.print(frequency, 1);
        Serial.print(" Hz | Magnitude: ");
        Serial.println(topPeaks[i].magnitude, 2);
    }
}
