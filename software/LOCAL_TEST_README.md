# ECE 445 local FFT / scheduling / attenuation tests

Run from project root: `bash tests/run_host_tests.sh` (requires g++). No board needed.

- `tests/test_dsp.cpp`: original signal conditioning and detection regression tests.
- `tests/test_cooperative.cpp`: direct equivalence between blocking and cooperative FFT (including mixed and noisy signals), frequency accuracy, frequency-dependent attenuation, and a synthetic DAC deadline simulation.
- `src/control/attenuation_controller.cpp`: experimental monotonic frequency weighting, from 0.40 at 3 Hz to 1.00 at 60 Hz, multiplied by modulation-based command. This is NOT a clinically validated risk curve.

**Limits:** scheduler simulation uses assumed per-step and DAC-write durations, not measured STM32 timing. Passing it cannot verify actual I2C waveform or LCD optical behavior. The 60 Hz drive is AC polarity inversion, not phase-synchronized cancellation of ambient flicker; static attenuation may not reduce percentage modulation. Hardware measurements and optical calibration remain necessary.
