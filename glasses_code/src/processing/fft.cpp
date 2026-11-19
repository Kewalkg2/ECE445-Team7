#include "processing/fft.h"
#include <math.h>
namespace FFT {
static float twiddleCos[Config::FFT_SIZE/2];
static float twiddleSin[Config::FFT_SIZE/2];
static bool ready=false;
void begin() {
  if (ready) return;
  for (unsigned k=0;k<Config::FFT_SIZE/2;k++) {
    const float a=-6.28318530718f*k/Config::FFT_SIZE;
    twiddleCos[k]=cosf(a); twiddleSin[k]=sinf(a);
  }
  ready=true;
}
void bitReverse(float re[Config::FFT_SIZE], float im[Config::FFT_SIZE]) {
  constexpr unsigned n=Config::FFT_SIZE;
  for (unsigned i=1,j=0;i<n;i++) {
    unsigned bit=n>>1;
    for (;j&bit;bit>>=1) j^=bit;
    j^=bit;
    if(i<j) {float t=re[i];re[i]=re[j];re[j]=t; t=im[i];im[i]=im[j];im[j]=t;}
  }
}
void butterflyStage(float re[Config::FFT_SIZE], float im[Config::FFT_SIZE], unsigned stage) {
  begin();
  const unsigned n=Config::FFT_SIZE;
  const unsigned len=1U<<(stage+1);
  if (len>n) return;
  const unsigned half=len>>1, step=n/len;
  for(unsigned start=0;start<n;start+=len) {
    for(unsigned j=0;j<half;j++) {
      const unsigned tw=j*step;
      const float wr=twiddleCos[tw], wi=twiddleSin[tw];
      const float vr=re[start+j+half]*wr-im[start+j+half]*wi;
      const float vi=re[start+j+half]*wi+im[start+j+half]*wr;
      const float ur=re[start+j],ui=im[start+j];
      re[start+j]=ur+vr;im[start+j]=ui+vi;
      re[start+j+half]=ur-vr;im[start+j+half]=ui-vi;
    }
  }
}
void transform(float re[Config::FFT_SIZE], float im[Config::FFT_SIZE]) {
  bitReverse(re,im);
  for(unsigned stage=0;(1U<<(stage+1))<=Config::FFT_SIZE;stage++)
    butterflyStage(re,im,stage);
}
}
