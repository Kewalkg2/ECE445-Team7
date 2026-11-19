#include <cmath>
#include <cstdio>
#include <cstdint>
#include <algorithm>
#include "processing/flicker_detector.h"
#include "control/attenuation_controller.h"
#include "config/glasses_config.h"
static int failures=0;
static void test(bool ok,const char* label){std::printf("%s %s\n",ok?"PASS":"FAIL",label);failures+=!ok;}
static FlickerDetector::Window makeWindow(float hz,float amp,float noise=0,float secondary=0) {
  FlickerDetector::Window w{};
  w.wideBaseline=350;w.narrowBaseline=320;w.feedbackBaseline=250;
  for(unsigned i=0;i<Config::WINDOW_SAMPLES;i++) {
    const float t=float(i)/Config::SAMPLE_RATE_HZ;
    const float wave=amp*std::sin(6.28318530718f*hz*t)+noise*std::sin(6.28318530718f*173*t)
      +secondary*std::sin(6.28318530718f*41*t);
    w.wide[i]=wave;
    w.narrow[i]=0.65f*wave;
  }
  return w;
}
static bool approx(float a,float b,float tol=0.0001f) {
  return std::fabs(a-b)<=tol*std::max(1.0f,std::max(std::fabs(a),std::fabs(b)));
}
static void equivalence(float hz,float amp,float noise,float second) {
  auto w=makeWindow(hz,amp,noise,second);
  auto blocking=FlickerDetector::analyze(w);
  FlickerDetector::startIncremental(w);
  FlickerDetector::Result coop{};
  int steps=0;bool done=false;
  while(steps<40 && !(done=FlickerDetector::stepIncremental(coop)))++steps;
  ++steps;
  char name[180];std::snprintf(name,sizeof(name),"incremental completes: %.1f Hz (steps=%d)",hz,steps);
  test(done && steps==25,name); // 2*(prepare+bitreverse+9 stages+finish)+final = 25
  std::snprintf(name,sizeof(name),"FFT result equivalence: %.1f Hz",hz);
  test(done && approx(blocking.frequencyHz,coop.frequencyHz) &&
       approx(blocking.wideAcCounts,coop.wideAcCounts) &&
       approx(blocking.narrowAcCounts,coop.narrowAcCounts) &&
       approx(blocking.wideModulation,coop.wideModulation) &&
       approx(blocking.peakProminence,coop.peakProminence) &&
       blocking.candidate==coop.candidate,name);
  if(amp>30 && hz>=6 && hz<=60 && second==0) {
    std::snprintf(name,sizeof(name),"frequency accuracy: %.1f Hz -> %.2f Hz",hz,coop.frequencyHz);
    test(std::fabs(coop.frequencyHz-hz)<2.0f,name);
  }
}
static void controllerTests() {
  float previous=-1;
  for(float hz : {3.f,10.f,20.f,30.f,40.f,50.f,60.f}) {
    AttenuationController::Controller c;c.reset();
    FlickerDetector::Result r{};r.candidate=true;r.frequencyHz=hz;
    r.wideModulation=0.28f;r.narrowModulation=0.10f;
    c.update(r);auto d=c.update(r);
    char label[140];std::snprintf(label,sizeof(label),"attenuation monotonic at %.0f Hz -> %.4f",hz,d.requestedLevel);
    test(d.state==AttenuationController::State::HazardCandidate && d.requestedLevel>previous &&
         d.requestedLevel>=0 && d.requestedLevel<=1,label);
    previous=d.requestedLevel;
  }
  for(float hz : {-10.f,3.f,60.f,120.f}) {
    const float w=AttenuationController::frequencyWeight(hz);
    test(w>=0.4f-1e-6f && w<=1.f+1e-6f,"frequency weight bounded");
  }
}
// Synthetic scheduler: assumes one step consumes at most stepCostUs and each DAC
// update consumes dacCostUs. Does NOT benchmark actual STM32 execution or I2C.
static uint32_t missedInSimulation(uint32_t stepCostUs,uint32_t dacCostUs,uint32_t durationUs) {
  constexpr uint32_t halfPeriodUs=1000000U/(2*Config::LCD_DRIVE_HZ);
  uint32_t now=0,next=halfPeriodUs,missed=0;
  while(now<durationUs) {
    now+=stepCostUs;
    if(now>=next) {
      const uint32_t elapsed=1U+(now-next)/halfPeriodUs;
      missed+=elapsed-1U;
      now+=dacCostUs;
      next+=elapsed*halfPeriodUs;
    }
  }
  return missed;
}
int main(){
  FlickerDetector::begin();
  for(float hz : {3.f,5.f,10.f,15.f,20.f,30.f,45.f,60.f,100.f})equivalence(hz,75,3,0);
  equivalence(20,0,0,0);equivalence(20,75,15,40);
  controllerTests();
  for(uint32_t step : {500U,1500U,4000U}) {
    const auto missed=missedInSimulation(step,800,1000000);
    char label[150];std::snprintf(label,sizeof(label),"synthetic scheduler step=%u us DAC=800 us: missed=%u",step,missed);
    test(missed==0,label);
  }
  const auto missed=missedInSimulation(20000,800,1000000);
  std::printf("20ms blocking step simulated missed deadlines: %u\n",missed);
  test(missed>0,"synthetic scheduler detects missed periods for blocking FFT");
  std::printf("FAILED: %d\n",failures);return failures?1:0;
}
