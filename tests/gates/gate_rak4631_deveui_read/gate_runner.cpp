/**
 * @file gate_runner.cpp
 * @brief Gate 0.5 — RAK4631 DevEUI Read
 * @version 1.0
 * @date 2026-02-24
 *
 * Read DevEUI from nRF52840 FICR Device ID registers.
 * No LoRa, no Modbus, no scheduler.
 * Entry point: gate_rak4631_deveui_read_run()
 */

#include <Arduino.h>

/* nRF52840 FICR Device ID registers */
#define NRF_FICR_ID0  (*(volatile uint32_t *)0x10000060)
#define NRF_FICR_ID1  (*(volatile uint32_t *)0x10000064)

void gate_rak4631_deveui_read_run(void) {
    Serial.println();
    Serial.println("[GATE0.5] === GATE START: gate_rak4631_deveui_read v1.0 ===");

    /* Read FICR Device ID */
    uint32_t id0 = NRF_FICR_ID0;
    uint32_t id1 = NRF_FICR_ID1;

    Serial.print("[GATE0.5] FICR ID0: 0x");
    Serial.println(id0, HEX);
    Serial.print("[GATE0.5] FICR ID1: 0x");
    Serial.println(id1, HEX);

    /* Build 8-byte DevEUI (MSB first, same order as BoardGetUniqueId) */
    uint8_t dev_eui[8];
    dev_eui[7] = (id0      ) & 0xFF;
    dev_eui[6] = (id0 >>  8) & 0xFF;
    dev_eui[5] = (id0 >> 16) & 0xFF;
    dev_eui[4] = (id0 >> 24) & 0xFF;
    dev_eui[3] = (id1      ) & 0xFF;
    dev_eui[2] = (id1 >>  8) & 0xFF;
    dev_eui[1] = (id1 >> 16) & 0xFF;
    dev_eui[0] = (id1 >> 24) & 0xFF;

    /* Print DevEUI colon-separated */
    Serial.print("[GATE0.5] DevEUI: ");
    for (int i = 0; i < 8; i++) {
        if (dev_eui[i] < 0x10) Serial.print("0");
        Serial.print(dev_eui[i], HEX);
        if (i < 7) Serial.print(":");
    }
    Serial.println();

    /* Print raw hex array */
    Serial.print("[GATE0.5] Raw: { ");
    for (int i = 0; i < 8; i++) {
        Serial.print("0x");
        if (dev_eui[i] < 0x10) Serial.print("0");
        Serial.print(dev_eui[i], HEX);
        if (i < 7) Serial.print(", ");
    }
    Serial.println(" }");

    /* Check for all-zero (invalid) */
    bool all_zero = true;
    for (int i = 0; i < 8; i++) {
        if (dev_eui[i] != 0) { all_zero = false; break; }
    }

    if (all_zero) {
        Serial.println("[GATE0.5] FAIL — DevEUI is all zeros");
        Serial.println("[GATE0.5] === GATE COMPLETE ===");
        return;
    }

    /* Blink blue LED once */
    pinMode(LED_BLUE, OUTPUT);
    digitalWrite(LED_BLUE, LED_STATE_ON);
    delay(500);
    digitalWrite(LED_BLUE, !LED_STATE_ON);

    Serial.println("[GATE0.5] PASS");
    Serial.println("[GATE0.5] === GATE COMPLETE ===");
}
