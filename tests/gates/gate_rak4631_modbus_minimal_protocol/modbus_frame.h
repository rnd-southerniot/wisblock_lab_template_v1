/**
 * @file modbus_frame.h
 * @brief Minimal Modbus RTU frame layer — Build, CRC, Parse
 * @version 1.0
 * @date 2026-02-24
 *
 * Self-contained Modbus frame utilities for gate validation.
 * NOT a full Modbus library. Scoped to gate_rak4631_modbus_minimal_protocol.
 *
 * Provides:
 *   - CRC-16/MODBUS (bitwise, no lookup table)
 *   - Frame builder for FC 0x03 (Read Holding Registers)
 *   - Response parser with normal + exception handling
 *   - byte_count consistency validation
 *   - ModbusResponse structured result
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 * Structured Modbus Response Result
 * ============================================================ */
struct ModbusResponse {
    bool     valid;        /**< true if response passed all validation */
    bool     exception;    /**< true if response is a Modbus exception */
    uint8_t  slave;        /**< Slave address from response */
    uint8_t  function;     /**< Function code from response (raw, incl. 0x80 bit) */
    uint8_t  exc_code;     /**< Exception code (only valid if exception==true) */
    uint8_t  byte_count;   /**< byte_count field from normal response (data[2]) */
    uint16_t value;        /**< Register value (only valid if normal FC 0x03 response) */
    uint8_t  raw_len;      /**< Raw response frame length */
    uint8_t  raw[32];      /**< Raw response bytes (copy) */
};

/* ============================================================
 * CRC-16/MODBUS
 * ============================================================ */
uint16_t modbus_crc16(const uint8_t *data, uint8_t len);

/* ============================================================
 * Frame Builder
 * ============================================================ */
uint8_t modbus_build_read_holding(uint8_t slave_addr,
                                   uint8_t reg_hi, uint8_t reg_lo,
                                   uint8_t qty_hi, uint8_t qty_lo,
                                   uint8_t *frame);

/* ============================================================
 * Response Parser — 7-step validation
 *
 *   1. Frame length >= 5 bytes
 *   2. CRC-16 match
 *   3. Slave address match
 *   4. Exception detection (func | 0x80)
 *   5. Function code match
 *   6. byte_count consistency: data[2] == (len - 5)
 *   7. Register value extraction
 * ============================================================ */
ModbusResponse modbus_parse_response(uint8_t expected_addr,
                                      uint8_t expected_func,
                                      const uint8_t *data,
                                      uint8_t len);
