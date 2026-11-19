#pragma once
#include <stdint.h>

// PROTOTYPE SETTINGS — not a validated seizure prevention product.
namespace Config {
constexpr uint32_t SAMPLE_RATE_HZ = 1024;            // complete 3-channel frames/s
constexpr uint16_t WINDOW_SAMPLES = 384;             // 375 ms FFT observation
constexpr uint16_t FFT_SIZE = 512;                  // zero-padding, NOT 2 Hz true resolution
constexpr uint16_t HOP_SAMPLES = 64;                // 62.5 ms analysis step
constexpr uint16_t ADC_OVERSAMPLING = 16;           // 16 raw conversions/output/channel
constexpr float LOWPASS_CUTOFF_HZ = 90.0f;         // digital, after analog ~995 Hz LPF
constexpr float BASELINE_CUTOFF_HZ = 1.0f;         // suppress DC / slow variations
constexpr float FLICKER_MIN_HZ = 3.0f;
constexpr float FLICKER_MAX_HZ = 60.0f;

// Tunable, UNVALIDATED thresholds. Tune with controlled light-source trials.
constexpr float MIN_AVERAGE_ADC = 20.0f;
constexpr float MIN_AC_COUNTS = 8.0f;
constexpr float MIN_MODULATION = 0.08f;
constexpr float MIN_PROMINENCE = 2.8f;
constexpr float NARROW_WIDE_MODULATION_RATIO = 0.60f;
constexpr uint8_t HAZARD_CONFIRM_WINDOWS = 2;
constexpr uint8_t HAZARD_CLEAR_WINDOWS = 4;

// Two independent gates: both must be deliberately set true after bench validation.
constexpr bool ENABLE_LENS_ACTUATION = false;
constexpr bool OPTICAL_CALIBRATION_COMPLETE = false;
constexpr float MAX_DIFFERENTIAL_V = 4.0f;  // deliberately below nominal 5 V
constexpr uint32_t LCD_DRIVE_HZ = 60;
constexpr uint32_t REPORT_INTERVAL_MS = 500;
}
