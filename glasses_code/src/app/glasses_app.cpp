#include <Arduino.h>

#include <HardwareTimer.h>

#include <Wire.h>

#include "drivers/dac60502.h"

#include "app/glasses_app.h"

#include "config/glasses_config.h"

#include "drivers/photodiodes.h"

#include "processing/signal_conditioner.h"

#include "processing/flicker_detector.h"

#include "control/attenuation_controller.h"

#include "control/lcd_driver.h"



// Timer callback performs short, bounded register-level ADC reads and IIR.

// It NEVER performs FFT, I2C, or serial I/O. FFT runs in the main loop.

namespace GlassesApp {

struct RingEntry {

  float wideAC, narrowAC;

  float wideDC, narrowDC, feedbackDC;

};

static RingEntry history[Config::WINDOW_SAMPLES] = {};

static volatile uint16_t writeIndex=0;

static volatile uint32_t sequence=0;

static volatile uint32_t isrFaults=0;

static volatile bool adcFault=false;

static volatile uint32_t maxIsrUs=0;

static SignalConditioner::ThreeChannel conditioner;

static HardwareTimer* sampleTimer=nullptr;

static FlickerDetector::Window frame;

static FlickerDetector::Result result;

static AttenuationController::Controller controller;

static AttenuationController::Decision decision;

static uint32_t lastAnalyzed=0;
static bool analysisRunning=false;
static uint32_t analysisStartedUs=0;
static uint32_t maxStepUs=0;

static uint32_t skippedWindows=0;

static uint32_t reportsAt=0;

static uint32_t maxAnalysisUs=0;

static uint32_t processingOverBudget=0;

static volatile uint32_t nearRailReadings=0;

static bool ok=false;



// EXPERIMENTAL DAC timing test only. This is not a validated LCD driver.
// Do not use these glasses for seizure protection.

static bool dacReady=false;

static bool dacWriteOK=false;

static float benchDiffV=0.0f; // commanded signed A-B voltage

static float requestedAmplitude=0.0f;

static constexpr uint32_t HALF_PERIOD_US=8333; // \~60 Hz polarity reversals

static uint32_t nextEdgeUs=0;

static uint32_t missedEdges=0;

static uint32_t maxEdgeLateUs=0;

static uint32_t polarityWrites=0;

static bool lastPositive=false;



static bool writeBenchDAC(float differentialV) {

  const float a=2.5f+0.5f*differentialV;

  const float b=2.5f-0.5f*differentialV;

  bool success=true;

  success = DAC60502::setVoltage(DAC60502::U3,DAC60502::CHANNEL_A,a) && success;

  success = DAC60502::setVoltage(DAC60502::U3,DAC60502::CHANNEL_B,b) && success;

  success = DAC60502::setVoltage(DAC60502::U6,DAC60502::CHANNEL_A,a) && success;

  success = DAC60502::setVoltage(DAC60502::U6,DAC60502::CHANNEL_B,b) && success;

  dacWriteOK=success;

  if(success) benchDiffV=differentialV;

  return success;

}



// No I2C in an interrupt. Schedule edges in foreground and report lateness.

static void serviceBenchWaveform() {

  if(!dacReady) return;

  const uint32_t now=micros();

  if((int32_t)(now-nextEdgeUs)<0) return;

  const uint32_t late=now-nextEdgeUs;

  if(late>maxEdgeLateUs) maxEdgeLateUs=late;

  const uint32_t edgesElapsed=1U+late/HALF_PERIOD_US;

  if(edgesElapsed>1U) missedEdges+=edgesElapsed-1U;

  nextEdgeUs+=edgesElapsed*HALF_PERIOD_US;

  // Derive sign from elapsed half-cycles (not from number of service calls).

  if(edgesElapsed&1U) lastPositive=!lastPositive;

  const float signedVoltage=lastPositive ? requestedAmplitude : -requestedAmplitude;

  if(!writeBenchDAC(signedVoltage)) {

    dacReady=false;
    Serial.println("BENCH AC DAC WRITE FAILED; DISCONNECT OUTPUTS");
  } else {

    ++polarityWrites;

  }

}



static void onSample() {

  if (adcFault) return;

  const uint32_t begun=micros();

  Photodiodes::Sample raw{};

  if (!Photodiodes::read(raw)) {

    adcFault=true; ++isrFaults; return;

  }

  // <\~50mV or >\~3.25V @ Vref=3.3V violates sensor-headroom test target.

  if (raw.wide < 62 || raw.wide > 4033 ||

      raw.narrow < 62 || raw.narrow > 4033 ||

      raw.feedback < 62 || raw.feedback > 4033) ++nearRailReadings;

  const SignalConditioner::Output s=conditioner.push(raw.wide,raw.narrow,raw.feedback);

RingEntry &e=history[writeIndex];

  e.wideAC=s.wideAC;

  e.narrowAC=s.narrowAC;

  e.wideDC=s.wideMean;

  e.narrowDC=s.narrowMean;

  e.feedbackDC=s.feedbackMean;

  writeIndex=(writeIndex+1)%Config::WINDOW_SAMPLES;

  ++sequence;

  const uint32_t duration=micros()-begun;

  if(duration>maxIsrUs)maxIsrUs=duration;

  if(duration >= (1000000UL/Config::SAMPLE_RATE_HZ)) {

    adcFault=true; ++isrFaults; // ISR exceeded its entire sampling period

  }

}

static bool snapshot() {

  if (adcFault) return false;

  noInterrupts();

  const uint32_t current=sequence;

  if (current<Config::WINDOW_SAMPLES ||

      (uint32_t)(current-lastAnalyzed)<Config::HOP_SAMPLES) {

    interrupts();return false;

  }

  if(lastAnalyzed && current-lastAnalyzed > 2U*Config::HOP_SAMPLES)

    ++skippedWindows;

  const uint16_t start=writeIndex;

  float wdc=0,ndc=0,fdc=0;

  // Critical section: \~384 short copies; profile resulting timer jitter.

  for(uint16_t i=0;i<Config::WINDOW_SAMPLES;i++) {

    const RingEntry &s=history[(start+i)%Config::WINDOW_SAMPLES];

    frame.wide[i]=s.wideAC;

    frame.narrow[i]=s.narrowAC;

    wdc+=s.wideDC;

    ndc+=s.narrowDC;

    fdc+=s.feedbackDC;

  }

  frame.wideBaseline=wdc/Config::WINDOW_SAMPLES;

  frame.narrowBaseline=ndc/Config::WINDOW_SAMPLES;

  frame.feedbackBaseline=fdc/Config::WINDOW_SAMPLES;

  lastAnalyzed=current;

  interrupts();

  return true;

}

void begin() {

  Serial.begin(115200);

  delay(1200);

  Serial.println("ECE445 Adaptive Glasses | FFT TO DAC BENCH TEST");

  Serial.println("PA0 wide, PA1 narrow, PA4 feedback; ADC 16x hw oversampling");

  Serial.println("2-pole digital LPF 90Hz; DC baseline 1Hz; 384pt window / 512pt FFT");

  Serial.println("Only PA0 controls detection; PA1/PA4 sampled for diagnostics");

  Serial.println("Bench waveform test; verify actual K-G drive on oscilloscope");

  conditioner.begin(Config::SAMPLE_RATE_HZ,Config::LOWPASS_CUTOFF_HZ,

                    Config::BASELINE_CUTOFF_HZ);

  FlickerDetector::begin();

  controller.reset();

  Wire.setSDA(PA10);

  Wire.setSCL(PA9);

  Wire.begin();

  Wire.setClock(100000);

  const bool benchMode = !(Config::ENABLE_LENS_ACTUATION && Config::OPTICAL_CALIBRATION_COMPLETE);

  if (benchMode) {
    const bool u3OK = DAC60502::init(DAC60502::U3);
    const bool u6OK = DAC60502::init(DAC60502::U6);
    dacReady = u3OK && u6OK;

    Serial.print("U3 initialization: "); Serial.println(u3OK ? "OK" : "FAILED");
    Serial.print("U6 initialization: "); Serial.println(u6OK ? "OK" : "FAILED");

    if (dacReady) dacReady = writeBenchDAC(0.0f);
  } else {
    dacReady = false;
  }

  nextEdgeUs=micros()+HALF_PERIOD_US;

  Serial.print("BENCH DAC init: ");Serial.println(dacReady ? "OK" : "FAILED");

  ok=Photodiodes::begin();

  if (!ok) {

    Serial.print("ADC init failed: ");Serial.println(Photodiodes::lastError());

    return;

  }

  // Never enable real actuation until both compilation-time gates are true.

  if (Config::ENABLE_LENS_ACTUATION && Config::OPTICAL_CALIBRATION_COMPLETE) {

    if (!LCDDriver::begin()) {

      Serial.println("LCD initialization failed; no actuation");

      ok=false;return;

    }

  }

  sampleTimer=new HardwareTimer(TIM2);

  sampleTimer->setOverflow(Config::SAMPLE_RATE_HZ,HERTZ_FORMAT);

  sampleTimer->attachInterrupt(onSample);

  sampleTimer->resume();

  reportsAt=millis();

  Serial.println("Timer started; FFT processing in foreground");

}

void update() {

  if(!ok) {delay(100);return;}

  if(adcFault) {

    sampleTimer->pause();

    if (dacReady) writeBenchDAC(0.0f);

    Serial.print("FATAL ADC ISR FAULT: ");Serial.println(Photodiodes::lastError());

    ok=false;return;

  }


  if (!(Config::ENABLE_LENS_ACTUATION && Config::OPTICAL_CALIBRATION_COMPLETE)) serviceBenchWaveform();

  if(!analysisRunning && snapshot()) {
    analysisStartedUs=micros();
    FlickerDetector::startIncremental(frame);
    analysisRunning=true;
  }
  if(analysisRunning) {
    const uint32_t stepStart=micros();
    const bool finished=FlickerDetector::stepIncremental(result);
    const uint32_t stepUs=micros()-stepStart;
    if(stepUs>maxStepUs) maxStepUs=stepUs;
    // Give the DAC a chance to reverse polarity after every FFT stage.
    if (!(Config::ENABLE_LENS_ACTUATION && Config::OPTICAL_CALIBRATION_COMPLETE)) serviceBenchWaveform();
    if(finished) {
    analysisRunning=false;
    decision=controller.update(result);

    requestedAmplitude = (decision.state == AttenuationController::State::HazardCandidate)
    ? Config::MAX_DIFFERENTIAL_V * constrain(decision.requestedLevel, 0.0f, 1.0f) : 0.0f;

    const uint32_t elapsed=micros()-analysisStartedUs;

    if (elapsed>maxAnalysisUs) maxAnalysisUs=elapsed;

    if (elapsed > (1000000UL*Config::HOP_SAMPLES/Config::SAMPLE_RATE_HZ))

      ++processingOverBudget;

    // PROTOTYPE ONLY. Feedback PI must be calibrated against measured optical

    // transmission. Currently the controller ONLY REPORTS its recommendation.

    if(Config::ENABLE_LENS_ACTUATION && Config::OPTICAL_CALIBRATION_COMPLETE) {

      if (!LCDDriver::setAmplitude(decision.requestedLevel*Config::MAX_DIFFERENTIAL_V)) {

        LCDDriver::stop();ok=false;

      }

    }
    }
  }

  if(Config::ENABLE_LENS_ACTUATION && Config::OPTICAL_CALIBRATION_COMPLETE)
    LCDDriver::service(micros());


  if (!(Config::ENABLE_LENS_ACTUATION && Config::OPTICAL_CALIBRATION_COMPLETE)) serviceBenchWaveform();

  const uint32_t now=millis();

  if ((uint32_t)(now-reportsAt)>=Config::REPORT_INTERVAL_MS) {

    reportsAt=now;

uint32_t count,maximum,rails;

    noInterrupts();count=sequence;maximum=maxIsrUs;rails=nearRailReadings;interrupts();

    Serial.print("frames=");Serial.print(count);

    Serial.print(" skippedWindows=");Serial.print(skippedWindows);

    Serial.print(" maxISR_us=");Serial.print(maximum);

    Serial.print(" maxFFT_us=");Serial.print(maxAnalysisUs);

    Serial.print(" maxStep_us=");Serial.print(maxStepUs);
    Serial.print(" overBudget=");Serial.print(processingOverBudget);

    Serial.print(" nearRailFrames=");Serial.print(rails);

    Serial.print(" wideADC_mean=");Serial.print(result.wideBaseline,1);

    Serial.print(" narrowADC_mean=");Serial.print(result.narrowBaseline,1);

    Serial.print(" feedbackADC_mean=");Serial.print(result.feedbackBaseline,1);

    Serial.print(" hz=");Serial.print(result.frequencyHz,2);

    Serial.print(" wideAmp=");Serial.print(result.wideAcCounts,1);

    Serial.print(" narrowAmp=");Serial.print(result.narrowAcCounts,1);

    Serial.print(" wideMod=");Serial.print(result.wideModulation,3);

    Serial.print(" narrowMod=");Serial.print(result.narrowModulation,3);

    Serial.print(" prominence=");Serial.print(result.peakProminence,2);

    Serial.print(" feedbackDC=");Serial.print(result.feedbackBaseline,1);

    Serial.print(" state=");Serial.print(AttenuationController::label(decision.state));

    Serial.print(" LCD=");

    Serial.print(decision.state==AttenuationController::State::HazardCandidate ? "ATTENUATE" : "CLEAR");

    Serial.print(" proposedLevel=");Serial.print(decision.requestedLevel,2);

    // Simulation only: proposed differential LCD drive amplitude in volts.

    const float proposedVoltage = decision.requestedLevel * Config::MAX_DIFFERENTIAL_V;

    Serial.print(" LCD_V=");Serial.print(proposedVoltage,2);

    Serial.print(" AC_amplitude_V=");Serial.print(requestedAmplitude,2);

    Serial.print(" AC_signed_V=");Serial.print(benchDiffV,2);

    Serial.print(" AC_missedEdges=");Serial.print(missedEdges);

    Serial.print(" AC_maxLate_us=");Serial.print(maxEdgeLateUs);

    Serial.print(" AC_writes=");Serial.print(polarityWrites);

    Serial.print(" BENCH_DAC=");Serial.println(dacReady && dacWriteOK ? "OK" : "FAILED");

  }

}

}
