/**
 * @file main.cpp
 * @brief Gate Entry Point — Multi-Platform
 * @version 4.0
 * @date 2026-02-25
 *
 * Selects the correct gate runner based on build flags.
 * Gate selection via -DGATE_N build flags (default: Gate 7).
 * CORE_RAK4631: RAK4631 (nRF52840) gate runners
 * CORE_RAK3312: RAK3312 (ESP32-S3) gate runners
 */

#include <Arduino.h>

#ifdef GATE_9
extern void gate_persistence_reboot_v1_run(void);
#elif defined(GATE_8)
extern void gate_downlink_command_framework_v1_run(void);
#elif defined(CORE_RAK4631)
extern void gate_rak4631_production_loop_soak_run(void);
#else
extern void gate_production_loop_soak_run(void);
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
#ifdef GATE_9
    Serial.println("[SYSTEM] Gate: 9 - Persistence Reboot Test");
#elif defined(GATE_8)
    Serial.println("[SYSTEM] Gate: 8 - Downlink Command Framework v1");
#else
    Serial.println("[SYSTEM] Gate: 7 - Production Loop Soak");
#endif
    Serial.println("========================================");

#ifdef GATE_9
    gate_persistence_reboot_v1_run();
#elif defined(GATE_8)
    gate_downlink_command_framework_v1_run();
#elif defined(CORE_RAK4631)
    gate_rak4631_production_loop_soak_run();
#else
    gate_production_loop_soak_run();
#endif
}

void loop() {
    delay(10000);
}
