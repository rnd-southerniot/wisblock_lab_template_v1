/**
 * @file main.cpp
 * @brief Gate 10 Entry Point — Secure Downlink Protocol v2 Validation
 * @version 1.0
 * @date 2026-02-26
 *
 * Minimal Arduino setup/loop for gate testing only.
 * Calls gate_secure_downlink_v2_run() once, then idles.
 */

#include <Arduino.h>

extern void gate_secure_downlink_v2_run(void);

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000) { ; }
    delay(500);

    Serial.println();
    Serial.println("========================================");
    Serial.println("[SYSTEM] WisBlock Gate Test Runner");
    Serial.println("[SYSTEM] Core: RAK3312 (ESP32-S3)");
    Serial.println("[SYSTEM] Gate: 10 - Secure Downlink v2");
    Serial.println("========================================");

    gate_secure_downlink_v2_run();
}

void loop() {
    delay(10000);
}
