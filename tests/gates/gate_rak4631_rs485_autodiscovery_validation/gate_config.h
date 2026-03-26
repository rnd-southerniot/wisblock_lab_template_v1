/**
 * @file gate_config.h
 * @brief Gate 2 — RAK4631 RS485 Modbus Autodiscovery Configuration
 * @version 1.0
 * @date 2026-02-24
 */

#pragma once
#include <stdint.h>

/* Gate Identity */
#define GATE_NAME       "gate_rak4631_rs485_autodiscovery_validation"
#define GATE_VERSION    "1.0"
#define GATE_ID         2

/* Serial Log Tag */
#define GATE_LOG_TAG    "[GATE2-R4631]"

/* ---- UART / Serial1 ----
 * nRF52840 variant.h defines:
 *   PIN_SERIAL1_TX = 16 (P0.16)
 *   PIN_SERIAL1_RX = 15 (P0.15)
 * Serial1.begin(baud, config) — no pin args on nRF52.
 */

/* ---- RS485 Control (RAK5802) ----
 * RAK5802 uses TP8485E transceiver with HARDWARE auto-direction:
 *   WB_IO2 (pin 34) = 3V3_S power enable (powers RAK5802 module)
 *   WB_IO1 (pin 17) = NC (not connected to DE/RE on RAK5802)
 *   DE/RE is driven automatically from UART TX line by on-board circuit.
 *   No manual GPIO toggling required for direction control.
 */
#define RS485_EN_PIN        34  /* WB_IO2 — 3V3_S power enable (HIGH=on) */

/* ---- Scan Parameters ---- */
static const uint32_t SCAN_BAUDS[] = { 4800, 9600, 19200, 115200 };
#define SCAN_BAUD_COUNT     4

/* nRF52840 UARTE supports NONE and EVEN only (no Odd parity in hardware) */
#define SCAN_PARITY_NONE    0
#define SCAN_PARITY_EVEN    2
static const uint8_t SCAN_PARITIES[] = { SCAN_PARITY_NONE, SCAN_PARITY_EVEN };
#define SCAN_PARITY_COUNT   2

#define SCAN_SLAVE_MIN      1
#define SCAN_SLAVE_MAX      100

/* ---- Modbus RTU Frame ---- */
#define MODBUS_FUNC_READ_HOLD   0x03
#define MODBUS_TARGET_REG_HI    0x00
#define MODBUS_TARGET_REG_LO    0x00
#define MODBUS_QUANTITY_HI      0x00
#define MODBUS_QUANTITY_LO      0x01
#define MODBUS_REQUEST_LEN      8       /* addr + func + reg(2) + qty(2) + crc(2) */
#define MODBUS_RESPONSE_MIN     5       /* addr + func + data(1+) + crc(2) */

/* ---- CRC-16/MODBUS ---- */
#define CRC_INIT    0xFFFF
#define CRC_POLY    0xA001  /* Reflected polynomial */

/* ---- Timing ---- */
#define UART_RESPONSE_TIMEOUT_MS    200     /* Max wait for first byte */
#define UART_INTERBYTE_GAP_MS       5       /* Inter-byte gap detection */
#define UART_FLUSH_DELAY_MS         50      /* Delay after config change */
