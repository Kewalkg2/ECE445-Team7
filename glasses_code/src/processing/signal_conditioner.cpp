#include "processing/signal_conditioner.h"
#include <math.h>

namespace SignalConditioner {
void Channel::reset() {
  initialized_ = false;
  x1_=x2_=y1_=y2_=baseline_=0.0f;
}
void Channel::configure(float fs, float lowpass, float baselineCutoff) {
  reset();
  // Bilinear-transform 2-pole Butterworth digital low-pass.
  // f < Nyquist; design filter operates on each oversampled ADC output.
  const float k = tanf(3.14159265358979323846f * lowpass / fs);
  const float norm = 1.0f / (1.0f + 1.41421356237f*k + k*k);
  b0_ = k*k*norm;
  b1_ = 2.0f*b0_;
  b2_ = b0_;
  a1_ = 2.0f*(k*k-1.0f)*norm;
  a2_ = (1.0f-1.41421356237f*k+k*k)*norm;
  baselineAlpha_ = 1.0f-expf(-6.28318530718f*baselineCutoff/fs);
}
void Channel::process(uint16_t raw, float &ac, float &mean) {
  const float x = static_cast<float>(raw);
  if (!initialized_) {
    initialized_=true;
    x1_=x2_=y1_=y2_=baseline_=x; // no startup impulse
  }
  const float y=b0_*x+b1_*x1_+b2_*x2_-a1_*y1_-a2_*y2_;
  x2_=x1_; x1_=x; y2_=y1_; y1_=y;
  baseline_ += baselineAlpha_*(y-baseline_);
  mean=baseline_;
  ac=y-baseline_;
}
void ThreeChannel::begin(float fs, float lowpass, float baselineCutoff) {
  wide_.configure(fs,lowpass,baselineCutoff);
  narrow_.configure(fs,lowpass,baselineCutoff);
  feedback_.configure(fs,lowpass,baselineCutoff);
}
Output ThreeChannel::push(uint16_t wide, uint16_t narrow, uint16_t feedback) {
  Output o{};
  wide_.process(wide,o.wideAC,o.wideMean);
  narrow_.process(narrow,o.narrowAC,o.narrowMean);
  feedback_.process(feedback,o.feedbackAC,o.feedbackMean);
  return o;
}
}
