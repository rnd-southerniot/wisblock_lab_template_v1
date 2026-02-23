/**
 * @file main.cpp
 * @brief Gate 2 Entry Point — RS485 Modbus Autodiscovery Validation
 * @version 2.1
 * @date 2026-02-24
 *
 * Minimal Arduino setup/loop for gate testing only.
 * v2.1: RAK5802 on IO slot with EN + DE pin control.
 */

#include <Arduino.h>

/* Gate runner function (defined in gate_runner.cpp) */
extern void gate_rs485_autodiscovery_run(void);

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
    Serial.println("[SYSTEM] Gate: 2 - RS485 Modbus Autodiscovery");
    Serial.println("[SYSTEM] Transceiver: RAK5802 (IO Slot)");
    Serial.println("========================================");

    /* ---- Run Gate Test ---- */
    gate_rs485_autodiscovery_run();
}

void loop() {
    /* Gate is one-shot; nothing to do in loop */
    delay(10000);
}
