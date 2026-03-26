/**
 * @file gate_config.h
 * @brief Gate 3 — RAK4631 Modbus Minimal Protocol Validation Configuration
 * @version 1.0
 * @date 2026-02-24
 *
 * All parameters for gate_rak4631_modbus_minimal_protocol.
 * Discovered device parameters from Gate 2 pass (RAK4631).
 * No hardware_profile.h dependency — uses variant.h defines directly.
 */

#pragma once

#include <stdint.h>

/* ============================================================
 * Gate Identity
 * ============================================================ */
#define GATE_NAME                   "gate_rak4631_modbus_minimal_protocol"
#define GATE_VERSION                "1.0"
#define GATE_ID                     3

/* ============================================================
 * Serial Log Tag
 * ============================================================ */
#define GATE_LOG_TAG                "[GATE3-R4631]"

/* ============================================================
 * UART / Serial1 Pin Configuration (nRF52840)
 *
 * Serial1 TX/RX fixed by variant.h — no pin args to begin().
 *   PIN_SERIAL1_TX = 16 (P0.16)
 *   PIN_SERIAL1_RX = 15 (P0.15)
 * ============================================================ */

/* ============================================================
 * RS485 Control (RAK5802)
 *
 * RAK5802 uses TP8485E with HARDWARE auto-direction:
 *   WB_IO2 (pin 34) = 3V3_S power enable (powers RAK5802)
 *   WB_IO1 (pin 17) = NC (not connected to DE/RE on RAK5802)
 *   DE/RE driven automatically from UART TX line.
 *   No manual GPIO toggling required.
 * ============================================================ */
#define GATE_RS485_EN_PIN           34      /* WB_IO2 — 3V3_S power enable */

/* ============================================================
 * Previously Discovered Device Parameters (from Gate 2 pass)
 *
 * Gate 2 result: Slave=1, Baud=4800, Parity=NONE (8N1)
 * Response: 01 03 02 00 07 F9 86
 * ============================================================ */
#define GATE_DISCOVERED_SLAVE       1
#define GATE_DISCOVERED_BAUD        4800
#define GATE_DISCOVERED_PARITY      0       /* 0=None, 2=Even (nRF52: no Odd) */

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
