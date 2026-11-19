
#pragma once

#include <Arduino.h>

namespace DAC60502 {

// 7-bit I2C addresses
constexpr uint8_t U3 = 0x48;
constexpr uint8_t U6 = 0x49;

// DAC60502 register addresses
constexpr uint8_t REG_DEVID  = 0x01;
constexpr uint8_t REG_SYNC   = 0x02;
constexpr uint8_t REG_CONFIG = 0x03;
constexpr uint8_t REG_GAIN   = 0x04;
constexpr uint8_t REG_DACA   = 0x08;
constexpr uint8_t REG_DACB   = 0x09;

// 12-bit DAC, nominal 0-5 V range
constexpr uint16_t MAX_CODE = 4095;
constexpr float FULL_SCALE_V = 5.0f;

// Channel numbers
constexpr uint8_t CHANNEL_A = 0;
constexpr uint8_t CHANNEL_B = 1;

// Low-level register access
bool writeRegister(uint8_t address, uint8_t reg, uint16_t value);
bool readRegister(uint8_t address, uint8_t reg, uint16_t &value);

// Initialize a DAC60502 for 0-5 V operation
bool init(uint8_t address);

// Write a 12-bit code
bool setRaw(uint8_t address, uint8_t channel, uint16_t code);

// Set a nominal output voltage in volts
bool setVoltage(uint8_t address, uint8_t channel, float voltage);

} // namespace DAC60502
