/**
 * @file main.cpp
 * @brief RAK4631 Gate 1 — I2C LIS3DH Validation Example
 * @version 1.0
 * @date 2026-02-24
 *
 * Standalone example demonstrating LIS3DH accelerometer validation
 * on the RAK4631 WisBlock Core (nRF52840 + SX1262).
 *
 * Hardware:
 *   - RAK4631 WisBlock Core (nRF52840 @ 64 MHz)
 *   - RAK1904 WisBlock 3-Axis Acceleration Sensor (LIS3DH)
 *   - RAK19007 WisBlock Base Board (or compatible)
 *
 * I2C Bus:
 *   - SDA = pin 13 (P0.13)
 *   - SCL = pin 14 (P0.14)
 *   - LIS3DH addr = 0x18
 *
 * Steps:
 *   1. Detect LIS3DH at 0x18
 *   2. Read WHO_AM_I (expect 0x33)
 *   3. Configure: 50 Hz, ±2g, high-resolution
 *   4. Read 100 consecutive XYZ samples
 *   5. Report statistics and PASS/FAIL
 */

#include <Arduino.h>
#include <Wire.h>
#include "pin_map.h"

/* ---- LIS3DH Registers ---- */
#define LIS3DH_WHO_AM_I    0x0F
#define LIS3DH_EXPECTED_ID 0x33
#define LIS3DH_CTRL_REG1   0x20
#define LIS3DH_CTRL_REG4   0x23
#define LIS3DH_OUT_X_L     0x28

/* ---- Test Parameters ---- */
#define READ_COUNT          100
#define READ_DELAY_MS       10

/* ---- Helpers ---- */

static uint8_t read_reg(uint8_t addr, uint8_t reg, bool *ok) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) { *ok = false; return 0xFF; }
    if (Wire.requestFrom(addr, (uint8_t)1) != 1) { *ok = false; return 0xFF; }
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
    Wire.beginTransmission(addr);
    Wire.write(LIS3DH_OUT_X_L | 0x80);  /* Auto-increment */
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(addr, (uint8_t)6) != 6) return false;
    uint8_t xl = Wire.read(), xh = Wire.read();
    uint8_t yl = Wire.read(), yh = Wire.read();
    uint8_t zl = Wire.read(), zh = Wire.read();
    *x = (int16_t)((xh << 8) | xl);
    *y = (int16_t)((yh << 8) | yl);
    *z = (int16_t)((zh << 8) | zl);
    return true;
}

/* ---- Main ---- */

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }
    delay(1000);

    Serial.println();
    Serial.println("========================================");
    Serial.println("[SYSTEM] RAK4631 I2C LIS3DH Example");
    Serial.println("========================================");

    /* Init I2C */
    Serial.print("[INFO] Wire SDA=");
    Serial.print(PIN_I2C_SDA);
    Serial.print(" SCL=");
    Serial.println(PIN_I2C_SCL);
    Wire.begin();
    delay(100);

    /* Detect */
    Wire.beginTransmission(LIS3DH_I2C_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("[FAIL] No ACK at 0x18");
        return;
    }
    Serial.println("[OK] LIS3DH detected at 0x18");

    /* WHO_AM_I */
    bool ok = false;
    uint8_t who = read_reg(LIS3DH_I2C_ADDR, LIS3DH_WHO_AM_I, &ok);
    Serial.print("[INFO] WHO_AM_I = 0x");
    Serial.println(who, HEX);
    if (!ok || who != LIS3DH_EXPECTED_ID) {
        Serial.println("[FAIL] WHO_AM_I mismatch");
        return;
    }

    /* Configure: 50 Hz, normal mode, XYZ on, ±2g, high-res */
    write_reg(LIS3DH_I2C_ADDR, LIS3DH_CTRL_REG1, 0x47);
    write_reg(LIS3DH_I2C_ADDR, LIS3DH_CTRL_REG4, 0x08);
    Serial.println("[OK] Configured: 50Hz, +/-2g, hi-res");
    delay(100);

    /* 100 reads */
    uint16_t pass_count = 0;
    int16_t x, y, z;
    int16_t z_min = 32767, z_max = -32768;

    for (int i = 0; i < READ_COUNT; i++) {
        if (read_accel(LIS3DH_I2C_ADDR, &x, &y, &z)) {
            pass_count++;
            if (z < z_min) z_min = z;
            if (z > z_max) z_max = z;
            if (i % 25 == 0) {
                Serial.print("[DATA] [");
                Serial.print(i);
                Serial.print("] X=");
                Serial.print(x);
                Serial.print(" Y=");
                Serial.print(y);
                Serial.print(" Z=");
                Serial.println(z);
            }
        }
        delay(READ_DELAY_MS);
    }

    /* Summary */
    Serial.println();
    Serial.print("[RESULT] Reads: ");
    Serial.print(pass_count);
    Serial.print("/");
    Serial.println(READ_COUNT);
    Serial.print("[RESULT] Z range: [");
    Serial.print(z_min);
    Serial.print(" .. ");
    Serial.print(z_max);
    Serial.println("]");

    if (pass_count == READ_COUNT) {
        Serial.println("[RESULT] PASS");
    } else {
        Serial.println("[RESULT] FAIL");
    }

    /* LED blink */
    pinMode(PIN_LED_BLUE, OUTPUT);
    digitalWrite(PIN_LED_BLUE, HIGH);
    delay(500);
    digitalWrite(PIN_LED_BLUE, LOW);
}

void loop() {
    delay(10000);
}
