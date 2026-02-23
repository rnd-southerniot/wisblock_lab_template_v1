/**
 * @file modbus_frame.cpp
 * @brief Minimal Modbus RTU frame layer — Implementation
 * @version 1.1
 * @date 2026-02-24
 *
 * Bitwise CRC-16/MODBUS, frame builder (FC 0x03), and response parser.
 * Handles both normal responses and Modbus exception responses.
 * v1.1: Added byte_count consistency validation for normal responses.
 *
 * Scoped to gate_modbus_minimal_protocol.
 * NOT a full Modbus library.
 */

#include <string.h>
#include "modbus_frame.h"
#include "gate_config.h"

/* ============================================================
 * CRC-16/MODBUS — Bitwise Calculation
 *
 * Polynomial: 0xA001 (reflected)
 * Init:       0xFFFF
 * Process:    LSB first
 * Result:     Low byte first when appended to frame
 * ============================================================ */
uint16_t modbus_crc16(const uint8_t *data, uint8_t len) {
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
uint8_t modbus_build_read_holding(uint8_t slave_addr,
                                   uint8_t reg_hi, uint8_t reg_lo,
                                   uint8_t qty_hi, uint8_t qty_lo,
                                   uint8_t *frame) {
    frame[0] = slave_addr;
    frame[1] = GATE_MODBUS_FUNC_READ_HOLD;
    frame[2] = reg_hi;
    frame[3] = reg_lo;
    frame[4] = qty_hi;
    frame[5] = qty_lo;
    uint16_t crc = modbus_crc16(frame, 6);
    frame[6] = (uint8_t)(crc & 0xFF);         /* CRC low byte */
    frame[7] = (uint8_t)((crc >> 8) & 0xFF);  /* CRC high byte */
    return GATE_MODBUS_REQ_FRAME_LEN;
}

/* ============================================================
 * Parse and Validate Modbus RTU Response
 *
 * Validation order (per pass criteria):
 *   1. Frame length >= 5 bytes
 *   2. CRC-16 match
 *   3. Slave address match
 *   4. Exception detection (func | 0x80) — accepted, flagged
 *   5. Normal response — function code match
 *   6. byte_count consistency: data[2] == (len - 5)
 *   7. Register value extraction
 * ============================================================ */
ModbusResponse modbus_parse_response(uint8_t expected_addr,
                                      uint8_t expected_func,
                                      const uint8_t *data,
                                      uint8_t len) {
    ModbusResponse resp;
    memset(&resp, 0, sizeof(resp));

    /* Store raw response data */
    resp.raw_len = (len <= sizeof(resp.raw)) ? len : sizeof(resp.raw);
    if (len > 0) {
        memcpy(resp.raw, data, resp.raw_len);
    }

    /* 1. Frame length check — reject short frames */
    if (len < GATE_MODBUS_RESP_MIN_LEN) {
        return resp;  /* valid=false */
    }

    /* 2. CRC-16 verification */
    uint16_t calc_crc = modbus_crc16(data, len - 2);
    uint16_t recv_crc = (uint16_t)data[len - 2]
                      | ((uint16_t)data[len - 1] << 8);
    if (calc_crc != recv_crc) {
        return resp;  /* valid=false */
    }

    /* Extract slave and function from response */
    resp.slave    = data[0];
    resp.function = data[1];

    /* 3. Slave address match */
    if (resp.slave != expected_addr) {
        return resp;  /* valid=false */
    }

    /* 4. Exception detection — function code with bit 7 set.
     *    Exception frame: [addr, func|0x80, exc_code, crc_lo, crc_hi]
     *    byte_count check is NOT applicable to exception frames. */
    if (resp.function & GATE_MODBUS_EXCEPTION_BIT) {
        resp.valid     = true;
        resp.exception = true;
        if (len >= 3) {
            resp.exc_code = data[2];
        }
        return resp;
    }

    /* 5. Normal response — function code must match */
    if (resp.function != expected_func) {
        return resp;  /* valid=false */
    }

    /* 6. byte_count consistency check.
     *    Normal FC 0x03 response: [addr, func, byte_count, data..., crc_lo, crc_hi]
     *    Overhead = addr(1) + func(1) + byte_count(1) + crc(2) = 5 bytes.
     *    Therefore: data[2] must equal (len - 5).
     *    This catches corrupted frames where the declared byte_count
     *    does not match the actual number of data bytes received. */
    resp.byte_count = data[2];
    uint8_t expected_data_bytes = len - 5;
    if (resp.byte_count != expected_data_bytes) {
        return resp;  /* valid=false — byte_count inconsistent */
    }

    /* 7. Extract register value for FC 0x03.
     *    For 1 register: byte_count == 2, frame = [addr, func, 0x02, val_hi, val_lo, crc_lo, crc_hi]
     *    value = (val_hi << 8) | val_lo */
    if (resp.byte_count >= 2 && len >= 7) {
        resp.value = ((uint16_t)data[3] << 8) | (uint16_t)data[4];
    }

    resp.valid = true;
    return resp;
}
