/**
 * @file modbus_peripheral.h
 * @brief ModbusPeripheral class — wraps Gate 3 Modbus/UART layer
 * @version 1.1
 * @date 2026-02-24
 *
 * Implements PeripheralInterface for Modbus RTU sensor reading.
 * Wraps Gate 3's modbus_frame and hal_uart shared layers.
 * Configuration via ModbusPeripheralConfig struct (no hardcoded pins).
 *
 * v1.1: read() now outputs SensorFrame instead of raw byte buffer.
 *
 * Part of firmware/runtime/ production abstraction layer.
 */

#pragma once
#include <stdint.h>
#include "peripheral_interface.h"

/**
 * Configuration for ModbusPeripheral — all parameters injectable.
 */
struct ModbusPeripheralConfig {
    uint8_t  uart_tx_pin;
    uint8_t  uart_rx_pin;
    uint8_t  rs485_en_pin;
    uint8_t  rs485_de_pin;
    uint8_t  slave_addr;
    uint32_t baud;
    uint8_t  parity;          // 0=None, 1=Odd, 2=Even
    uint8_t  reg_hi;
    uint8_t  reg_lo;
    uint8_t  qty_hi;
    uint8_t  qty_lo;
    uint8_t  retry_max;       // Number of retries (total attempts = retry_max + 1)
    uint32_t retry_delay_ms;
    uint32_t response_timeout_ms;
};

class ModbusPeripheral : public PeripheralInterface {
public:
    explicit ModbusPeripheral(const ModbusPeripheralConfig& cfg);

    bool init() override;
    bool read(SensorFrame& frame) override;
    void deinit() override;
    const char* name() const override { return "ModbusPeripheral"; }

    /** Get last read register value by index (for gate test logging) */
    uint16_t lastRegValue(uint8_t index) const;

    /** Get number of registers from last successful read */
    uint8_t lastRegCount() const;

    /**
     * Change Modbus slave address at runtime (downlink command).
     * @param addr  New slave address (1-247)
     * @return true if valid address and updated
     */
    bool setSlaveAddr(uint8_t addr);

private:
    ModbusPeripheralConfig m_cfg;
    bool     m_initialized;
    uint16_t m_last_values[8];  // Up to 8 registers cached
    uint8_t  m_last_count;
};
