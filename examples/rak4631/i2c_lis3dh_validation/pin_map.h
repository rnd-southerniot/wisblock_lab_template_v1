/**
 * @file pin_map.h
 * @brief Pin Map — RAK4631 WisBlock Core (nRF52840 + SX1262)
 *
 * Defines GPIO assignments for I2C LIS3DH accelerometer validation.
 * Based on WisCore_RAK4631_Board variant.h pin definitions.
 *
 * Source: Hardware Profile v3.1 (RAK4631)
 */

#pragma once

/* ---- I2C Bus (Wire) ---- */
#define PIN_I2C_SDA        13   /* nRF52840 P0.13 — I2C SDA */
#define PIN_I2C_SCL        14   /* nRF52840 P0.14 — I2C SCL */

/* ---- LIS3DH Accelerometer ---- */
#define LIS3DH_I2C_ADDR   0x18 /* 7-bit I2C address (SDO/SA0 = GND) */

/* ---- WisBlock IO Pins ---- */
#define PIN_WB_IO1         17   /* nRF52840 P0.17 — WisBlock IO Slot 1 */
#define PIN_WB_IO2         34   /* nRF52840 P1.02 — WisBlock IO Slot 2 */

/* ---- LEDs ---- */
#define PIN_LED_BLUE       36   /* nRF52840 P1.04 — Blue LED (active HIGH) */
