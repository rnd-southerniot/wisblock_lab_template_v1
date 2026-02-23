/**
 * @file gate_runner.cpp
 * @brief Gate 5 — LoRaWAN Join + Uplink Validation Runner
 * @version 1.0
 * @date 2026-02-24
 *
 * Validates LoRaWAN OTAA join, Modbus sensor read, and uplink TX:
 *   1. Initialize RAK Arduino LoRaWAN stack (BSP handles SX1262 hardware)
 *   2. Configure AS923-1 region (via lmh_init)
 *   3. Set OTAA credentials (DevEUI, AppEUI, AppKey)
 *   4. Initiate OTAA join (async, LoRaMAC handles retries)
 *   5. Wait for join accept callback (30s timeout)
 *   6. Read 2 Modbus registers from wind speed sensor (with retries)
 *   7. Build 4-byte uplink payload (big-endian register values)
 *   8. Send unconfirmed uplink on port 10
 *   9. Print structured summary
 *
 * Reuses Gate 3's modbus_frame (CRC, frame builder, 7-step parser)
 * and hal_uart (Serial1 + RAK5802 RS485 control) without duplication.
 *
 * Entry point: gate_lorawan_join_uplink_run()
 *
 * All parameters come from gate_config.h.
 * Zero hardcoded device or credential values in this file.
 */

#include <Arduino.h>
#include <LoRaWan-Arduino.h>    /* RAK Arduino LoRaWAN stack (BSP-provided) */
#include "gate_config.h"
#include "../gate_modbus_minimal_protocol/modbus_frame.h"
#include "../gate_modbus_minimal_protocol/hal_uart.h"

/* ============================================================
 * Gate Result State
 * ============================================================ */
static uint8_t gate_result = GATE_FAIL_SYSTEM_CRASH;

/* ============================================================
 * LoRaWAN Join State (set by async callbacks)
 * ============================================================ */
static volatile bool g_joined      = false;
static volatile bool g_join_failed = false;

/* ============================================================
 * OTAA Credentials (global arrays — required by LoRaWAN stack)
 * ============================================================ */
static uint8_t nodeDeviceEUI[8]  = GATE_LORAWAN_DEV_EUI;
static uint8_t nodeAppEUI[8]     = GATE_LORAWAN_APP_EUI;
static uint8_t nodeAppKey[16]    = GATE_LORAWAN_APP_KEY;

/* ============================================================
 * Modbus Read State
 * ============================================================ */
static uint16_t reg_values[GATE_MODBUS_QUANTITY];

/* ============================================================
 * LoRaWAN Callbacks (static)
 *
 * Join events are async — gate runner polls volatile bool flags.
 * ============================================================ */
static void lorawan_has_joined_handler(void) {
    g_joined = true;
}

static void lorawan_join_failed_handler(void) {
    g_join_failed = true;
}

static void lorawan_rx_handler(lmh_app_data_t *app_data) {
    /* Not used in gate test — no downlink handling */
    (void)app_data;
}

static void lorawan_confirm_class_handler(DeviceClass_t Class) {
    /* Class A only — no class change handling */
    (void)Class;
}

/* ============================================================
 * LoRaWAN Parameter Struct
 * ============================================================ */
static lmh_param_t lora_param_init = {
    GATE_LORAWAN_ADR,
    GATE_LORAWAN_DATARATE,
    GATE_LORAWAN_PUBLIC_NETWORK,
    GATE_LORAWAN_JOIN_TRIALS,
    GATE_LORAWAN_TX_POWER,
    LORAWAN_DUTYCYCLE_OFF
};

/* ============================================================
 * LoRaWAN Callback Table
 * ============================================================ */
static lmh_callback_t lora_callbacks = {
    BoardGetBatteryLevel,
    BoardGetUniqueId,
    BoardGetRandomSeed,
    lorawan_rx_handler,
    lorawan_has_joined_handler,
    lorawan_confirm_class_handler,
    lorawan_join_failed_handler
};

/* ============================================================
 * Helper — Print byte array as hex with spaces
 * ============================================================ */
