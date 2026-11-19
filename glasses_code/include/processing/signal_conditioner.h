#pragma once
#include <stdint.h>

namespace SignalConditioner {
struct Output {
  float wideAC, narrowAC, feedbackAC;
  float wideMean, narrowMean, feedbackMean;
};
class Channel {
 public:
  void reset();
  void configure(float samplingHz, float lowpassHz, float baselineHz);
  void process(uint16_t raw, float &ac, float &mean);
 private:
  bool initialized_ = false;
  float b0_=0, b1_=0, b2_=0, a1_=0, a2_=0;
  float x1_=0, x2_=0, y1_=0, y2_=0;
  float baseline_=0, baselineAlpha_=0;
};
class ThreeChannel {
 public:
  void begin(float samplingHz, float lowpassHz, float baselineHz);
  Output push(uint16_t wide, uint16_t narrow, uint16_t feedback);
 private:
  Channel wide_, narrow_, feedback_;
};
}
