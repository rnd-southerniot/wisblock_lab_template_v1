/**
 * @file gate_runner.cpp
 * @brief Gate 1 — I2C LIS3DH Validation Runner
 * @version 1.0
 * @date 2026-02-23
 *
 * Minimal validation logic for RAK1904 (LIS3DH) over I2C.
 * Does NOT implement a full driver. Only validates hardware presence
 * and basic bus integrity.
 *
 * PASS criteria:
 *   1. I2C bus initializes successfully
 *   2. Device ACKs at address 0x18
 *   3. WHO_AM_I register returns 0x33
 *   4. 100 consecutive register reads without error
 *   5. No bus timeout
 *   6. No system crash (implicit — reaching GATE COMPLETE)
 */

#include <Wire.h>
#include "gate_config.h"

/* ============================================================
 * Forward Declarations
 * ============================================================ */
static uint8_t gate_result = GATE_PASS;
static uint32_t read_success_count = 0;
static uint32_t read_fail_count = 0;

static bool step1_i2c_init(void);
static bool step2_device_detect(void);
static bool step3_who_am_i(void);
static bool step4_consecutive_reads(void);

/* ============================================================
 * Helper: Write single byte to register
 * ============================================================ */
static bool i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    return (Wire.endTransmission() == 0);
}

/* ============================================================
 * Helper: Read single byte from register
 * ============================================================ */
static bool i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }
    if (Wire.requestFrom(addr, (uint8_t)1) != 1) {
        return false;
    }
    *val = Wire.read();
    return true;
}

/* ============================================================
 * STEP 1: I2C Bus Initialization
 * ============================================================ */
static bool step1_i2c_init(void) {
    Serial.printf("%s %s STEP 1: I2C bus init (SDA=%d, SCL=%d, %d Hz)\r\n",
                  GATE_LOG_TAG, GATE_LOG_STEP,
                  GATE_I2C_SDA_PIN, GATE_I2C_SCL_PIN, GATE_I2C_FREQ_HZ);

    Wire.begin(GATE_I2C_SDA_PIN, GATE_I2C_SCL_PIN);
    Wire.setClock(GATE_I2C_FREQ_HZ);

    /* Verify bus is responsive with a quick scan at target address */
    Wire.beginTransmission(GATE_LIS3DH_ADDR);
    uint8_t error = Wire.endTransmission();

    if (error == 0 || error == 2) {
        /* error 0 = ACK, error 2 = NACK on address (bus working, device may not respond yet) */
        Serial.printf("%s %s STEP 1: I2C bus initialized OK\r\n",
                      GATE_LOG_TAG, GATE_LOG_PASS);
        return true;
    }

    Serial.printf("%s %s STEP 1: I2C bus init failed (error=%d)\r\n",
                  GATE_LOG_TAG, GATE_LOG_FAIL, error);
    gate_result = GATE_FAIL_I2C_INIT;
    return false;
}

/* ============================================================
 * STEP 2: Device Detection at 0x18
 * ============================================================ */
static bool step2_device_detect(void) {
    Serial.printf("%s %s STEP 2: Scanning for device at 0x%02X\r\n",
                  GATE_LOG_TAG, GATE_LOG_STEP, GATE_LIS3DH_ADDR);

    Wire.beginTransmission(GATE_LIS3DH_ADDR);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
        Serial.printf("%s %s STEP 2: Device ACK at 0x%02X\r\n",
                      GATE_LOG_TAG, GATE_LOG_PASS, GATE_LIS3DH_ADDR);
        return true;
    }

    Serial.printf("%s %s STEP 2: No device at 0x%02X (error=%d)\r\n",
                  GATE_LOG_TAG, GATE_LOG_FAIL, GATE_LIS3DH_ADDR, error);
    gate_result = GATE_FAIL_DEVICE_NOT_FOUND;
    return false;
}

/* ============================================================
 * STEP 3: WHO_AM_I Register Validation
 * ============================================================ */
static bool step3_who_am_i(void) {
    Serial.printf("%s %s STEP 3: Reading WHO_AM_I (reg=0x%02X, expect=0x%02X)\r\n",
                  GATE_LOG_TAG, GATE_LOG_STEP,
                  GATE_LIS3DH_WHO_AM_I_REG, GATE_LIS3DH_WHO_AM_I_VAL);

    uint8_t who_am_i = 0;
    if (!i2c_read_reg(GATE_LIS3DH_ADDR, GATE_LIS3DH_WHO_AM_I_REG, &who_am_i)) {
        Serial.printf("%s %s STEP 3: Failed to read WHO_AM_I register\r\n",
                      GATE_LOG_TAG, GATE_LOG_FAIL);
        gate_result = GATE_FAIL_BUS_TIMEOUT;
        return false;
    }

    if (who_am_i == GATE_LIS3DH_WHO_AM_I_VAL) {
        Serial.printf("%s %s STEP 3: WHO_AM_I = 0x%02X (match)\r\n",
                      GATE_LOG_TAG, GATE_LOG_PASS, who_am_i);
        return true;
    }

    Serial.printf("%s %s STEP 3: WHO_AM_I = 0x%02X (expected 0x%02X)\r\n",
                  GATE_LOG_TAG, GATE_LOG_FAIL, who_am_i, GATE_LIS3DH_WHO_AM_I_VAL);
    gate_result = GATE_FAIL_WHO_AM_I;
    return false;
}

