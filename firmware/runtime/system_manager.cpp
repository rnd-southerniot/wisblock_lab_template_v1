/**
 * @file system_manager.cpp
 * @brief SystemManager — Implementation
 * @version 1.0
 * @date 2026-02-24
 *
 * Orchestrates runtime initialization, OTAA join, and scheduler dispatch.
 * Builds LoRaTransport and ModbusPeripheral configs from gate_config.h defines.
 *
 * sensorUplinkTask(): reads Modbus → sends LoRaWAN uplink → increments cycle count.
 */

#include <Arduino.h>
#include "system_manager.h"
#include "gate_config.h"

/* ============================================================
 * Static Singleton
 * ============================================================ */
SystemManager* SystemManager::s_instance = nullptr;

/* ============================================================
 * Constructor — builds configs from gate_config.h defines
 * ============================================================ */
SystemManager::SystemManager()
    : m_transport([]() -> LoRaTransportConfig {
        LoRaTransportConfig cfg = {};
        cfg.pin_nss   = GATE_LORA_PIN_NSS;
        cfg.pin_sck   = GATE_LORA_PIN_SCK;
        cfg.pin_mosi  = GATE_LORA_PIN_MOSI;
        cfg.pin_miso  = GATE_LORA_PIN_MISO;
        cfg.pin_reset = GATE_LORA_PIN_RESET;
        cfg.pin_dio1  = GATE_LORA_PIN_DIO1;
        cfg.pin_busy  = GATE_LORA_PIN_BUSY;
        cfg.pin_ant_sw = GATE_LORA_PIN_ANT_SW;

        uint8_t dev_eui[] = GATE_LORAWAN_DEV_EUI;
        uint8_t app_eui[] = GATE_LORAWAN_APP_EUI;
        uint8_t app_key[] = GATE_LORAWAN_APP_KEY;
        memcpy(cfg.dev_eui, dev_eui, 8);
        memcpy(cfg.app_eui, app_eui, 8);
        memcpy(cfg.app_key, app_key, 16);

        cfg.adr_enable     = false;
        cfg.datarate       = GATE_LORAWAN_DATARATE;
        cfg.public_network = GATE_LORAWAN_PUBLIC_NETWORK;
        cfg.join_trials    = GATE_LORAWAN_JOIN_TRIALS;
        cfg.tx_power       = GATE_LORAWAN_TX_POWER;
        return cfg;
    }())
    , m_peripheral([]() -> ModbusPeripheralConfig {
        ModbusPeripheralConfig cfg = {};
        cfg.uart_tx_pin      = GATE_UART_TX_PIN;
        cfg.uart_rx_pin      = GATE_UART_RX_PIN;
        cfg.rs485_en_pin     = GATE_RS485_EN_PIN;
        cfg.rs485_de_pin     = GATE_RS485_DE_PIN;
        cfg.slave_addr       = GATE_DISCOVERED_SLAVE;
        cfg.baud             = GATE_DISCOVERED_BAUD;
        cfg.parity           = GATE_DISCOVERED_PARITY;
        cfg.reg_hi           = GATE_MODBUS_TARGET_REG_HI;
        cfg.reg_lo           = GATE_MODBUS_TARGET_REG_LO;
        cfg.qty_hi           = GATE_MODBUS_QUANTITY_HI;
        cfg.qty_lo           = GATE_MODBUS_QUANTITY_LO;
        cfg.retry_max        = GATE_MODBUS_RETRY_MAX;
        cfg.retry_delay_ms   = GATE_MODBUS_RETRY_DELAY_MS;
        cfg.response_timeout_ms = GATE_MODBUS_RESPONSE_TIMEOUT_MS;
        return cfg;
    }())
    , m_cycle_count(0)
    , m_stats{} {
}

/* ============================================================
 * init() — Initialize subsystems and register scheduler tasks
 *
 * Note: transport and peripheral are initialized separately
 * by the gate runner for granular fail codes. This method
 * is available for production use where a single init call
 * is preferred.
 * ============================================================ */
bool SystemManager::init(uint32_t join_timeout_ms) {
    s_instance = this;

    /* Initialize LoRa transport */
    if (!m_transport.init()) {
        return false;
    }

    /* Blocking OTAA join */
    if (!m_transport.connect(join_timeout_ms)) {
        return false;
    }

    /* Initialize Modbus peripheral */
    if (!m_peripheral.init()) {
        return false;
    }

    /* Register sensor-uplink task */
    int8_t idx = m_scheduler.registerTask(sensorUplinkTask,
                                           HW_SYSTEM_POLL_INTERVAL_MS,
                                           "sensor_uplink");
    if (idx < 0) {
        return false;
    }

    return true;
}

/* ============================================================
 * tick() — Non-blocking scheduler dispatch
 * ============================================================ */
void SystemManager::tick() {
    m_scheduler.tick();
}

/* ============================================================
 * Accessors (for gate test inspection)
 * ============================================================ */
Scheduler& SystemManager::scheduler() {
    return m_scheduler;
}

