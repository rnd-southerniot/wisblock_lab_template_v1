/**
 * @file main.cpp
 * @brief Gate 5 Entry Point — LoRaWAN Join + Uplink Validation
 * @version 1.0
 * @date 2026-02-24
 *
 * Minimal Arduino setup/loop for gate testing only.
 * Calls gate_lorawan_join_uplink_run() once, then idles.
 */

#include <Arduino.h>

/* Gate runner function (defined in gate_runner.cpp) */
extern void gate_lorawan_join_uplink_run(void);

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
    Serial.println("[SYSTEM] Gate: 5 - LoRaWAN Join + Uplink");
    Serial.println("========================================");

    /* ---- Run Gate Test ---- */
    gate_lorawan_join_uplink_run();
}

void loop() {
    /* Gate is one-shot; nothing to do in loop */
    delay(10000);
}
