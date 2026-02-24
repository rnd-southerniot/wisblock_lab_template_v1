/**
 * @file hal_uart.cpp
 * @brief Gate-local UART HAL — RAK4631 (nRF52840) Serial1
 * @version 1.0
 * @date 2026-02-24
 *
 * RAK5802 has hardware auto-direction control (TP8485E + on-board
 * RC/TCON circuit).  DE/RE is driven automatically from the UART TX
 * line — NO manual GPIO toggling required.
 *
 * WB_IO1 (pin 17) is NC (not connected) to DE on the RAK5802.
 * WB_IO2 (pin 34) controls the 3V3_S switched power rail.
 *
 * nRF52840 notes:
 *   - Serial1.begin(baud, config) — no pin args (variant.h)
 *   - Do NOT call Serial1.end() — hangs if not previously started
 *   - Serial1.flush() works correctly — blocks until TX DMA complete
 */

#include <Arduino.h>
#include "hal_uart.h"
#include "gate_config.h"

/* nRF52840 UARTE: only NONE and EVEN parity */
static uint32_t get_serial_config(uint8_t parity) {
    switch (parity) {
        case 2:  return SERIAL_8E1;
        default: return SERIAL_8N1;
    }
}

void hal_rs485_enable(uint8_t en_pin) {
    /* Power the RAK5802 module via 3V3_S rail */
    pinMode(en_pin, OUTPUT);
    digitalWrite(en_pin, HIGH);
    delay(100);   /* Allow module power-up */

    /* Do NOT configure WB_IO1 (pin 17) as DE/RE output.
     * RAK5802 auto-direction circuit handles DE/RE from TX line. */
}

bool hal_uart_init(uint32_t baud, uint8_t parity) {
    /* Do NOT call Serial1.end() — hangs on nRF52 if not started.
     * Calling begin() again reinitializes the peripheral. */
    uint32_t config = get_serial_config(parity);
    Serial1.begin(baud, config);
    delay(GATE_UART_STABILIZE_MS);

    /* Drain any stale RX data */
    unsigned long t = millis();
    while (Serial1.available() && (millis() - t) < 100) {
        Serial1.read();
    }

    return true;
}

bool hal_uart_write(const uint8_t *data, uint8_t len) {
    /* RAK5802 auto-direction: TX line activity drives DE HIGH.
     * Just write the data and flush — no GPIO toggling needed. */
    size_t written = Serial1.write(data, len);
    Serial1.flush();            /* Block until TX physically complete */
    delayMicroseconds(500);     /* Guard time for auto-direction switchback */
    return (written == len);
}

uint8_t hal_uart_read(uint8_t *buf, uint8_t max_len, uint32_t timeout_ms) {
    /* Phase 1: Wait for first byte or timeout */
    unsigned long start = millis();
    while (!Serial1.available()) {
        if ((millis() - start) >= timeout_ms) {
            return 0;
        }
    }

    /* Phase 2: Accumulate bytes with inter-byte gap detection */
    uint8_t count = 0;
    unsigned long last_byte = millis();
    while (count < max_len) {
        if (Serial1.available()) {
            buf[count++] = Serial1.read();
            last_byte = millis();
        } else if ((millis() - last_byte) >= GATE_INTER_BYTE_TIMEOUT_MS) {
            break;
        }
    }
    return count;
}

void hal_uart_flush(void) {
    Serial1.flush();            /* Drain TX */
    delay(GATE_UART_FLUSH_DELAY_MS);
    unsigned long t = millis();
    while (Serial1.available() && (millis() - t) < 100) {
        Serial1.read();         /* Drain RX */
    }
}
