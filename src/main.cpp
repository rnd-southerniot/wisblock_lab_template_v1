/**
 * @file main.cpp
 * @brief Gate 7 Entry Point — Production Loop Soak Validation
 * @version 1.0
 * @date 2026-02-24
 *
 * Minimal Arduino setup/loop for gate testing only.
 * Calls gate_production_loop_soak_run() once (runs for 5 min), then idles.
 */

#include <Arduino.h>

extern void gate_production_loop_soak_run(void);

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000) { ; }
    delay(500);

    Serial.println();
    Serial.println("========================================");
    Serial.println("[SYSTEM] WisBlock Gate Test Runner");
    Serial.println("[SYSTEM] Core: RAK3312 (ESP32-S3)");
    Serial.println("[SYSTEM] Gate: 7 - Production Loop Soak");
    Serial.println("========================================");

    gate_production_loop_soak_run();
}

void loop() {
    delay(10000);
}
