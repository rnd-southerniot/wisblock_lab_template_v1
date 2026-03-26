/**
 * @file gate_runner.cpp
 * @brief Gate 1B — RAK4631 I2C Identify 0x68/0x69
 * @version 1.0
 * @date 2026-02-24
 *
 * Read WHO_AM_I registers from devices at 0x68 and 0x69.
 * No driver implementation, just identification.
 * Entry point: gate_rak4631_i2c_identify_68_69_run()
 */

#include <Arduino.h>
#include <Wire.h>

static bool device_present(uint8_t addr) {
    Wire.beginTransmission(addr);
    return (Wire.endTransmission() == 0);
}

static uint8_t read_reg(uint8_t addr, uint8_t reg, bool *ok) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) {
        *ok = false;
        return 0xFF;
    }
    if (Wire.requestFrom(addr, (uint8_t)1) != 1) {
        *ok = false;
        return 0xFF;
    }
    *ok = true;
    return Wire.read();
}

static void probe_addr(uint8_t addr, bool *found_id) {
    Serial.print("[GATE1B] --- Probing 0x");
    if (addr < 0x10) Serial.print("0");
    Serial.print(addr, HEX);
    Serial.println(" ---");

    if (!device_present(addr)) {
        Serial.println("[GATE1B]   Not present (NACK)");
        return;
    }
    Serial.println("[GATE1B]   ACK — device present");

    /* WHO_AM_I register candidates */
    struct { uint8_t reg; const char *label; } regs[] = {
        { 0x75, "0x75 (MPU/ICM)" },
        { 0x0F, "0x0F (ST sensors)" },
        { 0x00, "0x00 (general)" },
    };

    for (auto &r : regs) {
        bool ok = false;
        uint8_t val = read_reg(addr, r.reg, &ok);

        Serial.print("[GATE1B]   WHO(");
        Serial.print(r.label);
        Serial.print(")=0x");
        if (val < 0x10) Serial.print("0");
        Serial.print(val, HEX);

        if (!ok) {
            Serial.println(" [read failed]");
        } else if (val != 0x00 && val != 0xFF) {
            Serial.println(" <-- valid ID");
            *found_id = true;
        } else {
            Serial.println("");
        }
    }
}

void gate_rak4631_i2c_identify_68_69_run(void) {
    Serial.println();
    Serial.println("[GATE1B] === GATE START: gate_rak4631_i2c_identify_68_69 v1.0 ===");

    Serial.print("[GATE1B] Wire SDA=");
    Serial.print(PIN_WIRE_SDA);
    Serial.print(" SCL=");
    Serial.println(PIN_WIRE_SCL);

    Wire.begin();
    delay(100);

    bool found_id = false;

    probe_addr(0x68, &found_id);
    probe_addr(0x69, &found_id);

    Serial.println();
    if (found_id) {
        Serial.println("[GATE1B] PASS");
    } else {
        Serial.println("[GATE1B] FAIL — no stable WHO_AM_I value found");
    }

    /* Blink blue LED once */
    pinMode(LED_BLUE, OUTPUT);
    digitalWrite(LED_BLUE, LED_STATE_ON);
    delay(500);
    digitalWrite(LED_BLUE, !LED_STATE_ON);

    Serial.println("[GATE1B] === GATE COMPLETE ===");
}
