/**
 * @file main.cpp
 * @brief Gate 6 Entry Point — Runtime Scheduler Integration Validation
 * @version 1.0
 * @date 2026-02-24
 *
 * Minimal Arduino setup/loop for gate testing only.
 * Calls gate_runtime_scheduler_integration_run() once, then idles.
 */

#include <Arduino.h>

extern void gate_runtime_scheduler_integration_run(void);

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000) { ; }
    delay(500);

    Serial.println();
    Serial.println("========================================");
    Serial.println("[SYSTEM] WisBlock Gate Test Runner");
    Serial.println("[SYSTEM] Core: RAK3312 (ESP32-S3)");
    Serial.println("[SYSTEM] Gate: 6 - Runtime Scheduler Integration");
    Serial.println("========================================");

    gate_runtime_scheduler_integration_run();
}

void loop() {
    delay(10000);
}
