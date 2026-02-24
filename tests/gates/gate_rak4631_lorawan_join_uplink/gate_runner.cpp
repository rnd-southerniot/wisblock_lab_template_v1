/**
 * @file gate_runner.cpp
 * @brief Gate 5 — RAK4631 LoRaWAN OTAA Join + Modbus Uplink Runner
 * @version 1.0
 * @date 2026-02-24
 *
 * Validates LoRaWAN OTAA join, Modbus sensor read, and uplink TX:
 *   1. Initialize SX1262 hardware via lora_rak4630_init() (BSP one-liner)
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
 * and hal_uart (Serial1 + RAK5802 RS485 auto-direction) without duplication.
 *
 * nRF52840 / RAK4631 notes:
 *   - lora_rak4630_init() replaces manual hw_config struct
 *   - Requires -DRAK4630 and -D_VARIANT_RAK4630_ build flags
 *   - RAK5802 auto DE/RE — no manual GPIO toggling
 *   - Serial1.begin(baud, config) — no pin args
 *   - Do NOT call Serial1.end() — hangs on nRF52840
 *   - Serial.print() only — no Serial.printf()
 *
 * Entry point: gate_rak4631_lorawan_join_uplink_run()
 *
 * All parameters come from gate_config.h.
 * Zero hardcoded device or credential values in this file.
 */

#include <Arduino.h>
#include <LoRaWan-Arduino.h>    /* SX126x-Arduino LoRaWAN stack */
#include <string.h>
#include "gate_config.h"
#include "../gate_rak4631_modbus_minimal_protocol/modbus_frame.h"
#include "../gate_rak4631_modbus_minimal_protocol/hal_uart.h"

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
 * Helper — Print 16-bit hex with 0x prefix and leading zeros
 * ============================================================ */
static void print_hex16(uint16_t val) {
    Serial.print("0x");
    if (val < 0x1000) Serial.print('0');
    if (val < 0x0100) Serial.print('0');
    if (val < 0x0010) Serial.print('0');
    Serial.print(val, HEX);
}

/* ============================================================
 * STEP 1 — LoRaWAN Stack Init (RAK4630 one-liner)
 *
 * On RAK4631 (nRF52840), lora_rak4630_init() handles all SX1262
 * pin configuration internally:
 *   RESET=38, NSS=42, SCLK=43, MISO=45, DIO1=47, BUSY=46, MOSI=44
 *   USE_DIO2_ANT_SWITCH=true, USE_DIO3_TCXO=true
 *
 * No manual hw_config struct needed (unlike ESP32-S3).
 * Requires -DRAK4630 build flag to compile lora_rak4630_init().
 * ============================================================ */
static bool step_lorawan_init(void) {
    Serial.println(GATE_LOG_TAG " STEP 1: LoRaWAN stack init");

    /* ---- Initialize SX1262 hardware (RAK4630 convenience function) ---- */
    uint32_t hw_err = lora_rak4630_init();
    if (hw_err != 0) {
        Serial.print(GATE_LOG_TAG "   lora_rak4630_init: FAIL (err=");
        Serial.print(hw_err);
        Serial.println(")");
        gate_result = GATE_FAIL_RADIO_INIT;
        return false;
    }
    Serial.println(GATE_LOG_TAG "   lora_rak4630_init: OK");

    /* ---- Initialize LoRaMAC handler ---- */
    uint32_t err = lmh_init(&lora_callbacks, lora_param_init, true,
                             GATE_LORAWAN_CLASS, GATE_LORAWAN_REGION);
    if (err != 0) {
        Serial.print(GATE_LOG_TAG "   lmh_init: FAIL (err=");
        Serial.print(err);
        Serial.println(")");
        gate_result = GATE_FAIL_RADIO_INIT;
        return false;
    }

    Serial.println(GATE_LOG_TAG "   lmh_init: OK");
    Serial.println(GATE_LOG_TAG "   LoRaWAN Init: OK");
    return true;
}

/* ============================================================
 * STEP 2 — Region Configuration (log only)
 *
 * Region is set via LORAMAC_REGION_AS923 in lmh_init().
 * This step logs the configuration for verification.
 * ============================================================ */
static bool step_region_config(void) {
    Serial.println(GATE_LOG_TAG " STEP 2: Region configuration");
    Serial.println(GATE_LOG_TAG "   Region: AS923-1 (Class A, ADR=OFF, DR=2)");
    return true;
}

/* ============================================================
 * STEP 3 — Set OTAA Credentials
 *
 * DevEUI from Gate 0.5 FICR read: 88:82:24:44:AE:ED:1E:B2
 * AppEUI and AppKey from ChirpStack v4 provisioning.
 * ============================================================ */
