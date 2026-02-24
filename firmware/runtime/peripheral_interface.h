/**
 * @file peripheral_interface.h
 * @brief Abstract interface for sensor/peripheral data sources
 * @version 1.0
 * @date 2026-02-24
 *
 * Pure virtual interface for sensor data providers.
 * Implementations: ModbusPeripheral, (future: I2CPeripheral, etc.)
 *
 * Part of firmware/runtime/ production abstraction layer.
 */

#pragma once
#include <stdint.h>

/**
 * Abstract interface for sensor/peripheral data sources.
 * Implementations: ModbusPeripheral, (future: I2CPeripheral, etc.)
 */
class PeripheralInterface {
public:
    virtual ~PeripheralInterface() = default;

    /** Initialize hardware (UART, I2C, etc.). Returns true on success. */
    virtual bool init() = 0;

    /**
     * Read sensor data into buffer.
     * @param buf   Output buffer (caller-allocated)
     * @param len   [in] max buffer size, [out] actual bytes written
     * @return true if read succeeded
     */
    virtual bool read(uint8_t* buf, uint8_t* len) = 0;

    /** Release hardware resources */
    virtual void deinit() = 0;

    /** Human-readable name for logging */
    virtual const char* name() const = 0;
};
