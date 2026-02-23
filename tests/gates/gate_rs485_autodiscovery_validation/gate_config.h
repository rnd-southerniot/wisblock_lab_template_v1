/**
 * @file gate_config.h
 * @brief Gate 2 — RS485 Modbus Autodiscovery Validation Configuration
 * @version 1.0
 * @date 2026-02-23
 *
 * Defines all parameters for gate_rs485_autodiscovery_validation.
 * References Hardware Profile v2.0 frozen values.
 */

#pragma once

#include <stdint.h>
#include "../../firmware/config/hardware_profile.h"

/* ============================================================
 * Gate Identity
 * ============================================================ */
#define GATE_NAME                   "gate_rs485_autodiscovery_validation"
#define GATE_VERSION                "1.0"
#define GATE_ID                     2

/* ============================================================
 * UART / RS485 Pin Configuration
 * ============================================================ */
#define GATE_UART_TX_PIN            HW_UART1_TX_PIN     /* GPIO 43 */
#define GATE_UART_RX_PIN            HW_UART1_RX_PIN     /* GPIO 44 */
#define GATE_UART_DATA_BITS         HW_RS485_DATA_BITS  /* 8 */
#define GATE_UART_STOP_BITS         HW_RS485_STOP_BITS  /* 1 */

/* ============================================================
 * RS485 Transceiver Control Pins (RAK5802 on RAK3312)
 *
 * WB_IO2 (GPIO 14) = RAK5802 module enable (HIGH = power on)
 * WB_IO1 (GPIO 21) = DE/RE driver enable (HIGH = TX, LOW = RX)
 * ============================================================ */
#define GATE_RS485_EN_PIN           HW_WB_IO2_PIN       /* GPIO 14 — module enable */
#define GATE_RS485_DE_PIN           HW_WB_IO1_PIN       /* GPIO 21 — driver enable */

/* ============================================================
 * Scan Matrix — Baud Rates
 * ============================================================ */
#define GATE_BAUD_COUNT             4
static const uint32_t GATE_BAUD_RATES[] = { 4800, 9600, 19200, 115200 };

/* ============================================================
 * Scan Matrix — Parity Options (0=None, 1=Odd, 2=Even)
 * ============================================================ */
#define GATE_PARITY_COUNT           3
#define GATE_PARITY_NONE            0
#define GATE_PARITY_ODD             1
#define GATE_PARITY_EVEN            2
static const uint8_t GATE_PARITY_OPTIONS[] = { 0, 1, 2 };

/* ============================================================
 * Scan Matrix — Slave Address Range
 * ============================================================ */
#define GATE_SLAVE_ADDR_MIN         1
#define GATE_SLAVE_ADDR_MAX         100     /* User-specified (HW profile allows 247) */

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
#define GATE_RESPONSE_TIMEOUT_MS    200     /* Per-attempt timeout (user-specified) */
#define GATE_UART_STABILIZE_MS      50      /* Delay after UART reconfiguration */
#define GATE_UART_FLUSH_DELAY_MS    10      /* Delay after flush before next op */
#define GATE_INTER_BYTE_TIMEOUT_MS  5       /* Frame delimiter inter-byte gap */
#define GATE_TOTAL_TIMEOUT_MS       300000  /* Overall gate timeout: 5 minutes */

/* ============================================================
 * Scan Totals (informational)
 * ============================================================ */
#define GATE_SCAN_COMBINATIONS      (GATE_BAUD_COUNT * GATE_PARITY_COUNT)           /* 12 */
#define GATE_SCAN_TOTAL_PROBES      (GATE_SCAN_COMBINATIONS * GATE_SLAVE_ADDR_MAX)  /* 1200 */

/* ============================================================
 * Pass/Fail Codes
 * ============================================================ */
#define GATE_PASS                   0
#define GATE_FAIL_UART_INIT         1   /* Serial1.begin() failed or not responsive */
#define GATE_FAIL_NO_DEVICE         2   /* Full scan complete, no valid Modbus response */
#define GATE_FAIL_CRC_ERROR         3   /* Response received but CRC never validated */
#define GATE_FAIL_BUS_ERROR         4   /* UART TX/RX fault or stuck bus */
#define GATE_FAIL_TIMEOUT           5   /* Overall gate timeout exceeded */
#define GATE_FAIL_SYSTEM_CRASH      6   /* Runner did not reach GATE COMPLETE */
#define GATE_FAIL_SHORT_FRAME       7   /* Response received but < 5 bytes */

/* ============================================================
 * Serial Log Tags
 * ============================================================ */
#define GATE_LOG_TAG                "[GATE2]"
#define GATE_LOG_PASS               "[PASS]"
#define GATE_LOG_FAIL               "[FAIL]"
#define GATE_LOG_INFO               "[INFO]"
#define GATE_LOG_STEP               "[STEP]"
