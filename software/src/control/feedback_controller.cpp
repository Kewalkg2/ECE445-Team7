#include "control/feedback_controller.h"
#include <math.h>
namespace FeedbackController {
void PI::configure(float kp,float ki,float minOutput,float maxOutput) {
  kp_=kp;ki_=ki;min_=minOutput;max_=maxOutput;reset();
}
void PI::reset(){integral_=0;}
float PI::update(float target,float measurement,float dtSeconds) {
  if (!(dtSeconds>0.0f && dtSeconds<1.0f) || !isfinite(target) || !isfinite(measurement))
    return fminf(max_,fmaxf(min_,integral_));
  // Error sign depends on actuator-to-transmission calibration; MUST verify
  // before closed-loop actuation. Positive error increases commanded drive.
  const float error=measurement-target;
  const float candidate=integral_+ki_*error*dtSeconds;
  const float unclamped=kp_*error+candidate;
  const float limited=fmaxf(min_,fminf(max_,unclamped));
  if((unclamped==limited) || (unclamped>max_ && error<0) ||
     (unclamped<min_ && error>0)) integral_=candidate;
  return limited;
}
}
