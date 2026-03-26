/**
 * @file gate_runner.cpp
 * @brief Gate 4 — RAK4631 Modbus Robustness Layer Validation Runner
 * @version 1.0
 * @date 2026-02-24
 *
 * Validates Modbus RTU robustness features:
 *   - Multi-register read (FC 0x03, qty configurable via gate_config.h)
 *   - Retry mechanism (max 3 retries, 50ms delay, retryable: TIMEOUT/CRC_FAIL)
 *   - Error classification enum per attempt
 *   - UART recovery (flush between retries, re-init after 2 consecutive failures)
 *
 * Reuses Gate 3's modbus_frame (CRC, frame builder, 7-step parser)
 * and hal_uart (Serial1 + RAK5802 RS485 control) without duplication.
 *
 * nRF52840 notes:
 *   - Serial1.begin(baud, config) — no pin args (variant.h)
 *   - Do NOT call Serial1.end() — hangs on nRF52840
 *   - RAK5802 auto DE/RE — no manual direction GPIO toggling
 *   - Serial.print() only — no Serial.printf() for consistency
 *
 * Entry point: gate_rak4631_modbus_robustness_layer_run()
 *
 * All device parameters come from gate_config.h GATE_DISCOVERED_* macros.
 * Zero hardcoded device values in this file.
 */

#include <Arduino.h>
#include <string.h>
#include "gate_config.h"
#include "../gate_rak4631_modbus_minimal_protocol/modbus_frame.h"
#include "../gate_rak4631_modbus_minimal_protocol/hal_uart.h"

/* ============================================================
 * Gate Result State
 * ============================================================ */
static uint8_t gate_result = GATE_FAIL_SYSTEM_CRASH;

/* ============================================================
 * Helper — Error name string
 * ============================================================ */
static const char* error_name(gate_error_t e) {
    switch (e) {
        case GATE_ERR_NONE:        return "NONE";
        case GATE_ERR_TIMEOUT:     return "TIMEOUT";
        case GATE_ERR_CRC_FAIL:    return "CRC_FAIL";
        case GATE_ERR_LENGTH_FAIL: return "LENGTH_FAIL";
        case GATE_ERR_EXCEPTION:   return "EXCEPTION";
        case GATE_ERR_UART_ERROR:  return "UART_ERROR";
        default:                   return "UNKNOWN";
    }
}

/* ============================================================
 * Helper — Parity name string
 * ============================================================ */
static const char* parity_name(uint8_t p) {
    switch (p) {
        case 1:  return "ODD";
        case 2:  return "EVEN";
        default: return "NONE";
    }
}

/* ============================================================
 * Helper — Frame config string (8N1 / 8O1 / 8E1)
 * ============================================================ */
static const char* frame_config_str(uint8_t p) {
    switch (p) {
        case 1:  return "8O1";
        case 2:  return "8E1";
        default: return "8N1";
    }
}

/* ============================================================
 * Helper — Print byte array as hex with spaces
 * ============================================================ */
static void print_hex(const uint8_t *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        if (i > 0) Serial.print(' ');
        if (buf[i] < 0x10) Serial.print('0');
        Serial.print(buf[i], HEX);
    }
}

/* ============================================================
 * Helper — Print 16-bit hex with 0x prefix and leading zeros
 * ============================================================ */
static void print_hex16(uint16_t val) {
    Serial.print("0x");
    if (val < 0x1000) Serial.print('0');
    if (val < 0x0100) Serial.print('0');
    if (val < 0x0010) Serial.print('0');
    Serial.print(val, HEX);
}

/* ============================================================
 * Helper — Print 8-bit hex with 0x prefix and leading zero
 * ============================================================ */
static void print_hex8(uint8_t val) {
    Serial.print("0x");
    if (val < 0x10) Serial.print('0');
    Serial.print(val, HEX);
}

/* ============================================================
 * Helper — Check if error is retryable
 *
 * Per requirement: retry ONLY on TIMEOUT and CRC_FAIL.
 * All other errors abort immediately.
 * ============================================================ */
static bool is_retryable(gate_error_t e) {
    return (e == GATE_ERR_TIMEOUT || e == GATE_ERR_CRC_FAIL);
}

/* ============================================================
 * Helper — Map last error to gate fail code
 * ============================================================ */
