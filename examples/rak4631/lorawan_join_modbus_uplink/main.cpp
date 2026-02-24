/**
 * @file main.cpp
 * @brief RAK4631 Gate 5 — LoRaWAN OTAA Join + Modbus Uplink Example
 * @version 1.0
 * @date 2026-02-25
 *
 * Standalone example demonstrating end-to-end LoRaWAN uplink from
 * sensor to network server on the RAK4631 WisBlock Core (nRF52840 + SX1262).
 *
 * Flow:
 *   1. Init SX1262 via lora_rak4630_init() (BSP one-liner)
 *   2. OTAA join to ChirpStack v4 gateway on AS923-1
 *   3. Modbus RTU read (2 registers from wind speed sensor)
 *   4. Encode 4-byte payload (big-endian register values)
 *   5. Send unconfirmed uplink on port 10
 *
 * Hardware:
 *   - RAK4631 WisBlock Core (nRF52840 @ 64 MHz + SX1262)
 *   - RAK5802 WisBlock RS485 Module (TP8485E, auto DE/RE)
 *   - RAK19007 WisBlock Base Board
 *   - External Modbus RTU slave (e.g., RS-FSJT-N01 wind sensor)
 *   - ChirpStack v4 gateway within LoRa range
 *
 * Build flags required: -DRAK4630 -D_VARIANT_RAK4630_
 * Lib deps: beegee-tokyo/SX126x-Arduino@^2.0.32
 */

#include <Arduino.h>
#include <LoRaWan-Arduino.h>
#include "pin_map.h"

/* ================================================================
 * OTAA Credentials — UPDATE THESE FOR YOUR DEVICE
 * ================================================================ */
static uint8_t nodeDeviceEUI[8]  = { 0x88, 0x82, 0x24, 0x44, 0xAE, 0xED, 0x1E, 0xB2 };
static uint8_t nodeAppEUI[8]     = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t nodeAppKey[16]    = { 0x91, 0x8C, 0x49, 0x08, 0xF0, 0x89, 0x50, 0x6B,
                                     0x30, 0x18, 0x0B, 0x62, 0x65, 0x9A, 0x4A, 0xD5 };

/* ================================================================
 * Join State (set by async callbacks)
 * ================================================================ */
static volatile bool g_joined      = false;
static volatile bool g_join_failed = false;

/* ================================================================
 * LoRaWAN Callbacks
 * ================================================================ */
static void lorawan_has_joined(void)    { g_joined = true; }
static void lorawan_join_failed(void)   { g_join_failed = true; }
static void lorawan_rx(lmh_app_data_t *data) { (void)data; }
static void lorawan_class_confirm(DeviceClass_t Class) { (void)Class; }

static lmh_param_t lora_param = {
    LORAWAN_ADR_OFF,       /* ADR */
    DR_2,                  /* Data rate (SF10) */
    true,                  /* Public network */
    3,                     /* Join trials */
    TX_POWER_0,            /* TX power */
    LORAWAN_DUTYCYCLE_OFF  /* Duty cycle */
};

static lmh_callback_t lora_callbacks = {
    BoardGetBatteryLevel,
    BoardGetUniqueId,
    BoardGetRandomSeed,
    lorawan_rx,
    lorawan_has_joined,
    lorawan_class_confirm,
    lorawan_join_failed
};

/* ================================================================
 * CRC-16/MODBUS
 * ================================================================ */
static uint16_t modbus_crc16(const uint8_t *data, uint8_t len) {
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 1) ? ((crc >> 1) ^ 0xA001) : (crc >> 1);
        }
    }
    return crc;
}

/* ================================================================
 * Hex dump helper
 * ================================================================ */
static void print_hex(const uint8_t *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        if (i > 0) Serial.print(' ');
        if (buf[i] < 0x10) Serial.print('0');
        Serial.print(buf[i], HEX);
    }
}

/* ================================================================
 * EUI print helper (colon-separated)
 * ================================================================ */
static void print_eui(const uint8_t *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        if (i > 0) Serial.print(':');
        if (buf[i] < 0x10) Serial.print('0');
        Serial.print(buf[i], HEX);
    }
}

/* ================================================================
 * UART read with timeout + inter-byte gap
 * ================================================================ */