static void print_hex(const uint8_t *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        if (i > 0) Serial.print(' ');
        if (buf[i] < 0x10) Serial.print('0');
        Serial.print(buf[i], HEX);
    }
}

/* ============================================================
 * Helper — Print EUI bytes as colon-separated hex
 * ============================================================ */
static void print_eui(const uint8_t *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        if (i > 0) Serial.print(':');
        if (buf[i] < 0x10) Serial.print('0');
        Serial.print(buf[i], HEX);
    }
}

/* ============================================================
 * STEP 1 — LoRaWAN Stack Init
 *
 * On ESP32-S3 (generic board), the SX126x-Arduino library
 * requires explicit hardware pin configuration via hw_config
 * struct + lora_hardware_init() before lmh_init().
 *
 * Pin mapping from gate_config.h (sourced from hardware_profile.h).
 * SX1262 uses DIO2 for antenna switch, DIO3 for TCXO control.
 * ============================================================ */
static bool step_lorawan_init(void) {
    Serial.printf("%s STEP 1: LoRaWAN stack init\r\n", GATE_LOG_TAG);

    /* ---- Configure SX1262 hardware pin mapping ---- */
    hw_config hwConfig;
    hwConfig.CHIP_TYPE         = SX1262_CHIP;
    hwConfig.PIN_LORA_RESET    = GATE_LORA_PIN_RESET;
    hwConfig.PIN_LORA_NSS      = GATE_LORA_PIN_NSS;
    hwConfig.PIN_LORA_SCLK     = GATE_LORA_PIN_SCK;
    hwConfig.PIN_LORA_MISO     = GATE_LORA_PIN_MISO;
    hwConfig.PIN_LORA_DIO_1    = GATE_LORA_PIN_DIO1;
    hwConfig.PIN_LORA_BUSY     = GATE_LORA_PIN_BUSY;
    hwConfig.PIN_LORA_MOSI     = GATE_LORA_PIN_MOSI;
    hwConfig.RADIO_TXEN        = -1;
    hwConfig.RADIO_RXEN        = GATE_LORA_PIN_ANT_SW;
    hwConfig.USE_DIO2_ANT_SWITCH = true;
    hwConfig.USE_DIO3_TCXO      = true;
    hwConfig.USE_DIO3_ANT_SWITCH = false;
    hwConfig.USE_LDO             = false;
    hwConfig.USE_RXEN_ANT_PWR    = false;

    Serial.printf("%s SX1262 pins: NSS=%d SCK=%d MOSI=%d MISO=%d RST=%d DIO1=%d BUSY=%d\r\n",
                  GATE_LOG_TAG,
                  GATE_LORA_PIN_NSS, GATE_LORA_PIN_SCK,
                  GATE_LORA_PIN_MOSI, GATE_LORA_PIN_MISO,
                  GATE_LORA_PIN_RESET, GATE_LORA_PIN_DIO1,
                  GATE_LORA_PIN_BUSY);

    /* ---- Initialize SX1262 hardware (SPI, timers, IRQ) ---- */
    uint32_t hw_err = lora_hardware_init(hwConfig);
    if (hw_err != 0) {
        Serial.printf("%s lora_hardware_init: FAIL (err=%lu)\r\n",
                      GATE_LOG_TAG, (unsigned long)hw_err);
        gate_result = GATE_FAIL_RADIO_INIT;
        return false;
    }
    Serial.printf("%s lora_hardware_init: OK\r\n", GATE_LOG_TAG);

    /* ---- Initialize LoRaMAC handler ---- */
    uint32_t err = lmh_init(&lora_callbacks, lora_param_init, true,
                             GATE_LORAWAN_CLASS, GATE_LORAWAN_REGION);
    if (err != 0) {
        Serial.printf("%s lmh_init: FAIL (err=%lu)\r\n",
                      GATE_LOG_TAG, (unsigned long)err);
        gate_result = GATE_FAIL_RADIO_INIT;
        return false;
    }

    Serial.printf("%s LoRaWAN Init: OK\r\n", GATE_LOG_TAG);
    return true;
}

/* ============================================================
 * STEP 2 — Region Configuration (log only)
 *
 * Region is set via LORAMAC_REGION_AS923 in lmh_init().
 * This step logs the configuration for verification.
 * ============================================================ */
