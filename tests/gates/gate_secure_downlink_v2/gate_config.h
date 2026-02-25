/**
 * @file gate_config.h
 * @brief Gate 10 — Secure Downlink Protocol v2 Validation Configuration
 * @version 2.0
 * @date 2026-02-26
 *
 * Validates the Secure Downlink Protocol v2 module:
 *   - HMAC-SHA256 authentication (truncated to 4 bytes)
 *   - Monotonic nonce replay protection
 *   - Protocol magic + version enforcement
 *   - Command dispatch via SystemManager
 *
 * Frame format:
 *   MAGIC(0xD1) | VERSION(0x02) | CMD_ID | FLAGS | NONCE(4 BE) | PAYLOAD | HMAC(4)
 *
 * This gate runs:
 *   Phase A: Local synthetic frame validation (unit tests)
 *   Phase B: Live downlink from ChirpStack + replay rejection
 *
 * References Hardware Profile v2.0 frozen values.
 */

#pragma once

#include <stdint.h>
#include "../../firmware/config/hardware_profile.h"

/* ============================================================
 * Gate Identity
 * ============================================================ */
#define GATE_NAME                       "gate_secure_downlink_v2"
#define GATE_VERSION                    "2.0"
#define GATE_ID                         10

/* ============================================================
 * Test HMAC Key (32 bytes)
 *
 * Sequential test key: 0x00..0x1F
 * FOR GATE TESTING ONLY — never use in production.
 * ============================================================ */
#define GATE_SDL_TEST_KEY           { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, \
                                      0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, \
                                      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, \
                                      0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F }

/* ============================================================
 * Test Frame: CMD_SET_INTERVAL to 10000ms, nonce=1
 *
 * Wire format (16 bytes):
 *   D1              MAGIC
 *   02              VERSION (0x02)
 *   01              CMD_SET_INTERVAL
 *   00              FLAGS (reserved, 0x00)
 *   00 00 00 01     Nonce = 1 (big-endian)
 *   00 00 27 10     Payload = 10000 ms (big-endian)
 *   A2 7F F4 AB     HMAC-SHA256 truncated (first 4 bytes)
 *
 * HMAC input (12 bytes): D1 02 01 00 00 00 00 01 00 00 27 10
 * HMAC key:              00 01 02 ... 1F (32 bytes)
 * Full HMAC-SHA256:      A27FF4AB 957D28A1 1418B1A8 28EFA56E ...
 * Truncated (4 bytes):   A2 7F F4 AB
 * ============================================================ */
#define GATE_SDL_TEST_FRAME_LEN     16
#define GATE_SDL_TEST_FRAME         { 0xD1, 0x02, 0x01, 0x00, \
                                      0x00, 0x00, 0x00, 0x01, \
                                      0x00, 0x00, 0x27, 0x10, \
                                      0xA2, 0x7F, 0xF4, 0xAB }

/** Expected command ID after successful parse */
#define GATE_SDL_TEST_EXPECTED_CMD          0x01

/** Expected interval value after successful parse (ms) */
#define GATE_SDL_TEST_EXPECTED_INTERVAL     10000

/** Expected nonce value in test frame */
#define GATE_SDL_TEST_EXPECTED_NONCE        1

/* ============================================================
 * Test Frame: Replay — identical to above (same nonce=1)
 *
 * Second submission of same frame must fail with replay_fail=true.
 * Frame bytes are identical to GATE_SDL_TEST_FRAME.
 * ============================================================ */
#define GATE_SDL_REPLAY_FRAME_LEN   GATE_SDL_TEST_FRAME_LEN
#define GATE_SDL_REPLAY_FRAME       GATE_SDL_TEST_FRAME

/* ============================================================
 * Test Frame: Wrong magic (0xAA instead of 0xD1)
 *
 * Wire format (12 bytes, no payload):
 *   AA              MAGIC WRONG (0xAA)
 *   02              VERSION
 *   01              CMD_SET_INTERVAL
 *   00              FLAGS
 *   00 00 00 02     Nonce = 2
 *   DE AD BE EF     HMAC (irrelevant — rejected before HMAC check)
 * ============================================================ */
#define GATE_SDL_BAD_VERSION_FRAME_LEN  12
#define GATE_SDL_BAD_VERSION_FRAME      { 0xAA, 0x02, 0x01, 0x00, \
                                          0x00, 0x00, 0x00, 0x02, \
                                          0xDE, 0xAD, 0xBE, 0xEF }

/* ============================================================
 * Test Frame: Bad HMAC (valid structure, zeroed HMAC)
 *
 * Same as test frame but with nonce=3 and zeroed HMAC.
 * ============================================================ */
#define GATE_SDL_BAD_HMAC_FRAME_LEN     16
#define GATE_SDL_BAD_HMAC_FRAME         { 0xD1, 0x02, 0x01, 0x00, \
                                          0x00, 0x00, 0x00, 0x03, \
                                          0x00, 0x00, 0x27, 0x10, \
                                          0x00, 0x00, 0x00, 0x00 }

