
#include "drivers/dac60502.h"
#include <Wire.h>

namespace DAC60502 {

bool writeRegister(uint8_t address, uint8_t reg, uint16_t value) {
    Wire.beginTransmission(address);

    // Register address
    Wire.write(reg);

    // 16-bit register data, MSB first
    Wire.write(static_cast<uint8_t>(value >> 8));
    Wire.write(static_cast<uint8_t>(value & 0xFF));

    return Wire.endTransmission() == 0;
}

bool readRegister(uint8_t address, uint8_t reg, uint16_t &value) {
    Wire.beginTransmission(address);
    Wire.write(reg);

    // Repeated START for register read
    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    if (Wire.requestFrom(address, static_cast<uint8_t>(2)) != 2) {
        return false;
    }

    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();

    value = (static_cast<uint16_t>(msb) << 8) | lsb;
    return true;
}

bool init(uint8_t address) {
    if (!writeRegister(address, REG_CONFIG, 0x0000))
        return false;

    if (!writeRegister(address, REG_GAIN, 0x0003))
        return false;

    // Both channels update immediately on I2C writes.
    if (!writeRegister(address, REG_SYNC, 0x0300))
        return false;

    if (!setRaw(address, CHANNEL_A, 0))
        return false;

    if (!setRaw(address, CHANNEL_B, 0))
        return false;

    return true;
}

bool setRaw(uint8_t address, uint8_t channel, uint16_t code) {
    if (code > MAX_CODE)
        return false;

    if (channel != CHANNEL_A && channel != CHANNEL_B)
        return false;

    uint8_t reg = (channel == CHANNEL_A) ? REG_DACA : REG_DACB;

    // DAC60502 stores 12-bit data in bits [15:4].
    uint16_t data = static_cast<uint16_t>(code << 4);

    return writeRegister(address, reg, data);
}

bool setVoltage(uint8_t address, uint8_t channel, float voltage) {
    // Reject NaN and out-of-range values.
    if (!(voltage >= 0.0f && voltage <= FULL_SCALE_V))
        return false;

    // Convert volts to a 12-bit DAC code.
    uint16_t code = static_cast<uint16_t>(
        voltage * 4096.0f / FULL_SCALE_V + 0.5f
    );

    // 5.0 V corresponds to the highest available code.
    if (code > MAX_CODE)
        code = MAX_CODE;

    return setRaw(address, channel, code);
}

} // namespace DAC60502
