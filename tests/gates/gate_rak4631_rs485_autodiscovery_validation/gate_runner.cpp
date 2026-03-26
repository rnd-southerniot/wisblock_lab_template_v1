/**
 * @file gate_runner.cpp
 * @brief Gate 2 — RAK4631 RS485 Modbus Autodiscovery
 * @version 1.0
 * @date 2026-02-24
 *
 * Scan baud rates × parity modes × slave IDs to discover
 * a Modbus RTU device on the RS485 bus (RAK5802).
 * No runtime, no LoRa, no shared HAL.
 *
 * Entry point: gate_rak4631_rs485_autodiscovery_run()
 */

#include <Arduino.h>
#include "gate_config.h"
#include "hal_uart.h"

/* ---- CRC-16/MODBUS ---- */
static uint16_t modbus_crc16(const uint8_t *data, uint8_t len) {
    uint16_t crc = CRC_INIT;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ CRC_POLY;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc;
}

/* ---- Build Modbus request frame ---- */
static void build_request(uint8_t slave, uint8_t *frame) {
    frame[0] = slave;
    frame[1] = MODBUS_FUNC_READ_HOLD;
    frame[2] = MODBUS_TARGET_REG_HI;
    frame[3] = MODBUS_TARGET_REG_LO;
    frame[4] = MODBUS_QUANTITY_HI;
    frame[5] = MODBUS_QUANTITY_LO;
    uint16_t crc = modbus_crc16(frame, 6);
    frame[6] = (uint8_t)(crc & 0xFF);
    frame[7] = (uint8_t)((crc >> 8) & 0xFF);
}

/* ---- Validate response ---- */
static bool validate_response(const uint8_t *buf, uint8_t len, uint8_t slave,
                               bool *is_exception) {
    *is_exception = false;

    /* 1. Frame length */
    if (len < MODBUS_RESPONSE_MIN) return false;

    /* 2. CRC check */
    uint16_t rx_crc = (uint16_t)buf[len - 2] | ((uint16_t)buf[len - 1] << 8);
    uint16_t calc_crc = modbus_crc16(buf, len - 2);
    if (rx_crc != calc_crc) return false;

    /* 3. Slave address match */
    if (buf[0] != slave) return false;

    /* 4. Exception check (func | 0x80) */
    if (buf[1] == (MODBUS_FUNC_READ_HOLD | 0x80)) {
        *is_exception = true;
        return true;    /* Valid discovery — device responded with exception */
    }

    /* 5. Normal function code match */
    if (buf[1] != MODBUS_FUNC_READ_HOLD) return false;

    return true;
}

/* ---- Parity name helper ---- */
static const char* parity_name(uint8_t p) {
    switch (p) {
        case 1:  return "ODD";
        case 2:  return "EVEN";
        default: return "NONE";
    }
}

/* ---- Hex dump helper ---- */
static void print_hex(const uint8_t *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        if (buf[i] < 0x10) Serial.print("0");
        Serial.print(buf[i], HEX);
        if (i < len - 1) Serial.print(" ");
    }
}