/* ============================================================
 * Nonce Storage Configuration
 * ============================================================ */

/** Storage key name (must match SDL_NONCE_STORAGE_KEY in downlink_security.h) */
#define GATE_SDL_NONCE_KEY          "sdl_nonce"

/** Initial nonce value (storage cleared before test) */
#define GATE_SDL_INITIAL_NONCE      0

/* ============================================================
 * Downlink Configuration
 * ============================================================ */

/** LoRaWAN port for secure downlink commands */
#define GATE_SDL_DOWNLINK_PORT      20

/** Timeout waiting for live downlink from ChirpStack (ms) */
#define GATE_SDL_DOWNLINK_TIMEOUT_MS  120000  /* 2 minutes */

/** Poll interval for checking downlink buffer (ms) */
#define GATE_SDL_POLL_MS            500

/* ============================================================
 * SX1262 Hardware Pin Configuration (RAK3312 / ESP32-S3)
 *
 * Required by SystemManager constructor.
 * Values from hardware_profile.h Section 5.
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
 * LoRaWAN OTAA Credentials (same device as Gate 5/6/7)
 * ============================================================ */
#define GATE_LORAWAN_DEV_EUI    { 0xAC, 0x1F, 0x09, 0xFF, 0xFE, 0x24, 0x02, 0x14 }
#define GATE_LORAWAN_APP_EUI    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define GATE_LORAWAN_APP_KEY    { 0x91, 0x8C, 0x49, 0x08, 0xF0, 0x89, 0x50, 0x6B, \
                                  0x30, 0x18, 0x0B, 0x62, 0x65, 0x9A, 0x4A, 0xD5 }

/* ============================================================
 * LoRaWAN Parameters
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
#define GATE_DISCOVERED_PARITY          0

/* ============================================================
 * Modbus Configuration (required by SystemManager constructor)
 * ============================================================ */
#define GATE_MODBUS_QUANTITY            2
#define GATE_MODBUS_TARGET_REG_HI       0x00
#define GATE_MODBUS_TARGET_REG_LO       0x00
#define GATE_MODBUS_QUANTITY_HI         0x00
#define GATE_MODBUS_QUANTITY_LO         0x02
#define GATE_MODBUS_RETRY_MAX           2
#define GATE_MODBUS_RETRY_DELAY_MS      50
#define GATE_MODBUS_RESPONSE_TIMEOUT_MS 200

/* ============================================================
 * Modbus RTU Protocol Constants (universal — same as Gate 3/5/6/7)
 * ============================================================ */
#define GATE_MODBUS_FUNC_READ_HOLD      0x03
#define GATE_MODBUS_REQ_FRAME_LEN       8
#define GATE_MODBUS_RESP_MIN_LEN        5
#define GATE_MODBUS_RESP_MAX_LEN        32
#define GATE_MODBUS_EXCEPTION_BIT       0x80

/* ============================================================
 * CRC-16/MODBUS Constants (universal — same as Gate 3/5/6/7)
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
 * Gate 10 — Downlink Security Specific Timing
 * ============================================================ */
#define GATE_SDL_JOIN_TIMEOUT_MS        30000   /* OTAA join timeout */

/* ============================================================
 * Pass/Fail Codes
 * ============================================================ */
#define GATE_PASS                       0   /* All security checks passed */
#define GATE_FAIL_INIT                  1   /* SystemManager::init() failed */
#define GATE_FAIL_VALID_REJECTED        2   /* Valid frame was incorrectly rejected */
#define GATE_FAIL_WRONG_CMD             3   /* Parsed cmd_id does not match expected */
#define GATE_FAIL_REPLAY_ACCEPTED       4   /* Replay frame was incorrectly accepted */
#define GATE_FAIL_REPLAY_FLAG           5   /* Replay rejected but replay_fail flag not set */
#define GATE_FAIL_BAD_VERSION_ACCEPTED  6   /* Wrong-magic/version frame was accepted */
#define GATE_FAIL_VERSION_FLAG          7   /* Rejected but version_fail not set */
#define GATE_FAIL_BAD_HMAC_ACCEPTED     8   /* Tampered HMAC frame was accepted */
#define GATE_FAIL_AUTH_FLAG             9   /* HMAC rejected but auth_fail flag not set */
#define GATE_FAIL_NONCE_PERSIST         10  /* Nonce not persisted after valid command */
#define GATE_FAIL_DL_TIMEOUT            11  /* No downlink received within timeout */
#define GATE_FAIL_DL_INVALID            12  /* Live downlink failed validation */
#define GATE_FAIL_DL_REPLAY_ACCEPTED    13  /* Live downlink replay was accepted */
#define GATE_FAIL_SYSTEM_CRASH          14  /* Runner did not reach GATE COMPLETE */

/* ============================================================
 * Serial Log Tags
 * ============================================================ */
#define GATE_LOG_TAG                    "[GATE10]"