static uint8_t error_to_fail_code(gate_error_t e) {
    switch (e) {
        case GATE_ERR_TIMEOUT:     return GATE_FAIL_NO_RESPONSE;
        case GATE_ERR_CRC_FAIL:    return GATE_FAIL_CRC_ERROR;
        case GATE_ERR_LENGTH_FAIL: return GATE_FAIL_SHORT_FRAME;
        case GATE_ERR_EXCEPTION:   return GATE_FAIL_EXCEPTION;
        case GATE_ERR_UART_ERROR:  return GATE_FAIL_TX_ERROR;
        default:                   return GATE_FAIL_RETRIES_EXHAUSTED;
    }
}

/* ============================================================
 * STEP 1 — UART + RS485 Initialization
 *
 * Enable RAK5802 transceiver (auto DE/RE, power via 3V3_S),
 * init Serial1 with discovered parameters, flush and stabilize.
 * ============================================================ */
static bool step_init(void) {
    Serial.print(GATE_LOG_TAG " STEP 1: UART + RS485 init (Slave=");
    Serial.print(GATE_DISCOVERED_SLAVE);
    Serial.print(", Baud=");
    Serial.print((unsigned long)GATE_DISCOVERED_BAUD);
    Serial.print(", Parity=");
    Serial.print(parity_name(GATE_DISCOVERED_PARITY));
    Serial.println(")");

    /* Enable RAK5802 module (auto DE/RE — no DE pin arg) */
    hal_rs485_enable(GATE_RS485_EN_PIN);
    Serial.print(GATE_LOG_TAG "   RS485 EN=HIGH (pin ");
    Serial.print(GATE_RS485_EN_PIN);
    Serial.println(", WB_IO2 — 3V3_S)");
    Serial.println(GATE_LOG_TAG "   DE/RE: auto (RAK5802 hardware)");

    /* Init UART with discovered parameters (no pin args on nRF52840) */
    if (!hal_uart_init(GATE_DISCOVERED_BAUD, GATE_DISCOVERED_PARITY)) {
        Serial.println(GATE_LOG_TAG "   UART Init: FAIL");
        gate_result = GATE_FAIL_UART_INIT;
        return false;
    }

    Serial.print(GATE_LOG_TAG "   UART Init: OK (");
    Serial.print((unsigned long)GATE_DISCOVERED_BAUD);
    Serial.print(" ");
    Serial.print(frame_config_str(GATE_DISCOVERED_PARITY));
    Serial.println(")");
    Serial.print(GATE_LOG_TAG "   Serial1 TX=");
    Serial.print(PIN_SERIAL1_TX);
    Serial.print(" RX=");
    Serial.println(PIN_SERIAL1_RX);

    /* Flush and stabilize */
    hal_uart_flush();
    delay(GATE_UART_STABILIZE_MS);

    return true;
}

/* ============================================================
 * STEP 2 — Multi-Register Read with Retry Loop
 *
 * Sends FC 0x03 Read Holding Registers with qty from config.
 * Retries on TIMEOUT and CRC_FAIL (up to GATE_RETRY_MAX).
 * Aborts immediately on LENGTH_FAIL, EXCEPTION, UART_ERROR.
 * Triggers UART re-init after GATE_CONSEC_FAIL_REINIT
 * consecutive retryable failures.
 *
 * On success: populates reg_values[] and final_resp.
 * ============================================================ */
static uint8_t      attempt_count = 0;
static uint8_t      consecutive_fails = 0;
static uint8_t      uart_recovery_count = 0;
static uint8_t      timeout_count = 0;
static uint8_t      crc_fail_count = 0;
static gate_error_t last_error = GATE_ERR_NONE;
static bool         read_success = false;
static uint16_t     reg_values[GATE_MODBUS_MAX_REGISTERS];
static ModbusResponse final_resp;

