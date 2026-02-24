/**
 * @file pin_map.h
 * @brief Pin Map — RAK4631 WisBlock Core (nRF52840 + SX1262)
 *
 * Defines GPIO assignments for Modbus RTU robustness layer validation.
 * Based on WisCore_RAK4631_Board variant.h pin definitions.
 *
 * Source: Hardware Profile v3.4 (RAK4631)
 */

#pragma once

/* ---- Serial1 (UART to RAK5802) ---- */
#define PIN_UART1_TX       16   /* nRF52840 P0.16 — Serial1 TX */
#define PIN_UART1_RX       15   /* nRF52840 P0.15 — Serial1 RX */

/* ---- RS485 Control (RAK5802) ---- */
#define PIN_RS485_EN       34   /* nRF52840 P1.02 — WB_IO2, 3V3_S power enable */
/*
 * RAK5802 uses TP8485E with hardware auto-direction circuit.
 * DE/RE is driven from UART TX line automatically.
 * WB_IO1 (pin 17) is NC — not connected to DE/RE.
 * No manual GPIO toggling required.
 */

/* ---- WisBlock IO Pins ---- */
#define PIN_WB_IO1         17   /* nRF52840 P0.17 — NC on RAK5802 */
#define PIN_WB_IO2         34   /* nRF52840 P1.02 — 3V3_S power enable */

/* ---- LEDs ---- */
#define PIN_LED_BLUE       36   /* nRF52840 P1.04 — Blue LED (active HIGH) */

/* ---- Discovered Modbus Device (from Gate 2) ---- */
#define MODBUS_SLAVE       1
#define MODBUS_BAUD        4800
#define MODBUS_PARITY      0    /* 0=NONE (8N1) */

/* ---- Multi-Register Configuration ---- */
#define MODBUS_QUANTITY    2    /* Read 2 consecutive registers */

/* ---- Retry Configuration ---- */
#define RETRY_MAX          3    /* 3 retries = 4 total attempts */
#define RETRY_DELAY_MS     50   /* Delay between retries */
#define RESPONSE_TIMEOUT   200  /* Per-attempt timeout (ms) */
#define CONSEC_FAIL_REINIT 2   /* UART re-init after N consecutive failures */
