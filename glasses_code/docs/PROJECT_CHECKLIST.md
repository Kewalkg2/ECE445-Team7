# Firmware implementation / ECE445 requirements checklist

Statuses: `[x]` coded or bench-tested; `[ ]` pending validation or implementation.
Source: Fall 2026 Design Document, especially §§1.3, 2.3, 2.5, 2.6.

## A — PCB and interfaces
- [x] ADC pins PA0 wide, PA1 narrow, PA4 feedback mapped to ADC1 channels 5/6/9.
- [x] ADC1 12-bit read with 16x hardware oversampling and per-conversion timeout.
- [ ] Validate all three sensors' voltage against independent multimeter/oscilloscope.
- [ ] Meet **~100k raw conversions/s per channel** (+/-5%) using timed scan/DMA and verify aggregate throughput. Current prototype ~16.4k raw conversions/s per channel.
- [ ] Verify all sensor conditioning outputs stay within 0.05-3.25 V over defined test lux range; collect minimum/maximum.
- [ ] Verify analog 995 Hz LPF and >=40 dB 10 kHz versus 60 Hz test using function generator/scope.

## B — digital signal processing
- [x] 90 Hz 2-pole Butterworth software low-pass on each channel (separate from physical analog LPF).
- [x] DC/baseline removal via 1 Hz running baseline.
- [x] Overlapping 375 ms analysis frames with 62.5 ms hop.
- [x] Windowed radix-2 FFT, magnitude spectrum, 3-60 Hz candidate estimation.
- [x] Compare wide/narrow normalized flicker amplitude and report directionality *candidate*.
- [x] FFT synthetic test at 10,30,60 Hz; reject constant and out-of-band test.
- [ ] Quantify robust 3 Hz performance; 375 ms contains just 1.125 cycles and cannot reliably meet +-5% frequency accuracy.
- [ ] Benchmark 3,10,30,60 Hz with real LED, ±1 Hz analog sensor tests and ±5% frequency error processing tests.
- [ ] Test false-positive performance under daylight, indoor LED lights, flashing displays, motion and bright-to-dark steps.
- [ ] Calibrate minimum modulation threshold and spectral prominence from measured light data.

## C — decision timing and hazard score
- [x] Provisional modulation-based candidate flag, two-window confirmation, four-window clearing hysteresis.
- [x] Candidate level 0..1 for a future calibrated attenuation map.
- [ ] Define hazard model and determine which frequencies/brightness ratios warrant intervention; this is not a clinically validated classifier.
- [ ] Measure photodiode onset -> DAC command <=0.5 s at each test frequency (note analysis window and algorithm execution time).
- [ ] Measure actual optical settling <=500 ms after DAC command; distinguish it from above detection-to-command requirement.
- [ ] Evaluate >=80% detection accuracy and <10% false positive rate using predeclared representative test cases.

## D — DAC, LCD and optics
- [x] Existing DAC60502 I2C driver included and preserved.
- [x] Optional software complementary 60 Hz drive module, 0–4 V commanded differential, compile-time gated OFF.
- [ ] Verify real DAC-to-LCD wiring, U3/U6 mapping, common-mode voltage, capacitive load, current and voltage limits.
- [ ] Scope +/-4.75 V differential target and mean DC within +/-50 mV over 10 seconds; current V4 conservative driver caps at 4.0 V until verified.
- [ ] Verify at least 10% optical transmission reduction at maximum versus minimum attenuation, and monotonic intermediate steps.
- [ ] Calibrate electrical voltage -> optical transmission and define verified fault state.
- [ ] Confirm 60 Hz service deadlines while the FFT and serial logging execute.

## E — feedback control / system
- [x] Three-channel acquisition captures the feedback sensor and logs its mean.
- [x] Standalone anti-windup PI module created, with host tests.
- [ ] Integrate optical PI after verifying feedback signal polarity, reference, and actuator response.
- [ ] Characterize feedback sensor under clear/intermediate/dark lens states; 3-repeat monotonic test.
- [ ] Verify that closed loop moves transmission the intended direction, stays within voltage bounds and does not oscillate.
- [ ] Validate power rails 3.3 V +/-5%, 5 V +/-5% during simultaneous operation.
- [ ] Demonstrate 8-hour continuous runtime on USB-C power bank.
- [ ] Verify physical optical leakage around goggles and wearer field-of-view.

## F — test process
- [x] Host DSP and controller unit tests passed in generating environment.
- [x] Application and DAC/LCD driver pass syntax checks with fake Arduino interfaces.
- [ ] Build with actual `pio run -e glasses` on user's macOS setup (toolchain unavailable here).
- [ ] Flash PCB and record full serial log, missed windows, ISR timing and in-band/out-of-band tests.
- [ ] Validate the final instrumented performance with logic analyzer and oscilloscope.

**Never represent these glasses as clinically validated or as a substitute for medical precautions.**
