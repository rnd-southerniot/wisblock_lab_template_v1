/**
 * @file main.cpp
 * @brief RAK4631 Gate 3 — Modbus Minimal Protocol Validation Example
 * @version 1.0
 * @date 2026-02-24
 *
 * Standalone example demonstrating Modbus RTU protocol validation on
 * the RAK4631 WisBlock Core (nRF52840 + SX1262).
 *
 * Sends a single Read Holding Register (FC 0x03) request to a known
 * Modbus slave, then validates the response with a strict 7-step parser:
 *   1) length >= 5
 *   2) CRC match
 *   3) slave match
 *   4) exception handling
 *   5) function match
 *   6) byte_count consistency
 *   7) value extraction
 *
 * Hardware:
 *   - RAK4631 WisBlock Core (nRF52840 @ 64 MHz)
 *   - RAK5802 WisBlock RS485 Module (TP8485E, auto DE/RE)
 *   - RAK19007 WisBlock Base Board
 *   - External Modbus RTU slave (e.g., RS-FSJT-N01 wind sensor)
 *
 * Discovered device: Slave=1, Baud=4800, Parity=NONE (8N1)
 */

#include <Arduino.h>
#include "pin_map.h"

/* ---- CRC-16/MODBUS ---- */
static uint16_t modbus_crc16(const uint8_t *data, uint8_t len) {
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 1) ? ((crc >> 1) ^ 0xA001) : (crc >> 1);
        }
    }
    return crc;
}

/* ---- Hex dump ---- */
static void print_hex(const uint8_t *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        if (buf[i] < 0x10) Serial.print("0");
        Serial.print(buf[i], HEX);
        if (i < len - 1) Serial.print(" ");
    }
}

