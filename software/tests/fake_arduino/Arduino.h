#pragma once
#include <stdint.h>
#include <stddef.h>
#define PA_9 9
#define PA_10 10
#define TIM2 ((void*)0)
#define HERTZ_FORMAT 1
static inline uint32_t micros(){return 0;}
static inline uint32_t millis(){return 0;}
static inline void delay(uint32_t){}
static inline void delayMicroseconds(uint32_t){}
static inline void noInterrupts(){}
static inline void interrupts(){}
struct FakeSerial {
 template<class T> void print(T){}
 template<class T> void print(T,int){}
 template<class T> void println(T){}
 template<class T> void println(T,int){}
 void println(){}
 void begin(int){}
};
extern FakeSerial Serial;
