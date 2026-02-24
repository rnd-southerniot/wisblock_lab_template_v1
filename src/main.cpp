/**
 * @file main.cpp
 * @brief Gate Entry Point — RAK4631
 * @version 1.8
 * @date 2026-02-24
 */

#include <Arduino.h>

extern void gate_rak4631_modbus_minimal_protocol_run(void);

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }  // Wait indefinitely for USB CDC host
    delay(1000);

    Serial.println();
    Serial.println("========================================");
    Serial.println("[SYSTEM] WisBlock Gate Test Runner");
    Serial.println("[SYSTEM] Core: RAK4631 (nRF52840)");
    Serial.println("[SYSTEM] Gate: 3 - Modbus Minimal Protocol");
    Serial.println("========================================");

    gate_rak4631_modbus_minimal_protocol_run();
}

void loop() {
    delay(10000);
}
