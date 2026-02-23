/**
 * @file hardware_profile.h
 * @brief Hardware Profile v1 — Frozen Configuration
 * @version 1.0
 * @date 2026-02-23
 *
 * RAK3212 (ESP32) + RAK19007 Base Board
 * Frozen at v1.0-hardware-freeze. Do not modify without version bump.
 */

#pragma once

#include <stdint.h>
#include "core_selector.h"

/* ============================================================
 * SECTION 1 — Core Module
 * ============================================================ */
#define HW_CORE_MODULE              "RAK3212"
#define HW_CORE_MCU                 "ESP32"
#define HW_FRAMEWORK_MODE           "HYBRID_ESPIDF_ARDUINO"

/* ============================================================
 * SECTION 2 — Base Board & Slot Layout
 * ============================================================ */
#define HW_BASE_BOARD               "RAK19007"
#define HW_SLOT_IO                  "RAK5802"   /* RS485 Interface */
#define HW_SLOT_A                   "EMPTY"
#define HW_SLOT_B                   "RAK1904"   /* 3-Axis Accelerometer */
#define HW_SLOT_C                   "EMPTY"
#define HW_SLOT_D                   "EMPTY"

/* ============================================================
 * SECTION 3 — I2C Bus Configuration
 * ============================================================ */
#define HW_I2C_BUS_SHARED           1

/* RAK1904 — LIS3DH 3-Axis Accelerometer (Slot B) */
#define HW_I2C_RAK1904_ADDR         0x18
#define HW_I2C_RAK1904_VOLTAGE      3.3f
#define HW_I2C_RAK1904_MODE         "POLLED"

/* ============================================================
 * SECTION 4 — UART / RS485 Configuration
 * ============================================================ */

/* RAK5802 RS485 Interface (I/O Slot) */
#define HW_RS485_PROTOCOL           "MODBUS_RTU"
#define HW_RS485_DERE_CONTROL       "AUTO"      /* Handled by RAK5802 */
#define HW_RS485_COMM_MODE          "POLLING"
#define HW_RS485_DATA_BITS          8
#define HW_RS485_STOP_BITS          1

/* Auto-Discovery Baud Rates */
#define HW_RS485_BAUD_COUNT         4
static const uint32_t HW_RS485_BAUD_RATES[] = { 4800, 9600, 19200, 115200 };

/* Auto-Discovery Parity Options: 0=None, 1=Odd, 2=Even */
#define HW_RS485_PARITY_COUNT       3
static const uint8_t HW_RS485_PARITY_OPTIONS[] = { 0, 1, 2 };

/* Auto-Discovery Slave Address Range */
#define HW_RS485_SCAN_ADDR_MIN      1
#define HW_RS485_SCAN_ADDR_MAX      247
#define HW_RS485_SCAN_STOP_ON_FIRST 1

/* Total scan combinations: BAUD_COUNT * PARITY_COUNT = 12 */
#define HW_RS485_SCAN_COMBINATIONS  (HW_RS485_BAUD_COUNT * HW_RS485_PARITY_COUNT)

/* ============================================================
 * SECTION 5 — SPI Configuration
 * ============================================================ */
#define HW_SPI_DEVICES_ACTIVE       0

/* ============================================================
 * SECTION 6 — Power Profile
 * ============================================================ */
#define HW_POWER_SOURCE_USB         1
#define HW_POWER_SOURCE_LIPO        1
#define HW_POWER_DUTY_CYCLE         "ALWAYS_ON"

/* ============================================================
 * SECTION 7 — LoRaWAN Configuration
 * ============================================================ */
#define HW_LORAWAN_ACTIVATION       "OTAA"
#define HW_LORAWAN_REGION           "AS923-1"
#define HW_LORAWAN_UPLINK_INTERVAL  30          /* seconds */
#define HW_LORAWAN_NETWORK_SERVER   "CHIRPSTACK_V4"

/* ============================================================
 * SECTION 8 — Constraints
 * ============================================================ */
#define HW_TIMING_CRITICAL          0
#define HW_MEMORY_CONSTRAINED       0

/* ============================================================
 * PROFILE METADATA
 * ============================================================ */
#define HW_PROFILE_VERSION          "1.0"
#define HW_PROFILE_TAG              "v1.0-hardware-freeze"
