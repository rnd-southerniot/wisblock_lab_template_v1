/**
 * @file hardware_profile.h
 * @brief Hardware Profile v2.0 — Frozen Configuration
 * @version 2.0
 * @date 2026-02-23
 *
 * RAK3312 (ESP32-S3 + SX1262) + RAK19007 Base Board
 * Frozen at v2.0-hardware-rak3312. Do not modify without version bump.
 *
 * MIGRATION: Replaces v1.0 (RAK3212/ESP32). See MIGRATION_NOTES.md.
 */

#pragma once

#include <stdint.h>
#include "core_selector.h"

/* ============================================================
 * SECTION 1 — Core Module
 * ============================================================ */
#define HW_CORE_MODULE              "RAK3312"
#define HW_CORE_MCU                 "ESP32-S3"
#define HW_CORE_MCU_VARIANT         "Xtensa LX7 Dual-Core"
#define HW_CORE_CLOCK_MHZ           240
#define HW_CORE_FLASH_MB            16
#define HW_CORE_SRAM_KB             512
#define HW_CORE_PSRAM_MB            8
#define HW_CORE_RTC_SRAM_KB         16
#define HW_CORE_LORA_CHIP           "SX1262"
#define HW_FRAMEWORK_MODE           "ARDUINO_ESP32"

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

/* I2C1 — Primary WisBlock bus (Slots A-D) */
#define HW_I2C1_SDA_PIN             9
#define HW_I2C1_SCL_PIN             40

/* I2C2 — Secondary bus */
#define HW_I2C2_SDA_PIN             17
#define HW_I2C2_SCL_PIN             18

/* RAK1904 — LIS3DH 3-Axis Accelerometer (Slot B, on I2C1) */
#define HW_I2C_RAK1904_ADDR         0x18
#define HW_I2C_RAK1904_VOLTAGE      3.3f
#define HW_I2C_RAK1904_MODE         "POLLED"
#define HW_I2C_RAK1904_BUS          1           /* I2C1 */

/* ============================================================
 * SECTION 4 — UART / RS485 Configuration
 * ============================================================ */

/* UART1 — General purpose / RS485 via I/O Slot */
#define HW_UART1_TX_PIN             43
#define HW_UART1_RX_PIN             44

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

/* SPI — Internal (ESP32-S3 <-> SX1262 LoRa) — DO NOT USE */
#define HW_SPI_LORA_NSS_PIN         7
#define HW_SPI_LORA_SCK_PIN         5
#define HW_SPI_LORA_MOSI_PIN        6
#define HW_SPI_LORA_MISO_PIN        3
#define HW_SPI_LORA_NRESET_PIN      8
#define HW_SPI_LORA_DIO1_PIN        47
#define HW_SPI_LORA_BUSY_PIN        48
#define HW_SPI_LORA_ANT_SW_PIN      4

/* SPI — WisBlock external bus (available for sensor modules) */
#define HW_SPI_WB_CS_PIN            12
#define HW_SPI_WB_CLK_PIN           13
#define HW_SPI_WB_MOSI_PIN          11
#define HW_SPI_WB_MISO_PIN          10

#define HW_SPI_DEVICES_ACTIVE       1           /* SX1262 (internal) */
#define HW_SPI_WB_DEVICES_ACTIVE    0           /* No external SPI devices */

/* ============================================================
 * SECTION 6 — WisBlock IO Pin Map
 * ============================================================ */
#define HW_WB_IO1_PIN               21
#define HW_WB_IO2_PIN               14
#define HW_WB_IO3_PIN               41
#define HW_WB_IO4_PIN               42
#define HW_WB_IO5_PIN               38
#define HW_WB_IO6_PIN               39

/* LEDs */
#define HW_LED_GREEN_PIN            46
#define HW_LED_BLUE_PIN             45

/* ============================================================
 * SECTION 7 — Power Profile
 * ============================================================ */
#define HW_POWER_SUPPLY_MIN_V       3.0f
#define HW_POWER_SUPPLY_MAX_V       3.6f
#define HW_POWER_SUPPLY_TYP_V       3.3f
#define HW_POWER_SOURCE_USB         1
#define HW_POWER_SOURCE_LIPO        1
#define HW_POWER_DUTY_CYCLE         "ALWAYS_ON"
#define HW_POWER_WIFI_TX_MA         340         /* Peak WiFi TX current */
#define HW_POWER_LORA_TX_MA         140         /* LoRa TX @ +22 dBm */

/* ============================================================
 * SECTION 8 — LoRaWAN Configuration
 * ============================================================ */
#define HW_LORAWAN_CHIP             "SX1262"
#define HW_LORAWAN_STACK            "ARDUINO_LMIC"  /* Arduino-based, NOT STM32WL */
#define HW_LORAWAN_SPEC             "1.0.2"
#define HW_LORAWAN_ACTIVATION       "OTAA"
#define HW_LORAWAN_REGION           "AS923-1"
#define HW_LORAWAN_FREQ_RANGE       "863-928 MHz"
#define HW_LORAWAN_UPLINK_INTERVAL  30          /* seconds */
#define HW_LORAWAN_NETWORK_SERVER   "CHIRPSTACK_V4"

/* ============================================================
 * SECTION 9 — Reserved / Conflict Pins
 * ============================================================
 *
 * The following GPIOs are internally connected to the SX1262
 * LoRa transceiver and MUST NOT be used for other purposes:
 *
 *   GPIO 3  — SX1262 MISO
 *   GPIO 4  — SX1262 ANT_SW    (was I2C SDA in v1.0!)
 *   GPIO 5  — SX1262 SCK       (was I2C SCL in v1.0!)
 *   GPIO 6  — SX1262 MOSI
 *   GPIO 7  — SX1262 NSS
 *   GPIO 8  — SX1262 NRESET
 *   GPIO 47 — SX1262 DIO1
 *   GPIO 48 — SX1262 BUSY
 */
#define HW_RESERVED_PIN_COUNT       8
static const uint8_t HW_RESERVED_PINS[] = { 3, 4, 5, 6, 7, 8, 47, 48 };

/* ============================================================
 * SECTION 10 — Constraints
 * ============================================================ */
#define HW_TIMING_CRITICAL          0
#define HW_MEMORY_CONSTRAINED       0

/* ============================================================
 * PROFILE METADATA
 * ============================================================ */
#define HW_PROFILE_VERSION          "2.0"
#define HW_PROFILE_TAG              "v2.0-hardware-rak3312"
#define HW_PROFILE_PREVIOUS         "v1.0-hardware-freeze"
