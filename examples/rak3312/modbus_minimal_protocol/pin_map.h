/**
 * @file pin_map.h
 * @brief Pin Map — RAK3312 + RAK5802 RS485 (IO Slot)
 *
 * Defines GPIO assignments for Modbus RTU communication
 * via the RAK5802 RS485 transceiver on the RAK19007 base board.
 *
 * Source: Hardware Profile v2.0 (RAK3312)
 */

#pragma once

/* ---- UART1 Pins (RS485 data bus) ---- */
#define PIN_UART_TX        43   /* ESP32-S3 GPIO 43 — UART1 TX */
#define PIN_UART_RX        44   /* ESP32-S3 GPIO 44 — UART1 RX */

/* ---- RS485 Transceiver Control (RAK5802 on IO Slot) ---- */
#define PIN_RS485_EN       14   /* WB_IO2 — RAK5802 module enable (HIGH=on) */
#define PIN_RS485_DE       21   /* WB_IO1 — RS485 driver enable (HIGH=TX, LOW=RX) */

/* ---- LEDs ---- */
#define PIN_LED_GREEN      46   /* RAK19007 green LED */
#define PIN_LED_BLUE       45   /* RAK19007 blue LED */
