#pragma once
#include <Arduino.h>
namespace Photodiodes {
struct Sample { uint16_t wide, narrow, feedback; uint32_t timestampUs; };
bool begin();
bool read(Sample &out);
const char* lastError();
}