static bool step_region_config(void) {
    Serial.printf("%s STEP 2: Region configuration\r\n", GATE_LOG_TAG);
    Serial.printf("%s Region: AS923-1 (Class A, ADR=OFF, DR=2)\r\n", GATE_LOG_TAG);
    return true;
}

/* ============================================================
 * STEP 3 — Set OTAA Credentials
 *
 * DevEUI, AppEUI, AppKey from gate_config.h placeholders.
 * ============================================================ */
static bool step_set_credentials(void) {
    Serial.printf("%s STEP 3: Set OTAA credentials\r\n", GATE_LOG_TAG);

    lmh_setDevEui(nodeDeviceEUI);
    lmh_setAppEui(nodeAppEUI);
    lmh_setAppKey(nodeAppKey);

    Serial.printf("%s   DevEUI : ", GATE_LOG_TAG);
    print_eui(nodeDeviceEUI, 8);
    Serial.printf("\r\n");

    Serial.printf("%s   AppEUI : ", GATE_LOG_TAG);
    print_eui(nodeAppEUI, 8);
    Serial.printf("\r\n");

    Serial.printf("%s   AppKey : ", GATE_LOG_TAG);
    print_eui(nodeAppKey, 16);
    Serial.printf("\r\n");

    Serial.printf("%s OTAA credentials configured\r\n", GATE_LOG_TAG);
    return true;
}

/* ============================================================
 * STEP 4 — Initiate OTAA Join
 *
 * lmh_join() starts async OTAA join. LoRaMAC handles retries
 * per JOIN_TRIALS setting.
 * ============================================================ */
static bool step_join_start(void) {
    Serial.printf("%s STEP 4: OTAA join (trials=%d, timeout=%ds)\r\n",
                  GATE_LOG_TAG, GATE_LORAWAN_JOIN_TRIALS,
                  GATE_LORAWAN_JOIN_TIMEOUT_MS / 1000);

    g_joined = false;
    g_join_failed = false;

    lmh_join();

    Serial.printf("%s Join Start...\r\n", GATE_LOG_TAG);
    return true;
}

/* ============================================================
 * STEP 5 — Wait for Join Accept
 *
 * Polls volatile bool flags with timeout.
 * g_joined == true     → success, continue
 * g_join_failed == true → FAIL code 3
 * timeout (neither)     → FAIL code 2
 * ============================================================ */
static bool step_join_wait(void) {
    Serial.printf("%s STEP 5: Waiting for join accept...\r\n", GATE_LOG_TAG);

    uint32_t start = millis();
    while (!g_joined && !g_join_failed) {
        if (millis() - start > GATE_LORAWAN_JOIN_TIMEOUT_MS) break;
        delay(100);
    }

    if (g_joined) {
        Serial.printf("%s Join Accept: OK\r\n", GATE_LOG_TAG);
        return true;
    }

    if (g_join_failed) {
        Serial.printf("%s Join Accept: DENIED (join_failed callback)\r\n", GATE_LOG_TAG);
        gate_result = GATE_FAIL_JOIN_DENIED;
        return false;
    }

    /* Timeout — neither flag set */
    Serial.printf("%s Join Accept: TIMEOUT (%ds elapsed)\r\n",
                  GATE_LOG_TAG, GATE_LORAWAN_JOIN_TIMEOUT_MS / 1000);
    gate_result = GATE_FAIL_JOIN_TIMEOUT;
    return false;
}

/* ============================================================
 * STEP 6 — Modbus Read (2 Registers)
 *
 * Simplified from Gate 4: single attempt + 2 retries.
 * Uses Gate 3's shared modbus_frame and hal_uart layers.
 * Own retry logic to avoid Gate 4 banners in Gate 5 output.
 * ============================================================ */
