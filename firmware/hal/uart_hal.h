/**
 * @file uart_hal.h
 * @brief Platform-agnostic UART + RS485 HAL interface
 * @version 1.0
 * @date 2026-02-25
 *
 * Unified UART interface used by firmware/runtime/modbus_peripheral.cpp.
 * Platform-specific implementations in uart_hal_nrf52.cpp and uart_hal_esp32.cpp.
 *
 * Replaces per-gate hal_uart.h variants with a single shared header.
 * Implementations read pin definitions from gate_config.h (already on include path).
 *
 * Key differences from gate-local versions:
 *   - rs485_enable(bool on) replaces platform-specific enable/disable pairs
 *   - uart_init(baud, parity) only — pins come from gate_config.h in each impl
 *   - uart_read() takes interbyte_gap_ms as parameter (not from gate_config.h)
 *   - uart_write() returns size_t (bytes written)
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Enable or disable the RS485 transceiver (RAK5802).
 *        When on=true: powers module, configures direction pins.
 *        When on=false: powers down module (platform-dependent behavior).
 *
 *        nRF52: Sets WB_IO2 (EN) HIGH/LOW. Auto DE/RE — no manual direction.
 *        ESP32: Sets EN + DE pins. DE starts LOW (RX mode) when enabled.
 *
 * @param on  true = enable, false = disable
 */
void uart_rs485_enable(bool on);

/**
 * @brief Initialize Serial1 UART with given baud rate and parity.
 *        Platform implementations know their own pin assignments from gate_config.h.
 *
 *        nRF52: Serial1.begin(baud, config) — pins from variant.h
 *        ESP32: Serial1.begin(baud, config, rx_pin, tx_pin) — pins from gate_config.h
 *
 * @param baud    Baud rate (4800, 9600, 19200, 115200)
 * @param parity  0=None, 1=Odd (ESP32 only), 2=Even
 * @return true on success
 */
bool uart_init(uint32_t baud, uint8_t parity);

/**
 * @brief Flush Serial1 TX and drain RX buffers.
 */
void uart_flush(void);

/**
 * @brief Write data to Serial1 (RS485 TX direction).
 *        Handles half-duplex direction control where needed.
 *        Blocks until TX physically complete.
 *
 *        nRF52: Auto DE/RE — just write + flush.
 *        ESP32: DE HIGH → write → flush → DE LOW.
 *
 * @param data  Pointer to TX buffer
 * @param len   Number of bytes to send
 * @return Number of bytes written
 */
size_t uart_write(const uint8_t* data, size_t len);

/**
 * @brief Read response from Serial1 with timeout and inter-byte gap detection.
 *        Phase 1: Wait up to timeout_ms for first byte.
 *        Phase 2: Accumulate bytes until inter-byte gap exceeds interbyte_gap_ms.
 *
 * @param out              Pointer to RX buffer
 * @param max_len          Max bytes to read
 * @param timeout_ms       Max wait for first byte
 * @param interbyte_gap_ms Max gap between consecutive bytes (frame delimiter)
 * @return Number of bytes read (0 = timeout)
 */
int uart_read(uint8_t* out, size_t max_len, uint32_t timeout_ms, uint32_t interbyte_gap_ms);

/**
 * @brief Deinitialize UART (Serial1).
 *        nRF52: No-op (Serial1.end() hangs on nRF52840).
 *        ESP32: Calls Serial1.end().
 */
void uart_deinit(void);
