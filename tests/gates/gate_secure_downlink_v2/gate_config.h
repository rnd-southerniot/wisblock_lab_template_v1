#pragma once

#include <stdint.h>

/* ============================================================
 * Gate Identity
 * ============================================================ */
#define GATE_NAME                       "gate_secure_downlink_v2"
#define GATE_VERSION                    "2.0"
#define GATE_ID                         10

/* ============================================================
 * Serial Log Tags
 * ============================================================ */
#define GATE_LOG_TAG                    "[GATE10]"
#define GATE_LOG_PASS                   "[PASS]"
#define GATE_LOG_FAIL                   "[FAIL]"
#define GATE_LOG_INFO                   "[INFO]"
#define GATE_LOG_STEP                   "[STEP]"

/* ============================================================
 * System Poll Interval
 * ============================================================ */
#define HW_SYSTEM_POLL_INTERVAL_MS      30000

/* ============================================================
 * Gate 10 Timing
 * ============================================================ */
#define GATE_SDL_JOIN_TIMEOUT_MS        30000
#define GATE_SDL_DOWNLINK_TIMEOUT_MS    120000
#define GATE_SDL_POLL_MS                50

/* ============================================================
 * Platform-Conditional Configuration
 * ============================================================ */
#ifdef CORE_RAK4631

/* RAK4631 uses lora_rak4630_init() so these are placeholders. */
#define GATE_LORA_PIN_NSS               0
#define GATE_LORA_PIN_SCK               0
#define GATE_LORA_PIN_MOSI              0
#define GATE_LORA_PIN_MISO              0
#define GATE_LORA_PIN_RESET             0
#define GATE_LORA_PIN_DIO1              0
#define GATE_LORA_PIN_BUSY              0
#define GATE_LORA_PIN_ANT_SW            0

#define GATE_LORAWAN_DEV_EUI    { 0x88, 0x82, 0x24, 0x44, 0xAE, 0xED, 0x1E, 0xB2 }
#define GATE_LORAWAN_APP_EUI    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define GATE_LORAWAN_APP_KEY    { 0x91, 0x8C, 0x49, 0x08, 0xF0, 0x89, 0x50, 0x6B, \
                                  0x30, 0x18, 0x0B, 0x62, 0x65, 0x9A, 0x4A, 0xD5 }

#define GATE_UART_TX_PIN                16
#define GATE_UART_RX_PIN                15
#define GATE_RS485_EN_PIN               34
#define GATE_RS485_DE_PIN               0xFF

#else  /* CORE_RAK3312 */

#include "../../firmware/config/hardware_profile.h"

#define GATE_LORA_PIN_NSS               HW_SPI_LORA_NSS_PIN
#define GATE_LORA_PIN_SCK               HW_SPI_LORA_SCK_PIN
#define GATE_LORA_PIN_MOSI              HW_SPI_LORA_MOSI_PIN
#define GATE_LORA_PIN_MISO              HW_SPI_LORA_MISO_PIN
#define GATE_LORA_PIN_RESET             HW_SPI_LORA_NRESET_PIN
#define GATE_LORA_PIN_DIO1              HW_SPI_LORA_DIO1_PIN
#define GATE_LORA_PIN_BUSY              HW_SPI_LORA_BUSY_PIN
#define GATE_LORA_PIN_ANT_SW            HW_SPI_LORA_ANT_SW_PIN

#define GATE_LORAWAN_DEV_EUI    { 0xAC, 0x1F, 0x09, 0xFF, 0xFE, 0x24, 0x02, 0x14 }
#define GATE_LORAWAN_APP_EUI    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define GATE_LORAWAN_APP_KEY    { 0x91, 0x8C, 0x49, 0x08, 0xF0, 0x89, 0x50, 0x6B, \
                                  0x30, 0x18, 0x0B, 0x62, 0x65, 0x9A, 0x4A, 0xD5 }

#define GATE_UART_TX_PIN                HW_UART1_TX_PIN
#define GATE_UART_RX_PIN                HW_UART1_RX_PIN
#define GATE_RS485_EN_PIN               HW_WB_IO2_PIN
#define GATE_RS485_DE_PIN               HW_WB_IO1_PIN

#endif

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
 * Previously Discovered Device Parameters
 * ============================================================ */
#define GATE_DISCOVERED_SLAVE           1
#define GATE_DISCOVERED_BAUD            4800
#define GATE_DISCOVERED_PARITY          0

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
 * Modbus RTU Protocol Constants
 * ============================================================ */
#define GATE_MODBUS_FUNC_READ_HOLD      0x03
#define GATE_MODBUS_REQ_FRAME_LEN       8
#define GATE_MODBUS_RESP_MIN_LEN        5
#define GATE_MODBUS_RESP_MAX_LEN        32
#define GATE_MODBUS_EXCEPTION_BIT       0x80

/* ============================================================
 * CRC-16/MODBUS Constants
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
 * Gate 10 Secure Downlink Test Data
 * ============================================================ */
#define GATE_SDL_NONCE_KEY              "sdl_nonce"
#define GATE_SDL_INITIAL_NONCE          0
#define GATE_SDL_TEST_EXPECTED_NONCE    1

#define GATE_SDL_TEST_KEY               { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, \
                                          0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, \
                                          0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, \
                                          0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F }

#define GATE_SDL_TEST_FRAME_LEN         16
#define GATE_SDL_TEST_FRAME             { 0xD1, 0x02, 0x01, 0x00, \
                                          0x00, 0x00, 0x00, 0x01, \
                                          0x00, 0x00, 0x27, 0x10, \
                                          0xA2, 0x7F, 0xF4, 0xAB }

#define GATE_SDL_TEST_EXPECTED_CMD      0x01
#define GATE_SDL_TEST_EXPECTED_INTERVAL 10000

#define GATE_SDL_BAD_MAGIC_FRAME        { 0xFF, 0x02, 0x01, 0x00, \
                                          0x00, 0x00, 0x00, 0x02, \
                                          0x00, 0x00, 0x27, 0x10, \
                                          0x00, 0x00, 0x00, 0x00 }

#define GATE_SDL_BAD_HMAC_FRAME         { 0xD1, 0x02, 0x01, 0x00, \
                                          0x00, 0x00, 0x00, 0x02, \
                                          0x00, 0x00, 0x27, 0x10, \
                                          0x00, 0x00, 0x00, 0x00 }

/* ============================================================
 * Pass/Fail Codes
 * ============================================================ */
#define GATE_PASS                       0
#define GATE_FAIL_INIT                  1
#define GATE_FAIL_VALID_REJECTED        2
#define GATE_FAIL_WRONG_CMD             3
#define GATE_FAIL_REPLAY_ACCEPTED       4
#define GATE_FAIL_REPLAY_FLAG           5
#define GATE_FAIL_BAD_VERSION_ACCEPTED  6
#define GATE_FAIL_VERSION_FLAG          7
#define GATE_FAIL_BAD_HMAC_ACCEPTED     8
#define GATE_FAIL_AUTH_FLAG             9
#define GATE_FAIL_NONCE_PERSIST         10
#define GATE_FAIL_DL_TIMEOUT            11
#define GATE_FAIL_DL_INVALID            12
#define GATE_FAIL_DL_REPLAY_ACCEPTED    13
#define GATE_FAIL_SYSTEM_CRASH          14
