#pragma once
#include <stdint.h>
namespace LCDDriver {
// Both DAC60502 units: U3 drives lens 1 and U6 drives lens 2.
// Each device's channel A/B drives opposite ends of one valve.
// Verify physical net mapping and current capabilities before enabling.
bool begin();
bool setAmplitude(float differentialVolts);
void service(uint32_t nowMicros);
void stop();
bool healthy();
}
