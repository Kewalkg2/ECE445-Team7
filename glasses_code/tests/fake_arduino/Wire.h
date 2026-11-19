#pragma once
#include <stdint.h>
struct FakeWire {
 void setSDA(int){}
 void setSCL(int){}
 void begin(){}
 void setClock(uint32_t){}
 void beginTransmission(uint8_t){}
 void write(uint8_t){}
 uint8_t endTransmission(bool=true){return 0;}
 uint8_t requestFrom(uint8_t,uint8_t){return 2;}
 uint8_t read(){return 0;}
};
extern FakeWire Wire;
