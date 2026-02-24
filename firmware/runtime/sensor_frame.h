/**
 * @file sensor_frame.h
 * @brief SensorFrame — typed intermediate between peripherals and transports
 * @version 1.0
 * @date 2026-02-24
 *
 * Structured data contract for the peripheral → transport boundary.
 * Peripherals populate SensorFrame with typed register/sensor values.
 * SystemManager encodes the frame into a wire-format payload before
 * passing to the transport layer. No raw byte buffers cross the boundary.
 *
 * Generic enough for Modbus registers, I2C sensor readings, ADC values, etc.
 * Each value is uint16_t (natural width for Modbus holding registers and
 * most 16-bit sensor readings).
 *
 * Part of firmware/runtime/ production abstraction layer.
 */

#pragma once
#include <stdint.h>

#define SENSOR_FRAME_MAX_VALUES  8

struct SensorFrame {
    uint16_t values[SENSOR_FRAME_MAX_VALUES];  /**< Sensor/register values */
    uint8_t  count;                             /**< Number of valid entries in values[] */
    uint32_t timestamp_ms;                      /**< millis() at time of read */
    bool     valid;                             /**< true if read succeeded */
};
