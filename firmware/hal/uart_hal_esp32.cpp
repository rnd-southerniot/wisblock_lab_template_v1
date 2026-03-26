/**
 * @file uart_hal_esp32.cpp
 * @brief UART + RS485 HAL — ESP32-S3 (RAK3312) implementation
 * @version 1.0
 * @date 2026-02-25
 *
 * Implements uart_hal.h for the RAK3312 (ESP32-S3) platform.
 * Uses HardwareSerial (Serial1) with explicit pin args to begin().
 * RAK5802 requires manual DE pin control for half-duplex RS485.
 *
 * Guarded by #ifdef PLATFORM_ESP32 — compiles to empty TU on nRF52.
 *
 * Pin definitions (EN, DE, TX, RX) read from gate_config.h at compile time.
 */

#include "platform_select.h"

#ifdef PLATFORM_ESP32

#include <Arduino.h>
#include "uart_hal.h"
#include "gate_config.h"

/* ============================================================
 * Parity Config Mapping
 * ESP32-S3 supports None, Odd, and Even parity
 * ============================================================ */
static uint32_t get_serial_config(uint8_t parity) {
    switch (parity) {
        case 1:  return SERIAL_8O1;  /* Odd parity */
        case 2:  return SERIAL_8E1;  /* Even parity */
        default: return SERIAL_8N1;  /* No parity (default) */
    }
}

/* ============================================================
 * RS485 Enable/Disable
 * RAK5802 on ESP32 needs EN pin (module power) + DE pin (direction).
 * ============================================================ */
void uart_rs485_enable(bool on) {
    if (on) {
        /* Enable RAK5802 module power */
        pinMode(GATE_RS485_EN_PIN, OUTPUT);
        digitalWrite(GATE_RS485_EN_PIN, HIGH);

        /* Configure DE pin — start in RX mode (LOW) */
        pinMode(GATE_RS485_DE_PIN, OUTPUT);
        digitalWrite(GATE_RS485_DE_PIN, LOW);

        delay(100);  /* Allow RAK5802 to power up and stabilize */
    } else {
        digitalWrite(GATE_RS485_DE_PIN, LOW);
        digitalWrite(GATE_RS485_EN_PIN, LOW);
    }
}

/* ============================================================
 * Init — Serial1.begin(baud, config, rx_pin, tx_pin)
 * ESP32 HardwareSerial requires pin args.
 * ============================================================ */
bool uart_init(uint32_t baud, uint8_t parity) {
    uint32_t config = get_serial_config(parity);
    Serial1.begin(baud, config, GATE_UART_RX_PIN, GATE_UART_TX_PIN);
    delay(10);  /* Brief stabilization */
    return true;
}

/* ============================================================
 * Flush — drain TX + RX
 * ============================================================ */
void uart_flush(void) {
    Serial1.flush();           /* Wait for TX to complete */
    while (Serial1.available()) {
        Serial1.read();        /* Drain RX buffer */
    }
}

/* ============================================================
 * Write — manual DE pin control for half-duplex RS485
 * EN HIGH + DE HIGH → write → flush → DE LOW
 * ============================================================ */
size_t uart_write(const uint8_t* data, size_t len) {
    /* Assert EN HIGH + switch to TX mode */
    digitalWrite(GATE_RS485_EN_PIN, HIGH);
    digitalWrite(GATE_RS485_DE_PIN, HIGH);
    delayMicroseconds(100);  /* Pre-TX settling */

    size_t written = Serial1.write(data, len);
    Serial1.flush();  /* Block until TX FIFO + shift register empty */

    delayMicroseconds(100);  /* Post-TX guard */

    /* Switch back to RX mode (EN stays HIGH) */
    digitalWrite(GATE_RS485_DE_PIN, LOW);

    return written;
}

/* ============================================================
 * Read — Phase 1 timeout + Phase 2 inter-byte gap
 * ============================================================ */
int uart_read(uint8_t* out, size_t max_len, uint32_t timeout_ms, uint32_t interbyte_gap_ms) {
    /* Phase 1: Wait for first byte or timeout */
    unsigned long start = millis();
    while (!Serial1.available()) {
        if ((millis() - start) >= timeout_ms) {
            return 0;
        }
    }

    /* Phase 2: Accumulate bytes with inter-byte gap detection */
    int count = 0;
    unsigned long last_byte = millis();
    while ((size_t)count < max_len) {
        if (Serial1.available()) {
            out[count++] = Serial1.read();
            last_byte = millis();
        } else if ((millis() - last_byte) > interbyte_gap_ms) {
            break;  /* Inter-byte gap exceeded — frame complete */
        }
    }

    return count;
}

/* ============================================================
 * Deinit — clean shutdown on ESP32
 * ============================================================ */
void uart_deinit(void) {
    Serial1.end();
}

#endif /* PLATFORM_ESP32 */
