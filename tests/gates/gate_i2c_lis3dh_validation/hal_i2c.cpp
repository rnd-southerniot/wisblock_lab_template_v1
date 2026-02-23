/**
 * @file hal_i2c.cpp
 * @brief Gate-local I2C HAL — Wire.h implementation (ESP32-S3 / RAK3312)
 * @version 1.2
 * @date 2026-02-23
 *
 * Minimal Wire.h wrappers for gate validation only.
 * NOT a production HAL. Scoped to gate_i2c_lis3dh_validation.
 *
 * v1.2: Updated for RAK3312 (ESP32-S3). Uses stop-start sequence
 *       instead of repeated-start for ESP32-S3 Wire library compat.
 */

#include <Wire.h>
#include "hal_i2c.h"

bool hal_i2c_init(uint8_t sda_pin, uint8_t scl_pin, uint32_t freq_hz) {
    Wire.begin(sda_pin, scl_pin);
    Wire.setClock(freq_hz);
    return true;
}

bool hal_i2c_write(uint8_t addr, uint8_t reg, const uint8_t *data, uint8_t len) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    for (uint8_t i = 0; i < len; i++) {
        Wire.write(data[i]);
    }
    return (Wire.endTransmission() == 0);
}

bool hal_i2c_read(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        return false;
    }
    if (Wire.requestFrom(addr, len) != len) {
        return false;
    }
    for (uint8_t i = 0; i < len; i++) {
        data[i] = Wire.read();
    }
    return true;
}

bool hal_i2c_probe(uint8_t addr) {
    Wire.beginTransmission(addr);
    return (Wire.endTransmission() == 0);
}