static bool step_modbus_read(void) {
    Serial.printf("%s STEP 6: Modbus read (Slave=%d, Baud=%lu, FC=0x%02X, Reg=0x%02X%02X, Qty=%d)\r\n",
                  GATE_LOG_TAG,
                  GATE_DISCOVERED_SLAVE,
                  (unsigned long)GATE_DISCOVERED_BAUD,
                  GATE_MODBUS_FUNC_READ_HOLD,
                  GATE_MODBUS_TARGET_REG_HI, GATE_MODBUS_TARGET_REG_LO,
                  GATE_MODBUS_QUANTITY);

    /* ---- Enable RS485 transceiver ---- */
    hal_rs485_enable(GATE_RS485_EN_PIN, GATE_RS485_DE_PIN);

    /* ---- Init UART ---- */
    if (!hal_uart_init(GATE_UART_TX_PIN, GATE_UART_RX_PIN,
                       GATE_DISCOVERED_BAUD, GATE_DISCOVERED_PARITY)) {
        Serial.printf("%s UART Init: FAIL\r\n", GATE_LOG_TAG);
        hal_rs485_disable();
        gate_result = GATE_FAIL_MODBUS_INIT;
        return false;
    }

    /* Flush and stabilize */
    hal_uart_flush();
    delay(GATE_UART_STABILIZE_MS);

    /* ---- Build request frame ---- */
    uint8_t tx_buf[GATE_MODBUS_REQ_FRAME_LEN];
    uint8_t rx_buf[GATE_MODBUS_RESP_MAX_LEN];

    modbus_build_read_holding(GATE_DISCOVERED_SLAVE,
                              GATE_MODBUS_TARGET_REG_HI,
                              GATE_MODBUS_TARGET_REG_LO,
                              GATE_MODBUS_QUANTITY_HI,
                              GATE_MODBUS_QUANTITY_LO,
                              tx_buf);

    /* ---- Retry loop (max 3 attempts: 1 initial + 2 retries) ---- */
    bool read_ok = false;
    uint8_t rx_len = 0;

    for (uint8_t attempt = 0; attempt <= GATE_MODBUS_RETRY_MAX; attempt++) {

        if (attempt > 0) {
            Serial.printf("%s   Retry %d/%d (delay=%dms)\r\n",
                          GATE_LOG_TAG, attempt, GATE_MODBUS_RETRY_MAX,
                          GATE_MODBUS_RETRY_DELAY_MS);
            delay(GATE_MODBUS_RETRY_DELAY_MS);
        }

        /* Flush stale RX data */
        hal_uart_flush();
        delay(GATE_UART_FLUSH_DELAY_MS);

        /* Log TX */
        Serial.printf("%s   TX (%d bytes): ", GATE_LOG_TAG, GATE_MODBUS_REQ_FRAME_LEN);
        print_hex(tx_buf, GATE_MODBUS_REQ_FRAME_LEN);
        Serial.printf("\r\n");

        /* Send request */
        if (!hal_uart_write(tx_buf, GATE_MODBUS_REQ_FRAME_LEN)) {
            Serial.printf("%s   TX: FAILED\r\n", GATE_LOG_TAG);
            continue;
        }

        /* Read response */
        rx_len = hal_uart_read(rx_buf, GATE_MODBUS_RESP_MAX_LEN,
                               GATE_MODBUS_RESPONSE_TIMEOUT_MS);

        if (rx_len == 0) {
            Serial.printf("%s   RX: timeout (%dms)\r\n",
                          GATE_LOG_TAG, GATE_MODBUS_RESPONSE_TIMEOUT_MS);
            continue;
        }

        /* Log RX */
        Serial.printf("%s   RX (%d bytes): ", GATE_LOG_TAG, rx_len);
        print_hex(rx_buf, rx_len);
        Serial.printf("\r\n");

        /* Short frame check */
        if (rx_len < GATE_MODBUS_RESP_MIN_LEN) {
            Serial.printf("%s   Parse: short frame (%d < %d)\r\n",
                          GATE_LOG_TAG, rx_len, GATE_MODBUS_RESP_MIN_LEN);
            continue;
        }

        /* CRC check */
        uint16_t calc_crc = modbus_crc16(rx_buf, rx_len - 2);
        uint16_t recv_crc = (uint16_t)rx_buf[rx_len - 2]
                          | ((uint16_t)rx_buf[rx_len - 1] << 8);
        if (calc_crc != recv_crc) {
            Serial.printf("%s   Parse: CRC mismatch (calc=0x%04X, recv=0x%04X)\r\n",
                          GATE_LOG_TAG, calc_crc, recv_crc);
            continue;
        }

        /* Full parse via 7-step parser */
        ModbusResponse resp = modbus_parse_response(
            GATE_DISCOVERED_SLAVE,
            GATE_MODBUS_FUNC_READ_HOLD,
            rx_buf, rx_len);

        if (!resp.valid || resp.exception) {
            Serial.printf("%s   Parse: invalid response\r\n", GATE_LOG_TAG);
            continue;
        }

        /* Validate byte_count == 2 * quantity */
        uint8_t expected_bc = 2 * GATE_MODBUS_QUANTITY;
        if (resp.byte_count != expected_bc) {
            Serial.printf("%s   Parse: byte_count=%d, expected=%d\r\n",
                          GATE_LOG_TAG, resp.byte_count, expected_bc);
            continue;
        }

        /* ---- SUCCESS — extract register values ---- */
        for (uint8_t i = 0; i < GATE_MODBUS_QUANTITY; i++) {
            reg_values[i] = ((uint16_t)resp.raw[3 + i * 2] << 8)
                          | (uint16_t)resp.raw[4 + i * 2];
        }
        read_ok = true;
        break;
    }

    /* ---- Cleanup UART/RS485 ---- */
    hal_uart_deinit();
    hal_rs485_disable();

    if (!read_ok) {
        Serial.printf("%s Modbus Read: FAIL (all retries exhausted)\r\n", GATE_LOG_TAG);
        gate_result = GATE_FAIL_MODBUS_READ;
        return false;
    }

    Serial.printf("%s Modbus Read: OK (%d registers)\r\n",
                  GATE_LOG_TAG, GATE_MODBUS_QUANTITY);
    for (uint8_t i = 0; i < GATE_MODBUS_QUANTITY; i++) {
        Serial.printf("%s   Register[%d] : 0x%04X (%d)\r\n",
                      GATE_LOG_TAG, i, reg_values[i], reg_values[i]);
    }

    return true;
}

