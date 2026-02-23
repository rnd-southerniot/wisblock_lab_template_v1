/**
 * @file gate_config.h
 * @brief Gate 1 — I2C LIS3DH Validation Configuration
 * @version 1.0
 * @date 2026-02-23
 *
 * Defines all parameters for gate_i2c_lis3dh_validation.
 * References Hardware Profile v1.0 frozen values.
 */

#pragma once

#include <stdint.h>
#include "../../firmware/config/hardware_profile.h"

/* ============================================================
 * Gate Identity
 * ============================================================ */
#define GATE_NAME                   "gate_i2c_lis3dh_validation"
#define GATE_VERSION                "1.0"
#define GATE_ID                     1

/* ============================================================
 * I2C Bus Parameters
 * ============================================================ */
#define GATE_I2C_SDA_PIN            9       /* WisBlock I2C1 SDA (RAK3312 ESP32-S3) */
#define GATE_I2C_SCL_PIN            40      /* WisBlock I2C1 SCL (RAK3312 ESP32-S3) */
#define GATE_I2C_FREQ_HZ            100000  /* 100 kHz standard mode */

/* ============================================================
 * LIS3DH Device Parameters
 * ============================================================ */
#define GATE_LIS3DH_ADDR            HW_I2C_RAK1904_ADDR  /* 0x18 */
#define GATE_LIS3DH_WHO_AM_I_REG   0x0F
#define GATE_LIS3DH_WHO_AM_I_VAL   0x33    /* Expected WHO_AM_I response */
#define GATE_LIS3DH_CTRL_REG1      0x20
#define GATE_LIS3DH_OUT_X_L        0x28

/* ============================================================
 * Validation Thresholds
 * ============================================================ */
#define GATE_CONSECUTIVE_READS      100
#define GATE_READ_INTERVAL_MS       10      /* ms between consecutive reads */
#define GATE_I2C_TIMEOUT_MS         50      /* per-transaction timeout */
#define GATE_TOTAL_TIMEOUT_MS       10000   /* overall gate timeout */

/* ============================================================
 * Pass/Fail Codes
 * ============================================================ */
#define GATE_PASS                   0
#define GATE_FAIL_I2C_INIT          1
#define GATE_FAIL_DEVICE_NOT_FOUND  2
#define GATE_FAIL_WHO_AM_I          3
#define GATE_FAIL_READ_ERROR        4
#define GATE_FAIL_BUS_TIMEOUT       5
#define GATE_FAIL_SYSTEM_CRASH      6

/* ============================================================
 * Serial Log Tags
 * ============================================================ */
#define GATE_LOG_TAG                "[GATE1]"
#define GATE_LOG_PASS               "[PASS]"
#define GATE_LOG_FAIL               "[FAIL]"
#define GATE_LOG_INFO               "[INFO]"
#define GATE_LOG_STEP               "[STEP]"