/* ---- Main ---- */
void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }
    delay(1000);

    Serial.println();
    Serial.println("========================================");
    Serial.println("[SYSTEM] RAK4631 Modbus Minimal Protocol");
    Serial.println("========================================");

    /* Enable RAK5802 */
    pinMode(PIN_RS485_EN, OUTPUT);
    digitalWrite(PIN_RS485_EN, HIGH);
    delay(300);
    Serial.println("[INFO] RS485 powered (WB_IO2=34 HIGH)");
    Serial.println("[INFO] DE/RE: auto (RAK5802 hardware)");

    /* Init Serial1 */
    Serial1.begin(MODBUS_BAUD, SERIAL_8N1);
    delay(50);
    while (Serial1.available()) Serial1.read();

    Serial.print("[INFO] Serial1 TX=");
    Serial.print(PIN_UART1_TX);
    Serial.print(" RX=");
    Serial.println(PIN_UART1_RX);
    Serial.print("[INFO] ");
    Serial.print(MODBUS_BAUD);
    Serial.println(" 8N1");

    /* Build Modbus request: Read Holding Register 0x0000, Qty=1 */
    uint8_t tx[8];
    tx[0] = MODBUS_SLAVE;
    tx[1] = 0x03;              /* FC: Read Holding Registers */
    tx[2] = 0x00; tx[3] = 0x00;  /* Reg 0x0000 */
    tx[4] = 0x00; tx[5] = 0x01;  /* Qty 1 */
    uint16_t crc = modbus_crc16(tx, 6);
    tx[6] = (uint8_t)(crc & 0xFF);
    tx[7] = (uint8_t)((crc >> 8) & 0xFF);

    Serial.print("[TX] (8 bytes): ");
    print_hex(tx, 8);
    Serial.println();

    /* Send */
    Serial1.write(tx, 8);
    Serial1.flush();
    delayMicroseconds(500);

    /* Read response (200ms timeout, 5ms inter-byte gap) */
    uint8_t rx[32];
    uint8_t rx_len = 0;
    unsigned long start = millis();
    while (!Serial1.available()) {
        if ((millis() - start) >= 200) break;
    }
    if (Serial1.available()) {
        unsigned long last_byte = millis();
        while (rx_len < sizeof(rx)) {
            if (Serial1.available()) {
                rx[rx_len++] = Serial1.read();
                last_byte = millis();
            } else if ((millis() - last_byte) >= 5) {
                break;
            }
        }
    }

    if (rx_len == 0) {
        Serial.println("[FAIL] No response (timeout)");
        return;
    }

    Serial.print("[RX] (");
    Serial.print(rx_len);
    Serial.print(" bytes): ");
    print_hex(rx, rx_len);
    Serial.println();

    /* ---- 7-step parser ---- */
    Serial.println();
    Serial.println("[PARSE] 7-step validation:");

    /* 1. Length */
    Serial.print("[PARSE] 1. Length >= 5: ");
    if (rx_len < 5) {
        Serial.println("FAIL");
        Serial.println("[FAIL] Short frame");
        return;
    }
    Serial.print(rx_len);
    Serial.println(" OK");

    /* 2. CRC */
    uint16_t rx_crc = (uint16_t)rx[rx_len - 2] | ((uint16_t)rx[rx_len - 1] << 8);
    uint16_t calc_crc = modbus_crc16(rx, rx_len - 2);
    Serial.print("[PARSE] 2. CRC match: calc=0x");
    Serial.print(calc_crc, HEX);
    Serial.print(" rx=0x");
    Serial.print(rx_crc, HEX);
    if (rx_crc != calc_crc) {
        Serial.println(" FAIL");
        Serial.println("[FAIL] CRC mismatch");
        return;
    }
    Serial.println(" OK");

    /* 3. Slave match */
    Serial.print("[PARSE] 3. Slave match: ");
    Serial.print(rx[0]);
    if (rx[0] != MODBUS_SLAVE) {
        Serial.println(" FAIL");
        Serial.println("[FAIL] Slave mismatch");
        return;
    }
    Serial.println(" OK");

    /* 4. Exception check */
    Serial.print("[PARSE] 4. Exception: ");
    if (rx[1] & 0x80) {
        Serial.print("YES code=0x");
        if (rx[2] < 0x10) Serial.print("0");
        Serial.println(rx[2], HEX);
        Serial.println("[RESULT] Exception response (valid)");
        Serial.println("[RESULT] PASS");
        return;
    }
    Serial.println("NO (normal)");

    /* 5. Function match */
    Serial.print("[PARSE] 5. Function match: 0x");
    if (rx[1] < 0x10) Serial.print("0");
    Serial.print(rx[1], HEX);
    if (rx[1] != 0x03) {
        Serial.println(" FAIL");
        Serial.println("[FAIL] Function mismatch");
        return;
    }
    Serial.println(" OK");

    /* 6. byte_count consistency */
    uint8_t byte_count = rx[2];
    uint8_t expected = rx_len - 5;
    Serial.print("[PARSE] 6. byte_count: declared=");
    Serial.print(byte_count);
    Serial.print(" actual=");
    Serial.print(expected);
    if (byte_count != expected) {
        Serial.println(" FAIL");
        Serial.println("[FAIL] byte_count mismatch");
        return;
    }
    Serial.println(" OK");

    /* 7. Value extraction */
    uint16_t value = ((uint16_t)rx[3] << 8) | (uint16_t)rx[4];
    Serial.print("[PARSE] 7. Value: 0x");
    if (value < 0x1000) Serial.print("0");
    if (value < 0x0100) Serial.print("0");
    if (value < 0x0010) Serial.print("0");
    Serial.print(value, HEX);
    Serial.print(" (");
    Serial.print(value);
    Serial.println(") OK");

    /* Summary */
    Serial.println();
    Serial.print("[RESULT] Slave=");
    Serial.print(MODBUS_SLAVE);
    Serial.print(" Value=0x");
    if (value < 0x1000) Serial.print("0");
    if (value < 0x0100) Serial.print("0");
    if (value < 0x0010) Serial.print("0");
    Serial.println(value, HEX);
    Serial.println("[RESULT] PASS");

    /* LED blink */
    pinMode(PIN_LED_BLUE, OUTPUT);
    digitalWrite(PIN_LED_BLUE, HIGH);
    delay(500);
    digitalWrite(PIN_LED_BLUE, LOW);
}

void loop() {
    delay(10000);
}
