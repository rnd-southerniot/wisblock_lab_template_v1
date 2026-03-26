/**
 * @file hal_uart.h
 * @brief Gate-local UART HAL — RAK4631 (nRF52840) Serial1
 * @version 1.0
 * @date 2026-02-24
 *
 * nRF52840 Serial1 uses pins defined in variant.h:
 *   TX = pin 16 (P0.16)
 *   RX = pin 15 (P0.15)
 *
 * No pin arguments to Serial1.begin() on nRF52 Adafruit BSP.
 * RAK5802 has auto DE/RE — no manual direction control needed.
 * Do NOT call Serial1.end() — it hangs on nRF52 if not started.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Enable RS485 transceiver power (RAK5802 via 3V3_S rail).
 * @param en_pin  Module enable pin (WB_IO2 = 34)
 */
void hal_rs485_enable(uint8_t en_pin);

/**
 * @brief Initialize Serial1 with given baud and parity.
 *        Calling begin() again reinitializes — no end() needed.
 * @param baud    Baud rate (4800, 9600, 19200, 115200)
 * @param parity  0=None, 2=Even (nRF52840: no Odd parity)
 * @return true on success
 */
bool hal_uart_init(uint32_t baud, uint8_t parity);

/**
 * @brief Write data to Serial1 (RS485 TX).
 *        RAK5802 auto DE/RE handles direction.
 *        Blocks until TX physically complete (flush).
 * @param data  Pointer to TX buffer
 * @param len   Number of bytes to send
 * @return true if all bytes written
 */
bool hal_uart_write(const uint8_t *data, uint8_t len);

/**
 * @brief Read response from Serial1 with timeout.
 *        Phase 1: Wait up to timeout_ms for first byte.
 *        Phase 2: Accumulate bytes with inter-byte gap detection.
 * @param buf         Pointer to RX buffer
 * @param max_len     Max bytes to read
 * @param timeout_ms  Max wait for first byte
 * @return Number of bytes read (0 = timeout)
 */
uint8_t hal_uart_read(uint8_t *buf, uint8_t max_len, uint32_t timeout_ms);

/**
 * @brief Flush Serial1 TX and RX buffers.
 */
void hal_uart_flush(void);
