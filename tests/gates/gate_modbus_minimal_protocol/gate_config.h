/**
 * @file gate_config.h
 * @brief Gate 3 — Modbus Minimal Protocol Validation Configuration
 * @version 1.1
 * @date 2026-02-24
 *
 * Defines all parameters for gate_modbus_minimal_protocol.
 * Discovered device parameters are centralised here (from Gate 2 pass).
 * Gate runner and modbus_frame reference only these #defines.
 * References Hardware Profile v2.0 frozen values.
 */

#pragma once

#include <stdint.h>
#include "../../firmware/config/hardware_profile.h"

/* ============================================================
 * Gate Identity
 * ============================================================ */
#define GATE_NAME                   "gate_modbus_minimal_protocol"
#define GATE_VERSION                "1.1"
#define GATE_ID                     3

/* ============================================================
 * UART / RS485 Pin Configuration (same physical bus as Gate 2)
 * ============================================================ */
#define GATE_UART_TX_PIN            HW_UART1_TX_PIN     /* GPIO 43 */
#define GATE_UART_RX_PIN            HW_UART1_RX_PIN     /* GPIO 44 */

/* ============================================================
 * RS485 Transceiver Control Pins (RAK5802 on RAK3312)
 *
 * WB_IO2 (GPIO 14) = RAK5802 module enable (HIGH = power on)
 * WB_IO1 (GPIO 21) = DE/RE driver enable (HIGH = TX, LOW = RX)
 * ============================================================ */
#define GATE_RS485_EN_PIN           HW_WB_IO2_PIN       /* GPIO 14 — module enable */
#define GATE_RS485_DE_PIN           HW_WB_IO1_PIN       /* GPIO 21 — driver enable */

/* ============================================================
 * Previously Discovered Device Parameters (from Gate 2 pass)
 *
 * Gate 2 result: Slave=1, Baud=4800, Parity=NONE (8N1)
 * Response: 01 03 02 00 00 B8 44
 * ============================================================ */
#define GATE_DISCOVERED_SLAVE       1
#define GATE_DISCOVERED_BAUD        4800
#define GATE_DISCOVERED_PARITY      0       /* 0=None, 1=Odd, 2=Even */

/* ============================================================
 * Modbus RTU Protocol Constants
 * ============================================================ */
#define GATE_MODBUS_FUNC_READ_HOLD  0x03    /* Read Holding Registers */
#define GATE_MODBUS_TARGET_REG_HI   0x00    /* Register address high byte */
#define GATE_MODBUS_TARGET_REG_LO   0x00    /* Register address low byte = 0x0000 */
#define GATE_MODBUS_QUANTITY_HI     0x00    /* Quantity high byte */
#define GATE_MODBUS_QUANTITY_LO     0x01    /* Quantity low byte = 1 register */
#define GATE_MODBUS_REQ_FRAME_LEN   8       /* addr(1)+func(1)+reg(2)+qty(2)+crc(2) */
#define GATE_MODBUS_RESP_MIN_LEN    5       /* addr(1)+func(1)+data(1)+crc(2) minimum */
#define GATE_MODBUS_RESP_MAX_LEN    32      /* Generous buffer for response */
#define GATE_MODBUS_EXCEPTION_BIT   0x80    /* OR'd with function code in exception */

/* ============================================================
 * CRC-16/MODBUS Constants
 * ============================================================ */
#define GATE_CRC_INIT               0xFFFF
#define GATE_CRC_POLY               0xA001  /* Reflected polynomial */

/* ============================================================
 * Timing Parameters
 * ============================================================ */
#define GATE_RESPONSE_TIMEOUT_MS    200     /* Per-attempt timeout */
#define GATE_UART_STABILIZE_MS      50      /* Delay after UART configuration */
#define GATE_UART_FLUSH_DELAY_MS    10      /* Delay after flush before next op */
#define GATE_INTER_BYTE_TIMEOUT_MS  5       /* Frame delimiter inter-byte gap */
#define GATE_TOTAL_TIMEOUT_MS       30000   /* Overall gate timeout: 30s (single device) */

/* ============================================================
 * Pass/Fail Codes
 * ============================================================ */
#define GATE_PASS                   0
#define GATE_FAIL_UART_INIT         1   /* Serial1.begin() failed */
#define GATE_FAIL_TX_ERROR          2   /* hal_uart_write() failed */
#define GATE_FAIL_NO_RESPONSE       3   /* Timeout — no data received */
#define GATE_FAIL_CRC_ERROR         4   /* Response received but CRC mismatch */
#define GATE_FAIL_ADDR_MISMATCH     5   /* Response slave address != expected */
#define GATE_FAIL_SHORT_FRAME       6   /* Response < 5 bytes */
#define GATE_FAIL_SYSTEM_CRASH      7   /* Runner did not reach GATE COMPLETE */
#define GATE_FAIL_BYTE_COUNT        8   /* byte_count field inconsistent with frame length */

/* ============================================================
 * Serial Log Tags
 * ============================================================ */
#define GATE_LOG_TAG                "[GATE3]"
#define GATE_LOG_PASS               "[PASS]"
#define GATE_LOG_FAIL               "[FAIL]"
#define GATE_LOG_INFO               "[INFO]"
#define GATE_LOG_STEP               "[STEP]"
