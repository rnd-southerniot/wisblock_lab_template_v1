/**
 * @file main.cpp
 * @brief RAK4631 Gate 4 — Modbus Robustness Layer Example
 * @version 1.0
 * @date 2026-02-24
 *
 * Standalone example demonstrating Modbus RTU robustness features on
 * the RAK4631 WisBlock Core (nRF52840 + SX1262).
 *
 * Features:
 *   - Multi-register read (FC 0x03, qty=2)
 *   - Retry mechanism (3 retries, 4 total attempts)
 *   - Error classification per attempt (NONE/TIMEOUT/CRC_FAIL/etc.)
 *   - UART recovery after 2 consecutive retryable failures
 *   - byte_count == 2 * quantity validation
 *
 * Hardware:
 *   - RAK4631 WisBlock Core (nRF52840 @ 64 MHz)
 *   - RAK5802 WisBlock RS485 Module (TP8485E, auto DE/RE)
 *   - RAK19007 WisBlock Base Board
 *   - External Modbus RTU slave (e.g., RS-FSJT-N01 wind sensor)
 *
 * Discovered device: Slave=1, Baud=4800, Parity=NONE (8N1)
 */

#include <Arduino.h>
#include "pin_map.h"

/* ---- Error Classification ---- */
enum ErrorType {
    ERR_NONE        = 0,
    ERR_TIMEOUT     = 1,
    ERR_CRC_FAIL    = 2,
    ERR_LENGTH_FAIL = 3,
    ERR_EXCEPTION   = 4,
    ERR_UART_ERROR  = 5
};

static const char* err_name(ErrorType e) {
    switch (e) {
        case ERR_NONE:        return "NONE";
        case ERR_TIMEOUT:     return "TIMEOUT";
        case ERR_CRC_FAIL:    return "CRC_FAIL";
        case ERR_LENGTH_FAIL: return "LENGTH_FAIL";
        case ERR_EXCEPTION:   return "EXCEPTION";
        case ERR_UART_ERROR:  return "UART_ERROR";
        default:              return "UNKNOWN";
    }
}

/* ---- CRC-16/MODBUS ---- */
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

/* ---- Hex dump ---- */
static void print_hex(const uint8_t *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        if (buf[i] < 0x10) Serial.print("0");
        Serial.print(buf[i], HEX);
        if (i < len - 1) Serial.print(" ");
    }
}

/* ---- UART read with timeout + inter-byte gap ---- */
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

/* ---- UART flush ---- */
static void uart_flush(void) {
    Serial1.flush();
    delay(10);
    unsigned long t = millis();
    while (Serial1.available() && (millis() - t) < 100) {
        Serial1.read();
    }
}

