/**
 * @file gate_config.h
 * @brief Gate 7 — Production Loop Soak Configuration
 * @version 1.0
 * @date 2026-02-24
 *
 * Validates the production runtime loop under real conditions for a
 * fixed duration. The scheduler runs naturally (no fireNow) and the
 * gate harness observes stability and uplink consistency.
 *
 * LoRaWAN/Modbus parameters reuse Gate 5/6 values (same device, same network).
 * Modbus RTU and CRC constants are redefined here (identical to Gate 3/5/6)
 * so that Gate 3's shared .cpp files resolve "gate_config.h" correctly.
 *
 * References Hardware Profile v2.0 frozen values.
 */

#pragma once

#include <stdint.h>
#include "../../firmware/config/hardware_profile.h"

/* ============================================================
 * Gate Identity
 * ============================================================ */
#define GATE_NAME                       "gate_production_loop_soak"
#define GATE_VERSION                    "1.0"
#define GATE_ID                         7

/* ============================================================
 * SX1262 Hardware Pin Configuration (RAK3312 / ESP32-S3)
 *
 * Values from hardware_profile.h Section 5 (SPI Configuration).
 * ============================================================ */
#define GATE_LORA_PIN_NSS               HW_SPI_LORA_NSS_PIN     /* GPIO 7  */
#define GATE_LORA_PIN_SCK               HW_SPI_LORA_SCK_PIN     /* GPIO 5  */
#define GATE_LORA_PIN_MOSI              HW_SPI_LORA_MOSI_PIN    /* GPIO 6  */
#define GATE_LORA_PIN_MISO              HW_SPI_LORA_MISO_PIN    /* GPIO 3  */
#define GATE_LORA_PIN_RESET             HW_SPI_LORA_NRESET_PIN  /* GPIO 8  */
#define GATE_LORA_PIN_DIO1              HW_SPI_LORA_DIO1_PIN    /* GPIO 47 */
#define GATE_LORA_PIN_BUSY              HW_SPI_LORA_BUSY_PIN    /* GPIO 48 */
#define GATE_LORA_PIN_ANT_SW            HW_SPI_LORA_ANT_SW_PIN  /* GPIO 4  */

/* ============================================================
 * LoRaWAN OTAA Credentials
 *
 * Same credentials as Gate 5/6 — same ChirpStack device.
 * DevEUI / AppEUI: 8 bytes each (MSB)
 * AppKey: 16 bytes (MSB)
 * ============================================================ */
#define GATE_LORAWAN_DEV_EUI    { 0xAC, 0x1F, 0x09, 0xFF, 0xFE, 0x24, 0x02, 0x14 }
#define GATE_LORAWAN_APP_EUI    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define GATE_LORAWAN_APP_KEY    { 0x91, 0x8C, 0x49, 0x08, 0xF0, 0x89, 0x50, 0x6B, \
                                  0x30, 0x18, 0x0B, 0x62, 0x65, 0x9A, 0x4A, 0xD5 }

/* ============================================================
 * LoRaWAN Parameters
 *
 * Region: AS923-1, Class A, no ADR, DR2 (SF10)
 * 3 join trials handled internally by LoRaMAC
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
 * UART / RS485 Pin Configuration
 * ============================================================ */
#define GATE_UART_TX_PIN                HW_UART1_TX_PIN     /* GPIO 43 */
#define GATE_UART_RX_PIN                HW_UART1_RX_PIN     /* GPIO 44 */

/* ============================================================
 * RS485 Transceiver Control Pins (RAK5802 on RAK3312)
 * ============================================================ */
#define GATE_RS485_EN_PIN               HW_WB_IO2_PIN       /* GPIO 14 */
#define GATE_RS485_DE_PIN               HW_WB_IO1_PIN       /* GPIO 21 */

/* ============================================================
 * Previously Discovered Device Parameters (from Gate 2 pass)
 * ============================================================ */
#define GATE_DISCOVERED_SLAVE           1
#define GATE_DISCOVERED_BAUD            4800
#define GATE_DISCOVERED_PARITY          0       /* 0=None, 1=Odd, 2=Even */

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
#define GATE_MODBUS_RETRY_MAX           2       /* 2 retries = 3 total attempts */
#define GATE_MODBUS_RETRY_DELAY_MS      50
#define GATE_MODBUS_RESPONSE_TIMEOUT_MS 200

/* ============================================================
 * Modbus RTU Protocol Constants (universal — same as Gate 3/5/6)
 *
 * Required by Gate 3's shared modbus_frame.cpp and hal_uart.cpp.
 * Values MUST remain identical to Gate 3's definitions.
 * ============================================================ */
#define GATE_MODBUS_FUNC_READ_HOLD      0x03
#define GATE_MODBUS_REQ_FRAME_LEN       8
#define GATE_MODBUS_RESP_MIN_LEN        5
#define GATE_MODBUS_RESP_MAX_LEN        32
#define GATE_MODBUS_EXCEPTION_BIT       0x80

/* ============================================================
 * CRC-16/MODBUS Constants (universal — same as Gate 3/5/6)
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
 * Gate 7 — Production Loop Soak Configuration
 * ============================================================ */
#define GATE_SOAK_DURATION_MS           300000  /* 5 minutes */
#define GATE_SOAK_MIN_UPLINKS           ((GATE_SOAK_DURATION_MS / HW_SYSTEM_POLL_INTERVAL_MS) - 1)  /* 9 */
#define GATE_SOAK_MAX_CONSEC_FAILS      2       /* Max consecutive modbus or uplink failures */
#define GATE_SOAK_MAX_CYCLE_MS          1500    /* Max acceptable cycle duration */
#define GATE_SOAK_LOG_EVERY_CYCLE       1       /* Log every cycle */
#define GATE_SOAK_JOIN_TIMEOUT_MS       30000   /* OTAA join timeout */

/* ============================================================
 * Pass/Fail Codes
 * ============================================================ */
#define GATE_PASS                       0   /* Soak completed, all criteria met */
#define GATE_FAIL_INIT                  1   /* SystemManager::init() failed */
#define GATE_FAIL_INSUFFICIENT_CYCLES   2   /* Total cycles < MIN_UPLINKS */
#define GATE_FAIL_INSUFFICIENT_UPLINKS  3   /* Successful uplinks < MIN_UPLINKS */
#define GATE_FAIL_CONSEC_MODBUS         4   /* Consecutive modbus failures > MAX_CONSEC_FAILS */
#define GATE_FAIL_CONSEC_UPLINK         5   /* Consecutive uplink failures > MAX_CONSEC_FAILS */
#define GATE_FAIL_CYCLE_DURATION        6   /* Cycle duration > MAX_CYCLE_MS */
#define GATE_FAIL_TRANSPORT_LOST        7   /* Transport disconnected during soak */
#define GATE_FAIL_SYSTEM_CRASH          8   /* Runner did not reach GATE COMPLETE */

/* ============================================================
 * Serial Log Tags
 * ============================================================ */
#define GATE_LOG_TAG                    "[GATE7]"
