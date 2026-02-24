/**
 * @file system_manager.h
 * @brief SystemManager — orchestrates init, join, scheduler
 * @version 1.0
 * @date 2026-02-24
 *
 * Top-level runtime coordinator. Owns Scheduler, LoRaTransport,
 * and ModbusPeripheral. Registers the sensor-uplink task and
 * dispatches scheduler ticks from loop().
 *
 * Part of firmware/runtime/ production abstraction layer.
 */

#pragma once
#include <stdint.h>
#include "scheduler.h"
#include "lora_transport.h"
#include "modbus_peripheral.h"

class SystemManager {
public:
    SystemManager();

    /**
     * Initialize all subsystems: LoRa radio, OTAA join, Modbus peripheral.
     * Registers periodic sensor-uplink task with scheduler.
     * @param join_timeout_ms  Max time to wait for OTAA join
     * @return true if all subsystems initialized and joined
     */
    bool init(uint32_t join_timeout_ms = 30000);

    /**
     * Call from loop(). Dispatches scheduler tasks.
     * Non-blocking — returns immediately.
     */
    void tick();

    /** Get reference to scheduler (for gate test inspection) */
    Scheduler& scheduler();

    /** Get reference to transport (for gate test inspection) */
    LoRaTransport& transport();

    /** Get reference to peripheral (for gate test inspection) */
    ModbusPeripheral& peripheral();

    /** Cycle counter — incremented each time sensor-uplink task fires */
    uint32_t cycleCount() const;

    /**
     * Set static singleton (required before sensorUplinkTask can fire).
     * Called automatically by init(), or manually for gate testing.
     */
    void activate();

    /**
     * Register the sensor-uplink task with the scheduler.
     * Requires activate() to have been called first.
     * @return Task index (>= 0) on success, -1 on failure
     */
    int8_t registerSensorTask();

private:
    Scheduler        m_scheduler;
    LoRaTransport    m_transport;
    ModbusPeripheral m_peripheral;
    uint32_t         m_cycle_count;

    /** Sensor read + uplink task — registered with scheduler */
    static void sensorUplinkTask();
    static SystemManager* s_instance;
};
