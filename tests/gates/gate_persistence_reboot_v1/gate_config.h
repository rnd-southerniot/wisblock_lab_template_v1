/**
 * @file gate_config.h
 * @brief Gate 9 — Persistence Reboot Test v1 Configuration
 * @version 1.0
 * @date 2026-02-25
 *
 * Validates that storage HAL persists poll interval and slave address
 * across a power cycle / software reset.
 *
 * Two-phase reboot test:
 *   Phase 1: Write test values + verify immediate read-back + reboot
 *   Phase 2: Verify persisted values + SystemManager auto-load + cleanup
 *
 * Single shared config for both platforms via #ifdef CORE_RAK4631.
 * LoRaWAN/Modbus parameters reuse Gate 5-8 values.
 */

#pragma once

#include <stdint.h>

/* ============================================================
 * Gate Identity
 * ============================================================ */
#define GATE_NAME                       "gate_persistence_reboot_v1"
#define GATE_VERSION                    "1.0"
#define GATE_ID                         9

/* ============================================================
 * Serial Log Tags
 * ============================================================ */
#define GATE_LOG_TAG                    "[GATE9]"
#define GATE_LOG_PASS                   "[PASS]"
#define GATE_LOG_FAIL                   "[FAIL]"
#define GATE_LOG_INFO                   "[INFO]"
#define GATE_LOG_STEP                   "[STEP]"

/* ============================================================
 * System Poll Interval
 * ============================================================ */
#define HW_SYSTEM_POLL_INTERVAL_MS      30000

/* ============================================================
 * Gate 9 — Two-Phase Reboot Test Config
 * ============================================================ */
#define GATE9_TEST_INTERVAL_MS          15000   /* Write 15s, verify after reboot */
#define GATE9_TEST_SLAVE_ADDR           42      /* Write addr 42, verify after reboot */
#define GATE9_PHASE_1                   1       /* Pre-reboot: write + verify + store phase marker */
#define GATE9_PHASE_2                   2       /* Post-reboot: verify persisted values */
#define GATE9_JOIN_TIMEOUT_MS           30000

/* ============================================================
 * Platform-Conditional Configuration
 * ============================================================ */
#ifdef CORE_RAK4631

/* --- SX1262 Pin Configuration (RAK4631 / nRF52840) --- */
/* RAK4631 uses lora_rak4630_init() — pins are placeholders */
#define GATE_LORA_PIN_NSS               0
#define GATE_LORA_PIN_SCK               0
#define GATE_LORA_PIN_MOSI              0
#define GATE_LORA_PIN_MISO              0
#define GATE_LORA_PIN_RESET             0
#define GATE_LORA_PIN_DIO1              0
#define GATE_LORA_PIN_BUSY              0
#define GATE_LORA_PIN_ANT_SW            0

/* --- LoRaWAN OTAA Credentials (RAK4631) --- */
#define GATE_LORAWAN_DEV_EUI    { 0x88, 0x82, 0x24, 0x44, 0xAE, 0xED, 0x1E, 0xB2 }
#define GATE_LORAWAN_APP_EUI    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define GATE_LORAWAN_APP_KEY    { 0x91, 0x8C, 0x49, 0x08, 0xF0, 0x89, 0x50, 0x6B, \
                                  0x30, 0x18, 0x0B, 0x62, 0x65, 0x9A, 0x4A, 0xD5 }

/* --- UART / RS485 Pins (RAK4631) --- */
#define GATE_UART_TX_PIN                16      /* P0.16 — documentation only */
#define GATE_UART_RX_PIN                15      /* P0.15 — documentation only */
#define GATE_RS485_EN_PIN               34      /* WB_IO2 — 3V3_S power enable */
#define GATE_RS485_DE_PIN               0xFF    /* Not applicable — auto DE/RE */

#else  /* CORE_RAK3312 (ESP32-S3) */

#include "../../firmware/config/hardware_profile.h"

/* --- SX1262 Pin Configuration (RAK3312 / ESP32-S3) --- */
#define GATE_LORA_PIN_NSS               HW_SPI_LORA_NSS_PIN
#define GATE_LORA_PIN_SCK               HW_SPI_LORA_SCK_PIN
#define GATE_LORA_PIN_MOSI              HW_SPI_LORA_MOSI_PIN
#define GATE_LORA_PIN_MISO              HW_SPI_LORA_MISO_PIN
#define GATE_LORA_PIN_RESET             HW_SPI_LORA_NRESET_PIN
#define GATE_LORA_PIN_DIO1              HW_SPI_LORA_DIO1_PIN
#define GATE_LORA_PIN_BUSY              HW_SPI_LORA_BUSY_PIN
#define GATE_LORA_PIN_ANT_SW            HW_SPI_LORA_ANT_SW_PIN

