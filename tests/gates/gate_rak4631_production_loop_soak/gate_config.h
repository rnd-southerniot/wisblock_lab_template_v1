/**
 * @file gate_config.h
 * @brief Gate 7 — RAK4631 Production Loop Soak Configuration
 * @version 1.0
 * @date 2026-02-25
 *
 * Validates the production runtime loop under real conditions for a
 * fixed 5-minute soak duration. The scheduler runs naturally via tick()
 * (no fireNow) and the gate harness observes stability metrics.
 *
 * LoRaWAN/Modbus parameters reuse Gate 5/6 RAK4631 values (same device,
 * same network). Modbus RTU and CRC constants are redefined here
 * (identical to Gate 3/5/6) so that Gate 3's shared .cpp files resolve
 * "gate_config.h" correctly.
 *
 * No hardware_profile.h dependency — defines go directly in this file.
 * RAK4631 uses lora_rak4630_init() for SX1262 — pin defines are placeholders.
 */

#pragma once

#include <stdint.h>

/* ============================================================
 * Gate Identity
 * ============================================================ */
#define GATE_NAME                       "gate_rak4631_production_loop_soak"
#define GATE_VERSION                    "1.0"
#define GATE_ID                         7

/* ============================================================
 * Serial Log Tags
 * ============================================================ */
#define GATE_LOG_TAG                    "[GATE7-R4631]"
#define GATE_LOG_PASS                   "[PASS]"
#define GATE_LOG_FAIL                   "[FAIL]"
#define GATE_LOG_INFO                   "[INFO]"
#define GATE_LOG_STEP                   "[STEP]"

/* ============================================================
 * System Poll Interval
 *
 * On RAK3312 this comes from hardware_profile.h. On RAK4631
 * we define it directly — no hardware_profile.h dependency.
 * ============================================================ */
#define HW_SYSTEM_POLL_INTERVAL_MS      30000

/* ============================================================
 * SX1262 Hardware Pin Configuration (RAK4631 / nRF52840)
 *
 * RAK4631 uses lora_rak4630_init() which handles all SX1262
 * pin configuration internally. These defines are set to 0
 * (placeholder) because SystemManager constructor still
 * references them to build LoRaTransportConfig, but the
 * values are never used on RAK4631.
 * ============================================================ */
#define GATE_LORA_PIN_NSS               0   /* Unused — lora_rak4630_init() */
#define GATE_LORA_PIN_SCK               0   /* Unused — lora_rak4630_init() */
#define GATE_LORA_PIN_MOSI              0   /* Unused — lora_rak4630_init() */
#define GATE_LORA_PIN_MISO              0   /* Unused — lora_rak4630_init() */
#define GATE_LORA_PIN_RESET             0   /* Unused — lora_rak4630_init() */
#define GATE_LORA_PIN_DIO1              0   /* Unused — lora_rak4630_init() */
#define GATE_LORA_PIN_BUSY              0   /* Unused — lora_rak4630_init() */
#define GATE_LORA_PIN_ANT_SW            0   /* Unused — lora_rak4630_init() */

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
 * UART Pin Configuration (documentation only)
 *
 * nRF52840 Serial1 uses pins from variant.h:
 *   TX = pin 16 (P0.16)
 *   RX = pin 15 (P0.15)
 * These are NOT passed to hal_uart_init() on RAK4631 (2-arg form).
 * Defined here so SystemManager constructor can populate
 * ModbusPeripheralConfig without #ifdef in system_manager.cpp.
 * ============================================================ */
#define GATE_UART_TX_PIN                16      /* P0.16 — documentation only */
#define GATE_UART_RX_PIN                15      /* P0.15 — documentation only */

/* ============================================================
 * RS485 Transceiver Control Pins (RAK5802 on RAK4631)
 *
 * WB_IO2 (pin 34) = 3V3_S power enable (powers RAK5802)
 * DE/RE: auto — RAK5802 TP8485E driven from UART TX line
 * GATE_RS485_DE_PIN = 0xFF (not applicable — auto DE/RE)
 * ============================================================ */
#define GATE_RS485_EN_PIN               34      /* WB_IO2 — 3V3_S power enable */
#define GATE_RS485_DE_PIN               0xFF    /* Not applicable — auto DE/RE */

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
 * Same as Gate 5/6.
 * ============================================================ */
#define GATE_MODBUS_QUANTITY            2
#define GATE_MODBUS_TARGET_REG_HI       0x00
#define GATE_MODBUS_TARGET_REG_LO       0x00
#define GATE_MODBUS_QUANTITY_HI         0x00
#define GATE_MODBUS_QUANTITY_LO         0x02

/* ============================================================
 * Modbus Retry Configuration (same as Gate 5/6)
 *
 * 2 retries after initial attempt = 3 total attempts maximum.
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
#define GATE_MODBUS_FUNC_READ_HOLD      0x03    /* Read Holding Registers */
#define GATE_MODBUS_REQ_FRAME_LEN       8       /* addr(1)+func(1)+reg(2)+qty(2)+crc(2) */
#define GATE_MODBUS_RESP_MIN_LEN        5       /* addr(1)+func(1)+data(1)+crc(2) minimum */
#define GATE_MODBUS_RESP_MAX_LEN        32      /* Generous buffer for response */
#define GATE_MODBUS_EXCEPTION_BIT       0x80    /* OR'd with function code in exception */

/* ============================================================
 * CRC-16/MODBUS Constants (universal — same as Gate 3/5/6)
 * ============================================================ */
#define GATE_CRC_INIT                   0xFFFF
#define GATE_CRC_POLY                   0xA001  /* Reflected polynomial */

/* ============================================================
 * Timing Parameters
 * ============================================================ */
#define GATE_UART_STABILIZE_MS          50      /* Delay after UART configuration */
#define GATE_UART_FLUSH_DELAY_MS        10      /* Delay after flush before next op */
#define GATE_INTER_BYTE_TIMEOUT_MS      5       /* Frame delimiter inter-byte gap */

/* ============================================================
 * Payload Configuration
 *
 * 2 registers x 2 bytes each = 4 bytes, big-endian.
 * Sent as unconfirmed uplink on port 10.
 * ============================================================ */
#define GATE_PAYLOAD_LEN                4
#define GATE_PAYLOAD_PORT               10

/* ============================================================
 * Gate 7 — Production Loop Soak Configuration
 * ============================================================ */
#define GATE7_SOAK_DURATION_MS          300000  /* 5 minutes */
#define GATE7_MIN_UPLINKS               3       /* Conservative — user-specified */
#define GATE7_MAX_CONSEC_FAILS          2       /* Max consecutive modbus or uplink failures */
#define GATE7_MAX_CYCLE_DURATION_MS     1500    /* Max acceptable cycle duration */
#define GATE7_JOIN_TIMEOUT_MS           30000   /* OTAA join timeout */

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
