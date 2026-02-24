/**
 * @file peripheral_interface.h
 * @brief Abstract interface for sensor/peripheral data sources
 * @version 1.1
 * @date 2026-02-24
 *
 * Pure virtual interface for sensor data providers.
 * Implementations: ModbusPeripheral, (future: I2CPeripheral, etc.)
 *
 * v1.1: read() now outputs SensorFrame (typed intermediate struct)
 *       instead of raw byte buffer. Payload encoding responsibility
 *       moves to SystemManager.
 *
 * Part of firmware/runtime/ production abstraction layer.
 */

#pragma once
#include <stdint.h>
#include "sensor_frame.h"

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
     * Read sensor data into a SensorFrame.
     * @param frame  Output frame — populated with typed values on success
     * @return true if read succeeded (frame.valid will also be true)
     */
    virtual bool read(SensorFrame& frame) = 0;

    /** Release hardware resources */
    virtual void deinit() = 0;

    /** Human-readable name for logging */
    virtual const char* name() const = 0;
};
