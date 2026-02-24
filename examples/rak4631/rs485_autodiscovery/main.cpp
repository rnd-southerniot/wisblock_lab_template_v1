/**
 * @file main.cpp
 * @brief RAK4631 Gate 2 — RS485 Modbus Autodiscovery Example
 * @version 1.0
 * @date 2026-02-24
 *
 * Standalone example demonstrating Modbus RTU device autodiscovery
 * over RS485 on the RAK4631 WisBlock Core (nRF52840 + SX1262).
 *
 * Hardware:
 *   - RAK4631 WisBlock Core (nRF52840 @ 64 MHz)
 *   - RAK5802 WisBlock RS485 Module (TP8485E, auto DE/RE)
 *   - RAK19007 WisBlock Base Board (or compatible)
 *   - External Modbus RTU slave (e.g., RS-FSJT-N01 wind sensor)
 *
 * RS485 Bus:
 *   - Serial1 TX = pin 16 (P0.16)
 *   - Serial1 RX = pin 15 (P0.15)
 *   - RAK5802 power = WB_IO2 (pin 34, 3V3_S enable)
 *   - DE/RE = auto (hardware circuit on RAK5802)
 *
 * Steps:
 *   1. Enable RAK5802 via 3V3_S power rail
 *   2. Scan baud rates × parity × slave IDs
 *   3. Send Modbus Read Holding Registers (FC 0x03) at each combo
 *   4. Validate response: length, CRC-16, slave match
 *   5. Report discovered device parameters
 */

#include <Arduino.h>
#include "pin_map.h"

/* ---- Scan Configuration ---- */
static const uint32_t SCAN_BAUDS[] = { 4800, 9600, 19200, 115200 };
#define SCAN_BAUD_COUNT     4
/* nRF52840 UARTE: only NONE and EVEN parity (no Odd in hardware) */
static const uint8_t SCAN_PARITIES[] = { 0, 2 };  /* 0=NONE, 2=EVEN */
#define SCAN_PARITY_COUNT   2
#define SCAN_SLAVE_MIN      1
#define SCAN_SLAVE_MAX      100

/* ---- Modbus ---- */
#define MODBUS_FUNC         0x03
#define RESPONSE_TIMEOUT_MS 200
#define INTERBYTE_GAP_MS    5

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

/* ---- Helpers ---- */

static uint32_t get_serial_config(uint8_t parity) {
    return (parity == 2) ? SERIAL_8E1 : SERIAL_8N1;
}

static const char* parity_name(uint8_t p) {
    switch (p) {
        case 1:  return "ODD";
        case 2:  return "EVEN";
        default: return "NONE";
    }
}

static void print_hex(const uint8_t *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        if (buf[i] < 0x10) Serial.print("0");
        Serial.print(buf[i], HEX);
        if (i < len - 1) Serial.print(" ");
    }
}

static void build_request(uint8_t slave, uint8_t *frame) {
    frame[0] = slave;
    frame[1] = MODBUS_FUNC;
    frame[2] = 0x00; frame[3] = 0x00;  /* Reg 0x0000 */
    frame[4] = 0x00; frame[5] = 0x01;  /* Qty 1 */
    uint16_t crc = modbus_crc16(frame, 6);
    frame[6] = (uint8_t)(crc & 0xFF);
    frame[7] = (uint8_t)((crc >> 8) & 0xFF);
}

static uint8_t read_response(uint8_t *buf, uint8_t max_len) {
    unsigned long start = millis();
    while (!Serial1.available()) {
        if ((millis() - start) >= RESPONSE_TIMEOUT_MS) return 0;
    }
    uint8_t count = 0;
    unsigned long last_byte = millis();
    while (count < max_len) {
        if (Serial1.available()) {
            buf[count++] = Serial1.read();
            last_byte = millis();
        } else if ((millis() - last_byte) >= INTERBYTE_GAP_MS) {
            break;
        }
    }
    return count;
}

static bool validate_response(const uint8_t *buf, uint8_t len, uint8_t slave) {
    if (len < 5) return false;
    uint16_t rx_crc = (uint16_t)buf[len - 2] | ((uint16_t)buf[len - 1] << 8);
    if (rx_crc != modbus_crc16(buf, len - 2)) return false;
    if (buf[0] != slave) return false;
    if (buf[1] != MODBUS_FUNC && buf[1] != (MODBUS_FUNC | 0x80)) return false;
    return true;
}