/* ---- Main ---- */
void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }
    delay(1000);

    Serial.println();
    Serial.println("========================================");
    Serial.println("[SYSTEM] RAK4631 Modbus Robustness Layer");
    Serial.println("========================================");

    /* Enable RAK5802 */
    pinMode(PIN_RS485_EN, OUTPUT);
    digitalWrite(PIN_RS485_EN, HIGH);
    delay(300);
    Serial.println("[INFO] RS485 powered (WB_IO2=34 HIGH)");
    Serial.println("[INFO] DE/RE: auto (RAK5802 hardware)");

    /* Init Serial1 */
    Serial1.begin(MODBUS_BAUD, SERIAL_8N1);
    delay(50);
    while (Serial1.available()) Serial1.read();

    Serial.print("[INFO] Serial1 TX=");
    Serial.print(PIN_UART1_TX);
    Serial.print(" RX=");
    Serial.println(PIN_UART1_RX);
    Serial.print("[INFO] ");
    Serial.print(MODBUS_BAUD);
    Serial.println(" 8N1");

    /* Build Modbus request: Read 2 Holding Registers starting at 0x0000 */
    uint8_t tx[8];
    tx[0] = MODBUS_SLAVE;
    tx[1] = 0x03;                 /* FC: Read Holding Registers */
    tx[2] = 0x00; tx[3] = 0x00;  /* Start Reg 0x0000 */
    tx[4] = 0x00; tx[5] = MODBUS_QUANTITY;  /* Qty */
    uint16_t crc = modbus_crc16(tx, 6);
    tx[6] = (uint8_t)(crc & 0xFF);
    tx[7] = (uint8_t)((crc >> 8) & 0xFF);

    Serial.print("[INFO] Request: FC=0x03, Reg=0x0000, Qty=");
    Serial.print(MODBUS_QUANTITY);
    Serial.print(", MaxRetries=");
    Serial.println(RETRY_MAX);

    /* ---- Retry Loop ---- */
    uint8_t rx[32];
    uint8_t rx_len = 0;
    ErrorType last_err = ERR_NONE;
    uint8_t consec_fails = 0;
    uint8_t uart_recoveries = 0;
    bool success = false;
    uint8_t final_attempt = 0;

    for (uint8_t attempt = 0; attempt <= RETRY_MAX; attempt++) {

        /* Pre-retry actions */
        if (attempt > 0) {
            Serial.print("[RETRY] ");
            Serial.print(attempt);
            Serial.print("/");
            Serial.print(RETRY_MAX);
            Serial.print(" (delay=");
            Serial.print(RETRY_DELAY_MS);
            Serial.print("ms, flush");

            delay(RETRY_DELAY_MS);
            uart_flush();

            /* UART recovery after consecutive failures */
            if (consec_fails >= CONSEC_FAIL_REINIT) {
                Serial.print(", UART re-init");
                delay(50);
                Serial1.begin(MODBUS_BAUD, SERIAL_8N1);
                delay(50);
                uart_flush();
                consec_fails = 0;
                uart_recoveries++;
            }
            Serial.println(")");
        }

        Serial.print("[ATTEMPT] ");
        Serial.print(attempt + 1);
        Serial.print("/");
        Serial.print(RETRY_MAX + 1);
        Serial.println(":");

        uart_flush();
        delay(10);

        /* TX */
        Serial.print("  TX (8 bytes): ");
        print_hex(tx, 8);
        Serial.println();

        size_t written = Serial1.write(tx, 8);
        Serial1.flush();
        delayMicroseconds(500);

        if (written != 8) {
            last_err = ERR_UART_ERROR;
            Serial.print("  Error: ");
            Serial.println(err_name(last_err));
            break;  /* Non-retryable */
        }

        /* RX */
        rx_len = uart_read(rx, sizeof(rx), RESPONSE_TIMEOUT);

        /* Classify */
        if (rx_len == 0) {
            last_err = ERR_TIMEOUT;
            Serial.print("  RX: timeout (");
            Serial.print(RESPONSE_TIMEOUT);
            Serial.println("ms)");
            Serial.print("  Error: ");
            Serial.println(err_name(last_err));
            consec_fails++;
            continue;
        }

        Serial.print("  RX (");
        Serial.print(rx_len);
        Serial.print(" bytes): ");
        print_hex(rx, rx_len);
        Serial.println();

        if (rx_len < 5) {
            last_err = ERR_LENGTH_FAIL;
            Serial.print("  Error: ");
            Serial.println(err_name(last_err));
            break;  /* Non-retryable */
        }

        /* CRC check */
        uint16_t calc = modbus_crc16(rx, rx_len - 2);
        uint16_t recv = (uint16_t)rx[rx_len - 2] | ((uint16_t)rx[rx_len - 1] << 8);
        if (calc != recv) {
            last_err = ERR_CRC_FAIL;
            Serial.print("  CRC mismatch: calc=0x");
            Serial.print(calc, HEX);
            Serial.print(" recv=0x");
            Serial.println(recv, HEX);
            Serial.print("  Error: ");
            Serial.println(err_name(last_err));
            consec_fails++;
            continue;  /* Retryable */
        }

        /* Exception check */
        if (rx[1] & 0x80) {
            last_err = ERR_EXCEPTION;
            Serial.print("  Exception code=0x");
            if (rx[2] < 0x10) Serial.print("0");
            Serial.println(rx[2], HEX);
            Serial.print("  Error: ");
            Serial.println(err_name(last_err));
            break;  /* Non-retryable */
        }

        /* Slave + function match */
        if (rx[0] != MODBUS_SLAVE || rx[1] != 0x03) {
            last_err = ERR_LENGTH_FAIL;
            Serial.print("  Error: ");
            Serial.println(err_name(last_err));
            break;  /* Non-retryable */
        }

        /* byte_count check */
        uint8_t byte_count = rx[2];
        uint8_t expected_bc = 2 * MODBUS_QUANTITY;
        if (byte_count != expected_bc) {
            last_err = ERR_LENGTH_FAIL;
            Serial.print("  byte_count=");
            Serial.print(byte_count);
            Serial.print(" expected=");
            Serial.println(expected_bc);
            Serial.print("  Error: ");
            Serial.println(err_name(last_err));
            break;  /* Non-retryable */
        }

        /* SUCCESS */
        last_err = ERR_NONE;
        success = true;
        final_attempt = attempt + 1;
        Serial.print("  Parse: OK (byte_count=");
        Serial.print(byte_count);
        Serial.print(", expected=");
        Serial.print(expected_bc);
        Serial.println(")");
        Serial.print("  Error: ");
        Serial.println(err_name(last_err));
        break;
    }

    /* ---- Results ---- */
    Serial.println();
    if (success) {
        Serial.print("[RESULT] SUCCESS on attempt ");
        Serial.print(final_attempt);
        Serial.print(" (");
        Serial.print(final_attempt - 1);
        Serial.print(" retries, ");
        Serial.print(uart_recoveries);
        Serial.println(" UART recoveries)");

        /* Extract register values */
        for (uint8_t i = 0; i < MODBUS_QUANTITY; i++) {
            uint16_t val = ((uint16_t)rx[3 + i * 2] << 8) | (uint16_t)rx[4 + i * 2];
            Serial.print("[RESULT] Register[");
            Serial.print(i);
            Serial.print("] = 0x");
            if (val < 0x1000) Serial.print("0");
            if (val < 0x0100) Serial.print("0");
            if (val < 0x0010) Serial.print("0");
            Serial.print(val, HEX);
            Serial.print(" (");
            Serial.print(val);
            Serial.println(")");
        }

        Serial.println("[RESULT] PASS");

        /* LED blink */
        pinMode(PIN_LED_BLUE, OUTPUT);
        digitalWrite(PIN_LED_BLUE, HIGH);
        delay(500);
        digitalWrite(PIN_LED_BLUE, LOW);
    } else {
        Serial.print("[RESULT] FAILED (last error: ");
        Serial.print(err_name(last_err));
        Serial.println(")");
    }
}

void loop() {
    delay(10000);
}