/* ---- Gate entry point ---- */
void gate_rak4631_rs485_autodiscovery_run(void) {
    Serial.println();
    Serial.println(GATE_LOG_TAG " === GATE START: " GATE_NAME " v" GATE_VERSION " ===");

    /* STEP 1: Enable RS485 transceiver */
    Serial.println(GATE_LOG_TAG " STEP 1: Enable RS485 (RAK5802)");
    Serial.print(GATE_LOG_TAG "   EN pin: ");
    Serial.print(RS485_EN_PIN);
    Serial.println(" (WB_IO2 — 3V3_S power)");
    Serial.print(GATE_LOG_TAG "   Serial1 TX=");
    Serial.print(PIN_SERIAL1_TX);
    Serial.print(" RX=");
    Serial.println(PIN_SERIAL1_RX);
    hal_rs485_enable(RS485_EN_PIN);
    Serial.println(GATE_LOG_TAG "   DE/RE: auto (RAK5802 hardware auto-direction)");

    /* STEP 2: Scan */
    Serial.println(GATE_LOG_TAG " STEP 2: Autodiscovery scan");
    Serial.print(GATE_LOG_TAG "   Bauds: ");
    for (int i = 0; i < SCAN_BAUD_COUNT; i++) {
        Serial.print(SCAN_BAUDS[i]);
        if (i < SCAN_BAUD_COUNT - 1) Serial.print(", ");
    }
    Serial.println();
    Serial.print(GATE_LOG_TAG "   Parity: NONE, EVEN (");
    Serial.print(SCAN_PARITY_COUNT);
    Serial.println(" modes — nRF52 has no Odd parity)");
    Serial.print(GATE_LOG_TAG "   Slaves: ");
    Serial.print(SCAN_SLAVE_MIN);
    Serial.print(".."); Serial.println(SCAN_SLAVE_MAX);

    uint32_t total_probes = (uint32_t)SCAN_BAUD_COUNT * SCAN_PARITY_COUNT *
                            (SCAN_SLAVE_MAX - SCAN_SLAVE_MIN + 1);
    Serial.print(GATE_LOG_TAG "   Max probes: ");
    Serial.println(total_probes);

    bool found = false;
    uint32_t found_baud = 0;
    uint8_t  found_parity = 0;
    uint8_t  found_slave = 0;
    bool     found_exception = false;
    uint8_t  found_frame[32];
    uint8_t  found_frame_len = 0;
    uint32_t probe_count = 0;

    uint8_t tx_frame[MODBUS_REQUEST_LEN];
    uint8_t rx_buf[32];

    for (int b = 0; b < SCAN_BAUD_COUNT && !found; b++) {
        for (uint8_t pi = 0; pi < SCAN_PARITY_COUNT && !found; pi++) {
            uint8_t p = SCAN_PARITIES[pi];
            uint32_t baud = SCAN_BAUDS[b];

            Serial.print(GATE_LOG_TAG "   Trying baud=");
            Serial.print(baud);
            Serial.print(" parity=");
            Serial.print(parity_name(p));
            Serial.println("...");

            hal_uart_init(baud, p);

            for (uint8_t slave = SCAN_SLAVE_MIN;
                 slave <= SCAN_SLAVE_MAX && !found; slave++) {

                probe_count++;
                build_request(slave, tx_frame);
                hal_uart_write(tx_frame, MODBUS_REQUEST_LEN);

                uint8_t rx_len = hal_uart_read(rx_buf, sizeof(rx_buf),
                                               UART_RESPONSE_TIMEOUT_MS);

                if (rx_len > 0) {
                    bool is_exception = false;
                    if (validate_response(rx_buf, rx_len, slave, &is_exception)) {
                        found = true;
                        found_baud = baud;
                        found_parity = p;
                        found_slave = slave;
                        found_exception = is_exception;
                        found_frame_len = rx_len;
                        memcpy(found_frame, rx_buf, rx_len);

                        Serial.print(GATE_LOG_TAG "   >>> FOUND at probe #");
                        Serial.println(probe_count);
                    }
                }
            }

            hal_uart_flush();
        }
    }

    /* STEP 3: Results */
    Serial.println();
    Serial.println(GATE_LOG_TAG " STEP 3: Results");
    Serial.print(GATE_LOG_TAG "   Probes sent: ");
    Serial.println(probe_count);

    if (!found) {
        Serial.println(GATE_LOG_TAG " FAIL — no Modbus device discovered");
        Serial.println(GATE_LOG_TAG " === GATE COMPLETE ===");
        return;
    }

    Serial.print(GATE_LOG_TAG "   Slave : ");
    Serial.println(found_slave);
    Serial.print(GATE_LOG_TAG "   Baud  : ");
    Serial.println(found_baud);
    Serial.print(GATE_LOG_TAG "   Parity: ");
    Serial.println(parity_name(found_parity));
    Serial.print(GATE_LOG_TAG "   Type  : ");
    Serial.println(found_exception ? "Exception response" : "Normal response");

    /* Print TX frame */
    Serial.print(GATE_LOG_TAG "   TX: ");
    uint8_t tx_display[MODBUS_REQUEST_LEN];
    build_request(found_slave, tx_display);
    print_hex(tx_display, MODBUS_REQUEST_LEN);
    Serial.println();

    /* Print RX frame */
    Serial.print(GATE_LOG_TAG "   RX: ");
    print_hex(found_frame, found_frame_len);
    Serial.println();

    /* CRC verification */
    uint16_t rx_crc = (uint16_t)found_frame[found_frame_len - 2] |
                      ((uint16_t)found_frame[found_frame_len - 1] << 8);
    Serial.print(GATE_LOG_TAG "   CRC: 0x");
    Serial.print(rx_crc, HEX);
    Serial.println(" (valid)");

    Serial.println();
    Serial.println(GATE_LOG_TAG " PASS");

    /* Blink blue LED */
    pinMode(LED_BLUE, OUTPUT);
    digitalWrite(LED_BLUE, LED_STATE_ON);
    delay(500);
    digitalWrite(LED_BLUE, !LED_STATE_ON);

    Serial.println(GATE_LOG_TAG " === GATE COMPLETE ===");
}
