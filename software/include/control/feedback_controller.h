#pragma once
namespace FeedbackController {
class PI {
 public:
  void configure(float kp, float ki, float minOutput, float maxOutput);
  void reset();
  float update(float target, float measurement, float dtSeconds);
 private:
  float kp_=0, ki_=0, min_=0, max_=1, integral_=0;
};
}
