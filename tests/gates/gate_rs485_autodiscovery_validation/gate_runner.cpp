/**
 * @file gate_runner.cpp
 * @brief Gate 2 — RS485 Modbus Autodiscovery Validation Runner (HAL-based)
 * @version 1.0
 * @date 2026-02-23
 *
 * Minimal Modbus RTU autodiscovery using hal_uart HAL.
 * Scans baud rates, parity modes, and slave addresses 1-100.
 * Sends Read Holding Register (0x03) probes. Validates CRC-16.
 * Stops on first valid response (normal or exception).
 *
 * Does NOT implement a full Modbus library.
 * Does NOT integrate into runtime/.
 * Aborts on first critical failure.
 */

#include <Arduino.h>
#include <string.h>
#include "gate_config.h"
#include "hal_uart.h"

/* ============================================================
 * State
 * ============================================================ */
static uint8_t gate_result = GATE_PASS;

/* Discovery result storage */
static bool     discovery_found       = false;
static uint8_t  discovered_addr       = 0;
static uint32_t discovered_baud       = 0;
static uint8_t  discovered_parity     = 0;
static bool     discovered_is_exception = false;
static uint8_t  discovered_exception_code = 0;
static uint8_t  discovered_response[GATE_MODBUS_RESP_MAX_LEN];
static uint8_t  discovered_resp_len   = 0;

/* ============================================================
 * CRC-16/MODBUS — Bitwise Calculation
 *
 * Polynomial: 0xA001 (reflected)
 * Init:       0xFFFF
 * Process:    LSB first
 * Result:     Low byte first when appended to frame
 * ============================================================ */
static uint16_t modbus_crc16(const uint8_t *data, uint8_t len) {
    uint16_t crc = GATE_CRC_INIT;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ GATE_CRC_POLY;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc;
}

/* ============================================================
 * Build Modbus Read Holding Register Request Frame
 *
 * Frame: [addr, 0x03, reg_hi, reg_lo, qty_hi, qty_lo, crc_lo, crc_hi]
 * Total: 8 bytes
 * ============================================================ */
static void build_request_frame(uint8_t slave_addr, uint8_t *frame) {
    frame[0] = slave_addr;
    frame[1] = GATE_MODBUS_FUNC_READ_HOLD;
    frame[2] = GATE_MODBUS_TARGET_REG_HI;
    frame[3] = GATE_MODBUS_TARGET_REG_LO;
    frame[4] = GATE_MODBUS_QUANTITY_HI;
    frame[5] = GATE_MODBUS_QUANTITY_LO;
    uint16_t crc = modbus_crc16(frame, 6);
    frame[6] = (uint8_t)(crc & 0xFF);         /* CRC low byte */
    frame[7] = (uint8_t)((crc >> 8) & 0xFF);  /* CRC high byte */
}

/* ============================================================
 * Validate Modbus Response
 *
 * Validation order (per pass criteria):
 *   1. Frame length >= 5 bytes (criterion #7)
 *   2. CRC-16 match
 *   3. Slave address match
 *   4. Exception detection (func | 0x80) — accepted as discovery
 *   5. Function code match for normal response
 * ============================================================ */
static bool validate_response(uint8_t expected_addr, const uint8_t *resp,
                               uint8_t resp_len, bool *is_exception,
                               uint8_t *exception_code) {
    *is_exception = false;
    *exception_code = 0;

    /* 1. Frame length check — reject short frames (criterion #7) */
    if (resp_len < GATE_MODBUS_RESP_MIN_LEN) {
        return false;
    }

    /* 2. CRC-16 verification */
    uint16_t calc_crc = modbus_crc16(resp, resp_len - 2);
    uint16_t recv_crc = (uint16_t)resp[resp_len - 2]
                      | ((uint16_t)resp[resp_len - 1] << 8);
    if (calc_crc != recv_crc) {
        return false;
    }

    /* 3. Slave address match */
    if (resp[0] != expected_addr) {
        return false;
    }

    /* 4. Exception detection — func code with bit 7 set */
    if (resp[1] & GATE_MODBUS_EXCEPTION_BIT) {
        *is_exception = true;
        if (resp_len >= 3) {
            *exception_code = resp[2];
        }
        return true;  /* Exception confirms device presence */
    }

    /* 5. Normal response — function code must match */
    if (resp[1] != GATE_MODBUS_FUNC_READ_HOLD) {
        return false;
    }

    return true;
}

