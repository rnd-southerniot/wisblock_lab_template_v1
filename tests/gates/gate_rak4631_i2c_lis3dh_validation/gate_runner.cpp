/**
 * @file gate_runner.cpp
 * @brief Gate 1 — RAK4631 I2C LIS3DH Validation
 * @version 1.0
 * @date 2026-02-24
 *
 * Validate LIS3DH accelerometer on I2C bus.
 * No LoRa, no Modbus, no runtime.
 * Entry point: gate_rak4631_i2c_lis3dh_validation_run()
 */

#include <Arduino.h>
#include <Wire.h>
#include "gate_config.h"

static uint8_t read_reg(uint8_t addr, uint8_t reg, bool *ok) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) {
        *ok = false;
        return 0xFF;
    }
    if (Wire.requestFrom(addr, (uint8_t)1) != 1) {
        *ok = false;
        return 0xFF;
    }
    *ok = true;
    return Wire.read();
}

static bool write_reg(uint8_t addr, uint8_t reg, uint8_t val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    return (Wire.endTransmission() == 0);
}

static bool read_accel(uint8_t addr, int16_t *x, int16_t *y, int16_t *z) {
    /* Multi-byte read: set MSB of register address for auto-increment */
    Wire.beginTransmission(addr);
    Wire.write(LIS3DH_OUT_X_L | 0x80);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(addr, (uint8_t)6) != 6) return false;

    uint8_t xl = Wire.read();
    uint8_t xh = Wire.read();
    uint8_t yl = Wire.read();
    uint8_t yh = Wire.read();
    uint8_t zl = Wire.read();
    uint8_t zh = Wire.read();

    *x = (int16_t)((xh << 8) | xl);
    *y = (int16_t)((yh << 8) | yl);
    *z = (int16_t)((zh << 8) | zl);
    return true;
}

