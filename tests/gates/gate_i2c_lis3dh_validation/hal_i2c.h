/**
 * @file hal_i2c.h
 * @brief Gate-local I2C HAL — Minimal abstraction for gate validation
 * @version 1.0
 * @date 2026-02-23
 *
 * Self-contained within gate_i2c_lis3dh_validation.
 * NOT a full HAL — only enough for gate validation.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize I2C bus with given SDA/SCL pins and frequency.
 * @param sda_pin  GPIO number for SDA
 * @param scl_pin  GPIO number for SCL
 * @param freq_hz  Clock frequency in Hz
 * @return true on success
 */
bool hal_i2c_init(uint8_t sda_pin, uint8_t scl_pin, uint32_t freq_hz);

/**
 * @brief Write bytes to an I2C device register.
 * @param addr    7-bit I2C slave address
 * @param reg     Register address to write to
 * @param data    Pointer to data bytes
 * @param len     Number of bytes to write
 * @return true on ACK, false on NACK or error
 */
bool hal_i2c_write(uint8_t addr, uint8_t reg, const uint8_t *data, uint8_t len);

/**
 * @brief Read bytes from an I2C device register.
 * @param addr    7-bit I2C slave address
 * @param reg     Register address to read from
 * @param data    Buffer to store read bytes
 * @param len     Number of bytes to read
 * @return true on success, false on NACK or timeout
 */
bool hal_i2c_read(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len);

/**
 * @brief Probe an I2C address for device presence (ACK check).
 * @param addr    7-bit I2C slave address
 * @return true if device ACKs, false otherwise
 */
bool hal_i2c_probe(uint8_t addr);