/* ============================================================
 * Parity Name Helper
 * ============================================================ */
static const char* parity_name(uint8_t p) {
    switch (p) {
        case GATE_PARITY_NONE: return "NONE";
        case GATE_PARITY_ODD:  return "ODD";
        case GATE_PARITY_EVEN: return "EVEN";
        default:               return "?";
    }
}

/* ============================================================
 * Frame Config String Helper (e.g. "8N1", "8E1", "8O1")
 * ============================================================ */
static char frame_config_char(uint8_t p) {
    switch (p) {
        case GATE_PARITY_NONE: return 'N';
        case GATE_PARITY_ODD:  return 'O';
        case GATE_PARITY_EVEN: return 'E';
        default:               return '?';
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
 * STEP 1: UART Init
 * ============================================================ */
static bool step_uart_init(void) {
    Serial.printf("%s STEP 1: UART + RS485 init (TX=%d, RX=%d, EN=%d, DE=%d)\r\n",
                  GATE_LOG_TAG,
                  GATE_UART_TX_PIN, GATE_UART_RX_PIN,
                  GATE_RS485_EN_PIN, GATE_RS485_DE_PIN);

    /* Enable RAK5802 transceiver (power on + configure DE pin) */
    hal_rs485_enable(GATE_RS485_EN_PIN, GATE_RS485_DE_PIN);
    Serial.printf("%s RS485 EN=HIGH (GPIO%d), DE pin configured (GPIO%d)\r\n",
                  GATE_LOG_TAG, GATE_RS485_EN_PIN, GATE_RS485_DE_PIN);

    /* Test init at 9600/None to verify UART hardware works */
    if (!hal_uart_init(GATE_UART_TX_PIN, GATE_UART_RX_PIN,
                       9600, GATE_PARITY_NONE)) {
        Serial.printf("%s UART Init: FAIL\r\n", GATE_LOG_TAG);
        gate_result = GATE_FAIL_UART_INIT;
        return false;
    }
    hal_uart_deinit();  /* Will be reconfigured per scan combination */

    Serial.printf("%s UART Init: OK\r\n", GATE_LOG_TAG);
    return true;
}

/* ============================================================
 * STEP 2: RS485 Bus Scan — Autodiscovery
 * ============================================================ */
static bool step_bus_scan(void) {
    Serial.printf("%s STEP 2: RS485 autodiscovery scan\r\n", GATE_LOG_TAG);
    Serial.printf("%s Scan: %d bauds x %d parity x %d addrs = %d probes\r\n",
                  GATE_LOG_TAG,
                  GATE_BAUD_COUNT, GATE_PARITY_COUNT,
                  GATE_SLAVE_ADDR_MAX, GATE_SCAN_TOTAL_PROBES);

    uint8_t  req_frame[GATE_MODBUS_REQ_FRAME_LEN];
    uint8_t  resp_buf[GATE_MODBUS_RESP_MAX_LEN];
    uint32_t scan_start = millis();
    uint32_t probes_sent = 0;
    uint8_t  combo_num = 0;

    /* --- Outer loop: baud rates --- */
    for (uint8_t b = 0; b < GATE_BAUD_COUNT && !discovery_found; b++) {
        uint32_t baud = GATE_BAUD_RATES[b];

        /* --- Middle loop: parity modes --- */
        for (uint8_t p = 0; p < GATE_PARITY_COUNT && !discovery_found; p++) {
            uint8_t parity = GATE_PARITY_OPTIONS[p];
            combo_num++;

            Serial.printf("%s Config [%d/%d]: %lu 8%c%d\r\n",
                          GATE_LOG_TAG,
                          combo_num, GATE_SCAN_COMBINATIONS,
                          (unsigned long)baud,
                          frame_config_char(parity),
                          GATE_UART_STOP_BITS);

            /* Reconfigure UART for this combination */
            hal_uart_deinit();
            if (!hal_uart_init(GATE_UART_TX_PIN, GATE_UART_RX_PIN,
                               baud, parity)) {
                Serial.printf("%s UART reconfig FAIL at %lu/%s\r\n",
                              GATE_LOG_TAG,
                              (unsigned long)baud, parity_name(parity));
                gate_result = GATE_FAIL_BUS_ERROR;
                return false;
            }

            /* Flush and stabilize after config change */
            hal_uart_flush();
            delay(GATE_UART_STABILIZE_MS);

            /* --- Inner loop: slave addresses --- */
            for (uint8_t addr = GATE_SLAVE_ADDR_MIN;
                 addr <= GATE_SLAVE_ADDR_MAX && !discovery_found;
                 addr++) {

                /* Check overall gate timeout */
                if ((millis() - scan_start) > GATE_TOTAL_TIMEOUT_MS) {
                    Serial.printf("%s TIMEOUT: gate limit %lu ms exceeded\r\n",
                                  GATE_LOG_TAG,
                                  (unsigned long)GATE_TOTAL_TIMEOUT_MS);
                    gate_result = GATE_FAIL_TIMEOUT;
                    return false;
                }

                /* Build Modbus request frame */
                build_request_frame(addr, req_frame);

                /* Flush stale RX data before sending */
                hal_uart_flush();
                delay(GATE_UART_FLUSH_DELAY_MS);

                /* Log TX */
                Serial.printf("%s Config: %lu 8%c%d Slave %d | TX: ",
                              GATE_LOG_TAG,
                              (unsigned long)baud,
                              frame_config_char(parity),
                              GATE_UART_STOP_BITS,
                              addr);
                print_hex(req_frame, GATE_MODBUS_REQ_FRAME_LEN);
                Serial.printf("\r\n");

                /* Send request */
                if (!hal_uart_write(req_frame, GATE_MODBUS_REQ_FRAME_LEN)) {
                    Serial.printf("%s TX FAIL at addr=%d\r\n",
                                  GATE_LOG_TAG, addr);
                    gate_result = GATE_FAIL_BUS_ERROR;
                    return false;
                }

                /* Wait for response */
                uint8_t rx_len = hal_uart_read(resp_buf,
                                               GATE_MODBUS_RESP_MAX_LEN,
                                               GATE_RESPONSE_TIMEOUT_MS);
                probes_sent++;

                if (rx_len > 0) {
                    /* Log RX */
                    Serial.printf("%s RX (%d bytes): ", GATE_LOG_TAG, rx_len);
                    print_hex(resp_buf, rx_len);
                    Serial.printf("\r\n");

                    /* Frame length check (criterion #7) */
                    if (rx_len < GATE_MODBUS_RESP_MIN_LEN) {
                        Serial.printf("%s RX short frame (%d < %d), skipping\r\n",
                                      GATE_LOG_TAG, rx_len,
                                      GATE_MODBUS_RESP_MIN_LEN);
                        continue;
                    }

                    /* Validate response */
                    bool is_exception = false;
                    uint8_t exc_code = 0;
                    if (validate_response(addr, resp_buf, rx_len,
                                          &is_exception, &exc_code)) {
                        /* Valid discovery */
                        discovery_found         = true;
                        discovered_addr         = addr;
                        discovered_baud         = baud;
                        discovered_parity       = parity;
                        discovered_is_exception = is_exception;
                        discovered_exception_code = exc_code;
                        discovered_resp_len     = rx_len;
                        memcpy(discovered_response, resp_buf, rx_len);

                        if (is_exception) {
                            Serial.printf("%s FOUND: Slave=%d Baud=%lu "
                                          "Parity=%s (exception code=0x%02X)\r\n",
                                          GATE_LOG_TAG,
                                          addr, (unsigned long)baud,
                                          parity_name(parity), exc_code);
                        } else {
                            Serial.printf("%s FOUND: Slave=%d Baud=%lu "
                                          "Parity=%s\r\n",
                                          GATE_LOG_TAG,
                                          addr, (unsigned long)baud,
                                          parity_name(parity));
                        }
                    } else {
                        Serial.printf("%s RX invalid (CRC/addr mismatch)\r\n",
                                      GATE_LOG_TAG);
                    }
                }

                /* Progress reporting every 100 probes */
                if (!discovery_found && probes_sent % 100 == 0) {
                    uint32_t elapsed = millis() - scan_start;
                    Serial.printf("%s Progress: %lu/%d probes (%lu ms)\r\n",
                                  GATE_LOG_TAG,
                                  (unsigned long)probes_sent,
                                  GATE_SCAN_TOTAL_PROBES,
                                  (unsigned long)elapsed);
                }
            }
        }
    }

    uint32_t total_elapsed = millis() - scan_start;
    Serial.printf("%s Scan done: %lu probes in %lu ms\r\n",
                  GATE_LOG_TAG,
                  (unsigned long)probes_sent,
                  (unsigned long)total_elapsed);

    if (!discovery_found) {
        Serial.printf("%s No Modbus device discovered\r\n", GATE_LOG_TAG);
        gate_result = GATE_FAIL_NO_DEVICE;
        return false;
    }

    return true;
}

/* ============================================================
 * STEP 3: Discovery Summary Report
 * ============================================================ */
static bool step_report_summary(void) {
    Serial.printf("%s STEP 3: Discovery summary\r\n", GATE_LOG_TAG);

    if (!discovery_found) {
        Serial.printf("%s No device to report\r\n", GATE_LOG_TAG);
        return false;
    }

    Serial.printf("%s   Slave Address : %d\r\n",
                  GATE_LOG_TAG, discovered_addr);
    Serial.printf("%s   Baud Rate     : %lu\r\n",
                  GATE_LOG_TAG, (unsigned long)discovered_baud);
    Serial.printf("%s   Parity        : %s\r\n",
                  GATE_LOG_TAG, parity_name(discovered_parity));
    Serial.printf("%s   Frame Config  : 8%c%d\r\n",
                  GATE_LOG_TAG,
                  frame_config_char(discovered_parity),
                  GATE_UART_STOP_BITS);
    Serial.printf("%s   Response Type : %s\r\n",
                  GATE_LOG_TAG,
                  discovered_is_exception ? "Exception" : "Normal");

    if (discovered_is_exception) {
        Serial.printf("%s   Exception Code: 0x%02X\r\n",
                      GATE_LOG_TAG, discovered_exception_code);
    }

    Serial.printf("%s   Response Len  : %d bytes\r\n",
                  GATE_LOG_TAG, discovered_resp_len);
    Serial.printf("%s   Response Hex  : ", GATE_LOG_TAG);
    print_hex(discovered_response, discovered_resp_len);
    Serial.printf("\r\n");

    return true;
}

/* ============================================================
 * Gate Entry Point
 * ============================================================ */
void gate_rs485_autodiscovery_run(void) {
    gate_result     = GATE_PASS;
    discovery_found = false;

    Serial.printf("\r\n%s === GATE START: %s v%s ===\r\n",
                  GATE_LOG_TAG, GATE_NAME, GATE_VERSION);

    if (!step_uart_init())      goto done;
    if (!step_bus_scan())       goto done;
    if (!step_report_summary()) goto done;

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
