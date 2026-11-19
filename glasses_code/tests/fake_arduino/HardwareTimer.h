#pragma once
#include "Arduino.h"
class HardwareTimer {
 public:
 explicit HardwareTimer(void*){}
 void setOverflow(uint32_t,int){}
 void attachInterrupt(void(*)()){}
 void resume(){}
 void pause(){}
};