/* ============================================================
 * STEP 4: 100 Consecutive Register Reads
 * ============================================================ */
static bool step4_consecutive_reads(void) {
    Serial.printf("%s %s STEP 4: Starting %d consecutive reads (interval=%d ms)\r\n",
                  GATE_LOG_TAG, GATE_LOG_STEP,
                  GATE_CONSECUTIVE_READS, GATE_READ_INTERVAL_MS);

    /* Enable LIS3DH output: ODR=10Hz, all axes enabled */
    if (!i2c_write_reg(GATE_LIS3DH_ADDR, GATE_LIS3DH_CTRL_REG1, 0x27)) {
        Serial.printf("%s %s STEP 4: Failed to write CTRL_REG1\r\n",
                      GATE_LOG_TAG, GATE_LOG_FAIL);
        gate_result = GATE_FAIL_READ_ERROR;
        return false;
    }

    read_success_count = 0;
    read_fail_count = 0;

    for (uint32_t i = 0; i < GATE_CONSECUTIVE_READS; i++) {
        uint8_t val = 0;
        if (i2c_read_reg(GATE_LIS3DH_ADDR, GATE_LIS3DH_OUT_X_L, &val)) {
            read_success_count++;
        } else {
            read_fail_count++;
            Serial.printf("%s %s STEP 4: Read %d/%d FAILED\r\n",
                          GATE_LOG_TAG, GATE_LOG_FAIL,
                          (int)(i + 1), GATE_CONSECUTIVE_READS);
            gate_result = GATE_FAIL_READ_ERROR;
            return false;
        }

        if ((i + 1) % 25 == 0) {
            Serial.printf("%s %s STEP 4: Progress %d/%d OK\r\n",
                          GATE_LOG_TAG, GATE_LOG_INFO,
                          (int)(i + 1), GATE_CONSECUTIVE_READS);
        }

        delay(GATE_READ_INTERVAL_MS);
    }

    Serial.printf("%s %s STEP 4: %d/%d reads OK, %d failures\r\n",
                  GATE_LOG_TAG, GATE_LOG_PASS,
                  (int)read_success_count, GATE_CONSECUTIVE_READS,
                  (int)read_fail_count);
    return true;
}

/* ============================================================
 * Gate Entry Point
 * ============================================================ */
void gate_i2c_lis3dh_run(void) {
    uint32_t start_ms = millis();

    Serial.printf("\r\n");
    Serial.printf("========================================\r\n");
    Serial.printf("%s %s GATE START: %s v%s\r\n",
                  GATE_LOG_TAG, GATE_LOG_INFO, GATE_NAME, GATE_VERSION);
    Serial.printf("========================================\r\n");

    /* Execute steps sequentially — stop on first failure */
    if (!step1_i2c_init())        goto gate_done;
    if (!step2_device_detect())   goto gate_done;
    if (!step3_who_am_i())        goto gate_done;
    if (!step4_consecutive_reads()) goto gate_done;

gate_done:
    uint32_t elapsed_ms = millis() - start_ms;

    Serial.printf("========================================\r\n");

    if (gate_result == GATE_PASS) {
        Serial.printf("%s %s ALL STEPS PASSED (%lu ms)\r\n",
                      GATE_LOG_TAG, GATE_LOG_PASS, (unsigned long)elapsed_ms);
    } else {
        Serial.printf("%s %s GATE FAILED (code=%d, %lu ms)\r\n",
                      GATE_LOG_TAG, GATE_LOG_FAIL,
                      gate_result, (unsigned long)elapsed_ms);
    }

    Serial.printf("%s %s RESULT: reads_ok=%lu, reads_fail=%lu\r\n",
                  GATE_LOG_TAG, GATE_LOG_INFO,
                  (unsigned long)read_success_count,
                  (unsigned long)read_fail_count);
    Serial.printf("%s %s GATE COMPLETE\r\n", GATE_LOG_TAG, GATE_LOG_INFO);
    Serial.printf("========================================\r\n");
}
