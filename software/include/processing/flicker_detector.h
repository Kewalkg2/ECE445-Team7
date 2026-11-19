#pragma once
#include <stdint.h>
#include "config/glasses_config.h"
namespace FlickerDetector {
struct Window {
  float wide[Config::WINDOW_SAMPLES];
  float narrow[Config::WINDOW_SAMPLES];
  float wideBaseline=0, narrowBaseline=0, feedbackBaseline=0;
};
struct Result {
  float frequencyHz=0;
  float wideAcCounts=0, narrowAcCounts=0;
  float wideModulation=0, narrowModulation=0;
  float peakProminence=0;
  float wideBaseline=0, narrowBaseline=0, feedbackBaseline=0;
  bool candidate=false, directional=false;
};
void begin();
Result analyze(const Window& data);
// Non-blocking cooperative analysis: begin one window, then run one stage per call.
void startIncremental(const Window& data);
bool stepIncremental(Result& output);
}
