# ECE 445 Adaptive Light-Filtering Glasses — actual firmware prototype

STM32L412CBT6 · STM32duino / PlatformIO · Team 7

**NOT A VALIDATED MEDICAL OR SEIZURE-PREVENTION DEVICE.** Detection thresholds are provisional. Lens drive is **disabled at compile time** until electrical, optical, and system-level safety testing has been completed. Do not wear this prototype while relying on it to protect against flashing lights.

## What this code really implements

| Subsystem | Files | Status |
|---|---|---|
| 3 photodiodes | `src/drivers/photodiodes.cpp` | STM32 ADC1 PA0/PA1/PA4 sequential reads; 12-bit; 16x *hardware* oversampling |
| Acquisition | `src/app/glasses_app.cpp` | TIM2 interrupt at 1024 three-channel frames/s; ISR only reads ADC + filters; ring buffer; sampled frames available during FFT |
| Analog low-pass | PCB, not software | Existing ~995 Hz active LPF from design document; code does **not** replace physical analog filter |
| Digital low-pass | `src/processing/signal_conditioner.cpp` | 2-pole Butterworth IIR, 90 Hz, on each channel |
| Baseline/AC isolation | `signal_conditioner.cpp` | 1 Hz running baseline; filter output minus baseline |
| FFT | `src/processing/fft.cpp` | Radix-2 complex 512-point FFT with 384 samples and 128 zero pads |
| Flicker detection | `src/processing/flicker_detector.cpp` | Hann taper, 3-60 Hz search, narrow/wide amplitudes, modulations, spectral prominence |
| Decision state machine | `src/control/attenuation_controller.cpp` | Consecutive-window confirmation, hysteresis, proportional *suggested* attenuation |
| DAC60502 support | `src/drivers/dac60502.cpp` | Existing U3/U6 DAC access retained, include path fixed |
| Lens AC waveform | `src/control/lcd_driver.cpp` | Optional 60 Hz complementary drive for both valves; **disabled**; needs calibration and scope check |
| PI feedback | `src/control/feedback_controller.cpp` | Bounded PI building block implemented; **not connected to actuator until optical plant is calibrated** |
| Bench diagnostics | `src/diagnostics/*` | Preserved, excluded from production build |
| Software tests | `tests/*` | Synthetic 10/30/60/3/100 Hz, constant light, hysteresis, PI, and C++ syntax checks |

## Pipeline

```
PA0 wide / PA1 narrow / PA4 feedback
    -> ADC1 (16x hardware oversample per channel)
    -> TIM2 1024 Hz interrupt (3 channels sequentially)
    -> per-channel 90 Hz Butterworth low-pass
    -> per-channel 1 Hz baseline estimator / baseline subtraction
    -> 384-sample ring buffer (375 ms window, 62.5 ms hop)
    -> 512-point zero-padded FFT, Hann window
    -> compare band peaks, normalized modulation, spectral prominence
    -> two-window candidate confirmation + clearing hysteresis
    -> serial log (no lens actuation by default)
```

**Spectral-resolution caveat:** 512 FFT bins spaced at 2 Hz *do not* mean 2 Hz true resolution, because only 384 actual samples were measured. The observation window is 375 ms (~2.67 Hz intrinsic resolution). At 3 Hz that contains ~1.125 periods, too little to guarantee ±5% accuracy or reliable early detection. Build a longer-window estimate or calibrated multi-period detector for the low end; evaluate against response latency separately. Satisfying the proposal's **0.5 s response target** is **not yet verified**, and may conflict with robust frequency discrimination near 3 Hz.

**100 kHz raw conversion requirement:** This version runs 1024 frames/s × 16 hardware oversamples = 16,384 raw conversions/s **per channel**, about 49,152 total, rather than 100,000 raw conversions/s/channel from the design document. The old 268.70 kSPS benchmark measured one-channel CPU-polled throughput *without* hardware oversampling, not simultaneous multichannel throughput. A TIM-triggered ADC scan + DMA/decimation system and oscilloscope validation are needed to meet the exact 100k/channel design requirement.

## Getting started on the Mac

1. Unzip and select the directory containing `platformio.ini` in VS Code using **File → Open Folder**.
2. Build first, before connecting any LCD load:

```bash
~/.platformio/penv/bin/pio run -e glasses
```

3. Upload over your known-good ST-Link connection:

```bash
~/.platformio/penv/bin/pio run -e glasses -t upload
```

4. Open the STM32 USB CDC port (your actual port name may change):

```bash
ls /dev/cu.usbmodem*
screen /dev/cu.usbmodem3642399031351 115200
```

5. During constant illumination, check `frames` increases ~1024 each second, `maxISR_us` stays well under ~977 us, and `skippedWindows`/`overBudget` remain low. Watch `nearRailFrames`, particularly under low light.
6. Apply a known pulsed LED at 10, 30, 60 Hz; record reported peak, modulation, candidate status, latency with scope/logic analyzer. Repeat at 3 Hz after improving low-frequency estimator.

## Host-side tests (no STM32 needed)

```bash
./tests/run_host_tests.sh
```

These compile the *actual portable DSP modules*, not independent Python stand-ins. An additional syntax-only check of the application and DAC/LCD code was run using fake Arduino headers. This does **not** replace a real STM32 PlatformIO build or on-board test. No STM32 compiler/PlatformIO installation was available in the environment where this archive was produced.

## Critical safety and electrical notes

- ADC mapping confirmed for STM32L412: PA0=ADC1_IN5, PA1=ADC1_IN6, PA4=ADC1_IN9. Recheck against actual PCB schematic and pin mux.
- DAC mapping assumed **U3 channel A/B for lens 1, U6 channel A/B for lens 2**. Confirm the actual PCB nets; past diagnostics labelled these as placeholders.
- Two output writes to each DAC are **not simultaneous** and 60 Hz service is driven from the foreground. Measure differential waveform, residual DC, voltage under real LCD load, and missed phase deadlines. A hardware-timed or buffered driver may be required.
- On I2C faults, firmware attempts equal-voltage output, but cannot guarantee a safe optical state if communications fail. Characterize the fail state and add hardware fail-safe.
- Feedback PI exists as a tested building block but is intentionally **not active**. You must calibrate sign, gain, transparency-versus-voltage and baseline sensor ratios before closing the loop.
- Current driver assumes 16 MHz actual SYSCLK from `src/system_clock.cpp`, even though board definition lists MCU *maximum* 80 MHz.
- ADC reads still use software triggers and a timer ISR, **not** true timer-triggered ADC/DMA. Current deadlines and jitter must be confirmed on hardware.

See `docs/PROJECT_CHECKLIST.md` for requirements and verification tasks.
