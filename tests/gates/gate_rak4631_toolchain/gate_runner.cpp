/**
 * @file gate_runner.cpp
 * @brief Gate 0 — RAK4631 Toolchain Validation
 * @version 1.0
 * @date 2026-02-24
 *
 * Minimal toolchain check. No LoRa, no Modbus, no scheduler.
 * Entry point: gate_rak4631_toolchain_run()
 */

#include <Arduino.h>

void gate_rak4631_toolchain_run(void) {
    Serial.println();
    Serial.println("[GATE0] RAK4631 Toolchain OK");

    /* Blink blue LED once (green LED not populated on this board) */
    pinMode(LED_BLUE, OUTPUT);
    digitalWrite(LED_BLUE, LED_STATE_ON);
    delay(500);
    digitalWrite(LED_BLUE, !LED_STATE_ON);

    Serial.println("[GATE0] PASS");
    Serial.println("[GATE0] === GATE COMPLETE ===");
}
