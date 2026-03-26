/**
 * @file gate_runner.cpp
 * @brief Gate 1A — RAK4631 I2C Bus Scan
 * @version 1.0
 * @date 2026-02-24
 *
 * Scan I2C bus (Wire) for all responding devices.
 * No LoRa, no Modbus, no runtime.
 * Entry point: gate_rak4631_i2c_scan_run()
 */

#include <Arduino.h>
#include <Wire.h>

void gate_rak4631_i2c_scan_run(void) {
    Serial.println();
    Serial.println("[GATE1A] === GATE START: gate_rak4631_i2c_scan v1.0 ===");

    Serial.print("[GATE1A] Wire SDA=");
    Serial.print(PIN_WIRE_SDA);
    Serial.print(" SCL=");
    Serial.println(PIN_WIRE_SCL);

    Wire.begin();
    delay(100);

    Serial.println("[GATE1A] Scanning 0x08..0x77...");

    uint8_t count = 0;
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        Wire.beginTransmission(addr);
        uint8_t err = Wire.endTransmission();
        if (err == 0) {
            Serial.print("[GATE1A] Found: 0x");
            if (addr < 0x10) Serial.print("0");
            Serial.println(addr, HEX);
            count++;
        }
    }

    Serial.print("[GATE1A] Devices found: ");
    Serial.println(count);

    if (count == 0) {
        Serial.println("[GATE1A] FAIL — no I2C devices detected");
    } else {
        Serial.println("[GATE1A] PASS");
    }

    /* Blink blue LED once */
    pinMode(LED_BLUE, OUTPUT);
    digitalWrite(LED_BLUE, LED_STATE_ON);
    delay(500);
    digitalWrite(LED_BLUE, !LED_STATE_ON);

    Serial.println("[GATE1A] === GATE COMPLETE ===");
}
