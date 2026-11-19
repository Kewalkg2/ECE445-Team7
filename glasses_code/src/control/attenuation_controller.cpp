#include "control/attenuation_controller.h"
#include "config/glasses_config.h"
#include <math.h>
namespace AttenuationController {
float frequencyWeight(float hz) {
  // Linear experimental ramp: 3 Hz -> 0.40, 60 Hz -> 1.00.
  // Only used for confirmed candidate signals. Not clinically validated.
  const float t=fminf(1.0f,fmaxf(0.0f,(hz-Config::FLICKER_MIN_HZ)/
                               (Config::FLICKER_MAX_HZ-Config::FLICKER_MIN_HZ)));
  return 0.40f+0.60f*t;
}
void Controller::reset(){onCount_=offCount_=0;active_=false;}
Decision Controller::update(const FlickerDetector::Result& r) {
  if (r.candidate) {
    offCount_=0;
    if(onCount_<Config::HAZARD_CONFIRM_WINDOWS) ++onCount_;
    if(onCount_>=Config::HAZARD_CONFIRM_WINDOWS) active_=true;
  } else {
    onCount_=0;
    if(offCount_<Config::HAZARD_CLEAR_WINDOWS) ++offCount_;
    if(offCount_>=Config::HAZARD_CLEAR_WINDOWS) active_=false;
  }
  Decision d;
  d.state=active_?State::HazardCandidate:(r.candidate?State::Pending:State::Clear);
  d.directional=r.directional;
  if(active_) {
    const float modulation=fmaxf(r.wideModulation,r.narrowModulation);
    const float base=fminf(1.0f,fmaxf(0.0f,(modulation-Config::MIN_MODULATION)/0.5f));
    d.requestedLevel=base*frequencyWeight(r.frequencyHz);
  }
  return d;
}
const char* label(State s) {
  switch(s) {
    case State::Pending: return "PENDING";
    case State::HazardCandidate: return "CANDIDATE_UNVALIDATED";
    default: return "CLEAR";
  }
}
}
