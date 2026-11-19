
#include <Arduino.h>
#include <math.h>

// ============================================================
// STM32L412 ADC HARDWARE OVERSAMPLING BENCHMARK
// PA0 = WideOut = ADC1 channel 5
// ============================================================

constexpr uint32_t NUM_SAMPLES = 10000;

// Change this to false to disable the noise statistics.
constexpr bool MEASURE_NOISE = true;

volatile uint32_t adcChecksum = 0;

struct OversamplingTest {
    const char* name;
    uint16_t ratio;
    uint8_t shift;
    uint8_t ratioBits;
};

const OversamplingTest tests[] = {
    {"Disabled", 1,   0, 0},
    {"4x",       4,   2, 1},
    {"16x",      16,  4, 3},
    {"64x",      64,  6, 5},
    {"256x",     256, 8, 7}
};

constexpr size_t NUM_TESTS = sizeof(tests) / sizeof(tests[0]);

// ============================================================
// Initialize ADC hardware once
// ============================================================

void initFastADC() {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    GPIOA->MODER |= GPIO_MODER_MODE0;
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD0;

    RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;

    // Disable ADC before reconfiguration/calibration.
    if (ADC1->CR & ADC_CR_ADEN) {
        ADC1->CR |= ADC_CR_ADDIS;
        while (ADC1->CR & ADC_CR_ADEN) {}
    }

    // Exit deep power down, enable regulator.
    ADC1->CR &= ~ADC_CR_DEEPPWD;
    ADC1->CR |= ADC_CR_ADVREGEN;
    delayMicroseconds(20);

    // Calibrate single-ended ADC.
    ADC1->CR &= ~ADC_CR_ADCALDIF;
    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL) {}

    // Synchronous ADC clock: PCLK / 1.
    // Must be checked against actual PCLK and ADC clock limits.
    ADC12_COMMON->CCR &= ~ADC_CCR_CKMODE;
    ADC12_COMMON->CCR |= ADC_CCR_CKMODE_0;

    // Single conversion, software trigger, 12-bit.
    ADC1->CFGR = 0;
    ADC1->CFGR2 = 0;

    // One channel: ADC1_IN5 (PA0).
    ADC1->SQR1 = (5U << ADC_SQR1_SQ1_Pos);

    // Sampling time: 24.5 ADC cycles.
    ADC1->SMPR1 &= ~(7U << ADC_SMPR1_SMP5_Pos);
    ADC1->SMPR1 |= (3U << ADC_SMPR1_SMP5_Pos);

    // Clear ADRDY and enable ADC.
    ADC1->ISR = ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) {}
}

// ============================================================
// Change hardware oversampling configuration
// Must be done while ADC is disabled.
// ============================================================

void configureOversampling(const OversamplingTest& test) {

    // Stop conversions if necessary.
    if (ADC1->CR & ADC_CR_ADSTART) {
        ADC1->CR |= ADC_CR_ADSTP;
        while (ADC1->CR & ADC_CR_ADSTP) {}
    }

    // Disable ADC.
    if (ADC1->CR & ADC_CR_ADEN) {
        ADC1->CR |= ADC_CR_ADDIS;
        while (ADC1->CR & ADC_CR_ADEN) {}
    }

    uint32_t cfgr2 = 0;

    if (test.ratio > 1) {
        // Enable regular-channel hardware oversampling.
        cfgr2 |= ADC_CFGR2_ROVSE;

        // Hardware oversampling ratio.
        cfgr2 |=
            ((uint32_t)test.ratioBits << ADC_CFGR2_OVSR_Pos);

        // Divide accumulated result by ratio.
        cfgr2 |=
            ((uint32_t)test.shift << ADC_CFGR2_OVSS_Pos);

        // TROVS = 0: all conversions on one trigger.
        // ROVSM = 0: continued oversampling mode.
    }

    ADC1->CFGR2 = cfgr2;

    ADC1->ISR = ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;

    while (!(ADC1->ISR & ADC_ISR_ADRDY)) {}
}

// ============================================================
// Direct ADC read
// One call = one completed output result.
// With oversampling, hardware performs N conversions first.
// ============================================================

static inline uint16_t fastAnalogRead() {
    ADC1->CR |= ADC_CR_ADSTART;

    while (!(ADC1->ISR & ADC_ISR_EOC)) {}

    return static_cast<uint16_t>(ADC1->DR);
}

// ============================================================
// Benchmark one oversampling setting
// ============================================================

void runBenchmark(const OversamplingTest& test) {

    configureOversampling(test);

    // Allow a short settling period.
    delay(10);

    // Discard initial readings.
    for (int i = 0; i < 16; i++) {
        (void)fastAnalogRead();
    }

    uint64_t sum = 0;
    uint64_t sumSquares = 0;

    uint32_t startUs = micros();

    for (uint32_t i = 0; i < NUM_SAMPLES; i++) {
        uint32_t value = fastAnalogRead();

        sum += value;

        if (MEASURE_NOISE) {
            sumSquares += (uint64_t)value * value;
        }
    }

    uint32_t elapsedUs = micros() - startUs;

    adcChecksum = static_cast<uint32_t>(sum);

    double mean = (double)sum / NUM_SAMPLES;
    double variance = 0.0;

    if (MEASURE_NOISE) {
        variance =
            ((double)sumSquares / NUM_SAMPLES)
            - (mean * mean);

        if (variance < 0.0) variance = 0.0;
    }

    double stdDev = sqrt(variance);

    double outputRate =
        (NUM_SAMPLES * 1000000.0) / elapsedUs;

    double rawRate =
        outputRate * test.ratio;

    Serial.println();
    Serial.println("================================");
    Serial.print("OVERSAMPLING: ");
    Serial.println(test.name);
    Serial.println("================================");

    Serial.print("Conversions per result: ");
    Serial.println(test.ratio);

    Serial.print("Total time: ");
    Serial.print(elapsedUs);
    Serial.println(" us");

    Serial.print("Time per output sample: ");
    Serial.print((double)elapsedUs / NUM_SAMPLES, 3);
    Serial.println(" us");

    Serial.print("Output sampling rate: ");
    Serial.print(outputRate / 1000.0, 2);
    Serial.println(" kSPS");

    Serial.print("Estimated raw conversion rate: ");
    Serial.print(rawRate / 1000.0, 2);
    Serial.println(" kconv/s");

    Serial.print("Average ADC code: ");
    Serial.println(mean, 2);

    Serial.print("Standard deviation: ");
    Serial.print(stdDev, 3);
    Serial.println(" ADC counts");
}

// ============================================================
// Arduino setup / loop
// ============================================================

void adcBenchmarkSetup() {
    Serial.begin(115200);
    delay(3000);

    Serial.println();
    Serial.println("================================");
    Serial.println("STM32L412 HARDWARE OVERSAMPLING");
    Serial.println("================================");

    initFastADC();

    Serial.println("ADC initialized");
    Serial.println("Pin: PA0");
    Serial.println("ADC channel: 5");
    Serial.println("Resolution: 12-bit");
    Serial.println("Samples per test: 10000");
}

void adcBenchmarkLoop() {

    for (size_t i = 0; i < NUM_TESTS; i++) {
        runBenchmark(tests[i]);
        delay(500);
    }

    Serial.println();
    Serial.println("===== ALL TESTS COMPLETE =====");
    Serial.println("Repeating in 5 seconds...");

    delay(5000);
}
