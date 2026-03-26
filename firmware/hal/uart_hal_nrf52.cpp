/**
 * @file uart_hal_nrf52.cpp
 * @brief UART + RS485 HAL — nRF52840 (RAK4631) implementation
 * @version 1.0
 * @date 2026-02-25
 *
 * Implements uart_hal.h for the RAK4631 (nRF52840) platform.
 * Uses Serial1 with pins from variant.h (no pin args to begin()).
 * RAK5802 has auto DE/RE — no manual direction GPIO toggling.
 *
 * Guarded by #ifdef PLATFORM_NRF52 — compiles to empty TU on ESP32.
 *
 * Pin definitions (EN pin) read from gate_config.h at compile time.
 */

#include "platform_select.h"

#ifdef PLATFORM_NRF52

#include <Arduino.h>
#include "uart_hal.h"
#include "gate_config.h"

/* ============================================================
 * Parity Config Mapping
 * nRF52840 UARTE: only NONE and EVEN parity supported
 * ============================================================ */
static uint32_t get_serial_config(uint8_t parity) {
    switch (parity) {
        case 2:  return SERIAL_8E1;
        default: return SERIAL_8N1;
    }
}

/* ============================================================
 * RS485 Enable/Disable
 * RAK5802 auto DE/RE — only EN pin (WB_IO2 = 3V3_S rail) needed.
 * ============================================================ */
void uart_rs485_enable(bool on) {
    if (on) {
        pinMode(GATE_RS485_EN_PIN, OUTPUT);
        digitalWrite(GATE_RS485_EN_PIN, HIGH);
        delay(100);  /* Allow RAK5802 module power-up */
    } else {
        digitalWrite(GATE_RS485_EN_PIN, LOW);
    }
}

/* ============================================================
 * Init — Serial1.begin(baud, config)
 * No pin args on nRF52 Adafruit BSP (variant.h defines them).
 * Do NOT call Serial1.end() — hangs on nRF52 if not started.
 * ============================================================ */
bool uart_init(uint32_t baud, uint8_t parity) {
    uint32_t config = get_serial_config(parity);
    Serial1.begin(baud, config);
    delay(50);  /* UART stabilization */

    /* Drain any stale RX data */
    unsigned long t = millis();
    while (Serial1.available() && (millis() - t) < 100) {
        Serial1.read();
    }

    return true;
}

/* ============================================================
 * Flush — drain TX + RX
 * ============================================================ */
void uart_flush(void) {
    Serial1.flush();           /* Drain TX */
    delay(10);                 /* Flush delay */
    unsigned long t = millis();
    while (Serial1.available() && (millis() - t) < 100) {
        Serial1.read();        /* Drain RX */
    }
}

/* ============================================================
 * Write — auto DE/RE handles direction
 * ============================================================ */
size_t uart_write(const uint8_t* data, size_t len) {
    size_t written = Serial1.write(data, len);
    Serial1.flush();           /* Block until TX physically complete */
    delayMicroseconds(500);    /* Guard time for auto-direction switchback */
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
        } else if ((millis() - last_byte) >= interbyte_gap_ms) {
            break;
        }
    }
    return count;
}

/* ============================================================
 * Deinit — no-op on nRF52840 (Serial1.end() hangs)
 * ============================================================ */
void uart_deinit(void) {
    /* Intentionally empty — Serial1.end() hangs on nRF52840
     * if not previously started. Safe to just leave it. */
}

#endif /* PLATFORM_NRF52 */