/* ============================================================
 * STEP 7 — Build Uplink Payload
 *
 * Pack 2 register values as 4 bytes, big-endian:
 *   payload[0] = reg0 >> 8    (Reg0 MSB)
 *   payload[1] = reg0 & 0xFF  (Reg0 LSB)
 *   payload[2] = reg1 >> 8    (Reg1 MSB)
 *   payload[3] = reg1 & 0xFF  (Reg1 LSB)
 * ============================================================ */
static uint8_t payload[GATE_PAYLOAD_LEN];

static bool step_build_payload(void) {
    Serial.printf("%s STEP 7: Build uplink payload\r\n", GATE_LOG_TAG);

    payload[0] = (uint8_t)(reg_values[0] >> 8);
    payload[1] = (uint8_t)(reg_values[0] & 0xFF);
    payload[2] = (uint8_t)(reg_values[1] >> 8);
    payload[3] = (uint8_t)(reg_values[1] & 0xFF);

    Serial.printf("%s Payload (%d bytes): ", GATE_LOG_TAG, GATE_PAYLOAD_LEN);
    print_hex(payload, GATE_PAYLOAD_LEN);
    Serial.printf("\r\n");

    Serial.printf("%s   Port: %d\r\n", GATE_LOG_TAG, GATE_PAYLOAD_PORT);
    Serial.printf("%s   Confirmed: false\r\n", GATE_LOG_TAG);

    return true;
}

/* ============================================================
 * STEP 8 — Send Unconfirmed Uplink
 *
 * Uses lmh_send() with LMH_UNCONFIRMED_MSG on configured port.
 * ============================================================ */
