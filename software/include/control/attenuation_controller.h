#pragma once
#include <stdint.h>
#include "processing/flicker_detector.h"
namespace AttenuationController {
enum class State : uint8_t { Clear, Pending, HazardCandidate };
struct Decision {
  State state=State::Clear;
  float requestedLevel=0; // [0,1], never a validated medical dose
  bool directional=false;
};
// Experimental monotonic frequency multiplier, not a medical risk model.
float frequencyWeight(float frequencyHz);
class Controller {
 public:
  void reset();
  Decision update(const FlickerDetector::Result& result);
 private:
  uint8_t onCount_=0, offCount_=0;
  bool active_=false;
};
const char* label(State state);
}
