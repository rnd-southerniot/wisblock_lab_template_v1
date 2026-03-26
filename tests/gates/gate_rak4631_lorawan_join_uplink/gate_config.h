/**
 * @file gate_config.h
 * @brief Gate 5 — RAK4631 LoRaWAN OTAA Join + Modbus Uplink Configuration
 * @version 1.0
 * @date 2026-02-24
 *
 * Defines all parameters for gate_rak4631_lorawan_join_uplink.
 * LoRaWAN OTAA credentials are from Gate 0.5 DevEUI read and
 * ChirpStack v4 device provisioning.
 *
 * Modbus parameters reuse Gate 2 discovery results (same as Gate 3/4).
 * Modbus RTU and CRC constants are redefined here (identical to Gate 3)
 * so that Gate 3's shared .cpp files resolve "gate_config.h" correctly
 * from each compilation unit's own directory.
 *
 * No hardware_profile.h dependency — uses variant.h defines directly.
 *
 * LoRaWAN stack: SX126x-Arduino library (beegee-tokyo/SX126x-Arduino)
 * On RAK4631 (nRF52840), uses lora_rak4630_init() convenience function
 * instead of manual hw_config struct. Requires -DRAK4630 and
 * -D_VARIANT_RAK4630_ build flags for SPI_LORA instantiation.
 */

#pragma once

#include <stdint.h>

/* ============================================================
 * Gate Identity
 * ============================================================ */
#define GATE_NAME                       "gate_rak4631_lorawan_join_uplink"
#define GATE_VERSION                    "1.0"
#define GATE_ID                         5

/* ============================================================
 * Serial Log Tags
 * ============================================================ */
#define GATE_LOG_TAG                    "[GATE5-R4631]"
#define GATE_LOG_PASS                   "[PASS]"
#define GATE_LOG_FAIL                   "[FAIL]"
#define GATE_LOG_INFO                   "[INFO]"
#define GATE_LOG_STEP                   "[STEP]"

/* ============================================================
 * LoRaWAN OTAA Credentials
 *
 * DevEUI: from Gate 0.5 FICR read (88:82:24:44:AE:ED:1E:B2)
 * AppEUI: all zeros (ChirpStack v4 default)
 * AppKey: provisioned in ChirpStack v4
 * ============================================================ */
#define GATE_LORAWAN_DEV_EUI    { 0x88, 0x82, 0x24, 0x44, 0xAE, 0xED, 0x1E, 0xB2 }
#define GATE_LORAWAN_APP_EUI    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define GATE_LORAWAN_APP_KEY    { 0x91, 0x8C, 0x49, 0x08, 0xF0, 0x89, 0x50, 0x6B, \
                                  0x30, 0x18, 0x0B, 0x62, 0x65, 0x9A, 0x4A, 0xD5 }

/* ============================================================
 * LoRaWAN Parameters
 *
 * Region: AS923-1 (LORAMAC_REGION_AS923)
 * Class A, no ADR, DR2 (SF10 — good range for gate test)
 * 3 join trials handled internally by LoRaMAC
 * 30s polling timeout for join callback
 *
 * SX1262 hardware init: lora_rak4630_init() handles all pin
 * configuration internally — no explicit pin mapping needed.
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
#define GATE_LORAWAN_JOIN_TIMEOUT_MS    30000
#define GATE_LORAWAN_PUBLIC_NETWORK     true

/* ============================================================
 * RS485 Control (RAK5802)
 *
 * RAK5802 uses TP8485E with HARDWARE auto-direction:
 *   WB_IO2 (pin 34) = 3V3_S power enable (powers RAK5802)
 *   WB_IO1 (pin 17) = NC (not connected to DE/RE on RAK5802)
 *   DE/RE driven automatically from UART TX line.
 *   No manual GPIO toggling required.
 * ============================================================ */
#define GATE_RS485_EN_PIN               34      /* WB_IO2 — 3V3_S power enable */

/* ============================================================
 * Previously Discovered Device Parameters (from Gate 2 pass)
 *
 * Gate 2 result: Slave=1, Baud=4800, Parity=NONE (8N1)
 * ============================================================ */
