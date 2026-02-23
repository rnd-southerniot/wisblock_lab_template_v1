/**
 * @file gate1_i2c_lis3dh.ino
 * @brief Example — Gate 1 I2C LIS3DH Validation (RAK3312 / ESP32-S3)
 *
 * Demonstrates minimal I2C communication with the LIS3DH accelerometer
 * on WisBlock Slot B (RAK1904) using the RAK3312 core module.
 *
 * Hardware:
 *   - RAK3312 WisBlock Core (ESP32-S3)
 *   - RAK19007 Base Board
 *   - RAK1904 (LIS3DH) in Slot B
 *
 * I2C1 Pins (RAK3312):
 *   - SDA = GPIO 9
 *   - SCL = GPIO 40
 *
 * Based on Gate 1 validation — passed 2026-02-23.
 */

#include <Wire.h>

#define LIS3DH_ADDR       0x18
#define WHO_AM_I_REG       0x0F
#define WHO_AM_I_EXPECTED  0x33
#define CTRL_REG1          0x20
#define OUT_X_L            0x28

#define I2C_SDA  9
#define I2C_SCL  40

bool i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) return false;
    if (Wire.requestFrom(addr, (uint8_t)1) != 1) return false;
    *val = Wire.read();
    return true;
}

bool i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    return (Wire.endTransmission() == 0);
}

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000);
    delay(500);

    Serial.println("\n=== LIS3DH I2C Example (RAK3312) ===\n");

    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000);

    /* Check device presence */
    Wire.beginTransmission(LIS3DH_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("ERROR: LIS3DH not found at 0x18");
        return;
    }
    Serial.println("LIS3DH detected at 0x18");

    /* Read WHO_AM_I */
    uint8_t who = 0;
    if (i2c_read_reg(LIS3DH_ADDR, WHO_AM_I_REG, &who)) {
        Serial.printf("WHO_AM_I = 0x%02X %s\n", who,
                      (who == WHO_AM_I_EXPECTED) ? "(OK)" : "(MISMATCH)");
    }

    /* Enable: ODR=10Hz, X/Y/Z */
    i2c_write_reg(LIS3DH_ADDR, CTRL_REG1, 0x27);
}

void loop() {
    uint8_t val = 0;
    if (i2c_read_reg(LIS3DH_ADDR, OUT_X_L, &val)) {
        Serial.printf("OUT_X_L = 0x%02X\n", val);
    }
    delay(500);
}
