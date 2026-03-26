/**
 * @file gate_runner.cpp
 * @brief Gate 2-PROBE — RAK4631 RS485 Single Modbus Probe
 * @version 2.0
 * @date 2026-02-24
 *
 * Send ONE Modbus RTU request (slave 1, read holding reg 0x0000,
 * qty 1) at 4800 8N1, then listen for response.
 * Minimal: no scan loops, no parsing beyond CRC check on RX.
 *
 * RAK5802 auto-direction: WB_IO2 (pin 34) powers the module.
 * DE/RE handled automatically by on-board circuit.
 *
 * Entry point: gate_rak4631_rs485_raw_monitor_run()
 */

#include <Arduino.h>

#define RS485_EN_PIN    34  /* WB_IO2 — 3V3_S power enable */

/* ---- CRC-16/MODBUS ---- */
static uint16_t modbus_crc16(const uint8_t *data, uint8_t len) {
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc;
}

/* ---- Hex print helper ---- */
static void print_hex(const uint8_t *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        if (buf[i] < 0x10) Serial.print("0");
        Serial.print(buf[i], HEX);
        if (i < len - 1) Serial.print(" ");
    }
}

void gate_rak4631_rs485_raw_monitor_run(void) {
    Serial.println();
    Serial.println("[GATE2-PROBE] === GATE START: gate_rak4631_rs485_probe v2.0 ===");

    /* STEP 1: Power enable */
    Serial.println("[GATE2-PROBE] STEP 1: Power enable");
    pinMode(RS485_EN_PIN, OUTPUT);
    digitalWrite(RS485_EN_PIN, HIGH);
    delay(300);
    Serial.println("[GATE2-PROBE]   WB_IO2=34 HIGH (3V3_S on)");

    /* STEP 2: UART config */
    Serial.println("[GATE2-PROBE] STEP 2: UART config");
    Serial1.begin(4800, SERIAL_8N1);
    delay(50);
    Serial.print("[GATE2-PROBE]   Serial1 TX=");
    Serial.print(PIN_SERIAL1_TX);
    Serial.print(" RX=");
    Serial.println(PIN_SERIAL1_RX);
    Serial.println("[GATE2-PROBE]   4800 8N1");
    Serial.println("[GATE2-PROBE]   DE/RE: auto (RAK5802 hardware)");

    /* Drain any garbage */
    while (Serial1.available()) Serial1.read();

    /* STEP 3: Build ONE Modbus request */
    Serial.println("[GATE2-PROBE] STEP 3: Build TX frame");
    uint8_t tx[8];
    tx[0] = 0x01;  /* Slave 1 */
    tx[1] = 0x03;  /* Function: Read Holding Registers */
    tx[2] = 0x00;  /* Reg hi */
    tx[3] = 0x00;  /* Reg lo */
    tx[4] = 0x00;  /* Qty hi */
    tx[5] = 0x01;  /* Qty lo */
    uint16_t crc = modbus_crc16(tx, 6);
    tx[6] = (uint8_t)(crc & 0xFF);
    tx[7] = (uint8_t)((crc >> 8) & 0xFF);

    Serial.print("[GATE2-PROBE]   TX: ");
    print_hex(tx, 8);
    Serial.println();

    /* STEP 4: Send TX once */
    Serial.println("[GATE2-PROBE] STEP 4: Transmit");
    Serial1.write(tx, 8);
    Serial1.flush();
    delayMicroseconds(500);  /* Guard time for auto-direction switchback */
    Serial.println("[GATE2-PROBE]   Sent 8 bytes, flush done");

    /* STEP 5: Listen for response (200ms) */
    Serial.println("[GATE2-PROBE] STEP 5: Listen (200ms timeout)");
    uint8_t rx[32];
    uint8_t rx_len = 0;
    unsigned long start = millis();

    /* Wait for first byte */
    while (!Serial1.available()) {
        if ((millis() - start) >= 200) break;
    }

    /* Accumulate bytes with 5ms inter-byte gap */
    if (Serial1.available()) {
        unsigned long last_byte = millis();
        while (rx_len < sizeof(rx)) {
            if (Serial1.available()) {
                rx[rx_len] = Serial1.read();
                Serial.print("[GATE2-PROBE]   RX: 0x");
                if (rx[rx_len] < 0x10) Serial.print("0");
                Serial.println(rx[rx_len], HEX);
                rx_len++;
                last_byte = millis();
            } else if ((millis() - last_byte) >= 5) {
                break;
            }
        }
    }

    Serial.print("[GATE2-PROBE]   RX total: ");
    Serial.print(rx_len);
    Serial.println(" bytes");

    if (rx_len > 0) {
        Serial.print("[GATE2-PROBE]   RX hex : ");
        print_hex(rx, rx_len);
        Serial.println();
    }

    /* STEP 6: PASS/FAIL */
    Serial.println("[GATE2-PROBE] STEP 6: Verdict");

    if (rx_len == 0) {
        Serial.println("[GATE2-PROBE] FAIL — no response (timeout)");
        Serial.println("[GATE2-PROBE] === GATE COMPLETE ===");
        return;
    }

    if (rx_len < 5) {
        Serial.print("[GATE2-PROBE] FAIL — response too short (");
        Serial.print(rx_len);
        Serial.println(" bytes, need >= 5)");
        Serial.println("[GATE2-PROBE] === GATE COMPLETE ===");
        return;
    }

    if (rx[0] != 0x01) {
        Serial.print("[GATE2-PROBE] FAIL — slave mismatch (got 0x");
        if (rx[0] < 0x10) Serial.print("0");
        Serial.print(rx[0], HEX);
        Serial.println(", expected 0x01)");
        Serial.println("[GATE2-PROBE] === GATE COMPLETE ===");
        return;
    }

    /* CRC check */
    uint16_t rx_crc = (uint16_t)rx[rx_len - 2] | ((uint16_t)rx[rx_len - 1] << 8);
    uint16_t calc_crc = modbus_crc16(rx, rx_len - 2);
    if (rx_crc != calc_crc) {
        Serial.print("[GATE2-PROBE] FAIL — CRC mismatch (rx=0x");
        Serial.print(rx_crc, HEX);
        Serial.print(" calc=0x");
        Serial.print(calc_crc, HEX);
        Serial.println(")");
        Serial.println("[GATE2-PROBE] === GATE COMPLETE ===");
        return;
    }

    Serial.print("[GATE2-PROBE]   CRC: 0x");
    Serial.print(rx_crc, HEX);
    Serial.println(" (valid)");
    Serial.println();
    Serial.println("[GATE2-PROBE] PASS");
    Serial.println("[GATE2-PROBE] === GATE COMPLETE ===");
}