void gate_rak4631_i2c_lis3dh_validation_run(void) {
    Serial.println();
    Serial.println(GATE_LOG_TAG " === GATE START: " GATE_NAME " v" GATE_VERSION " ===");

    /* STEP 1: Init I2C */
    Serial.print(GATE_LOG_TAG " Wire SDA=");
    Serial.print(PIN_WIRE_SDA);
    Serial.print(" SCL=");
    Serial.println(PIN_WIRE_SCL);

    Wire.begin();
    delay(100);

    /* STEP 2: Detect device */
    Serial.println(GATE_LOG_TAG " STEP 1: Detect LIS3DH at 0x18");
    Wire.beginTransmission(LIS3DH_ADDR);
    uint8_t err = Wire.endTransmission();
    if (err != 0) {
        Serial.print(GATE_LOG_TAG " FAIL — no ACK at 0x18 (err=");
        Serial.print(err);
        Serial.println(")");
        Serial.println(GATE_LOG_TAG " === GATE COMPLETE ===");
        return;
    }
    Serial.println(GATE_LOG_TAG "   ACK — device present");

    /* STEP 3: WHO_AM_I */
    Serial.println(GATE_LOG_TAG " STEP 2: Read WHO_AM_I (0x0F)");
    bool ok = false;
    uint8_t who = read_reg(LIS3DH_ADDR, LIS3DH_WHO_AM_I, &ok);
    Serial.print(GATE_LOG_TAG "   WHO_AM_I = 0x");
    if (who < 0x10) Serial.print("0");
    Serial.print(who, HEX);
    Serial.print(" (expected 0x");
    Serial.print(LIS3DH_EXPECTED_ID, HEX);
    Serial.println(")");

    if (!ok || who != LIS3DH_EXPECTED_ID) {
        Serial.println(GATE_LOG_TAG " FAIL — WHO_AM_I mismatch");
        Serial.println(GATE_LOG_TAG " === GATE COMPLETE ===");
        return;
    }
    Serial.println(GATE_LOG_TAG "   LIS3DH confirmed");

    /* STEP 4: Configure — 50 Hz, normal mode, all axes enabled */
    Serial.println(GATE_LOG_TAG " STEP 3: Configure sensor");
    if (!write_reg(LIS3DH_ADDR, LIS3DH_CTRL_REG1, 0x47)) {
        Serial.println(GATE_LOG_TAG " FAIL — CTRL_REG1 write failed");
        Serial.println(GATE_LOG_TAG " === GATE COMPLETE ===");
        return;
    }
    /* +/- 2g, high-res mode */
    if (!write_reg(LIS3DH_ADDR, LIS3DH_CTRL_REG4, 0x08)) {
        Serial.println(GATE_LOG_TAG " FAIL — CTRL_REG4 write failed");
        Serial.println(GATE_LOG_TAG " === GATE COMPLETE ===");
        return;
    }
    Serial.println(GATE_LOG_TAG "   CTRL_REG1=0x47 (50Hz, XYZ on)");
    Serial.println(GATE_LOG_TAG "   CTRL_REG4=0x08 (+/-2g, hi-res)");
    delay(100);  /* Let sensor settle */

    /* STEP 5: 100 consecutive reads */
    Serial.print(GATE_LOG_TAG " STEP 4: ");
    Serial.print(GATE_READ_COUNT);
    Serial.println(" consecutive accel reads");

    uint16_t pass_count = 0;
    uint16_t fail_count = 0;
    int16_t x, y, z;
    int16_t x_min = 32767, x_max = -32768;
    int16_t y_min = 32767, y_max = -32768;
    int16_t z_min = 32767, z_max = -32768;

    for (int i = 0; i < GATE_READ_COUNT; i++) {
        if (read_accel(LIS3DH_ADDR, &x, &y, &z)) {
            pass_count++;
            if (x < x_min) x_min = x;
            if (x > x_max) x_max = x;
            if (y < y_min) y_min = y;
            if (y > y_max) y_max = y;
            if (z < z_min) z_min = z;
            if (z > z_max) z_max = z;

            /* Print every 25th sample */
            if (i % 25 == 0) {
                Serial.print(GATE_LOG_TAG "   [");
                Serial.print(i);
                Serial.print("] X=");
                Serial.print(x);
                Serial.print(" Y=");
                Serial.print(y);
                Serial.print(" Z=");
                Serial.println(z);
            }
        } else {
            fail_count++;
            Serial.print(GATE_LOG_TAG "   [");
            Serial.print(i);
            Serial.println("] READ FAILED");
        }
        delay(GATE_READ_DELAY_MS);
    }

    /* Summary */
    Serial.println();
    Serial.print(GATE_LOG_TAG " Reads: ");
    Serial.print(pass_count);
    Serial.print("/");
    Serial.print(GATE_READ_COUNT);
    Serial.print(" passed, ");
    Serial.print(fail_count);
    Serial.println(" failed");

    Serial.print(GATE_LOG_TAG " X range: [");
    Serial.print(x_min);
    Serial.print(" .. ");
    Serial.print(x_max);
    Serial.println("]");
    Serial.print(GATE_LOG_TAG " Y range: [");
    Serial.print(y_min);
    Serial.print(" .. ");
    Serial.print(y_max);
    Serial.println("]");
    Serial.print(GATE_LOG_TAG " Z range: [");
    Serial.print(z_min);
    Serial.print(" .. ");
    Serial.print(z_max);
    Serial.println("]");

    Serial.println();
    if (fail_count == 0) {
        Serial.println(GATE_LOG_TAG " PASS");
    } else {
        Serial.println(GATE_LOG_TAG " FAIL — read errors detected");
    }

    /* Blink blue LED once */
    pinMode(LED_BLUE, OUTPUT);
    digitalWrite(LED_BLUE, LED_STATE_ON);
    delay(500);
    digitalWrite(LED_BLUE, !LED_STATE_ON);

    Serial.println(GATE_LOG_TAG " === GATE COMPLETE ===");
}
