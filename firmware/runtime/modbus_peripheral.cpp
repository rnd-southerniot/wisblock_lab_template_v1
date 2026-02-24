/**
 * @file modbus_peripheral.cpp
 * @brief ModbusPeripheral — Implementation
 * @version 1.0
 * @date 2026-02-24
 *
 * Wraps Gate 3's shared modbus_frame and hal_uart layers.
 * Implements PeripheralInterface::read() with retry logic.
 * Output: registers packed big-endian into caller buffer.
 */

#include <Arduino.h>
#include <string.h>
#include "modbus_peripheral.h"
#include "../../tests/gates/gate_modbus_minimal_protocol/modbus_frame.h"
#include "../../tests/gates/gate_modbus_minimal_protocol/hal_uart.h"

ModbusPeripheral::ModbusPeripheral(const ModbusPeripheralConfig& cfg)
    : m_cfg(cfg)
    , m_initialized(false)
    , m_last_count(0) {
    memset(m_last_values, 0, sizeof(m_last_values));
}

bool ModbusPeripheral::init() {
    /* Enable RS485 transceiver (RAK5802) */
    hal_rs485_enable(m_cfg.rs485_en_pin, m_cfg.rs485_de_pin);

    /* Initialize UART */
    if (!hal_uart_init(m_cfg.uart_tx_pin, m_cfg.uart_rx_pin,
                       m_cfg.baud, m_cfg.parity)) {
        hal_rs485_disable();
        return false;
    }

    /* Flush and stabilize */
    hal_uart_flush();
    delay(50);

    m_initialized = true;
    return true;
}

bool ModbusPeripheral::read(uint8_t* buf, uint8_t* len) {
    if (!m_initialized || buf == nullptr || len == nullptr) {
        return false;
    }

    /* Build Modbus request frame */
    uint8_t tx_buf[8];
    uint8_t rx_buf[32];

    modbus_build_read_holding(m_cfg.slave_addr,
                              m_cfg.reg_hi, m_cfg.reg_lo,
                              m_cfg.qty_hi, m_cfg.qty_lo,
                              tx_buf);

    /* Calculate expected quantity from config */
    uint16_t quantity = ((uint16_t)m_cfg.qty_hi << 8) | m_cfg.qty_lo;
    uint8_t expected_bc = (uint8_t)(quantity * 2);

    /* Retry loop (up to retry_max + 1 attempts) */
    for (uint8_t attempt = 0; attempt <= m_cfg.retry_max; attempt++) {

        if (attempt > 0) {
            delay(m_cfg.retry_delay_ms);
        }

        /* Flush stale RX data */
        hal_uart_flush();
        delay(10);

        /* Send request */
        if (!hal_uart_write(tx_buf, 8)) {
            continue;
        }

        /* Read response */
        uint8_t rx_len = hal_uart_read(rx_buf, 32, m_cfg.response_timeout_ms);
        if (rx_len == 0) {
            continue;
        }

        /* Parse response via Gate 3's 7-step parser */
        ModbusResponse resp = modbus_parse_response(
            m_cfg.slave_addr, 0x03, rx_buf, rx_len);

        if (!resp.valid || resp.exception) {
            continue;
        }

        /* Validate byte_count */
        if (resp.byte_count != expected_bc) {
            continue;
        }

        /* Extract register values and pack into output buffer (big-endian) */
        uint8_t out_len = 0;
        m_last_count = 0;

        for (uint8_t i = 0; i < quantity && i < 8; i++) {
            uint8_t hi = resp.raw[3 + i * 2];
            uint8_t lo = resp.raw[4 + i * 2];

            m_last_values[i] = ((uint16_t)hi << 8) | lo;
            m_last_count++;

            if (out_len + 2 <= *len) {
                buf[out_len++] = hi;
                buf[out_len++] = lo;
            }
        }

        *len = out_len;
        return true;
    }

    /* All attempts exhausted */
    return false;
}

void ModbusPeripheral::deinit() {
    if (m_initialized) {
        hal_uart_deinit();
        hal_rs485_disable();
        m_initialized = false;
    }
}

uint16_t ModbusPeripheral::lastRegValue(uint8_t index) const {
    if (index >= m_last_count) {
        return 0;
    }
    return m_last_values[index];
}

uint8_t ModbusPeripheral::lastRegCount() const {
    return m_last_count;
}
