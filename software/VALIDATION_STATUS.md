# ECE 445 validation status (November 2026)

This is an **experimental prototype**, not a medically validated or safe-to-wear seizure prevention device. Do not use it for protection against photosensitive seizures.

## What is tested locally
- Blocking and cooperative FFT equivalence for synthetic signals.
- Synthetic scheduling under assumed DAC write times (not MCU measurements).
- Monotonic frequency weighting of the attenuation *command* (not optical attenuation).
- End-to-end synthetic wide/narrow light input -> cooperative FFT -> hysteresis -> controller -> differential DAC voltage math -> clearing.

## What remains unverified
- STM32 compilation and timing, actual ADC data, I2C throughput, and DAC physical voltage.
- Stable 60 Hz differential waveform on K and G; edge latency under worst-case firmware load.
- LCD optical response, actual attenuation and whether attenuation reduces *flicker modulation*, not just average brightness.
- Feedback photodiode orientation and closed-loop optical calibration.
- Battery runtime and safety requirements.

## Important implementation details
- Only DAC U3 (0x48) is used for the single connected LCD; A and B are complementary outputs.
- The experimental bench amplitude is capped to Config::MAX_DIFFERENTIAL_V (4 V nominal).
- Calibrated LCDDriver and bench waveform service are mutually exclusive.
- The calibrated LCD driver remains disabled by default: both safety gates in config/glasses_config.h are false.
- Frequency-increasing weighting is an **experimental user-requested control curve**, not a medically supported risk curve.
- A static attenuator can reduce total light without reducing relative flicker modulation.

## Run on a Mac/Linux computer
`bash tests/run_host_tests.sh`

**Passing host tests are not evidence that physical glasses work or are safe.**