LoRaTransport& SystemManager::transport() {
    return m_transport;
}

ModbusPeripheral& SystemManager::peripheral() {
    return m_peripheral;
}

uint32_t SystemManager::cycleCount() const {
    return m_cycle_count;
}

const RuntimeStats& SystemManager::stats() const {
    return m_stats;
}

void SystemManager::activate() {
    s_instance = this;
}

int8_t SystemManager::registerSensorTask() {
    if (!s_instance) s_instance = this;
    return m_scheduler.registerTask(sensorUplinkTask,
                                     HW_SYSTEM_POLL_INTERVAL_MS,
                                     "sensor_uplink");
}

/* ============================================================
 * Downlink command dispatch methods
 * ============================================================ */
bool SystemManager::setUplinkInterval(uint32_t interval_ms) {
    /* Task 0 is always the sensor_uplink task registered in init() */
    return m_scheduler.setTaskInterval(0, interval_ms);
}

bool SystemManager::setSlaveAddr(uint8_t addr) {
    return m_peripheral.setSlaveAddr(addr);
}

void SystemManager::requestStatusUplink() {
    /* Fire sensor_uplink task immediately (index 0) */
    m_scheduler.fireNow(0);
}

void SystemManager::requestReboot() {
    /* Platform-specific soft reset — ESP32: ESP.restart(), nRF52: NVIC_SystemReset() */
    Serial.printf("[RUNTIME] Reboot requested via downlink command\r\n");
    Serial.flush();
    delay(100);
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    ESP.restart();
#elif defined(NRF52_SERIES) || defined(ARDUINO_ARCH_NRF52)
    NVIC_SystemReset();
#else
    /* Fallback: infinite loop triggers watchdog */
    while (1) { ; }
#endif
}

/* ============================================================
 * sensorUplinkTask() — Static callback for scheduler
 *
 * 1. Read Modbus registers into SensorFrame (typed data)
 * 2. Encode payload locally (big-endian register packing)
 * 3. Send encoded payload via LoRaWAN transport
 * 4. Increment cycle count
 * 5. Log result with [RUNTIME] tag
 *
 * Data flow:
 *   ModbusPeripheral → SensorFrame → local encode → LoRaTransport
 *   No raw byte buffer passes between peripheral and transport.
 * ============================================================ */
void SystemManager::sensorUplinkTask() {
    if (!s_instance) return;

    uint32_t t0 = millis();
    RuntimeStats& st = s_instance->m_stats;

    /* 1. Read sensor data into typed frame */
    SensorFrame frame;
    bool read_ok = s_instance->m_peripheral.read(frame);

    if (read_ok && frame.valid && frame.count > 0) {
        /* Track modbus success */
        st.modbus_ok++;
        st.consec_modbus_fail = 0;
        st.last_modbus_ok = true;

        /* 2. Encode payload: pack uint16_t values as big-endian bytes */
        uint8_t payload[SENSOR_FRAME_MAX_VALUES * 2];
        uint8_t payload_len = 0;

        for (uint8_t i = 0; i < frame.count; i++) {
            payload[payload_len++] = (uint8_t)(frame.values[i] >> 8);
            payload[payload_len++] = (uint8_t)(frame.values[i] & 0xFF);
        }

        /* 3. Send encoded payload via transport */
        bool send_ok = s_instance->m_transport.send(payload, payload_len, GATE_PAYLOAD_PORT);

        /* Track uplink success/fail */
        if (send_ok) {
            st.uplink_ok++;
            st.consec_uplink_fail = 0;
            st.last_uplink_ok = true;
        } else {
            st.uplink_fail++;
            st.consec_uplink_fail++;
            if (st.consec_uplink_fail > st.max_consec_uplink_fail)
                st.max_consec_uplink_fail = st.consec_uplink_fail;
            st.last_uplink_ok = false;
        }

        Serial.printf("[RUNTIME] Cycle %lu: read=OK (%d regs), uplink=%s (port=%d, %d bytes)\r\n",
                      (unsigned long)(s_instance->m_cycle_count + 1),
                      frame.count,
                      send_ok ? "OK" : "FAIL",
                      GATE_PAYLOAD_PORT, payload_len);
    } else {
        /* Track modbus failure */
        st.modbus_fail++;
        st.consec_modbus_fail++;
        if (st.consec_modbus_fail > st.max_consec_modbus_fail)
            st.max_consec_modbus_fail = st.consec_modbus_fail;
        st.last_modbus_ok = false;
        st.last_uplink_ok = false;  /* No uplink attempted */

        Serial.printf("[RUNTIME] Cycle %lu: read=FAIL\r\n",
                      (unsigned long)(s_instance->m_cycle_count + 1));
    }

    /* Track cycle duration */
    uint32_t dur = millis() - t0;
    st.last_cycle_ms = dur;
    if (dur > st.max_cycle_ms)
        st.max_cycle_ms = dur;

    s_instance->m_cycle_count++;
}
