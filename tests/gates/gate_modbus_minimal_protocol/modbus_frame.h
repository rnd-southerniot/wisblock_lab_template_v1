/**
 * @file modbus_frame.h
 * @brief Minimal Modbus RTU frame layer — Build, CRC, Parse
 * @version 1.1
 * @date 2026-02-24
 *
 * Self-contained Modbus frame utilities for gate validation.
 * NOT a full Modbus library. Scoped to gate_modbus_minimal_protocol.
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

/**
 * @brief Structured Modbus response after validation and parsing.
 *
 * Populated by modbus_parse_response(). Check `valid` first,
 * then `exception` to determine response type.
 */
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

/**
 * @brief Compute CRC-16/MODBUS (bitwise, no lookup table).
 *
 * Polynomial: 0xA001 (reflected)
 * Init:       0xFFFF
 * Process:    LSB first
 * Result:     Low byte first when appended to frame
 *
 * @param data  Pointer to data bytes
 * @param len   Number of bytes
 * @return CRC-16 value
 */
uint16_t modbus_crc16(const uint8_t *data, uint8_t len);

/* ============================================================
 * Frame Builder
 * ============================================================ */

/**
 * @brief Build Modbus Read Holding Register (FC 0x03) request frame.
 *
 * Output frame: [addr, 0x03, reg_hi, reg_lo, qty_hi, qty_lo, crc_lo, crc_hi]
 * Total: 8 bytes.
 *
 * @param slave_addr  Target slave address (1-247)
 * @param reg_hi      Register address high byte
 * @param reg_lo      Register address low byte
 * @param qty_hi      Quantity high byte
 * @param qty_lo      Quantity low byte
 * @param frame       Output buffer (must be >= 8 bytes)
 * @return Frame length (always 8)
 */
uint8_t modbus_build_read_holding(uint8_t slave_addr,
                                   uint8_t reg_hi, uint8_t reg_lo,
                                   uint8_t qty_hi, uint8_t qty_lo,
                                   uint8_t *frame);

/* ============================================================
 * Response Parser
 * ============================================================ */

/**
 * @brief Parse and validate a Modbus RTU response frame.
 *
 * Validation order:
 *   1. Frame length >= 5 bytes (reject short frames)
 *   2. CRC-16 match
 *   3. Slave address match
 *   4. Exception detection (func | 0x80) — parsed, flagged, valid=true
 *   5. Function code match (normal responses)
 *   6. byte_count consistency: data[2] must equal (len - 5)
 *      i.e. declared byte_count == actual data bytes in frame
 *   7. Register value extraction
 *
 * For FC 0x03 normal response with 1 register:
 *   Frame = [addr, 0x03, byte_count(=2), val_hi, val_lo, crc_lo, crc_hi]
 *   byte_count must == (7 - 5) == 2  (consistent)
 *   value = (val_hi << 8) | val_lo
 *
 * For exception response:
 *   Frame = [addr, func|0x80, exc_code, crc_lo, crc_hi]
 *   byte_count check is skipped (not applicable to exceptions)
 *   exc_code stored in ModbusResponse.exc_code
 *
 * @param expected_addr  Expected slave address
 * @param expected_func  Expected function code (e.g. 0x03)
 * @param data           Raw response bytes
 * @param len            Response length
 * @return ModbusResponse with validation results
 */
ModbusResponse modbus_parse_response(uint8_t expected_addr,
                                      uint8_t expected_func,
                                      const uint8_t *data,
                                      uint8_t len);
