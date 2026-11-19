# Implementation plan

## Milestone 0 — preserve working diagnostics
- Keep existing tests operational; record known-good serial and scope results.
- Verify board pin mapping and I2C DAC channel-to-lens wiring.

## Milestone 1 — integrated sensing
- Build an ADC driver with selectable 16x oversampling and three ADC inputs.
- Timer-triggered acquisition, ideally DMA and bounded ring buffers.
- Timestamp and log sample continuity, channel skew, saturation, effective rate.
- Confirm ~100 kHz raw conversions/channel target is achievable simultaneously.

## Milestone 2 — signal processing
- DC/baseline removal; select anti-alias/decimation strategy.
- Frequency detection at 3, 10, 30, 60 Hz; characterize amplitude and noise.
- Use overlapping short windows for early detection and longer windows for accurate 3 Hz estimation; establish latency budget.

## Milestone 3 — actuator service
- Initialize two DAC60502 devices once; maintain 60 Hz differential waveform without blocking acquisition.
- Map command amplitude to measured optical transmission; verify DC balance and loading.
- On I2C failure or ADC saturation, log fault and enter a characterized safe state (must be defined experimentally).

## Milestone 4 — hazard logic and feedback
- Define relative flicker modulation metric and calibrated thresholds.
- Fuse wide/narrow photodiodes; hysteresis and debounce.
- Feedback photodiode control; PI only after plant response characterization.

## Milestone 5 — verification
- Detection accuracy / false-positive matrix; onset-to-command <=500 ms.
- Optical settling and attenuation tests; 8-hour endurance; power budget.

## Open decisions
- Confirm intended ADC sample rate is raw conversion rate per channel or averaged output rate.
- Confirm lens wiring and desired AC drive frequency.
- Confirm whether both lenses are always driven together or independently.
- Define medically conservative user-facing limitations; no guarantee of seizure prevention.