static bool step_multi_register_read(void) {
    uint8_t tx_buf[GATE_MODBUS_REQ_FRAME_LEN];
    uint8_t rx_buf[GATE_MODBUS_RESP_MAX_LEN];

    Serial.print(GATE_LOG_TAG " STEP 2: Multi-register read (FC=");
    print_hex8(GATE_MODBUS_FUNC_READ_HOLD);
    Serial.print(", Reg=0x");
    if (GATE_MODBUS_TARGET_REG_HI < 0x10) Serial.print('0');
    Serial.print(GATE_MODBUS_TARGET_REG_HI, HEX);
    if (GATE_MODBUS_TARGET_REG_LO < 0x10) Serial.print('0');
    Serial.print(GATE_MODBUS_TARGET_REG_LO, HEX);
    Serial.print(", Qty=");
    Serial.print(GATE_MODBUS_QUANTITY);
    Serial.print(", MaxRetries=");
    Serial.print(GATE_RETRY_MAX);
    Serial.println(")");

    /* Build request frame (same for all attempts) */
    modbus_build_read_holding(GATE_DISCOVERED_SLAVE,
                              GATE_MODBUS_TARGET_REG_HI,
                              GATE_MODBUS_TARGET_REG_LO,
                              GATE_MODBUS_QUANTITY_HI,
                              GATE_MODBUS_QUANTITY_LO,
                              tx_buf);

    /* --- Reset retry state --- */
    uint8_t attempt = 0;
    attempt_count = 0;
    consecutive_fails = 0;
    uart_recovery_count = 0;
    timeout_count = 0;
    crc_fail_count = 0;
    last_error = GATE_ERR_NONE;
    read_success = false;
    memset(&final_resp, 0, sizeof(final_resp));
    memset(reg_values, 0, sizeof(reg_values));

    while (attempt <= GATE_RETRY_MAX) {

        /* ---- Pre-retry actions (attempt > 0) ---- */
        if (attempt > 0) {
            Serial.print(GATE_LOG_TAG " Retry ");
            Serial.print(attempt);
            Serial.print("/");
            Serial.print(GATE_RETRY_MAX);
            Serial.print(" (delay=");
            Serial.print(GATE_RETRY_DELAY_MS);
            Serial.print("ms, flush");

            delay(GATE_RETRY_DELAY_MS);
            hal_uart_flush();

            /* UART recovery: re-init after consecutive retryable failures.
             * On nRF52840: no Serial1.end() (hangs) — just call
             * hal_uart_init() again which re-calls Serial1.begin(). */
            if (consecutive_fails >= GATE_CONSEC_FAIL_REINIT) {
                Serial.print(", UART recovery: re-init after ");
                Serial.print(consecutive_fails);
                Serial.print(" consecutive failures");

                /* Skip hal_uart_deinit() — Serial1.end() hangs on nRF52840.
                 * Calling hal_uart_init() (which calls Serial1.begin()) will
                 * reinitialize the UARTE peripheral automatically. */
                delay(GATE_UART_STABILIZE_MS);

                if (!hal_uart_init(GATE_DISCOVERED_BAUD, GATE_DISCOVERED_PARITY)) {
                    Serial.println(")");
                    Serial.println(GATE_LOG_TAG "   UART re-init: FAIL");
                    gate_result = GATE_FAIL_UART_RECOVERY;
                    return false;
                }

                /* Reset retry state immediately after successful re-init */
                consecutive_fails = 0;
                uart_recovery_count++;

                Serial.println(")");
                Serial.print(GATE_LOG_TAG "   UART re-init: OK (");
                Serial.print((unsigned long)GATE_DISCOVERED_BAUD);
                Serial.print(" ");
                Serial.print(frame_config_str(GATE_DISCOVERED_PARITY));
                Serial.println(")");

                hal_uart_flush();
                delay(GATE_UART_STABILIZE_MS);
            } else {
                Serial.println(")");
            }
        }

        /* ---- Attempt header ---- */
        Serial.print(GATE_LOG_TAG " Attempt ");
        Serial.print(attempt + 1);
        Serial.print("/");
        Serial.print(GATE_RETRY_MAX + 1);
        Serial.println(":");

        /* ---- Flush stale RX data ---- */
        hal_uart_flush();
        delay(GATE_UART_FLUSH_DELAY_MS);

        /* ---- Log TX ---- */
        Serial.print(GATE_LOG_TAG "   TX (");
        Serial.print(GATE_MODBUS_REQ_FRAME_LEN);
        Serial.print(" bytes): ");
        print_hex(tx_buf, GATE_MODBUS_REQ_FRAME_LEN);
        Serial.println();

        /* ---- Send request ---- */
        if (!hal_uart_write(tx_buf, GATE_MODBUS_REQ_FRAME_LEN)) {
            Serial.println(GATE_LOG_TAG "   TX: FAILED");
            last_error = GATE_ERR_UART_ERROR;
            Serial.print(GATE_LOG_TAG "   Error: ");
            Serial.println(error_name(last_error));
            /* UART_ERROR is non-retryable */
            gate_result = GATE_FAIL_TX_ERROR;
            return false;
        }

        /* ---- Read response ---- */
        uint8_t rx_len = hal_uart_read(rx_buf, GATE_MODBUS_RESP_MAX_LEN,
                                       GATE_RESPONSE_TIMEOUT_MS);

        /* ---- Classify response ---- */

        /* Case 1: No response (timeout) — retryable */
        if (rx_len == 0) {
            Serial.print(GATE_LOG_TAG "   RX: timeout (");
            Serial.print(GATE_RESPONSE_TIMEOUT_MS);
            Serial.println("ms)");
            last_error = GATE_ERR_TIMEOUT;
            Serial.print(GATE_LOG_TAG "   Error: ");
            Serial.println(error_name(last_error));
            timeout_count++;
            consecutive_fails++;
            attempt++;
            continue;
        }

        /* Log RX */
        Serial.print(GATE_LOG_TAG "   RX (");
        Serial.print(rx_len);
        Serial.print(" bytes): ");
        print_hex(rx_buf, rx_len);
        Serial.println();

        /* Case 2: Short frame (< 5 bytes) — non-retryable */
        if (rx_len < GATE_MODBUS_RESP_MIN_LEN) {
            last_error = GATE_ERR_LENGTH_FAIL;
            Serial.print(GATE_LOG_TAG "   Parse: FAILED (short frame: ");
            Serial.print(rx_len);
            Serial.print(" < ");
            Serial.print(GATE_MODBUS_RESP_MIN_LEN);
            Serial.println(")");
            Serial.print(GATE_LOG_TAG "   Error: ");
            Serial.println(error_name(last_error));
            gate_result = GATE_FAIL_SHORT_FRAME;
            return false;
        }

        /* Case 3: CRC check before full parse — for error classification.
         * Re-compute CRC to distinguish CRC_FAIL from other parse failures. */
        uint16_t calc_crc = modbus_crc16(rx_buf, rx_len - 2);
        uint16_t recv_crc = (uint16_t)rx_buf[rx_len - 2]
                          | ((uint16_t)rx_buf[rx_len - 1] << 8);

        if (calc_crc != recv_crc) {
            last_error = GATE_ERR_CRC_FAIL;
            Serial.print(GATE_LOG_TAG "   Parse: FAILED (CRC mismatch: calc=");
            print_hex16(calc_crc);
            Serial.print(", recv=");
            print_hex16(recv_crc);
            Serial.println(")");
            Serial.print(GATE_LOG_TAG "   Error: ");
            Serial.println(error_name(last_error));
            crc_fail_count++;
            consecutive_fails++;
            attempt++;
            continue;   /* CRC_FAIL is retryable */
        }

        /* Case 4: Full parse via 7-step parser */
        ModbusResponse resp = modbus_parse_response(
            GATE_DISCOVERED_SLAVE,
            GATE_MODBUS_FUNC_READ_HOLD,
            rx_buf, rx_len);

        /* Case 4a: Parse failed (CRC passed, so this is addr/func/byte_count issue).
         *          Determine specific cause for accurate fail code. */
        if (!resp.valid) {
            last_error = GATE_ERR_LENGTH_FAIL;
            if (resp.slave != GATE_DISCOVERED_SLAVE) {
                /* Parser step 3 failed: slave address mismatch */
                Serial.print(GATE_LOG_TAG "   Parse: FAILED (slave mismatch: got=");
                Serial.print(resp.slave);
                Serial.print(", expected=");
                Serial.print(GATE_DISCOVERED_SLAVE);
                Serial.println(")");
                gate_result = GATE_FAIL_ADDR_MISMATCH;
            } else if (resp.function != GATE_MODBUS_FUNC_READ_HOLD) {
                /* Parser step 5 failed: unexpected function code */
                Serial.print(GATE_LOG_TAG "   Parse: FAILED (func mismatch: got=");
                print_hex8(resp.function);
                Serial.print(", expected=");
                print_hex8(GATE_MODBUS_FUNC_READ_HOLD);
                Serial.println(")");
                gate_result = GATE_FAIL_SHORT_FRAME;
            } else {
                /* Parser step 6 failed: byte_count != (frame_len - 5) */
                Serial.print(GATE_LOG_TAG "   Parse: FAILED (byte_count inconsistency: declared=");
                Serial.print(resp.byte_count);
                Serial.print(", frame_data=");
                Serial.print(rx_len - 5);
                Serial.println(")");
                gate_result = GATE_FAIL_BYTE_COUNT;
            }
            Serial.print(GATE_LOG_TAG "   Error: ");
            Serial.println(error_name(last_error));
            return false;   /* Non-retryable */
        }

        /* Case 4b: Exception response — non-retryable */
        if (resp.exception) {
            last_error = GATE_ERR_EXCEPTION;
            Serial.print(GATE_LOG_TAG "   Parse: Exception (code=");
            print_hex8(resp.exc_code);
            Serial.println(")");
            Serial.print(GATE_LOG_TAG "   Error: ");
            Serial.println(error_name(last_error));
            gate_result = GATE_FAIL_EXCEPTION;
            return false;
        }

        /* Case 4c: Valid normal response — check byte_count == 2 * quantity */
        uint8_t expected_byte_count = 2 * GATE_MODBUS_QUANTITY;
        if (resp.byte_count != expected_byte_count) {
            last_error = GATE_ERR_LENGTH_FAIL;
            Serial.print(GATE_LOG_TAG "   Parse: FAILED (byte_count=");
            Serial.print(resp.byte_count);
            Serial.print(", expected=");
            Serial.print(expected_byte_count);
            Serial.print(" for qty=");
            Serial.print(GATE_MODBUS_QUANTITY);
            Serial.println(")");
            Serial.print(GATE_LOG_TAG "   Error: ");
            Serial.println(error_name(last_error));
            gate_result = GATE_FAIL_BYTE_COUNT;
            return false;   /* Non-retryable */
        }

        /* ---- SUCCESS ---- */
        last_error = GATE_ERR_NONE;
        Serial.print(GATE_LOG_TAG "   Parse: OK (normal, byte_count=");
        Serial.print(resp.byte_count);
        Serial.print(", expected=");
        Serial.print(expected_byte_count);
        Serial.println(")");
        Serial.print(GATE_LOG_TAG "   Error: ");
        Serial.println(error_name(last_error));

        /* Extract all register values from raw response */
        for (uint8_t i = 0; i < GATE_MODBUS_QUANTITY; i++) {
            reg_values[i] = ((uint16_t)resp.raw[3 + i * 2] << 8)
                          | (uint16_t)resp.raw[4 + i * 2];
        }

        final_resp = resp;
        read_success = true;
        consecutive_fails = 0;
        attempt_count = attempt + 1;
        break;

    } /* end while */

    /* ---- Post-loop result ---- */
    if (read_success) {
        Serial.print(GATE_LOG_TAG " Result: SUCCESS on attempt ");
        Serial.print(attempt_count);
        Serial.print(" (");
        Serial.print(attempt_count - 1);
        Serial.print(" retries, ");
        Serial.print(uart_recovery_count);
        Serial.println(" UART recoveries)");
        return true;
    }

    /* All retries exhausted */
    attempt_count = GATE_RETRY_MAX + 1;
    Serial.print(GATE_LOG_TAG " Result: FAILED after ");
    Serial.print(attempt_count);
    Serial.print(" attempts (last error: ");
    Serial.print(error_name(last_error));
    Serial.println(")");

    /* Map retries-exhausted to specific fail code:
     *   Code 3 = all attempts were timeouts
     *   Code 4 = all attempts were CRC failures
     *   Code 9 = mixed retryable errors across attempts */
    if (gate_result == GATE_FAIL_SYSTEM_CRASH) {
        uint8_t total_retryable = timeout_count + crc_fail_count;
        if (total_retryable > 0 && timeout_count == total_retryable) {
            gate_result = GATE_FAIL_NO_RESPONSE;
        } else if (total_retryable > 0 && crc_fail_count == total_retryable) {
            gate_result = GATE_FAIL_CRC_ERROR;
        } else {
            gate_result = GATE_FAIL_RETRIES_EXHAUSTED;
        }
    }
    return false;
}

