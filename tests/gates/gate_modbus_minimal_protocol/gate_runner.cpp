/**
 * @file gate_runner.cpp
 * @brief Gate 3 — Modbus Minimal Protocol Validation Runner
 * @version 1.1
 * @date 2026-02-24
 *
 * Validates Modbus RTU protocol layer using previously discovered
 * device parameters defined in gate_config.h (from Gate 2).
 *
 * Sends a single Read Holding Register (FC 0x03) request, parses the
 * response using the modbus_frame layer, and prints structured output.
 *
 * All device-specific parameters (slave, baud, parity) are sourced
 * exclusively from gate_config.h GATE_DISCOVERED_* defines.
 *
 * Uses hal_uart HAL (from Gate 2) for RS485 communication.
 * Uses modbus_frame for CRC-16, frame building, and response parsing.
 *
 * Does NOT implement a full Modbus library.
 * Does NOT integrate into runtime/.
 * Aborts on first critical failure.
 */

#include <Arduino.h>
#include "gate_config.h"
#include "modbus_frame.h"
#include "hal_uart.h"

/* ============================================================
 * State
 * ============================================================ */
static uint8_t gate_result = GATE_PASS;

/* Parsed response storage */
static ModbusResponse parsed_response;

/* ============================================================
 * Parity Name Helper
 * ============================================================ */
static const char* parity_name(uint8_t p) {
    switch (p) {
        case 0:  return "NONE";
        case 1:  return "ODD";
        case 2:  return "EVEN";
        default: return "?";
    }
}

/* ============================================================
 * Frame Config Char Helper (e.g. 'N' for 8N1)
 * ============================================================ */
static char frame_config_char(uint8_t p) {
    switch (p) {
        case 0:  return 'N';
        case 1:  return 'O';
        case 2:  return 'E';
        default: return '?';
    }
}

/* ============================================================
 * Hex Dump Helper — prints byte array as hex string
 * ============================================================ */
static void print_hex(const uint8_t *data, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        Serial.printf("%02X", data[i]);
        if (i < len - 1) Serial.printf(" ");
    }
}

/* ============================================================
 * STEP 1: UART + RS485 Initialization
 *
 * All parameters sourced from gate_config.h GATE_DISCOVERED_* defines.
 * ============================================================ */
static bool step_init(void) {
    Serial.printf("%s STEP 1: UART + RS485 init "
                  "(Slave=%d, Baud=%lu, Parity=%s)\r\n",
                  GATE_LOG_TAG,
                  GATE_DISCOVERED_SLAVE,
                  (unsigned long)GATE_DISCOVERED_BAUD,
                  parity_name(GATE_DISCOVERED_PARITY));

    /* Enable RAK5802 transceiver (power on + configure DE pin) */
    hal_rs485_enable(GATE_RS485_EN_PIN, GATE_RS485_DE_PIN);
    Serial.printf("%s RS485 EN=HIGH (GPIO%d), DE configured (GPIO%d)\r\n",
                  GATE_LOG_TAG, GATE_RS485_EN_PIN, GATE_RS485_DE_PIN);

    /* Init UART with discovered parameters from gate_config.h */
    if (!hal_uart_init(GATE_UART_TX_PIN, GATE_UART_RX_PIN,
                       GATE_DISCOVERED_BAUD, GATE_DISCOVERED_PARITY)) {
        Serial.printf("%s UART Init: FAIL\r\n", GATE_LOG_TAG);
        gate_result = GATE_FAIL_UART_INIT;
        return false;
    }

    /* Flush and stabilize */
    hal_uart_flush();
    delay(GATE_UART_STABILIZE_MS);

    Serial.printf("%s UART Init: OK (%lu 8%c1)\r\n",
                  GATE_LOG_TAG,
                  (unsigned long)GATE_DISCOVERED_BAUD,
                  frame_config_char(GATE_DISCOVERED_PARITY));
    return true;
}

/* ============================================================
 * STEP 2: Modbus Read — Send Request + Parse Response
 *
 * Sends FC 0x03 (Read Holding Registers) to configured register.
 * Parses response using modbus_frame layer.
 * Prints structured output per spec.
 * ============================================================ */
