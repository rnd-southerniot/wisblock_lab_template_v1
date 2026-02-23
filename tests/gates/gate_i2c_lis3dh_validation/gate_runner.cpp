/**
 * @file gate_runner.cpp
 * @brief Gate 1 — I2C LIS3DH Validation Runner (HAL-based)
 * @version 1.1
 * @date 2026-02-23
 *
 * Minimal validation using hal_i2c_init / hal_i2c_write / hal_i2c_read.
 * Does NOT implement a full LIS3DH driver.
 * Aborts on first failure.
 */

#include <Arduino.h>
#include "gate_config.h"
#include "hal_i2c.h"

/* ============================================================
 * State
 * ============================================================ */
static uint8_t gate_result = GATE_PASS;

/* ============================================================
 * STEP 1: I2C Init
 * ============================================================ */
static bool step_i2c_init(void) {
    if (!hal_i2c_init(GATE_I2C_SDA_PIN, GATE_I2C_SCL_PIN, GATE_I2C_FREQ_HZ)) {
        Serial.printf("%s I2C Init: FAIL\r\n", GATE_LOG_TAG);
        gate_result = GATE_FAIL_I2C_INIT;
        return false;
    }
    Serial.printf("%s I2C Init: OK\r\n", GATE_LOG_TAG);
    return true;
}

/* ============================================================
 * STEP 2: Device Detect
 * ============================================================ */
static bool step_device_detect(void) {
    if (!hal_i2c_probe(GATE_LIS3DH_ADDR)) {
        Serial.printf("%s Device Detect: FAIL (no ACK at 0x%02X)\r\n",
                      GATE_LOG_TAG, GATE_LIS3DH_ADDR);
        gate_result = GATE_FAIL_DEVICE_NOT_FOUND;
        return false;
    }
    Serial.printf("%s Device Detect: OK\r\n", GATE_LOG_TAG);
    return true;
}

/* ============================================================
 * STEP 3: WHO_AM_I
 * ============================================================ */
static bool step_who_am_i(void) {
    uint8_t val = 0;
    if (!hal_i2c_read(GATE_LIS3DH_ADDR, GATE_LIS3DH_WHO_AM_I_REG, &val, 1)) {
        Serial.printf("%s WHO_AM_I: FAIL (read error)\r\n", GATE_LOG_TAG);
        gate_result = GATE_FAIL_BUS_TIMEOUT;
        return false;
    }
    if (val != GATE_LIS3DH_WHO_AM_I_VAL) {
        Serial.printf("%s WHO_AM_I: FAIL (got 0x%02X, expected 0x%02X)\r\n",
                      GATE_LOG_TAG, val, GATE_LIS3DH_WHO_AM_I_VAL);
        gate_result = GATE_FAIL_WHO_AM_I;
        return false;
    }
    Serial.printf("%s WHO_AM_I: 0x%02X\r\n", GATE_LOG_TAG, val);
    return true;
}

/* ============================================================
 * STEP 4: 100 Consecutive Reads
 * ============================================================ */
static bool step_consecutive_reads(void) {
    /* Enable LIS3DH: ODR=10Hz, X/Y/Z enabled */
    uint8_t ctrl1 = 0x27;
    if (!hal_i2c_write(GATE_LIS3DH_ADDR, GATE_LIS3DH_CTRL_REG1, &ctrl1, 1)) {
        Serial.printf("%s Read 0/%d FAIL (CTRL_REG1 write error)\r\n",
                      GATE_LOG_TAG, GATE_CONSECUTIVE_READS);
        gate_result = GATE_FAIL_READ_ERROR;
        return false;
    }

    for (int i = 1; i <= GATE_CONSECUTIVE_READS; i++) {
        uint8_t val = 0;
        if (!hal_i2c_read(GATE_LIS3DH_ADDR, GATE_LIS3DH_OUT_X_L, &val, 1)) {
            Serial.printf("%s Read %d/%d FAIL\r\n",
                          GATE_LOG_TAG, i, GATE_CONSECUTIVE_READS);
            gate_result = GATE_FAIL_READ_ERROR;
            return false;
        }
        Serial.printf("%s Read %d/%d OK\r\n",
                      GATE_LOG_TAG, i, GATE_CONSECUTIVE_READS);
        delay(GATE_READ_INTERVAL_MS);
    }

    return true;
}

/* ============================================================
 * Gate Entry Point
 * ============================================================ */
void gate_i2c_lis3dh_run(void) {
    gate_result = GATE_PASS;

    Serial.printf("\r\n%s === GATE START: %s v%s ===\r\n",
                  GATE_LOG_TAG, GATE_NAME, GATE_VERSION);

    if (!step_i2c_init())         goto done;
    if (!step_device_detect())    goto done;
    if (!step_who_am_i())         goto done;
    if (!step_consecutive_reads()) goto done;

done:
    if (gate_result == GATE_PASS) {
        Serial.printf("%s PASS\r\n", GATE_LOG_TAG);
    } else {
        Serial.printf("%s FAIL (code=%d)\r\n", GATE_LOG_TAG, gate_result);
    }
    Serial.printf("%s === GATE COMPLETE ===\r\n", GATE_LOG_TAG);
}