/* ============================================================
 * STEP 3 — Robustness Validation Summary
 *
 * Prints structured summary of all parameters, retry stats,
 * and extracted register values.
 * ============================================================ */
static void step_summary(void) {
    Serial.println(GATE_LOG_TAG " STEP 3: Robustness validation summary");

    Serial.print(GATE_LOG_TAG "   Target Slave   : ");
    Serial.println(GATE_DISCOVERED_SLAVE);

    Serial.print(GATE_LOG_TAG "   Baud Rate      : ");
    Serial.println((unsigned long)GATE_DISCOVERED_BAUD);

    Serial.print(GATE_LOG_TAG "   Frame Config   : ");
    Serial.println(frame_config_str(GATE_DISCOVERED_PARITY));

    Serial.print(GATE_LOG_TAG "   Function Code  : ");
    print_hex8(GATE_MODBUS_FUNC_READ_HOLD);
    Serial.println(" (Read Holding Registers)");

    Serial.print(GATE_LOG_TAG "   Registers      : 0x");
    if (GATE_MODBUS_TARGET_REG_HI < 0x10) Serial.print('0');
    Serial.print(GATE_MODBUS_TARGET_REG_HI, HEX);
    if (GATE_MODBUS_TARGET_REG_LO < 0x10) Serial.print('0');
    Serial.print(GATE_MODBUS_TARGET_REG_LO, HEX);
    Serial.print(", Qty=");
    Serial.println(GATE_MODBUS_QUANTITY);

    Serial.print(GATE_LOG_TAG "   Attempts       : ");
    Serial.println(attempt_count);

    Serial.print(GATE_LOG_TAG "   Retries Used   : ");
    Serial.println((attempt_count > 0) ? attempt_count - 1 : 0);

    Serial.print(GATE_LOG_TAG "   Last Error     : ");
    Serial.println(error_name(last_error));

    Serial.print(GATE_LOG_TAG "   UART Recoveries: ");
    Serial.println(uart_recovery_count);

    /* byte_count */
    uint8_t expected_bc = 2 * GATE_MODBUS_QUANTITY;
    Serial.print(GATE_LOG_TAG "   byte_count     : ");
    Serial.print(final_resp.byte_count);
    Serial.print(" (expected: ");
    Serial.print(expected_bc);
    Serial.println(")");

    /* Register values */
    for (uint8_t i = 0; i < GATE_MODBUS_QUANTITY; i++) {
        Serial.print(GATE_LOG_TAG "   Register[");
        Serial.print(i);
        Serial.print("]    : ");
        print_hex16(reg_values[i]);
        Serial.print(" (");
        Serial.print(reg_values[i]);
        Serial.println(")");
    }

    /* Response frame */
    Serial.print(GATE_LOG_TAG "   Response Len   : ");
    Serial.print(final_resp.raw_len);
    Serial.println(" bytes");

    Serial.print(GATE_LOG_TAG "   Response Hex   : ");
    print_hex(final_resp.raw, final_resp.raw_len);
    Serial.println();
}

