/**
 * @file main.cpp
 * @brief Example — Modbus RTU Read Holding Register (RAK3312 / ESP32-S3)
 *
 * Demonstrates minimal Modbus RTU communication with an RS485 slave
 * device via RAK5802 transceiver on the WisBlock IO Slot.
 *
 * Sends FC 0x03 (Read Holding Registers) to register 0x0000 on slave 1,
 * parses the response, and prints the register value.
 *
 * Hardware:
 *   - RAK3312 WisBlock Core (ESP32-S3)
 *   - RAK19007 Base Board
 *   - RAK5802 RS485 Transceiver (IO Slot)
 *   - RS485 Modbus RTU slave device (e.g. RS-FSJT-N01 wind sensor)
 *
 * RS485 Pins (RAK3312):
 *   - UART1 TX = GPIO 43
 *   - UART1 RX = GPIO 44
 *   - RS485 EN = GPIO 14 (WB_IO2 — module power)
 *   - RS485 DE = GPIO 21 (WB_IO1 — driver enable)
 *
 * Based on Gate 3 validation — passed 2026-02-24.
 */

#include <Arduino.h>
#include "pin_map.h"

/* ---- Modbus RTU Constants ---- */
#define MODBUS_SLAVE_ADDR     1
#define MODBUS_FUNC_READ_HOLD 0x03
#define MODBUS_REG_HI         0x00
#define MODBUS_REG_LO         0x00
#define MODBUS_QTY_HI         0x00
#define MODBUS_QTY_LO         0x01
#define MODBUS_BAUD           4800
#define MODBUS_RESP_TIMEOUT   200   /* ms */
#define MODBUS_INTER_BYTE_MS  5     /* ms — frame delimiter */

/* ---- CRC-16/MODBUS (bitwise) ---- */
uint16_t modbus_crc16(const uint8_t *data, uint8_t len) {
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

/* ---- RS485 Direction Control ---- */
void rs485_enable(void) {
    pinMode(PIN_RS485_EN, OUTPUT);
    digitalWrite(PIN_RS485_EN, HIGH);
    pinMode(PIN_RS485_DE, OUTPUT);
    digitalWrite(PIN_RS485_DE, LOW);
    delay(100);
}

void rs485_tx_mode(void) {
    digitalWrite(PIN_RS485_EN, HIGH);
    digitalWrite(PIN_RS485_DE, HIGH);
    delayMicroseconds(100);
}

void rs485_rx_mode(void) {
    delayMicroseconds(100);
    digitalWrite(PIN_RS485_DE, LOW);
}

/* ---- Build Modbus Request ---- */
void build_request(uint8_t slave, uint8_t *frame) {
    frame[0] = slave;
    frame[1] = MODBUS_FUNC_READ_HOLD;
    frame[2] = MODBUS_REG_HI;
    frame[3] = MODBUS_REG_LO;
    frame[4] = MODBUS_QTY_HI;
    frame[5] = MODBUS_QTY_LO;
    uint16_t crc = modbus_crc16(frame, 6);
    frame[6] = (uint8_t)(crc & 0xFF);
    frame[7] = (uint8_t)((crc >> 8) & 0xFF);
}

/* ---- Read Response with Timeout ---- */
uint8_t read_response(uint8_t *buf, uint8_t max_len, uint32_t timeout_ms) {
    uint8_t count = 0;
    uint32_t start = millis();

    while (!Serial1.available()) {
        if ((millis() - start) >= timeout_ms) return 0;
    }

    uint32_t last_byte = millis();
    while (count < max_len) {
        if (Serial1.available()) {
            buf[count++] = Serial1.read();
            last_byte = millis();
        } else if ((millis() - last_byte) > MODBUS_INTER_BYTE_MS) {
            break;
        }
    }
    return count;
}

/* ---- Hex Print Helper ---- */
void print_hex(const uint8_t *data, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        Serial.printf("%02X", data[i]);
        if (i < len - 1) Serial.printf(" ");
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000);
    delay(500);

    Serial.println("\n=== Modbus RTU Read Example (RAK3312 + RAK5802) ===\n");

    /* Init RS485 transceiver */
    rs485_enable();
    Serial.printf("RS485: EN=GPIO%d, DE=GPIO%d\n", PIN_RS485_EN, PIN_RS485_DE);

    /* Init UART1 */
    Serial1.begin(MODBUS_BAUD, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);
    delay(50);
    Serial.printf("UART1: TX=GPIO%d, RX=GPIO%d, %d 8N1\n",
                  PIN_UART_TX, PIN_UART_RX, MODBUS_BAUD);
}

void loop() {
    /* Build request frame */
    uint8_t req[8];
    build_request(MODBUS_SLAVE_ADDR, req);

    /* Flush stale data */
    Serial1.flush();
    while (Serial1.available()) Serial1.read();
    delay(10);

    /* Send request */
    Serial.printf("\nTX: ");
    print_hex(req, 8);
    Serial.printf("\n");

    rs485_tx_mode();
    Serial1.write(req, 8);
    Serial1.flush();
    rs485_rx_mode();

    /* Read response */
    uint8_t resp[32];
    uint8_t rx_len = read_response(resp, 32, MODBUS_RESP_TIMEOUT);

    if (rx_len == 0) {
        Serial.printf("RX: timeout\n");
        delay(2000);
        return;
    }

    Serial.printf("RX: ");
    print_hex(resp, rx_len);
    Serial.printf("\n");

    /* Validate */
    if (rx_len < 5) {
        Serial.printf("Short frame (%d bytes)\n", rx_len);
    } else {
        uint16_t calc = modbus_crc16(resp, rx_len - 2);
        uint16_t recv = (uint16_t)resp[rx_len - 2]
                      | ((uint16_t)resp[rx_len - 1] << 8);
        if (calc != recv) {
            Serial.printf("CRC mismatch: calc=0x%04X recv=0x%04X\n", calc, recv);
        } else if (resp[0] != MODBUS_SLAVE_ADDR) {
            Serial.printf("Address mismatch: %d\n", resp[0]);
        } else if (resp[1] & 0x80) {
            Serial.printf("Exception: code=0x%02X\n", resp[2]);
        } else if (resp[2] != (rx_len - 5)) {
            Serial.printf("byte_count mismatch: %d vs %d\n",
                          resp[2], rx_len - 5);
        } else {
            uint16_t value = ((uint16_t)resp[3] << 8) | resp[4];
            Serial.printf("Slave=%d Reg=0x0000 Value=0x%04X (%u)\n",
                          MODBUS_SLAVE_ADDR, value, value);
        }
    }

    delay(2000);
}
