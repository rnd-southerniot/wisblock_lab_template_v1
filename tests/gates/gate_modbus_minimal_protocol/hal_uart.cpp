/**
 * @file hal_uart.cpp
 * @brief Gate-local UART HAL — HardwareSerial implementation (ESP32-S3 / RAK3312)
 * @version 2.1
 * @date 2026-02-24
 *
 * Minimal HardwareSerial wrappers for gate validation only.
 * NOT a production HAL. Scoped to gate_modbus_minimal_protocol.
 * Identical to Gate 2 HAL — copied to avoid gate_config.h name collision.
 *
 * Uses Serial1 (UART1) for RS485 communication via RAK5802.
 *
 * v2.1: RAK5802 with EN pin (module power) + DE pin (half-duplex).
 *       EN HIGH + DE HIGH before TX, DE LOW after TX for RX mode.
 *       100us pre/post delays per reference implementation.
 */

#include <Arduino.h>
#include "hal_uart.h"
#include "gate_config.h"

/* ============================================================
 * RS485 Control Pin State
 * ============================================================ */
static uint8_t rs485_en_pin = 0;
static uint8_t rs485_de_pin = 0;

/* ============================================================
 * Parity Config Mapping
 * ============================================================ */
static uint32_t get_serial_config(uint8_t parity) {
    switch (parity) {
        case 1:  return SERIAL_8O1;  /* Odd parity */
        case 2:  return SERIAL_8E1;  /* Even parity */
        default: return SERIAL_8N1;  /* No parity (default) */
    }
}

/* ============================================================
 * RS485 Enable — power on RAK5802 + configure DE pin
 * ============================================================ */
void hal_rs485_enable(uint8_t en_pin, uint8_t de_pin) {
    rs485_en_pin = en_pin;
    rs485_de_pin = de_pin;

    /* Enable RAK5802 module power */
    pinMode(rs485_en_pin, OUTPUT);
    digitalWrite(rs485_en_pin, HIGH);

    /* Configure DE pin — start in RX mode (LOW) */
    pinMode(rs485_de_pin, OUTPUT);
    digitalWrite(rs485_de_pin, LOW);

    delay(100);  /* Allow RAK5802 to power up and stabilize */
}

/* ============================================================
 * RS485 Disable — power off RAK5802
 * ============================================================ */
void hal_rs485_disable(void) {
    digitalWrite(rs485_de_pin, LOW);
    digitalWrite(rs485_en_pin, LOW);
}

/* ============================================================
 * Init
 * ============================================================ */
bool hal_uart_init(uint8_t tx_pin, uint8_t rx_pin, uint32_t baud, uint8_t parity) {
    uint32_t config = get_serial_config(parity);
    Serial1.begin(baud, config, rx_pin, tx_pin);
    delay(10);  /* Brief stabilization */
    return true;
}

/* ============================================================
 * Deinit
 * ============================================================ */
void hal_uart_deinit(void) {
    Serial1.end();
}

/* ============================================================
 * Write (with EN + DE pin control for half-duplex RS485)
 *
 * EN HIGH + DE HIGH -> write -> flush -> DE LOW
 * 100us pre/post delays per reference implementation.
 * ============================================================ */
bool hal_uart_write(const uint8_t *data, uint8_t len) {
    /* Assert EN HIGH + switch to TX mode */
    digitalWrite(rs485_en_pin, HIGH);
    digitalWrite(rs485_de_pin, HIGH);
    delayMicroseconds(100);  /* Pre-TX settling */

    size_t written = Serial1.write(data, len);
    Serial1.flush();  /* Block until TX FIFO + shift register empty */

    delayMicroseconds(100);  /* Post-TX guard */

    /* Switch back to RX mode (EN stays HIGH) */
    digitalWrite(rs485_de_pin, LOW);

    return (written == len);
}

/* ============================================================
 * Read (with timeout + inter-byte gap frame detection)
 * DE pin is already LOW (RX mode) after write.
 * ============================================================ */
uint8_t hal_uart_read(uint8_t *buf, uint8_t max_len, uint32_t timeout_ms) {
    uint8_t count = 0;
    uint32_t start = millis();

    /* Phase 1: Wait for first byte or timeout */
    while (!Serial1.available()) {
        if ((millis() - start) >= timeout_ms) {
            return 0;  /* Timeout — no data received */
        }
    }

    /* Phase 2: Accumulate bytes with inter-byte gap detection.
     * Modbus RTU uses 3.5-character silent interval as frame delimiter.
     * We use GATE_INTER_BYTE_TIMEOUT_MS (5ms) as a practical threshold. */
    uint32_t last_byte_time = millis();
    while (count < max_len) {
        if (Serial1.available()) {
            buf[count++] = Serial1.read();
            last_byte_time = millis();
        } else if ((millis() - last_byte_time) > GATE_INTER_BYTE_TIMEOUT_MS) {
            break;  /* Inter-byte gap exceeded — frame complete */
        }
    }

    return count;
}

/* ============================================================
 * Flush (TX drain + RX discard)
 * ============================================================ */
void hal_uart_flush(void) {
    Serial1.flush();           /* Wait for TX to complete */
    while (Serial1.available()) {
        Serial1.read();        /* Drain RX buffer */
    }
}

/* ============================================================
 * Available
 * ============================================================ */
uint8_t hal_uart_available(void) {
    return (uint8_t)Serial1.available();
}
