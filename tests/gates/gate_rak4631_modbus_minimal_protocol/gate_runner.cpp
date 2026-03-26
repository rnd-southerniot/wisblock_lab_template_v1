/**
 * @file gate_runner.cpp
 * @brief Gate 3 — RAK4631 Modbus Minimal Protocol Validation Runner
 * @version 1.0
 * @date 2026-02-24
 *
 * Validates Modbus RTU protocol layer using previously discovered
 * device parameters defined in gate_config.h (from Gate 2).
 *
 * Sends a single Read Holding Register (FC 0x03) request, parses the
 * response using the 7-step modbus_frame parser, and prints structured output.
 *
 * All device-specific parameters sourced from gate_config.h GATE_DISCOVERED_*.
 * Uses hal_uart HAL for RS485 communication (auto DE/RE via RAK5802).
 *
 * Does NOT implement a full Modbus library.
 * Does NOT integrate into runtime/.
 * Aborts on first critical failure.
 *
 * Entry point: gate_rak4631_modbus_minimal_protocol_run()
 */

#include <Arduino.h>
#include <string.h>
#include "gate_config.h"
#include "modbus_frame.h"
#include "hal_uart.h"

/* ============================================================
 * State
 * ============================================================ */
static uint8_t gate_result = GATE_PASS;
static ModbusResponse parsed_response;

/* ============================================================
 * Parity Name Helper
 * ============================================================ */
static const char* parity_name(uint8_t p) {
    switch (p) {
        case 1:  return "ODD";
        case 2:  return "EVEN";
        default: return "NONE";
    }
}

static char frame_config_char(uint8_t p) {
    switch (p) {
        case 1:  return 'O';
        case 2:  return 'E';
        default: return 'N';
    }
}

/* ============================================================
 * Hex Dump Helper
 * ============================================================ */
static void print_hex(const uint8_t *data, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        if (data[i] < 0x10) Serial.print("0");
        Serial.print(data[i], HEX);
        if (i < len - 1) Serial.print(" ");
    }
}

/* ============================================================
 * STEP 1: UART + RS485 Initialization
 * ============================================================ */
static bool step_init(void) {
    Serial.print(GATE_LOG_TAG " STEP 1: UART + RS485 init (Slave=");
    Serial.print(GATE_DISCOVERED_SLAVE);
    Serial.print(", Baud=");
    Serial.print(GATE_DISCOVERED_BAUD);
    Serial.print(", Parity=");
    Serial.print(parity_name(GATE_DISCOVERED_PARITY));
    Serial.println(")");

    /* Enable RAK5802 transceiver (power via 3V3_S) */
    hal_rs485_enable(GATE_RS485_EN_PIN);
    Serial.print(GATE_LOG_TAG "   RS485 EN=HIGH (pin ");
    Serial.print(GATE_RS485_EN_PIN);
    Serial.println(", WB_IO2 — 3V3_S)");
    Serial.println(GATE_LOG_TAG "   DE/RE: auto (RAK5802 hardware)");

    /* Init UART with discovered parameters */
    if (!hal_uart_init(GATE_DISCOVERED_BAUD, GATE_DISCOVERED_PARITY)) {
        Serial.println(GATE_LOG_TAG "   UART Init: FAIL");
        gate_result = GATE_FAIL_UART_INIT;
        return false;
    }

    /* Flush and stabilize */
    hal_uart_flush();
    delay(GATE_UART_STABILIZE_MS);

    Serial.print(GATE_LOG_TAG "   UART Init: OK (");
    Serial.print(GATE_DISCOVERED_BAUD);
    Serial.print(" 8");
    Serial.print(frame_config_char(GATE_DISCOVERED_PARITY));
    Serial.println("1)");
    Serial.print(GATE_LOG_TAG "   Serial1 TX=");
    Serial.print(PIN_SERIAL1_TX);
    Serial.print(" RX=");
    Serial.println(PIN_SERIAL1_RX);

    return true;
}

/* ============================================================
 * STEP 2: Modbus Read — Send Request + Parse Response
 * ============================================================ */