static bool step_modbus_read(void) {
    Serial.printf("%s STEP 2: Modbus Read Holding Register "
                  "(FC=0x%02X, Reg=0x%02X%02X, Qty=%d)\r\n",
                  GATE_LOG_TAG,
                  GATE_MODBUS_FUNC_READ_HOLD,
                  GATE_MODBUS_TARGET_REG_HI,
                  GATE_MODBUS_TARGET_REG_LO,
                  GATE_MODBUS_QUANTITY_LO);

    /* ---- Build request frame ---- */
    uint8_t req_frame[GATE_MODBUS_REQ_FRAME_LEN];
    uint8_t req_len = modbus_build_read_holding(
        GATE_DISCOVERED_SLAVE,
        GATE_MODBUS_TARGET_REG_HI, GATE_MODBUS_TARGET_REG_LO,
        GATE_MODBUS_QUANTITY_HI, GATE_MODBUS_QUANTITY_LO,
        req_frame);

    /* ---- Flush stale data ---- */
    hal_uart_flush();
    delay(GATE_UART_FLUSH_DELAY_MS);

    /* ---- Log TX ---- */
    Serial.printf("%s TX (%d bytes): ", GATE_LOG_TAG, req_len);
    print_hex(req_frame, req_len);
    Serial.printf("\r\n");

    /* ---- Send request ---- */
    if (!hal_uart_write(req_frame, req_len)) {
        Serial.printf("%s TX FAIL\r\n", GATE_LOG_TAG);
        gate_result = GATE_FAIL_TX_ERROR;
        return false;
    }

    /* ---- Wait for response ---- */
    uint8_t resp_buf[GATE_MODBUS_RESP_MAX_LEN];
    uint8_t rx_len = hal_uart_read(resp_buf,
                                    GATE_MODBUS_RESP_MAX_LEN,
                                    GATE_RESPONSE_TIMEOUT_MS);

    if (rx_len == 0) {
        Serial.printf("%s RX: timeout (no response in %d ms)\r\n",
                      GATE_LOG_TAG, GATE_RESPONSE_TIMEOUT_MS);
        gate_result = GATE_FAIL_NO_RESPONSE;
        return false;
    }

    /* ---- Log RX ---- */
    Serial.printf("%s RX (%d bytes): ", GATE_LOG_TAG, rx_len);
    print_hex(resp_buf, rx_len);
    Serial.printf("\r\n");

    /* ---- Short frame check ---- */
    if (rx_len < GATE_MODBUS_RESP_MIN_LEN) {
        Serial.printf("%s Short frame (%d < %d bytes)\r\n",
                      GATE_LOG_TAG, rx_len, GATE_MODBUS_RESP_MIN_LEN);
        gate_result = GATE_FAIL_SHORT_FRAME;
        return false;
    }

    /* ---- Parse response using modbus_frame layer ---- */
    parsed_response = modbus_parse_response(
        GATE_DISCOVERED_SLAVE,
        GATE_MODBUS_FUNC_READ_HOLD,
        resp_buf, rx_len);

    if (!parsed_response.valid) {
        /* Determine specific failure reason for diagnostics */
        uint16_t calc_crc = modbus_crc16(resp_buf, rx_len - 2);
        uint16_t recv_crc = (uint16_t)resp_buf[rx_len - 2]
                          | ((uint16_t)resp_buf[rx_len - 1] << 8);
        if (calc_crc != recv_crc) {
            Serial.printf("%s CRC mismatch: calc=0x%04X recv=0x%04X\r\n",
                          GATE_LOG_TAG, calc_crc, recv_crc);
            gate_result = GATE_FAIL_CRC_ERROR;
        } else if (resp_buf[0] != GATE_DISCOVERED_SLAVE) {
            Serial.printf("%s Address mismatch: expected=%d received=%d\r\n",
                          GATE_LOG_TAG, GATE_DISCOVERED_SLAVE, resp_buf[0]);
            gate_result = GATE_FAIL_ADDR_MISMATCH;
        } else if (rx_len >= 3 && resp_buf[2] != (rx_len - 5)) {
            Serial.printf("%s byte_count mismatch: declared=%d actual=%d\r\n",
                          GATE_LOG_TAG, resp_buf[2], rx_len - 5);
            gate_result = GATE_FAIL_BYTE_COUNT;
        } else {
            Serial.printf("%s Response validation failed\r\n", GATE_LOG_TAG);
            gate_result = GATE_FAIL_CRC_ERROR;
        }
        return false;
    }

    /* ---- Print structured output ---- */
    if (parsed_response.exception) {
        Serial.printf("%s Exception Code=0x%02X\r\n",
                      GATE_LOG_TAG, parsed_response.exc_code);
    } else {
        Serial.printf("%s Slave=%d Value=0x%04X\r\n",
                      GATE_LOG_TAG,
                      parsed_response.slave,
                      parsed_response.value);
    }

    return true;
}