static uint8_t uart_read(uint8_t *buf, uint8_t max_len, uint32_t timeout_ms) {
    unsigned long start = millis();
    while (!Serial1.available()) {
        if ((millis() - start) >= timeout_ms) return 0;
    }
    uint8_t count = 0;
    unsigned long last_byte = millis();
    while (count < max_len) {
        if (Serial1.available()) {
            buf[count++] = Serial1.read();
            last_byte = millis();
        } else if ((millis() - last_byte) >= 5) {
            break;
        }
    }
    return count;
}

/* ================================================================
 * UART flush
 * ================================================================ */
static void uart_flush(void) {
    Serial1.flush();
    delay(10);
    unsigned long t = millis();
    while (Serial1.available() && (millis() - t) < 100) {
        Serial1.read();
    }
}

/* ================================================================
 * Main
 * ================================================================ */
void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }
    delay(1000);

    Serial.println();
    Serial.println("========================================");
    Serial.println("[SYSTEM] RAK4631 LoRaWAN + Modbus Uplink");
    Serial.println("========================================");

    /* ---- Step 1: LoRaWAN init ---- */
    Serial.println("[LORA] Initializing SX1262...");
    uint32_t hw_err = lora_rak4630_init();
    if (hw_err != 0) {
        Serial.print("[LORA] lora_rak4630_init FAILED: ");
        Serial.println(hw_err);
        return;
    }
    Serial.println("[LORA] lora_rak4630_init: OK");

    uint32_t lmh_err = lmh_init(&lora_callbacks, lora_param, true,
                                  CLASS_A, LORAMAC_REGION_AS923);
    if (lmh_err != 0) {
        Serial.print("[LORA] lmh_init FAILED: ");
        Serial.println(lmh_err);
        return;
    }
    Serial.println("[LORA] lmh_init: OK");
    Serial.println("[LORA] Region: AS923-1, Class A, DR2, ADR=OFF");

    /* ---- Step 2: Set credentials ---- */
    lmh_setDevEui(nodeDeviceEUI);
    lmh_setAppEui(nodeAppEUI);
    lmh_setAppKey(nodeAppKey);

    Serial.print("[LORA] DevEUI: ");
    print_eui(nodeDeviceEUI, 8);
    Serial.println();

    /* ---- Step 3: OTAA join ---- */
    Serial.println("[LORA] Starting OTAA join...");
    g_joined = false;
    g_join_failed = false;
    lmh_join();

    unsigned long join_start = millis();
    while (!g_joined && !g_join_failed) {
        if (millis() - join_start > LORAWAN_JOIN_TIMEOUT) break;
        delay(100);
    }

    if (!g_joined) {
        if (g_join_failed) {
            Serial.println("[LORA] Join DENIED");
        } else {
            Serial.println("[LORA] Join TIMEOUT (30s)");
        }
        return;
    }

    unsigned long join_ms = millis() - join_start;
    Serial.print("[LORA] Join Accept: OK (");
    Serial.print(join_ms);
    Serial.println("ms)");

    /* ---- Step 4: Modbus read ---- */
    Serial.println("[MODBUS] Initializing RS485...");
    pinMode(PIN_RS485_EN, OUTPUT);
    digitalWrite(PIN_RS485_EN, HIGH);
    delay(300);
    Serial.println("[MODBUS] RS485 powered (WB_IO2=34 HIGH, auto DE/RE)");

    Serial1.begin(MODBUS_BAUD, SERIAL_8N1);
    delay(50);
    uart_flush();

    /* Build request: Read 2 Holding Registers from 0x0000 */
    uint8_t tx[8];
    tx[0] = MODBUS_SLAVE;
    tx[1] = 0x03;
    tx[2] = 0x00; tx[3] = 0x00;  /* Start Reg */
    tx[4] = 0x00; tx[5] = MODBUS_QUANTITY;
    uint16_t crc = modbus_crc16(tx, 6);
    tx[6] = (uint8_t)(crc & 0xFF);
    tx[7] = (uint8_t)((crc >> 8) & 0xFF);

    uint8_t rx[32];
    uint8_t rx_len = 0;
    bool read_ok = false;
    uint16_t reg_values[MODBUS_QUANTITY];

    for (uint8_t attempt = 0; attempt <= RETRY_MAX; attempt++) {
        if (attempt > 0) {
            Serial.print("[MODBUS] Retry ");
            Serial.print(attempt);
            Serial.println("...");
            delay(RETRY_DELAY_MS);
        }

        uart_flush();
        delay(10);

        Serial.print("[MODBUS] TX: ");
        print_hex(tx, 8);
        Serial.println();

        Serial1.write(tx, 8);
        Serial1.flush();
        delayMicroseconds(500);

        rx_len = uart_read(rx, sizeof(rx), RESPONSE_TIMEOUT);
        if (rx_len == 0) {
            Serial.println("[MODBUS] RX: timeout");
            continue;
        }

        Serial.print("[MODBUS] RX: ");
        print_hex(rx, rx_len);
        Serial.println();

        /* Validate */
        if (rx_len < 5) { Serial.println("[MODBUS] Short frame"); continue; }

        uint16_t calc = modbus_crc16(rx, rx_len - 2);
        uint16_t recv = (uint16_t)rx[rx_len - 2] | ((uint16_t)rx[rx_len - 1] << 8);
        if (calc != recv) { Serial.println("[MODBUS] CRC mismatch"); continue; }

        if (rx[0] != MODBUS_SLAVE || rx[1] != 0x03) {
            Serial.println("[MODBUS] Addr/FC mismatch");
            continue;
        }

        if (rx[1] & 0x80) {
            Serial.print("[MODBUS] Exception: 0x");
            Serial.println(rx[2], HEX);
            break;
        }

        if (rx[2] != 2 * MODBUS_QUANTITY) {
            Serial.println("[MODBUS] byte_count mismatch");
            continue;
        }

        /* Extract register values */
        for (uint8_t i = 0; i < MODBUS_QUANTITY; i++) {
            reg_values[i] = ((uint16_t)rx[3 + i * 2] << 8) | (uint16_t)rx[4 + i * 2];
        }
        read_ok = true;
        break;
    }

    if (!read_ok) {
        Serial.println("[MODBUS] Read FAILED (all retries exhausted)");
        return;
    }

    Serial.print("[MODBUS] Register[0] = 0x");
    Serial.print(reg_values[0], HEX);
    Serial.print(" (");
    Serial.print(reg_values[0]);
    Serial.println(")");
    Serial.print("[MODBUS] Register[1] = 0x");
    Serial.print(reg_values[1], HEX);
    Serial.print(" (");
    Serial.print(reg_values[1]);
    Serial.println(")");

    /* ---- Step 5: Build payload and send uplink ---- */
    uint8_t payload[4];
    payload[0] = (uint8_t)(reg_values[0] >> 8);
    payload[1] = (uint8_t)(reg_values[0] & 0xFF);
    payload[2] = (uint8_t)(reg_values[1] >> 8);
    payload[3] = (uint8_t)(reg_values[1] & 0xFF);

    Serial.print("[UPLINK] Payload: ");
    print_hex(payload, 4);
    Serial.print(" (port=");
    Serial.print(LORAWAN_UPLINK_PORT);
    Serial.println(", unconfirmed)");

    lmh_app_data_t lora_data;
    lora_data.buffer   = payload;
    lora_data.buffsize = 4;
    lora_data.port     = LORAWAN_UPLINK_PORT;

    lmh_error_status tx_err = lmh_send(&lora_data, LMH_UNCONFIRMED_MSG);
    if (tx_err != LMH_SUCCESS) {
        Serial.print("[UPLINK] FAILED (err=");
        Serial.print((int)tx_err);
        Serial.println(")");
        return;
    }

    Serial.println("[UPLINK] TX: OK");

    /* ---- Summary ---- */
    Serial.println();
    Serial.println("[RESULT] PASS");
    Serial.print("[RESULT] Join: OK (");
    Serial.print(join_ms);
    Serial.println("ms)");
    Serial.print("[RESULT] Modbus: Reg[0]=0x");
    Serial.print(reg_values[0], HEX);
    Serial.print(", Reg[1]=0x");
    Serial.println(reg_values[1], HEX);
    Serial.print("[RESULT] Uplink: port=");
    Serial.print(LORAWAN_UPLINK_PORT);
    Serial.print(", payload=");
    print_hex(payload, 4);
    Serial.println();

    /* LED blink */
    pinMode(PIN_LED_BLUE, OUTPUT);
    digitalWrite(PIN_LED_BLUE, HIGH);
    delay(500);
    digitalWrite(PIN_LED_BLUE, LOW);
}

void loop() {
    delay(10000);
}
