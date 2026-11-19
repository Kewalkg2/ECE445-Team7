#include "processing/flicker_detector.h"
#include "processing/fft.h"
#include <math.h>

namespace FlickerDetector {
static float taper[Config::WINDOW_SAMPLES];
static float sumTaper=0;
static bool initialized=false;
static float re[Config::FFT_SIZE], im[Config::FFT_SIZE];
static float spectrumWide[Config::FFT_SIZE/2+1];
static float spectrumNarrow[Config::FFT_SIZE/2+1];

void begin() {
  if(initialized) return;
  initialized=true;
  sumTaper=0;
  for(unsigned i=0;i<Config::WINDOW_SAMPLES;i++) {
    taper[i]=0.5f-0.5f*cosf(6.28318530718f*i/(Config::WINDOW_SAMPLES-1));
    sumTaper+=taper[i];
  }
  FFT::begin();
}
static void spectrum(const float *data, float* out) {
  float mean=0;
  for(unsigned i=0;i<Config::WINDOW_SAMPLES;i++) mean+=data[i];
  mean/=Config::WINDOW_SAMPLES;
  for(unsigned i=0;i<Config::FFT_SIZE;i++) {
    re[i]=(i<Config::WINDOW_SAMPLES) ? (data[i]-mean)*taper[i] : 0.0f;
    im[i]=0;
  }
  FFT::transform(re,im);
  for(unsigned i=0;i<=Config::FFT_SIZE/2;i++)
    out[i]=(2.0f/sumTaper)*sqrtf(re[i]*re[i]+im[i]*im[i]);
}
static float parabolicOffset(const float* power, unsigned i) {
  // Magnitude interpolation estimates the peak position; zero-padding only
  // interpolates existing finite-window information, not true resolution.
  const float a=power[i-1],b=power[i],c=power[i+1];
  const float denom=a-2*b+c;
  if(fabsf(denom)<1e-6f) return 0;
  float d=0.5f*(a-c)/denom;
  if(d>0.5f)d=0.5f;
  if(d< -0.5f)d=-0.5f;
  return d;
}
// The caller must keep its Window unchanged until stepIncremental returns true.
static const Window* activeWindow=nullptr;
static unsigned phase=0;
static unsigned fftStage=0;
static unsigned channel=0;
static void prepareChannel(const float* data) {
  float mean=0;
  for(unsigned i=0;i<Config::WINDOW_SAMPLES;i++) mean+=data[i];
  mean/=Config::WINDOW_SAMPLES;
  for(unsigned i=0;i<Config::FFT_SIZE;i++) {
    re[i]=(i<Config::WINDOW_SAMPLES)?(data[i]-mean)*taper[i]:0.0f;
    im[i]=0;
  }
}
static void finishChannel(float* out) {
  for(unsigned i=0;i<=Config::FFT_SIZE/2;i++)
    out[i]=(2.0f/sumTaper)*sqrtf(re[i]*re[i]+im[i]*im[i]);
}
void startIncremental(const Window& w) {
  begin();
  activeWindow=&w;
  channel=0;
  phase=0;
  fftStage=0;
}
bool stepIncremental(Result& output) {
  if(!activeWindow) return false;
  if(phase==0) {
    prepareChannel(channel==0?activeWindow->wide:activeWindow->narrow);
    phase=1;
  } else if(phase==1) {
    FFT::bitReverse(re,im);
    phase=2;
  } else if(phase==2) {
    FFT::butterflyStage(re,im,fftStage++);
    if((1U<<(fftStage+1))>Config::FFT_SIZE) phase=3;
  } else if(phase==3) {
    finishChannel(channel==0?spectrumWide:spectrumNarrow);
    if(channel==0) {channel=1;phase=0;fftStage=0;}
    else phase=4;
  } else {
    const Window& w=*activeWindow;
    Result r;
    r.wideBaseline=w.wideBaseline;
    r.narrowBaseline=w.narrowBaseline;
    r.feedbackBaseline=w.feedbackBaseline;
    const float binHz=(float)Config::SAMPLE_RATE_HZ/Config::FFT_SIZE;
    const unsigned first=(unsigned)ceilf(Config::FLICKER_MIN_HZ/binHz);
    const unsigned last=(unsigned)floorf(Config::FLICKER_MAX_HZ/binHz);
    float bestScore=0; unsigned best=first;
    for(unsigned i=first;i<=last;i++) {
      const float score=spectrumWide[i]/fmaxf(w.wideBaseline,Config::MIN_AVERAGE_ADC);
      if(score>bestScore) {bestScore=score;best=i;}
    }
    const float interp=parabolicOffset(spectrumWide,best);
    r.frequencyHz=fmaxf(Config::FLICKER_MIN_HZ,fminf(Config::FLICKER_MAX_HZ,(best+interp)*binHz));
    r.wideAcCounts=spectrumWide[best];r.narrowAcCounts=spectrumNarrow[best];
    r.wideModulation=r.wideAcCounts/fmaxf(w.wideBaseline,1.0f);
    r.narrowModulation=r.narrowAcCounts/fmaxf(w.narrowBaseline,1.0f);
    float background=0;unsigned backgroundCount=0;
    for(unsigned i=first;i<=last;i++) {
      if(i+2>=best && i<=best+2) continue;
      background+=spectrumWide[i];backgroundCount++;
    }
    background=backgroundCount?background/backgroundCount:0;
    r.peakProminence=spectrumWide[best]/fmaxf(background,0.5f);
    r.candidate=(r.wideAcCounts>=Config::MIN_AC_COUNTS &&
                 r.wideModulation>=Config::MIN_MODULATION &&
                 r.peakProminence>=Config::MIN_PROMINENCE &&
                 w.wideBaseline>=Config::MIN_AVERAGE_ADC);
    r.directional=false;
    output=r;
    activeWindow=nullptr;
    return true;
  }
  return false;
}
Result analyze(const Window& w) {
  begin();
  spectrum(w.wide,spectrumWide);
  spectrum(w.narrow,spectrumNarrow);
  Result r;
  r.wideBaseline=w.wideBaseline;
  r.narrowBaseline=w.narrowBaseline;
  r.feedbackBaseline=w.feedbackBaseline;
  const float binHz=(float)Config::SAMPLE_RATE_HZ/Config::FFT_SIZE;
  // Do not claim to identify sub-bin differences from just 375ms of data.
  const unsigned first=(unsigned)ceilf(Config::FLICKER_MIN_HZ/binHz);
  const unsigned last=(unsigned)floorf(Config::FLICKER_MAX_HZ/binHz);
  float bestScore=0;
  unsigned best=first;
  for(unsigned i=first;i<=last;i++) {
    float score=spectrumWide[i]/fmaxf(w.wideBaseline,Config::MIN_AVERAGE_ADC);
    if(score>bestScore) {bestScore=score;best=i;}
  }
  const float* peakSpectrum=spectrumWide;
  float interp=parabolicOffset(peakSpectrum,best);
  r.frequencyHz=fmaxf(Config::FLICKER_MIN_HZ,fminf(Config::FLICKER_MAX_HZ,(best+interp)*binHz));
  r.wideAcCounts=spectrumWide[best];
  r.narrowAcCounts=spectrumNarrow[best];
  r.wideModulation=r.wideAcCounts/fmaxf(w.wideBaseline,1.0f);
  r.narrowModulation=r.narrowAcCounts/fmaxf(w.narrowBaseline,1.0f);
  // Spectral prominence: compare strongest bin to other non-neighboring bins.
  float background=0; unsigned backgroundCount=0;
  for(unsigned i=first;i<=last;i++) {
    if(i+2>=best && i<=best+2) continue;
    background+=peakSpectrum[i]; backgroundCount++;
  }
  background=(backgroundCount>0)?background/backgroundCount:0;
  r.peakProminence=peakSpectrum[best]/fmaxf(background,0.5f);
  r.candidate=(r.wideAcCounts>=Config::MIN_AC_COUNTS &&
               r.wideModulation>=Config::MIN_MODULATION &&
               r.peakProminence>=Config::MIN_PROMINENCE &&
               w.wideBaseline>=Config::MIN_AVERAGE_ADC);
  r.directional=false; // Wide-only detection mode.
  return r;
}
}
