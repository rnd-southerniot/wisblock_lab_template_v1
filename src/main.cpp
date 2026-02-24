/**
 * @file main.cpp
 * @brief Gate Entry Point — Multi-Platform
 * @version 3.0
 * @date 2026-02-25
 *
 * Selects the correct gate runner based on build flags.
 * CORE_RAK4631: RAK4631 (nRF52840) gate runners
 * CORE_RAK3312: RAK3312 (ESP32-S3) gate runners
 */

#include <Arduino.h>

#ifdef CORE_RAK4631
extern void gate_rak4631_runtime_scheduler_integration_run(void);
#else
extern void gate_runtime_scheduler_integration_run(void);
#endif

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }  // Wait indefinitely for USB CDC host
    delay(1000);

    Serial.println();
    Serial.println("========================================");
    Serial.println("[SYSTEM] WisBlock Gate Test Runner");
#ifdef CORE_RAK4631
    Serial.println("[SYSTEM] Core: RAK4631 (nRF52840)");
#else
    Serial.println("[SYSTEM] Core: RAK3312 (ESP32-S3)");
#endif
    Serial.println("[SYSTEM] Gate: 6 - Runtime Scheduler Integration");
    Serial.println("========================================");

#ifdef CORE_RAK4631
    gate_rak4631_runtime_scheduler_integration_run();
#else
    gate_runtime_scheduler_integration_run();
#endif
}

void loop() {
    delay(10000);
}