static bool step_set_credentials(void) {
    Serial.println(GATE_LOG_TAG " STEP 3: Set OTAA credentials");

    lmh_setDevEui(nodeDeviceEUI);
    lmh_setAppEui(nodeAppEUI);
    lmh_setAppKey(nodeAppKey);

    Serial.print(GATE_LOG_TAG "   DevEUI : ");
    print_eui(nodeDeviceEUI, 8);
    Serial.println();

    Serial.print(GATE_LOG_TAG "   AppEUI : ");
    print_eui(nodeAppEUI, 8);
    Serial.println();

    Serial.print(GATE_LOG_TAG "   AppKey : ");
    print_eui(nodeAppKey, 16);
    Serial.println();

    Serial.println(GATE_LOG_TAG "   OTAA credentials configured");
    return true;
}

/* ============================================================
 * STEP 4 — Initiate OTAA Join
 *
 * lmh_join() starts async OTAA join. LoRaMAC handles retries
 * per JOIN_TRIALS setting.
 * ============================================================ */
static bool step_join_start(void) {
    Serial.print(GATE_LOG_TAG " STEP 4: OTAA join (trials=");
    Serial.print(GATE_LORAWAN_JOIN_TRIALS);
    Serial.print(", timeout=");
    Serial.print(GATE_LORAWAN_JOIN_TIMEOUT_MS / 1000);
    Serial.println("s)");

    g_joined = false;
    g_join_failed = false;

    lmh_join();

    Serial.println(GATE_LOG_TAG "   Join Start...");
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
    Serial.println(GATE_LOG_TAG " STEP 5: Waiting for join accept...");

    uint32_t start = millis();
    while (!g_joined && !g_join_failed) {
        if (millis() - start > GATE_LORAWAN_JOIN_TIMEOUT_MS) break;
        delay(100);
    }

    if (g_joined) {
        uint32_t elapsed = millis() - start;
        Serial.print(GATE_LOG_TAG "   Join Accept: OK (");
        Serial.print(elapsed);
        Serial.println("ms)");
        return true;
    }

    if (g_join_failed) {
        Serial.println(GATE_LOG_TAG "   Join Accept: DENIED (join_failed callback)");
        gate_result = GATE_FAIL_JOIN_DENIED;
        return false;
    }

    /* Timeout — neither flag set */
    Serial.print(GATE_LOG_TAG "   Join Accept: TIMEOUT (");
    Serial.print(GATE_LORAWAN_JOIN_TIMEOUT_MS / 1000);
    Serial.println("s elapsed)");
    gate_result = GATE_FAIL_JOIN_TIMEOUT;
    return false;
}

/* ============================================================
 * STEP 6 — Modbus Read (2 Registers)
 *
 * Simplified from Gate 4: single attempt + 2 retries.
 * Uses Gate 3's shared modbus_frame and hal_uart layers.
 * Own retry logic to avoid Gate 4 banners in Gate 5 output.
 *
 * nRF52840 HAL:
 *   - hal_rs485_enable(en_pin) — 1 arg, auto DE/RE
 *   - hal_uart_init(baud, parity) — 2 args, no pin args
 *   - No hal_uart_deinit() — Serial1.end() hangs
 * ============================================================ */
