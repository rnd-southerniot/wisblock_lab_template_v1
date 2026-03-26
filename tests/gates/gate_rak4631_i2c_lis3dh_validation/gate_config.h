/**
 * @file gate_config.h
 * @brief Gate 1 — RAK4631 I2C LIS3DH Validation Configuration
 * @version 1.0
 * @date 2026-02-24
 */

#pragma once
#include <stdint.h>

/* Gate Identity */
#define GATE_NAME       "gate_rak4631_i2c_lis3dh_validation"
#define GATE_VERSION    "1.0"
#define GATE_ID         1

/* LIS3DH I2C */
#define LIS3DH_ADDR        0x18
#define LIS3DH_WHO_AM_I    0x0F
#define LIS3DH_EXPECTED_ID 0x33

/* LIS3DH registers */
#define LIS3DH_CTRL_REG1   0x20
#define LIS3DH_CTRL_REG4   0x23
#define LIS3DH_OUT_X_L     0x28

/* Test parameters */
#define GATE_READ_COUNT     100     /* Consecutive accel reads */
#define GATE_READ_DELAY_MS  10      /* Delay between reads */

/* Serial Log Tags */
#define GATE_LOG_TAG    "[GATE1-R4631]"
