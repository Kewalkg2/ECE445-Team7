#include <cmath>
#include <cstdio>
#include <algorithm>
#include "processing/flicker_detector.h"
#include "control/attenuation_controller.h"
#include "config/glasses_config.h"
static int failures=0;
static void check(bool good,const char* what){std::printf("%s %s\n",good?"PASS":"FAIL",what);failures+=!good;}
static FlickerDetector::Window make(float hz,float amplitude){
  FlickerDetector::Window w{};
  w.wideBaseline=350;w.narrowBaseline=300;w.feedbackBaseline=240;
  for(int i=0;i<Config::WINDOW_SAMPLES;++i){
    const float v=amplitude*std::sin(6.283185307179586f*hz*i/Config::SAMPLE_RATE_HZ);
    w.wide[i]=v;w.narrow[i]=0.65f*v;
  }
  return w;
}
static FlickerDetector::Result process(const FlickerDetector::Window& w){
  FlickerDetector::startIncremental(w);FlickerDetector::Result result{};
  bool finished=false;
  for(int i=0;i<40;++i){if(FlickerDetector::stepIncremental(result)){finished=true;break;}}
  check(finished,"cooperative pipeline completes");return result;
}
int main(){
  FlickerDetector::begin();AttenuationController::Controller controller;controller.reset();
  auto quiet=process(make(20,0));
  auto d=controller.update(quiet);
  check(!quiet.candidate && d.requestedLevel==0,"steady illumination: clear command");
  auto flicker=process(make(20,75));
  check(flicker.candidate && std::fabs(flicker.frequencyHz-20)<2,"20 Hz flicker detected");
  d=controller.update(flicker);
  check(d.state==AttenuationController::State::Pending && d.requestedLevel==0,"first detection: pending");
  d=controller.update(flicker);
  check(d.state==AttenuationController::State::HazardCandidate && d.requestedLevel>0,"second detection: attenuation requested");
  const float commandV=Config::MAX_DIFFERENTIAL_V*std::clamp(d.requestedLevel,0.f,1.f);
  const float a=2.5f+0.5f*commandV,b=2.5f-0.5f*commandV;
  check(a>=0 && a<=5 && b>=0 && b<=5 && std::fabs((a-b)-commandV)<0.0001f,"DAC A/B differential command within nominal range");
  for(int i=0;i<Config::HAZARD_CLEAR_WINDOWS-1;++i){d=controller.update(quiet);check(d.state==AttenuationController::State::HazardCandidate,"clear hysteresis holds attenuation");}
  d=controller.update(quiet);check(d.state==AttenuationController::State::Clear && d.requestedLevel==0,"clear after hysteresis");
  float prev=-1;
  for(float hz : {10.f,20.f,30.f,40.f,50.f,60.f}){
    AttenuationController::Controller c;c.reset();auto r=process(make(hz,75));c.update(r);auto decision=c.update(r);
    char label[120];std::snprintf(label,sizeof(label),"real FFT -> controller increasing command at %.0f Hz",hz);
    check(r.candidate && decision.requestedLevel>prev,label);prev=decision.requestedLevel;
  }
  std::printf("END_TO_END_FAILED: %d\n",failures);return failures?1:0;
}
