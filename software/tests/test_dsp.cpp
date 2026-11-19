#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <initializer_list>
#include "config/glasses_config.h"
#include "processing/signal_conditioner.h"
#include "processing/flicker_detector.h"
#include "control/attenuation_controller.h"
#include "control/feedback_controller.h"
static constexpr float PI=3.14159265359f;
static int failed=0;
static void check(bool ok,const char* msg) {
  std::printf("%s %s\n",ok?"PASS":"FAIL",msg);
  if(!ok) failed++;
}
struct Detection {FlickerDetector::Result result;int at;};
static Detection simulate(float hz,float amplitude,float dcWide,float dcNarrow,
                          float ampNarrow, bool withNoise=false) {
  SignalConditioner::ThreeChannel filters;
  filters.begin(Config::SAMPLE_RATE_HZ,Config::LOWPASS_CUTOFF_HZ,Config::BASELINE_CUTOFF_HZ);
  FlickerDetector::Window w{};
  // process 2 seconds to get a steady filter / baseline; capture final 375 ms.
  for(int i=0;i<2048;i++) {
    const float angle=2*PI*hz*i/Config::SAMPLE_RATE_HZ;
    const float noise=withNoise?2.2f*sinf(2*PI*151*i/Config::SAMPLE_RATE_HZ):0;
    float wide=dcWide+amplitude*sinf(angle)+noise;
    float narrow=dcNarrow+ampNarrow*sinf(angle)+noise;
    if(wide<0)wide=0;
    if(narrow<0)narrow=0;
    auto s=filters.push((uint16_t)std::lround(wide),(uint16_t)std::lround(narrow),
                        (uint16_t)std::lround(dcWide));
    if (i>=2048-Config::WINDOW_SAMPLES) {
      int idx=i-(2048-Config::WINDOW_SAMPLES);
      w.wide[idx]=s.wideAC;
      w.narrow[idx]=s.narrowAC;
      w.wideBaseline+=s.wideMean/Config::WINDOW_SAMPLES;
      w.narrowBaseline+=s.narrowMean/Config::WINDOW_SAMPLES;
      w.feedbackBaseline+=s.feedbackMean/Config::WINDOW_SAMPLES;
    }
  }
  return {FlickerDetector::analyze(w),0};
}
int main() {
  FlickerDetector::begin();
  const auto clear=simulate(10,0,300,290,0);
  check(!clear.result.candidate,"constant lighting does not trigger");
  for(float hz : {10.0f,30.0f,60.0f}) {
    auto d=simulate(hz,75,350,320,60,true);
    std::printf("%gHz -> detected %.2fHz wideAmp=%.1f narrowAmp=%.1f mod=%.3f prominence=%.2f candidate=%d\n",
      hz,d.result.frequencyHz,d.result.wideAcCounts,d.result.narrowAcCounts,
      d.result.wideModulation,d.result.peakProminence,d.result.candidate);
    check(std::fabs(d.result.frequencyHz-hz)<3.0f,"FFT identifies frequency in 10-60Hz range");
    check(d.result.candidate,"modulated light triggers unvalidated candidate");
  }
  const auto low=simulate(3,75,350,320,60,true);
  std::printf("3Hz -> estimated %.2fHz candidate=%d\n",low.result.frequencyHz,low.result.candidate);
  // 3Hz signal is only 1.125 periods/375ms; classification accuracy is NOT assured.
  auto above=simulate(100,75,350,320,60);
  std::printf("100Hz -> strongest candidate frequency %.2f; amp %.1f; candidate %d\n",
   above.result.frequencyHz,above.result.wideAcCounts,above.result.candidate);
  check(!above.result.candidate,"100Hz tone rejected by 3-60Hz band criterion");
  AttenuationController::Controller c;
  c.reset();
  auto high=simulate(30,100,350,350,90).result;
  auto d1=c.update(high);auto d2=c.update(high);
  check(d1.state==AttenuationController::State::Pending &&
        d2.state==AttenuationController::State::HazardCandidate,"two-window confirmation");
  for(int i=0;i<Config::HAZARD_CLEAR_WINDOWS;i++)c.update(clear.result);
  auto dc=c.update(clear.result);
  check(dc.state==AttenuationController::State::Clear,"clear hysteresis");
  FeedbackController::PI pi;
  pi.configure(0.4f,0.1f,0,1);
  float u=pi.update(0.5f,0.8f,0.05f);
  check(u>0 && u<1,"PI bounded output");
  std::printf("FAILED: %d\n",failed);
  return failed?1:0;
}
