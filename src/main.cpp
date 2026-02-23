/**
 * @file main.cpp
 * @brief Gate 1 Entry Point — I2C LIS3DH Validation
 * @version 1.0
 * @date 2026-02-23
 *
 * Minimal Arduino setup/loop for gate testing only.
 * No LoRaWAN initialization.
 * No peripheral driver initialization.
 * Serial initialized early for gate output capture.
 */

#include <Arduino.h>

/* Gate runner function (defined in gate_runner.cpp) */
extern void gate_i2c_lis3dh_run(void);

void setup() {
    /* ---- Early Serial Init ---- */
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {
        ; /* Wait up to 3 seconds for USB serial */
    }
    delay(500); /* Stabilization delay */

    /* ---- System Banner ---- */
    Serial.println();
    Serial.println("========================================");
    Serial.println("[SYSTEM] WisBlock Gate Test Runner");
    Serial.println("[SYSTEM] Core: RAK3312 (ESP32-S3)");
    Serial.println("[SYSTEM] Gate: 1 - I2C LIS3DH Validation");
    Serial.println("========================================");

    /* ---- Run Gate Test ---- */
    gate_i2c_lis3dh_run();
}

void loop() {
    /* Gate is one-shot; nothing to do in loop */
    delay(10000);
}