static bool step_modbus_read(void) {
    Serial.print(GATE_LOG_TAG " STEP 6: Modbus read (Slave=");
    Serial.print(GATE_DISCOVERED_SLAVE);
    Serial.print(", Baud=");
    Serial.print((unsigned long)GATE_DISCOVERED_BAUD);
    Serial.print(", FC=0x03, Reg=0x");
    if (GATE_MODBUS_TARGET_REG_HI < 0x10) Serial.print('0');
    Serial.print(GATE_MODBUS_TARGET_REG_HI, HEX);
    if (GATE_MODBUS_TARGET_REG_LO < 0x10) Serial.print('0');
    Serial.print(GATE_MODBUS_TARGET_REG_LO, HEX);
    Serial.print(", Qty=");
    Serial.print(GATE_MODBUS_QUANTITY);
    Serial.println(")");

    /* ---- Enable RS485 transceiver (auto DE/RE — 1 arg) ---- */
    hal_rs485_enable(GATE_RS485_EN_PIN);
    Serial.println(GATE_LOG_TAG "   RS485 EN=HIGH (auto DE/RE)");

    /* ---- Init UART (no pin args on nRF52840) ---- */
    if (!hal_uart_init(GATE_DISCOVERED_BAUD, GATE_DISCOVERED_PARITY)) {
        Serial.println(GATE_LOG_TAG "   UART Init: FAIL");
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
            Serial.print(GATE_LOG_TAG "   Retry ");
            Serial.print(attempt);
            Serial.print("/");
            Serial.print(GATE_MODBUS_RETRY_MAX);
            Serial.print(" (delay=");
            Serial.print(GATE_MODBUS_RETRY_DELAY_MS);
            Serial.println("ms)");
            delay(GATE_MODBUS_RETRY_DELAY_MS);
        }

        /* Flush stale RX data */
        hal_uart_flush();
        delay(GATE_UART_FLUSH_DELAY_MS);

        /* Log TX */
        Serial.print(GATE_LOG_TAG "   TX (");
        Serial.print(GATE_MODBUS_REQ_FRAME_LEN);
        Serial.print(" bytes): ");
        print_hex(tx_buf, GATE_MODBUS_REQ_FRAME_LEN);
        Serial.println();

        /* Send request */
        if (!hal_uart_write(tx_buf, GATE_MODBUS_REQ_FRAME_LEN)) {
            Serial.println(GATE_LOG_TAG "   TX: FAILED");
            continue;
        }

        /* Read response */
        rx_len = hal_uart_read(rx_buf, GATE_MODBUS_RESP_MAX_LEN,
                               GATE_MODBUS_RESPONSE_TIMEOUT_MS);

        if (rx_len == 0) {
            Serial.print(GATE_LOG_TAG "   RX: timeout (");
            Serial.print(GATE_MODBUS_RESPONSE_TIMEOUT_MS);
            Serial.println("ms)");
            continue;
        }

        /* Log RX */
        Serial.print(GATE_LOG_TAG "   RX (");
        Serial.print(rx_len);
        Serial.print(" bytes): ");
        print_hex(rx_buf, rx_len);
        Serial.println();

        /* Short frame check */
        if (rx_len < GATE_MODBUS_RESP_MIN_LEN) {
            Serial.print(GATE_LOG_TAG "   Parse: short frame (");
            Serial.print(rx_len);
            Serial.print(" < ");
            Serial.print(GATE_MODBUS_RESP_MIN_LEN);
            Serial.println(")");
            continue;
        }

        /* CRC check */
        uint16_t calc_crc = modbus_crc16(rx_buf, rx_len - 2);
        uint16_t recv_crc = (uint16_t)rx_buf[rx_len - 2]
                          | ((uint16_t)rx_buf[rx_len - 1] << 8);
        if (calc_crc != recv_crc) {
            Serial.print(GATE_LOG_TAG "   Parse: CRC mismatch (calc=");
            print_hex16(calc_crc);
            Serial.print(", recv=");
            print_hex16(recv_crc);
            Serial.println(")");
            continue;
        }

        /* Full parse via 7-step parser */
        ModbusResponse resp = modbus_parse_response(
            GATE_DISCOVERED_SLAVE,
            GATE_MODBUS_FUNC_READ_HOLD,
            rx_buf, rx_len);

        if (!resp.valid || resp.exception) {
            Serial.println(GATE_LOG_TAG "   Parse: invalid response");
            continue;
        }

        /* Validate byte_count == 2 * quantity */
        uint8_t expected_bc = 2 * GATE_MODBUS_QUANTITY;
        if (resp.byte_count != expected_bc) {
            Serial.print(GATE_LOG_TAG "   Parse: byte_count=");
            Serial.print(resp.byte_count);
            Serial.print(", expected=");
            Serial.println(expected_bc);
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

    /* No cleanup: Serial1.end() hangs on nRF52840,
     * RS485 module power left on — harmless for gate test */

    if (!read_ok) {
        Serial.println(GATE_LOG_TAG "   Modbus Read: FAIL (all retries exhausted)");
        gate_result = GATE_FAIL_MODBUS_READ;
        return false;
    }

    Serial.print(GATE_LOG_TAG "   Modbus Read: OK (");
    Serial.print(GATE_MODBUS_QUANTITY);
    Serial.println(" registers)");
    for (uint8_t i = 0; i < GATE_MODBUS_QUANTITY; i++) {
        Serial.print(GATE_LOG_TAG "   Register[");
        Serial.print(i);
        Serial.print("] : ");
        print_hex16(reg_values[i]);
        Serial.print(" (");
        Serial.print(reg_values[i]);
        Serial.println(")");
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
    Serial.println(GATE_LOG_TAG " STEP 7: Build uplink payload");

    payload[0] = (uint8_t)(reg_values[0] >> 8);
    payload[1] = (uint8_t)(reg_values[0] & 0xFF);
    payload[2] = (uint8_t)(reg_values[1] >> 8);
    payload[3] = (uint8_t)(reg_values[1] & 0xFF);

    Serial.print(GATE_LOG_TAG "   Payload (");
    Serial.print(GATE_PAYLOAD_LEN);
    Serial.print(" bytes): ");
    print_hex(payload, GATE_PAYLOAD_LEN);
    Serial.println();

    Serial.print(GATE_LOG_TAG "   Port: ");
    Serial.println(GATE_PAYLOAD_PORT);
    Serial.println(GATE_LOG_TAG "   Confirmed: false");

    return true;
}

/* ============================================================
 * STEP 8 — Send Unconfirmed Uplink
 *
 * Uses lmh_send() with LMH_UNCONFIRMED_MSG on configured port.
 * ============================================================ */
static bool step_send_uplink(void) {
    Serial.println(GATE_LOG_TAG " STEP 8: Send uplink");
    Serial.println(GATE_LOG_TAG "   Uplink TX...");

    lmh_app_data_t m_lora_app_data;
    m_lora_app_data.buffer   = payload;
    m_lora_app_data.buffsize = GATE_PAYLOAD_LEN;
    m_lora_app_data.port     = GATE_PAYLOAD_PORT;

    lmh_error_status err = lmh_send(&m_lora_app_data, GATE_LORAWAN_CONFIRMED);

    if (err != LMH_SUCCESS) {
        Serial.print(GATE_LOG_TAG "   Uplink TX: FAIL (err=");
        Serial.print((int)err);
        Serial.println(")");
        gate_result = GATE_FAIL_UPLINK_TX;
        return false;
    }

    Serial.print(GATE_LOG_TAG "   Uplink TX: OK (port=");
    Serial.print(GATE_PAYLOAD_PORT);
    Serial.print(", ");
    Serial.print(GATE_PAYLOAD_LEN);
    Serial.println(" bytes)");
    return true;
}

/* ============================================================
 * STEP 9 — Gate Summary
 * ============================================================ */
static void step_summary(void) {
    Serial.println(GATE_LOG_TAG " STEP 9: Gate summary");
    Serial.println(GATE_LOG_TAG "   Radio        : SX1262 (RAK4630 built-in)");
    Serial.println(GATE_LOG_TAG "   Region       : AS923-1");
    Serial.println(GATE_LOG_TAG "   Activation   : OTAA");
    Serial.println(GATE_LOG_TAG "   Join Status  : OK");

    Serial.print(GATE_LOG_TAG "   Modbus Slave : ");
    Serial.println(GATE_DISCOVERED_SLAVE);

    for (uint8_t i = 0; i < GATE_MODBUS_QUANTITY; i++) {
        Serial.print(GATE_LOG_TAG "   Register[");
        Serial.print(i);
        Serial.print("]  : ");
        print_hex16(reg_values[i]);
        Serial.print(" (");
        Serial.print(reg_values[i]);
        Serial.println(")");
    }

    Serial.print(GATE_LOG_TAG "   Payload Hex  : ");
    print_hex(payload, GATE_PAYLOAD_LEN);
    Serial.println();

    Serial.print(GATE_LOG_TAG "   Uplink Port  : ");
    Serial.println(GATE_PAYLOAD_PORT);
    Serial.println(GATE_LOG_TAG "   Uplink Status: OK");
}

/* ============================================================
 * Gate Entry Point
 * ============================================================ */
void gate_rak4631_lorawan_join_uplink_run(void) {
    gate_result = GATE_FAIL_SYSTEM_CRASH;
    g_joined = false;
    g_join_failed = false;
    memset(reg_values, 0, sizeof(reg_values));
    memset(payload, 0, sizeof(payload));

    /* ---- GATE START banner ---- */
    Serial.println();
    Serial.print(GATE_LOG_TAG " === GATE START: ");
    Serial.print(GATE_NAME);
    Serial.print(" v");
    Serial.print(GATE_VERSION);
    Serial.println(" ===");

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
    Serial.println();
    if (gate_result == GATE_PASS) {
        Serial.println(GATE_LOG_TAG " PASS");

        /* Blink blue LED on PASS */
        pinMode(LED_BLUE, OUTPUT);
        digitalWrite(LED_BLUE, LED_STATE_ON);
        delay(500);
        digitalWrite(LED_BLUE, !LED_STATE_ON);
    } else {
        Serial.print(GATE_LOG_TAG " FAIL (code=");
        Serial.print(gate_result);
        Serial.println(")");
    }

    Serial.println(GATE_LOG_TAG " === GATE COMPLETE ===");
}