/* ---- Main ---- */

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }
    delay(1000);

    Serial.println();
    Serial.println("========================================");
    Serial.println("[SYSTEM] RAK4631 RS485 Autodiscovery");
    Serial.println("========================================");

    /* Enable RAK5802 */
    Serial.print("[INFO] RS485 power enable: pin ");
    Serial.println(PIN_RS485_EN);
    pinMode(PIN_RS485_EN, OUTPUT);
    digitalWrite(PIN_RS485_EN, HIGH);
    delay(300);

    Serial.print("[INFO] Serial1 TX=");
    Serial.print(PIN_UART1_TX);
    Serial.print(" RX=");
    Serial.println(PIN_UART1_RX);
    Serial.println("[INFO] DE/RE: auto (RAK5802 hardware)");

    /* Scan */
    uint32_t total = (uint32_t)SCAN_BAUD_COUNT * SCAN_PARITY_COUNT *
                     (SCAN_SLAVE_MAX - SCAN_SLAVE_MIN + 1);
    Serial.print("[INFO] Max probes: ");
    Serial.println(total);

    bool found = false;
    uint32_t found_baud = 0;
    uint8_t  found_parity = 0, found_slave = 0;
    uint8_t  found_frame[32];
    uint8_t  found_frame_len = 0;
    uint32_t probe_count = 0;
    uint8_t  tx[8], rx[32];

    for (int b = 0; b < SCAN_BAUD_COUNT && !found; b++) {
        for (uint8_t pi = 0; pi < SCAN_PARITY_COUNT && !found; pi++) {
            uint32_t baud = SCAN_BAUDS[b];
            uint8_t p = SCAN_PARITIES[pi];

            Serial.print("[SCAN] baud=");
            Serial.print(baud);
            Serial.print(" parity=");
            Serial.println(parity_name(p));

            Serial1.begin(baud, get_serial_config(p));
            delay(50);
            while (Serial1.available()) Serial1.read();

            for (uint8_t slave = SCAN_SLAVE_MIN;
                 slave <= SCAN_SLAVE_MAX && !found; slave++) {
                probe_count++;
                build_request(slave, tx);
                Serial1.write(tx, 8);
                Serial1.flush();
                delayMicroseconds(500);

                uint8_t rx_len = read_response(rx, sizeof(rx));
                if (rx_len > 0 && validate_response(rx, rx_len, slave)) {
                    found = true;
                    found_baud = baud;
                    found_parity = p;
                    found_slave = slave;
                    found_frame_len = rx_len;
                    memcpy(found_frame, rx, rx_len);
                }
            }
        }
    }

    /* Results */
    Serial.println();
    Serial.print("[RESULT] Probes: ");
    Serial.println(probe_count);

    if (!found) {
        Serial.println("[RESULT] FAIL — no device discovered");
    } else {
        Serial.print("[RESULT] Slave : ");
        Serial.println(found_slave);
        Serial.print("[RESULT] Baud  : ");
        Serial.println(found_baud);
        Serial.print("[RESULT] Parity: ");
        Serial.println(parity_name(found_parity));

        Serial.print("[RESULT] TX: ");
        build_request(found_slave, tx);
        print_hex(tx, 8);
        Serial.println();

        Serial.print("[RESULT] RX: ");
        print_hex(found_frame, found_frame_len);
        Serial.println();

        uint16_t crc = (uint16_t)found_frame[found_frame_len - 2] |
                       ((uint16_t)found_frame[found_frame_len - 1] << 8);
        Serial.print("[RESULT] CRC: 0x");
        Serial.print(crc, HEX);
        Serial.println(" (valid)");

        Serial.println("[RESULT] PASS");

        pinMode(PIN_LED_BLUE, OUTPUT);
        digitalWrite(PIN_LED_BLUE, HIGH);
        delay(500);
        digitalWrite(PIN_LED_BLUE, LOW);
    }
}

void loop() {
    delay(10000);
}
