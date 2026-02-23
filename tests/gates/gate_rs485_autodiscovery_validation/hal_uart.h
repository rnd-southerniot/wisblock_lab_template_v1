/**
 * @file hal_uart.h
 * @brief Gate-local UART HAL — Minimal abstraction for gate validation
 * @version 2.1
 * @date 2026-02-24
 *
 * Self-contained within gate_rs485_autodiscovery_validation.
 * NOT a full UART/Modbus HAL — only enough for gate validation.
 *
 * v2.1: RAK5802 with EN pin (module power) + DE pin (half-duplex control).
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Enable RS485 transceiver (RAK5802). Must be called once before init.
 *        Sets module enable pin HIGH and configures DE pin as output (RX mode).
 * @param en_pin    GPIO for RAK5802 module enable (WB_IO2)
 * @param de_pin    GPIO for RS485 driver enable / DE (WB_IO1)
 */
void hal_rs485_enable(uint8_t en_pin, uint8_t de_pin);

/**
 * @brief Disable RS485 transceiver (RAK5802). Sets DE LOW, EN LOW.
 */
void hal_rs485_disable(void);

/**
 * @brief Initialize UART (Serial1) with given pins, baud rate, and parity.
 * @param tx_pin    GPIO number for TX
 * @param rx_pin    GPIO number for RX
 * @param baud      Baud rate (4800, 9600, 19200, or 115200)
 * @param parity    Parity mode: 0=None, 1=Odd, 2=Even
 * @return true on success
 */
bool hal_uart_init(uint8_t tx_pin, uint8_t rx_pin, uint32_t baud, uint8_t parity);

/**
 * @brief Deinitialize UART (Serial1). Required before reinit with new params.
 */
void hal_uart_deinit(void);

/**
 * @brief Write bytes to UART (Serial1) with DE half-duplex control.
 *        EN HIGH + DE HIGH → write → flush → DE LOW. Blocks until TX complete.
 * @param data    Pointer to data bytes to send
 * @param len     Number of bytes to write
 * @return true if all bytes written
 */
bool hal_uart_write(const uint8_t *data, uint8_t len);

/**
 * @brief Read bytes from UART (Serial1) with timeout.
 *        Uses inter-byte gap detection for Modbus RTU frame delimiting.
 *        DE pin is already LOW (RX mode) after write.
 * @param buf         Buffer to store received bytes
 * @param max_len     Maximum number of bytes to read
 * @param timeout_ms  Timeout in milliseconds to wait for first byte
 * @return Number of bytes actually read (0 if timeout with no data)
 */
uint8_t hal_uart_read(uint8_t *buf, uint8_t max_len, uint32_t timeout_ms);

/**
 * @brief Flush UART RX and TX buffers. Discards all pending data.
 */
void hal_uart_flush(void);

/**
 * @brief Check number of bytes available in UART RX buffer.
 * @return Number of bytes available
 */
uint8_t hal_uart_available(void);