/* ============================================================
 * STEP 3: Protocol Validation Summary
 * ============================================================ */
static bool step_summary(void) {
    Serial.printf("%s STEP 3: Protocol validation summary\r\n", GATE_LOG_TAG);

    Serial.printf("%s   Target Slave  : %d\r\n",
                  GATE_LOG_TAG, GATE_DISCOVERED_SLAVE);
    Serial.printf("%s   Baud Rate     : %lu\r\n",
                  GATE_LOG_TAG, (unsigned long)GATE_DISCOVERED_BAUD);
    Serial.printf("%s   Frame Config  : 8%c1\r\n",
                  GATE_LOG_TAG,
                  frame_config_char(GATE_DISCOVERED_PARITY));
    Serial.printf("%s   Function Code : 0x%02X (Read Holding Registers)\r\n",
                  GATE_LOG_TAG, GATE_MODBUS_FUNC_READ_HOLD);
    Serial.printf("%s   Register      : 0x%02X%02X\r\n",
                  GATE_LOG_TAG,
                  GATE_MODBUS_TARGET_REG_HI,
                  GATE_MODBUS_TARGET_REG_LO);
    Serial.printf("%s   Protocol      : Modbus RTU\r\n", GATE_LOG_TAG);

    if (parsed_response.valid) {
        Serial.printf("%s   Response Type : %s\r\n",
                      GATE_LOG_TAG,
                      parsed_response.exception ? "Exception" : "Normal");
        if (parsed_response.exception) {
            Serial.printf("%s   Exception Code: 0x%02X\r\n",
                          GATE_LOG_TAG, parsed_response.exc_code);
        } else {
            Serial.printf("%s   byte_count    : %d\r\n",
                          GATE_LOG_TAG, parsed_response.byte_count);
            Serial.printf("%s   Register Value: 0x%04X (%u)\r\n",
                          GATE_LOG_TAG,
                          parsed_response.value,
                          parsed_response.value);
        }
        Serial.printf("%s   Response Len  : %d bytes\r\n",
                      GATE_LOG_TAG, parsed_response.raw_len);
        Serial.printf("%s   Response Hex  : ", GATE_LOG_TAG);
        print_hex(parsed_response.raw, parsed_response.raw_len);
        Serial.printf("\r\n");
    }

    return true;
}

/* ============================================================
 * Gate Entry Point
 * ============================================================ */
void gate_modbus_minimal_protocol_run(void) {
    gate_result = GATE_PASS;
    memset(&parsed_response, 0, sizeof(parsed_response));

    Serial.printf("\r\n%s === GATE START: %s v%s ===\r\n",
                  GATE_LOG_TAG, GATE_NAME, GATE_VERSION);

    if (!step_init())         goto done;
    if (!step_modbus_read())  goto done;
    if (!step_summary())      goto done;

done:
    /* Cleanup: deinit UART + disable RS485 transceiver */
    hal_uart_deinit();
    hal_rs485_disable();

    if (gate_result == GATE_PASS) {
        Serial.printf("%s PASS\r\n", GATE_LOG_TAG);
    } else {
        Serial.printf("%s FAIL (code=%d)\r\n", GATE_LOG_TAG, gate_result);
    }
    Serial.printf("%s === GATE COMPLETE ===\r\n", GATE_LOG_TAG);
}