static bool step_modbus_read(void) {
    Serial.print(GATE_LOG_TAG " STEP 2: Modbus Read Holding Register (FC=0x");
    if (GATE_MODBUS_FUNC_READ_HOLD < 0x10) Serial.print("0");
    Serial.print(GATE_MODBUS_FUNC_READ_HOLD, HEX);
    Serial.print(", Reg=0x");
    if (GATE_MODBUS_TARGET_REG_HI < 0x10) Serial.print("0");
    Serial.print(GATE_MODBUS_TARGET_REG_HI, HEX);
    if (GATE_MODBUS_TARGET_REG_LO < 0x10) Serial.print("0");
    Serial.print(GATE_MODBUS_TARGET_REG_LO, HEX);
    Serial.print(", Qty=");
    Serial.print(GATE_MODBUS_QUANTITY_LO);
    Serial.println(")");

    /* Build request frame */
    uint8_t req_frame[GATE_MODBUS_REQ_FRAME_LEN];
    uint8_t req_len = modbus_build_read_holding(
        GATE_DISCOVERED_SLAVE,
        GATE_MODBUS_TARGET_REG_HI, GATE_MODBUS_TARGET_REG_LO,
        GATE_MODBUS_QUANTITY_HI, GATE_MODBUS_QUANTITY_LO,
        req_frame);

    /* Flush stale data */
    hal_uart_flush();
    delay(GATE_UART_FLUSH_DELAY_MS);

    /* Log TX */
    Serial.print(GATE_LOG_TAG "   TX (");
    Serial.print(req_len);
    Serial.print(" bytes): ");
    print_hex(req_frame, req_len);
    Serial.println();

    /* Send request */
    if (!hal_uart_write(req_frame, req_len)) {
        Serial.println(GATE_LOG_TAG "   TX FAIL");
        gate_result = GATE_FAIL_TX_ERROR;
        return false;
    }

    /* Wait for response */
    uint8_t resp_buf[GATE_MODBUS_RESP_MAX_LEN];
    uint8_t rx_len = hal_uart_read(resp_buf,
                                    GATE_MODBUS_RESP_MAX_LEN,
                                    GATE_RESPONSE_TIMEOUT_MS);

    if (rx_len == 0) {
        Serial.print(GATE_LOG_TAG "   RX: timeout (no response in ");
        Serial.print(GATE_RESPONSE_TIMEOUT_MS);
        Serial.println(" ms)");
        gate_result = GATE_FAIL_NO_RESPONSE;
        return false;
    }

    /* Log RX */
    Serial.print(GATE_LOG_TAG "   RX (");
    Serial.print(rx_len);
    Serial.print(" bytes): ");
    print_hex(resp_buf, rx_len);
    Serial.println();

    /* Short frame check */
    if (rx_len < GATE_MODBUS_RESP_MIN_LEN) {
        Serial.print(GATE_LOG_TAG "   Short frame (");
        Serial.print(rx_len);
        Serial.print(" < ");
        Serial.print(GATE_MODBUS_RESP_MIN_LEN);
        Serial.println(" bytes)");
        gate_result = GATE_FAIL_SHORT_FRAME;
        return false;
    }

    /* Parse response using 7-step modbus_frame layer */
    parsed_response = modbus_parse_response(
        GATE_DISCOVERED_SLAVE,
        GATE_MODBUS_FUNC_READ_HOLD,
        resp_buf, rx_len);

    if (!parsed_response.valid) {
        /* Determine specific failure reason */
        uint16_t calc_crc = modbus_crc16(resp_buf, rx_len - 2);
        uint16_t recv_crc = (uint16_t)resp_buf[rx_len - 2]
                          | ((uint16_t)resp_buf[rx_len - 1] << 8);

        if (calc_crc != recv_crc) {
            Serial.print(GATE_LOG_TAG "   CRC mismatch: calc=0x");
            Serial.print(calc_crc, HEX);
            Serial.print(" recv=0x");
            Serial.println(recv_crc, HEX);
            gate_result = GATE_FAIL_CRC_ERROR;
        } else if (resp_buf[0] != GATE_DISCOVERED_SLAVE) {
            Serial.print(GATE_LOG_TAG "   Address mismatch: expected=");
            Serial.print(GATE_DISCOVERED_SLAVE);
            Serial.print(" received=");
            Serial.println(resp_buf[0]);
            gate_result = GATE_FAIL_ADDR_MISMATCH;
        } else if (rx_len >= 3 && resp_buf[2] != (rx_len - 5)) {
            Serial.print(GATE_LOG_TAG "   byte_count mismatch: declared=");
            Serial.print(resp_buf[2]);
            Serial.print(" actual=");
            Serial.println(rx_len - 5);
            gate_result = GATE_FAIL_BYTE_COUNT;
        } else {
            Serial.println(GATE_LOG_TAG "   Response validation failed");
            gate_result = GATE_FAIL_CRC_ERROR;
        }
        return false;
    }

    /* Print structured output */
    if (parsed_response.exception) {
        Serial.print(GATE_LOG_TAG "   Exception Code=0x");
        if (parsed_response.exc_code < 0x10) Serial.print("0");
        Serial.println(parsed_response.exc_code, HEX);
    } else {
        Serial.print(GATE_LOG_TAG "   Slave=");
        Serial.print(parsed_response.slave);
        Serial.print(" Value=0x");
        if (parsed_response.value < 0x1000) Serial.print("0");
        if (parsed_response.value < 0x0100) Serial.print("0");
        if (parsed_response.value < 0x0010) Serial.print("0");
        Serial.println(parsed_response.value, HEX);
    }

    return true;
}