static bool step_send_uplink(void) {
    Serial.printf("%s STEP 8: Send uplink\r\n", GATE_LOG_TAG);
    Serial.printf("%s Uplink TX...\r\n", GATE_LOG_TAG);

    lmh_app_data_t m_lora_app_data;
    m_lora_app_data.buffer   = payload;
    m_lora_app_data.buffsize = GATE_PAYLOAD_LEN;
    m_lora_app_data.port     = GATE_PAYLOAD_PORT;

    lmh_error_status err = lmh_send(&m_lora_app_data, GATE_LORAWAN_CONFIRMED);

    if (err != LMH_SUCCESS) {
        Serial.printf("%s Uplink TX: FAIL (err=%d)\r\n", GATE_LOG_TAG, (int)err);
        gate_result = GATE_FAIL_UPLINK_TX;
        return false;
    }

    Serial.printf("%s Uplink TX: OK (port=%d, %d bytes)\r\n",
                  GATE_LOG_TAG, GATE_PAYLOAD_PORT, GATE_PAYLOAD_LEN);
    return true;
}

/* ============================================================
 * STEP 9 — Gate Summary
 * ============================================================ */
static void step_summary(void) {
    Serial.printf("%s STEP 9: Gate summary\r\n", GATE_LOG_TAG);
    Serial.printf("%s   Radio        : SX1262\r\n", GATE_LOG_TAG);
    Serial.printf("%s   Region       : AS923-1\r\n", GATE_LOG_TAG);
    Serial.printf("%s   Activation   : OTAA\r\n", GATE_LOG_TAG);
    Serial.printf("%s   Join Status  : OK\r\n", GATE_LOG_TAG);
    Serial.printf("%s   Modbus Slave : %d\r\n", GATE_LOG_TAG, GATE_DISCOVERED_SLAVE);

    for (uint8_t i = 0; i < GATE_MODBUS_QUANTITY; i++) {
        Serial.printf("%s   Register[%d]  : 0x%04X (%d)\r\n",
                      GATE_LOG_TAG, i, reg_values[i], reg_values[i]);
    }

    Serial.printf("%s   Payload Hex  : ", GATE_LOG_TAG);
    print_hex(payload, GATE_PAYLOAD_LEN);
    Serial.printf("\r\n");

    Serial.printf("%s   Uplink Port  : %d\r\n", GATE_LOG_TAG, GATE_PAYLOAD_PORT);
    Serial.printf("%s   Uplink Status: OK\r\n", GATE_LOG_TAG);
}

/* ============================================================
 * Gate Entry Point
 * ============================================================ */
void gate_lorawan_join_uplink_run(void) {
    gate_result = GATE_FAIL_SYSTEM_CRASH;
    g_joined = false;
    g_join_failed = false;
    memset(reg_values, 0, sizeof(reg_values));
    memset(payload, 0, sizeof(payload));

    /* ---- GATE START banner ---- */
    Serial.printf("\r\n%s === GATE START: %s v%s ===\r\n",
                  GATE_LOG_TAG, GATE_NAME, GATE_VERSION);

    /* ---- Step 1: LoRaWAN stack init ---- */
    if (!step_lorawan_init()) goto done;

    /* ---- Step 2: Region configuration (log) ---- */
    if (!step_region_config()) goto done;

    /* ---- Step 3: Set OTAA credentials ---- */
    if (!step_set_credentials()) goto done;

    /* ---- Step 4: Initiate OTAA join ---- */
    if (!step_join_start()) goto done;

    /* ---- Step 5: Wait for join accept ---- */
    if (!step_join_wait()) goto done;

    /* ---- Step 6: Modbus read (2 registers) ---- */
    if (!step_modbus_read()) goto done;

    /* ---- Step 7: Build payload ---- */
    if (!step_build_payload()) goto done;

    /* ---- Step 8: Send uplink ---- */
    if (!step_send_uplink()) goto done;

    /* ---- Step 9: Summary (only on success) ---- */
    step_summary();

    /* ---- All steps passed ---- */
    gate_result = GATE_PASS;

done:
    /* ---- Result ---- */
    if (gate_result == GATE_PASS) {
        Serial.printf("%s PASS\r\n", GATE_LOG_TAG);
    } else {
        Serial.printf("%s FAIL (code=%d)\r\n", GATE_LOG_TAG, gate_result);
    }

    Serial.printf("%s === GATE COMPLETE ===\r\n", GATE_LOG_TAG);
}