/* ============================================================
 * Gate Entry Point
 * ============================================================ */
void gate_rak4631_modbus_robustness_layer_run(void) {
    gate_result = GATE_FAIL_SYSTEM_CRASH;

    /* ---- GATE START banner ---- */
    Serial.println();
    Serial.print(GATE_LOG_TAG " === GATE START: ");
    Serial.print(GATE_NAME);
    Serial.print(" v");
    Serial.print(GATE_VERSION);
    Serial.println(" ===");

    /* ---- Step 1: UART + RS485 init ---- */
    if (!step_init()) {
        goto done;
    }

    /* ---- Step 2: Multi-register read with retries ---- */
    if (!step_multi_register_read()) {
        goto done;
    }

    /* ---- Step 3: Summary (only on success) ---- */
    step_summary();

    /* ---- All steps passed ---- */
    gate_result = GATE_PASS;

done:
    /* No cleanup needed on nRF52840:
     *   - Serial1.end() hangs — do NOT call it
     *   - RAK5802 power (WB_IO2) left on — harmless for gate test
     *   - Auto DE/RE — no GPIO state to restore */

    /* ---- Result ---- */
    Serial.println();
    if (gate_result == GATE_PASS) {
        Serial.println(GATE_LOG_TAG " PASS");

        /* Blink blue LED on PASS */
        pinMode(LED_BLUE, OUTPUT);
        digitalWrite(LED_BLUE, LED_STATE_ON);
        delay(500);
        digitalWrite(LED_BLUE, !LED_STATE_ON);
    } else {
        Serial.print(GATE_LOG_TAG " FAIL (code=");
        Serial.print(gate_result);
        Serial.println(")");
    }

    Serial.println(GATE_LOG_TAG " === GATE COMPLETE ===");
}