/* --- LoRaWAN OTAA Credentials (RAK3312) --- */
#define GATE_LORAWAN_DEV_EUI    { 0xAC, 0x1F, 0x09, 0xFF, 0xFE, 0x24, 0x02, 0x14 }
#define GATE_LORAWAN_APP_EUI    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define GATE_LORAWAN_APP_KEY    { 0x91, 0x8C, 0x49, 0x08, 0xF0, 0x89, 0x50, 0x6B, \
                                  0x30, 0x18, 0x0B, 0x62, 0x65, 0x9A, 0x4A, 0xD5 }

/* --- UART / RS485 Pins (RAK3312) --- */
#define GATE_UART_TX_PIN                HW_UART1_TX_PIN
#define GATE_UART_RX_PIN                HW_UART1_RX_PIN
#define GATE_RS485_EN_PIN               HW_WB_IO2_PIN
#define GATE_RS485_DE_PIN               HW_WB_IO1_PIN

#endif  /* CORE_RAK4631 */

/* ============================================================
 * LoRaWAN Parameters (shared — both platforms)
 * ============================================================ */
#define GATE_LORAWAN_REGION             LORAMAC_REGION_AS923
#define GATE_LORAWAN_CLASS              CLASS_A
#define GATE_LORAWAN_SPEC               "1.0.2"
#define GATE_LORAWAN_UPLINK_PORT        10
#define GATE_LORAWAN_CONFIRMED          LMH_UNCONFIRMED_MSG
#define GATE_LORAWAN_ADR                LORAWAN_ADR_OFF
#define GATE_LORAWAN_DATARATE           DR_2
#define GATE_LORAWAN_TX_POWER           TX_POWER_0
#define GATE_LORAWAN_JOIN_TRIALS        3
#define GATE_LORAWAN_PUBLIC_NETWORK     true

/* ============================================================
 * Previously Discovered Device Parameters (from Gate 2)
 * ============================================================ */
#define GATE_DISCOVERED_SLAVE           1
#define GATE_DISCOVERED_BAUD            4800
#define GATE_DISCOVERED_PARITY          0       /* 0=None */

/* ============================================================
 * Modbus Multi-Register Configuration
 * ============================================================ */
#define GATE_MODBUS_QUANTITY            2
#define GATE_MODBUS_TARGET_REG_HI       0x00
#define GATE_MODBUS_TARGET_REG_LO       0x00
#define GATE_MODBUS_QUANTITY_HI         0x00
#define GATE_MODBUS_QUANTITY_LO         0x02

/* ============================================================
 * Modbus Retry Configuration
 * ============================================================ */
#define GATE_MODBUS_RETRY_MAX           2
#define GATE_MODBUS_RETRY_DELAY_MS      50
#define GATE_MODBUS_RESPONSE_TIMEOUT_MS 200

/* ============================================================
 * Modbus RTU Protocol Constants (universal — same as Gate 3)
 * ============================================================ */
#define GATE_MODBUS_FUNC_READ_HOLD      0x03
#define GATE_MODBUS_REQ_FRAME_LEN       8
#define GATE_MODBUS_RESP_MIN_LEN        5
#define GATE_MODBUS_RESP_MAX_LEN        32
#define GATE_MODBUS_EXCEPTION_BIT       0x80

/* ============================================================
 * CRC-16/MODBUS Constants (universal — same as Gate 3)
 * ============================================================ */
#define GATE_CRC_INIT                   0xFFFF
#define GATE_CRC_POLY                   0xA001

/* ============================================================
 * Timing Parameters
 * ============================================================ */
#define GATE_UART_STABILIZE_MS          50
#define GATE_UART_FLUSH_DELAY_MS        10
#define GATE_INTER_BYTE_TIMEOUT_MS      5

/* ============================================================
 * Payload Configuration
 * ============================================================ */
#define GATE_PAYLOAD_LEN                4
#define GATE_PAYLOAD_PORT               10

/* ============================================================
 * Pass/Fail Codes
 * ============================================================ */
#define GATE_PASS                       0   /* Gate passed */
#define GATE_FAIL_INIT                  1   /* SystemManager::init() failed */
#define GATE_FAIL_JOIN                  2   /* OTAA join failed */
#define GATE_FAIL_STORAGE               3   /* Storage HAL init failed */
#define GATE_FAIL_WRITE                 4   /* Storage write failed */
#define GATE_FAIL_READBACK              5   /* Immediate read-back mismatch */
#define GATE_FAIL_PERSIST               6   /* Post-reboot persistence mismatch */
#define GATE_FAIL_AUTOLOAD              7   /* SystemManager didn't auto-load persisted values */
