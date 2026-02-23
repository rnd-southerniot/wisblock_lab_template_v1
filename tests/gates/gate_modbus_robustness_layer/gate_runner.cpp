/**
 * @file gate_runner.cpp
 * @brief Gate 4 — Modbus Robustness Layer Validation Runner
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
 * Entry point: gate_modbus_robustness_layer_run()
 *
 * All device parameters come from gate_config.h GATE_DISCOVERED_* macros.
 * Zero hardcoded device values in this file.
 */

#include <Arduino.h>
#include "gate_config.h"
#include "../gate_modbus_minimal_protocol/modbus_frame.h"
#include "../gate_modbus_minimal_protocol/hal_uart.h"

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
 * Enable RAK5802 transceiver, init Serial1 with discovered
 * parameters, flush and stabilize.
 * ============================================================ */
static bool step_init(void) {
    Serial.printf("%s STEP 1: UART + RS485 init (Slave=%d, Baud=%lu, Parity=%s)\r\n",
                  GATE_LOG_TAG,
                  GATE_DISCOVERED_SLAVE,
                  (unsigned long)GATE_DISCOVERED_BAUD,
                  parity_name(GATE_DISCOVERED_PARITY));

    /* Enable RAK5802 module */
    hal_rs485_enable(GATE_RS485_EN_PIN, GATE_RS485_DE_PIN);
    Serial.printf("%s RS485 EN=HIGH (GPIO%d), DE configured (GPIO%d)\r\n",
                  GATE_LOG_TAG, GATE_RS485_EN_PIN, GATE_RS485_DE_PIN);

    /* Init UART with discovered parameters */
    if (!hal_uart_init(GATE_UART_TX_PIN, GATE_UART_RX_PIN,
                       GATE_DISCOVERED_BAUD, GATE_DISCOVERED_PARITY)) {
        Serial.printf("%s UART Init: FAIL\r\n", GATE_LOG_TAG);
        gate_result = GATE_FAIL_UART_INIT;
        return false;
    }

    Serial.printf("%s UART Init: OK (%lu %s)\r\n",
                  GATE_LOG_TAG,
                  (unsigned long)GATE_DISCOVERED_BAUD,
                  frame_config_str(GATE_DISCOVERED_PARITY));

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

    Serial.printf("%s STEP 2: Multi-register read (FC=0x03, Reg=0x%02X%02X, Qty=%d, MaxRetries=%d)\r\n",
                  GATE_LOG_TAG,
                  GATE_MODBUS_TARGET_REG_HI, GATE_MODBUS_TARGET_REG_LO,
                  GATE_MODBUS_QUANTITY,
                  GATE_RETRY_MAX);

    /* Build request frame (same for all attempts) */
    modbus_build_read_holding(GATE_DISCOVERED_SLAVE,
                              GATE_MODBUS_TARGET_REG_HI,
                              GATE_MODBUS_TARGET_REG_LO,
                              GATE_MODBUS_QUANTITY_HI,
                              GATE_MODBUS_QUANTITY_LO,
                              tx_buf);

    /* --- Retry loop --- */
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
            Serial.printf("%s Retry %d/%d (delay=%dms, flush",
                          GATE_LOG_TAG, attempt, GATE_RETRY_MAX,
                          GATE_RETRY_DELAY_MS);

            delay(GATE_RETRY_DELAY_MS);
            hal_uart_flush();

            /* UART recovery: re-init after consecutive retryable failures */
            if (consecutive_fails >= GATE_CONSEC_FAIL_REINIT) {
                Serial.printf(", UART recovery: re-init after %d consecutive failures",
                              consecutive_fails);

                hal_uart_deinit();
                delay(GATE_UART_STABILIZE_MS);

                if (!hal_uart_init(GATE_UART_TX_PIN, GATE_UART_RX_PIN,
                                   GATE_DISCOVERED_BAUD, GATE_DISCOVERED_PARITY)) {
                    Serial.printf(")\r\n");
                    Serial.printf("%s UART re-init: FAIL\r\n", GATE_LOG_TAG);
                    gate_result = GATE_FAIL_UART_RECOVERY;
                    return false;
                }

                /* Reset retry state immediately after successful re-init */
                consecutive_fails = 0;
                uart_recovery_count++;

                Serial.printf(")\r\n");
                Serial.printf("%s UART re-init: OK (%lu %s)\r\n",
                              GATE_LOG_TAG,
                              (unsigned long)GATE_DISCOVERED_BAUD,
                              frame_config_str(GATE_DISCOVERED_PARITY));

                hal_uart_flush();
                delay(GATE_UART_STABILIZE_MS);
            } else {
                Serial.printf(")\r\n");
            }
        }

        /* ---- Attempt header ---- */
        Serial.printf("%s Attempt %d/%d:\r\n",
                      GATE_LOG_TAG, attempt + 1, GATE_RETRY_MAX + 1);

        /* ---- Flush stale RX data ---- */
        hal_uart_flush();
        delay(GATE_UART_FLUSH_DELAY_MS);

        /* ---- Log TX ---- */
        Serial.printf("%s   TX (%d bytes): ", GATE_LOG_TAG, GATE_MODBUS_REQ_FRAME_LEN);
        print_hex(tx_buf, GATE_MODBUS_REQ_FRAME_LEN);
        Serial.printf("\r\n");

        /* ---- Send request ---- */
        if (!hal_uart_write(tx_buf, GATE_MODBUS_REQ_FRAME_LEN)) {
            Serial.printf("%s   TX: FAILED\r\n", GATE_LOG_TAG);
            last_error = GATE_ERR_UART_ERROR;
            Serial.printf("%s   Error: %s\r\n", GATE_LOG_TAG, error_name(last_error));
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
            Serial.printf("%s   RX: timeout (%dms)\r\n",
                          GATE_LOG_TAG, GATE_RESPONSE_TIMEOUT_MS);
            last_error = GATE_ERR_TIMEOUT;
            Serial.printf("%s   Error: %s\r\n", GATE_LOG_TAG, error_name(last_error));
            timeout_count++;
            consecutive_fails++;
            attempt++;
            continue;
        }

        /* Log RX */
        Serial.printf("%s   RX (%d bytes): ", GATE_LOG_TAG, rx_len);
        print_hex(rx_buf, rx_len);
        Serial.printf("\r\n");

        /* Case 2: Short frame (< 5 bytes) — non-retryable */
        if (rx_len < GATE_MODBUS_RESP_MIN_LEN) {
            last_error = GATE_ERR_LENGTH_FAIL;
            Serial.printf("%s   Parse: FAILED (short frame: %d < %d)\r\n",
                          GATE_LOG_TAG, rx_len, GATE_MODBUS_RESP_MIN_LEN);
            Serial.printf("%s   Error: %s\r\n", GATE_LOG_TAG, error_name(last_error));
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
            Serial.printf("%s   Parse: FAILED (CRC mismatch: calc=0x%04X, recv=0x%04X)\r\n",
                          GATE_LOG_TAG, calc_crc, recv_crc);
            Serial.printf("%s   Error: %s\r\n", GATE_LOG_TAG, error_name(last_error));
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
                Serial.printf("%s   Parse: FAILED (slave mismatch: got=%d, expected=%d)\r\n",
                              GATE_LOG_TAG, resp.slave, GATE_DISCOVERED_SLAVE);
                gate_result = GATE_FAIL_ADDR_MISMATCH;
            } else if (resp.function != GATE_MODBUS_FUNC_READ_HOLD) {
                /* Parser step 5 failed: unexpected function code */
                Serial.printf("%s   Parse: FAILED (func mismatch: got=0x%02X, expected=0x%02X)\r\n",
                              GATE_LOG_TAG, resp.function, GATE_MODBUS_FUNC_READ_HOLD);
                gate_result = GATE_FAIL_SHORT_FRAME;
            } else {
                /* Parser step 6 failed: byte_count != (frame_len - 5) */
                Serial.printf("%s   Parse: FAILED (byte_count inconsistency: declared=%d, frame_data=%d)\r\n",
                              GATE_LOG_TAG, resp.byte_count, rx_len - 5);
                gate_result = GATE_FAIL_BYTE_COUNT;
            }
            Serial.printf("%s   Error: %s\r\n", GATE_LOG_TAG, error_name(last_error));
            return false;   /* Non-retryable */
        }

        /* Case 4b: Exception response — non-retryable */
        if (resp.exception) {
            last_error = GATE_ERR_EXCEPTION;
            Serial.printf("%s   Parse: Exception (code=0x%02X)\r\n",
                          GATE_LOG_TAG, resp.exc_code);
            Serial.printf("%s   Error: %s\r\n", GATE_LOG_TAG, error_name(last_error));
            gate_result = GATE_FAIL_EXCEPTION;
            return false;
        }

        /* Case 4c: Valid normal response — check byte_count == 2 * quantity */
        uint8_t expected_byte_count = 2 * GATE_MODBUS_QUANTITY;
        if (resp.byte_count != expected_byte_count) {
            last_error = GATE_ERR_LENGTH_FAIL;
            Serial.printf("%s   Parse: FAILED (byte_count=%d, expected=%d for qty=%d)\r\n",
                          GATE_LOG_TAG, resp.byte_count, expected_byte_count,
                          GATE_MODBUS_QUANTITY);
            Serial.printf("%s   Error: %s\r\n", GATE_LOG_TAG, error_name(last_error));
            gate_result = GATE_FAIL_BYTE_COUNT;
            return false;   /* Non-retryable */
        }

        /* ---- SUCCESS ---- */
        last_error = GATE_ERR_NONE;
        Serial.printf("%s   Parse: OK (normal, byte_count=%d, expected=%d)\r\n",
                      GATE_LOG_TAG, resp.byte_count, expected_byte_count);
        Serial.printf("%s   Error: %s\r\n", GATE_LOG_TAG, error_name(last_error));

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
        Serial.printf("%s Result: SUCCESS on attempt %d (%d retries, %d UART recoveries)\r\n",
                      GATE_LOG_TAG, attempt_count,
                      attempt_count - 1, uart_recovery_count);
        return true;
    }

    /* All retries exhausted */
    attempt_count = GATE_RETRY_MAX + 1;
    Serial.printf("%s Result: FAILED after %d attempts (last error: %s)\r\n",
                  GATE_LOG_TAG, attempt_count, error_name(last_error));

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
    Serial.printf("%s STEP 3: Robustness validation summary\r\n", GATE_LOG_TAG);
    Serial.printf("%s   Target Slave   : %d\r\n", GATE_LOG_TAG, GATE_DISCOVERED_SLAVE);
    Serial.printf("%s   Baud Rate      : %lu\r\n", GATE_LOG_TAG, (unsigned long)GATE_DISCOVERED_BAUD);
    Serial.printf("%s   Frame Config   : %s\r\n", GATE_LOG_TAG,
                  frame_config_str(GATE_DISCOVERED_PARITY));
    Serial.printf("%s   Function Code  : 0x%02X (Read Holding Registers)\r\n",
                  GATE_LOG_TAG, GATE_MODBUS_FUNC_READ_HOLD);
    Serial.printf("%s   Registers      : 0x%02X%02X, Qty=%d\r\n",
                  GATE_LOG_TAG,
                  GATE_MODBUS_TARGET_REG_HI, GATE_MODBUS_TARGET_REG_LO,
                  GATE_MODBUS_QUANTITY);
    Serial.printf("%s   Attempts       : %d\r\n", GATE_LOG_TAG, attempt_count);
    Serial.printf("%s   Retries Used   : %d\r\n", GATE_LOG_TAG,
                  (attempt_count > 0) ? attempt_count - 1 : 0);
    Serial.printf("%s   Last Error     : %s\r\n", GATE_LOG_TAG, error_name(last_error));
    Serial.printf("%s   UART Recoveries: %d\r\n", GATE_LOG_TAG, uart_recovery_count);

    /* byte_count */
    uint8_t expected_bc = 2 * GATE_MODBUS_QUANTITY;
    Serial.printf("%s   byte_count     : %d (expected: %d)\r\n",
                  GATE_LOG_TAG, final_resp.byte_count, expected_bc);

    /* Register values */
    for (uint8_t i = 0; i < GATE_MODBUS_QUANTITY; i++) {
        Serial.printf("%s   Register[%d]    : 0x%04X (%d)\r\n",
                      GATE_LOG_TAG, i, reg_values[i], reg_values[i]);
    }

    /* Response frame */
    Serial.printf("%s   Response Len   : %d bytes\r\n", GATE_LOG_TAG, final_resp.raw_len);
    Serial.printf("%s   Response Hex   : ", GATE_LOG_TAG);
    print_hex(final_resp.raw, final_resp.raw_len);
    Serial.printf("\r\n");
}

/* ============================================================
 * Gate Entry Point
 * ============================================================ */
void gate_modbus_robustness_layer_run(void) {
    gate_result = GATE_FAIL_SYSTEM_CRASH;

    /* ---- GATE START banner ---- */
    Serial.printf("\r\n%s === GATE START: %s v%s ===\r\n",
                  GATE_LOG_TAG, GATE_NAME, GATE_VERSION);

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
    /* ---- Cleanup ---- */
    hal_uart_deinit();
    hal_rs485_disable();

    /* ---- Result ---- */
    if (gate_result == GATE_PASS) {
        Serial.printf("%s PASS\r\n", GATE_LOG_TAG);
    } else {
        Serial.printf("%s FAIL (code=%d)\r\n", GATE_LOG_TAG, gate_result);
    }

    Serial.printf("%s === GATE COMPLETE ===\r\n", GATE_LOG_TAG);
}