/* ============================================================
 * STEP 3: Protocol Validation Summary
 * ============================================================ */
static bool step_summary(void) {
    Serial.println(GATE_LOG_TAG " STEP 3: Protocol validation summary");

    Serial.print(GATE_LOG_TAG "   Target Slave  : ");
    Serial.println(GATE_DISCOVERED_SLAVE);
    Serial.print(GATE_LOG_TAG "   Baud Rate     : ");
    Serial.println(GATE_DISCOVERED_BAUD);
    Serial.print(GATE_LOG_TAG "   Frame Config  : 8");
    Serial.print(frame_config_char(GATE_DISCOVERED_PARITY));
    Serial.println("1");
    Serial.print(GATE_LOG_TAG "   Function Code : 0x");
    if (GATE_MODBUS_FUNC_READ_HOLD < 0x10) Serial.print("0");
    Serial.print(GATE_MODBUS_FUNC_READ_HOLD, HEX);
    Serial.println(" (Read Holding Registers)");
    Serial.print(GATE_LOG_TAG "   Register      : 0x");
    if (GATE_MODBUS_TARGET_REG_HI < 0x10) Serial.print("0");
    Serial.print(GATE_MODBUS_TARGET_REG_HI, HEX);
    if (GATE_MODBUS_TARGET_REG_LO < 0x10) Serial.print("0");
    Serial.println(GATE_MODBUS_TARGET_REG_LO, HEX);
    Serial.println(GATE_LOG_TAG "   Protocol      : Modbus RTU");

    if (parsed_response.valid) {
        Serial.print(GATE_LOG_TAG "   Response Type : ");
        Serial.println(parsed_response.exception ? "Exception" : "Normal");

        if (parsed_response.exception) {
            Serial.print(GATE_LOG_TAG "   Exception Code: 0x");
            if (parsed_response.exc_code < 0x10) Serial.print("0");
            Serial.println(parsed_response.exc_code, HEX);
        } else {
            Serial.print(GATE_LOG_TAG "   byte_count    : ");
            Serial.println(parsed_response.byte_count);
            Serial.print(GATE_LOG_TAG "   Register Value: 0x");
            if (parsed_response.value < 0x1000) Serial.print("0");
            if (parsed_response.value < 0x0100) Serial.print("0");
            if (parsed_response.value < 0x0010) Serial.print("0");
            Serial.print(parsed_response.value, HEX);
            Serial.print(" (");
            Serial.print(parsed_response.value);
            Serial.println(")");
        }

        Serial.print(GATE_LOG_TAG "   Response Len  : ");
        Serial.print(parsed_response.raw_len);
        Serial.println(" bytes");
        Serial.print(GATE_LOG_TAG "   Response Hex  : ");
        print_hex(parsed_response.raw, parsed_response.raw_len);
        Serial.println();
    }

    return true;
}

/* ============================================================
 * Gate Entry Point
 * ============================================================ */
void gate_rak4631_modbus_minimal_protocol_run(void) {
    gate_result = GATE_PASS;
    memset(&parsed_response, 0, sizeof(parsed_response));

    Serial.println();
    Serial.print(GATE_LOG_TAG " === GATE START: ");
    Serial.print(GATE_NAME);
    Serial.print(" v");
    Serial.print(GATE_VERSION);
    Serial.println(" ===");

    if (!step_init())         goto done;
    if (!step_modbus_read())  goto done;
    if (!step_summary())      goto done;

done:
    /* Print PASS/FAIL */
    Serial.println();
    if (gate_result == GATE_PASS) {
        Serial.println(GATE_LOG_TAG " PASS");
    } else {
        Serial.print(GATE_LOG_TAG " FAIL (code=");
        Serial.print(gate_result);
        Serial.println(")");
    }

    /* Blink blue LED on PASS */
    if (gate_result == GATE_PASS) {
        pinMode(LED_BLUE, OUTPUT);
        digitalWrite(LED_BLUE, LED_STATE_ON);
        delay(500);
        digitalWrite(LED_BLUE, !LED_STATE_ON);
    }

    Serial.println(GATE_LOG_TAG " === GATE COMPLETE ===");
}