#define GATE_DISCOVERED_SLAVE           1
#define GATE_DISCOVERED_BAUD            4800
#define GATE_DISCOVERED_PARITY          0       /* 0=None, 2=Even (nRF52: no Odd) */

/* ============================================================
 * Modbus Multi-Register Configuration
 *
 * Read 2 consecutive holding registers starting at 0x0000.
 * Expected response: [addr, 0x03, byte_count=4, v0_hi, v0_lo,
 *                     v1_hi, v1_lo, crc_lo, crc_hi] = 9 bytes.
 * ============================================================ */
#define GATE_MODBUS_QUANTITY            2
#define GATE_MODBUS_TARGET_REG_HI       0x00
#define GATE_MODBUS_TARGET_REG_LO       0x00
#define GATE_MODBUS_QUANTITY_HI         0x00
#define GATE_MODBUS_QUANTITY_LO         0x02

/* ============================================================
 * Modbus Retry Configuration (simplified from Gate 4)
 *
 * 2 retries after initial attempt = 3 total attempts maximum.
 * ============================================================ */
#define GATE_MODBUS_RETRY_MAX           2       /* 2 retries = 3 total attempts */
#define GATE_MODBUS_RETRY_DELAY_MS      50
#define GATE_MODBUS_RESPONSE_TIMEOUT_MS 200

/* ============================================================
 * Modbus RTU Protocol Constants (universal — same as Gate 3)
 *
 * These values are also defined in Gate 3's gate_config.h and
 * are used by the shared modbus_frame.cpp and hal_uart.cpp.
 * They MUST remain identical to Gate 3's definitions.
 * ============================================================ */
#define GATE_MODBUS_FUNC_READ_HOLD      0x03    /* Read Holding Registers */
#define GATE_MODBUS_REQ_FRAME_LEN       8       /* addr(1)+func(1)+reg(2)+qty(2)+crc(2) */
#define GATE_MODBUS_RESP_MIN_LEN        5       /* addr(1)+func(1)+data(1)+crc(2) minimum */
#define GATE_MODBUS_RESP_MAX_LEN        32      /* Generous buffer for response */
#define GATE_MODBUS_EXCEPTION_BIT       0x80    /* OR'd with function code in exception */

/* ============================================================
 * CRC-16/MODBUS Constants (universal — same as Gate 3)
 * ============================================================ */
#define GATE_CRC_INIT                   0xFFFF
#define GATE_CRC_POLY                   0xA001  /* Reflected polynomial */

/* ============================================================
 * Timing Parameters
 * ============================================================ */
#define GATE_UART_STABILIZE_MS          50      /* Delay after UART configuration */
#define GATE_UART_FLUSH_DELAY_MS        10      /* Delay after flush before next op */
#define GATE_INTER_BYTE_TIMEOUT_MS      5       /* Frame delimiter inter-byte gap */
#define GATE_TOTAL_TIMEOUT_MS           120000  /* Overall gate timeout: 2 min */

/* ============================================================
 * Payload Configuration
 *
 * 2 registers x 2 bytes each = 4 bytes, big-endian.
 * Sent as unconfirmed uplink on port 10.
 * ============================================================ */
#define GATE_PAYLOAD_LEN                4
#define GATE_PAYLOAD_PORT               10

/* ============================================================
 * Pass/Fail Codes
 * ============================================================ */
#define GATE_PASS                       0   /* Join + Modbus read + uplink all succeeded */
#define GATE_FAIL_RADIO_INIT            1   /* LoRaWAN stack init failed (lmh_init) */
#define GATE_FAIL_JOIN_TIMEOUT          2   /* OTAA join timed out (callback not received) */
#define GATE_FAIL_JOIN_DENIED           3   /* Join failed callback received */
#define GATE_FAIL_MODBUS_INIT           4   /* RS485/UART initialization failed */
#define GATE_FAIL_MODBUS_READ           5   /* Modbus read failed (all retries exhausted) */
#define GATE_FAIL_UPLINK_TX             6   /* lmh_send() returned error */
#define GATE_FAIL_SYSTEM_CRASH          7   /* Runner did not reach GATE COMPLETE */
