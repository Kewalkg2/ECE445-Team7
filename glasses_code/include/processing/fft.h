#pragma once
#include <stdint.h>
#include "config/glasses_config.h"
namespace FFT {
// In-place radix-2 complex FFT of 512 real samples, with zero-padded tail.
// Buffers are scratch space owned by the caller; precompute twiddles once.
void begin();
void transform(float real[Config::FFT_SIZE], float imag[Config::FFT_SIZE]);
void bitReverse(float real[Config::FFT_SIZE], float imag[Config::FFT_SIZE]);
void butterflyStage(float real[Config::FFT_SIZE], float imag[Config::FFT_SIZE], unsigned stage);
}
