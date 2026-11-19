# Wide-only FFT / simulated LCD control

PA0 alone selects the strongest 3–60 Hz FFT peak and determines the hazard candidate. PA1 and PA4 remain sampled and reported but do not affect the decision. The DAC and LCD are not driven (actuation remains disabled in `include/config/glasses_config.h`).

`wideADC_mean`, `narrowADC_mean`, and `feedbackADC_mean` are filtered ADC baselines, not instantaneous raw ADC samples. `hz` always shows the strongest in-band FFT peak even for noise. `LCD=ATTENUATE` is printed only after the unvalidated amplitude, modulation, prominence and persistence checks pass. `proposedLevel` is unitless and not a calibrated LCD voltage.

Upload with `~/.platformio/penv/bin/pio run -e glasses -t upload` and view USB serial at 115200 baud.

Prototype only; not validated protection against photosensitive seizures.
