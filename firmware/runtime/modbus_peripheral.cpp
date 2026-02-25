/**
 * @file modbus_peripheral.cpp
 * @brief ModbusPeripheral — Implementation
 * @version 1.2
 * @date 2026-02-25
 *
 * Wraps Gate 3's shared modbus_frame layer and platform-agnostic uart_hal.
 * Implements PeripheralInterface::read() with retry logic.
 *
 * v1.2: Uses firmware/hal/ UART abstraction — zero platform #ifdefs.
 *       uart_hal.h replaces per-gate hal_uart.h variants.
 * v1.1: read() outputs SensorFrame with typed uint16_t register values.
 *       Payload encoding is no longer done here — that responsibility
 *       belongs to SystemManager (the orchestrator).
 */

#include <Arduino.h>
#include <string.h>
#include "modbus_peripheral.h"
#include "modbus_frame.h"
#include "uart_hal.h"
#include "gate_config.h"

ModbusPeripheral::ModbusPeripheral(const ModbusPeripheralConfig& cfg)
    : m_cfg(cfg)
    , m_initialized(false)
    , m_last_count(0) {
    memset(m_last_values, 0, sizeof(m_last_values));
}

bool ModbusPeripheral::init() {
    /* Enable RS485 transceiver via platform HAL */
    uart_rs485_enable(true);

    /* Initialize UART via platform HAL (pins resolved internally) */
    if (!uart_init(m_cfg.baud, m_cfg.parity)) {
        uart_rs485_enable(false);
        return false;
    }

    /* Flush and stabilize */
    uart_flush();
    delay(50);

    m_initialized = true;
    return true;
}

bool ModbusPeripheral::read(SensorFrame& frame) {
    frame.valid = false;
    frame.count = 0;
    frame.timestamp_ms = 0;

    if (!m_initialized) {
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
        uart_flush();
        delay(10);

        /* Send request */
        if (uart_write(tx_buf, 8) != 8) {
            continue;
        }

        /* Read response */
        int rx_len = uart_read(rx_buf, 32, m_cfg.response_timeout_ms, GATE_INTER_BYTE_TIMEOUT_MS);
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

        /* Extract register values into SensorFrame */
        frame.count = 0;
        m_last_count = 0;

        for (uint8_t i = 0; i < quantity && i < SENSOR_FRAME_MAX_VALUES; i++) {
            uint16_t val = ((uint16_t)resp.raw[3 + i * 2] << 8)
                         | (uint16_t)resp.raw[4 + i * 2];

            frame.values[i] = val;
            frame.count++;

            m_last_values[i] = val;
            m_last_count++;
        }

        frame.timestamp_ms = millis();
        frame.valid = true;
        return true;
    }

    /* All attempts exhausted */
    return false;
}

void ModbusPeripheral::deinit() {
    if (m_initialized) {
        uart_deinit();           /* nRF52: no-op (safe), ESP32: Serial1.end() */
        uart_rs485_enable(false); /* Power down RS485 module */
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

void ModbusPeripheral::setSlaveAddr(uint8_t addr) {
    m_cfg.slave_addr = addr;
}

uint8_t ModbusPeripheral::slaveAddr() const {
    return m_cfg.slave_addr;
}
