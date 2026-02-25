/**
 * @file lora_transport.cpp
 * @brief LoRaTransport — Implementation
 * @version 2.0
 * @date 2026-02-25
 *
 * Implements TransportInterface for LoRaWAN uplink via lorawan_hal.h.
 * All SX126x-Arduino library calls are encapsulated in the HAL layer.
 *
 * v2.0: Fully decoupled from SX126x-Arduino library.
 *       No <LoRaWan-Arduino.h> include. No lmh_* calls. No callback trampolines.
 *       Join state managed by HAL (lorawan_hal_is_joined()).
 *       Static singleton removed — no longer needed (callbacks are in HAL).
 * v1.1: Uses firmware/hal/ LoRaWAN abstraction — zero platform #ifdefs.
 * v1.0: Direct SX126x-Arduino dependency with static singleton + C trampolines.
 */

#include <Arduino.h>
#include "lora_transport.h"
#include "lorawan_hal.h"

/* ============================================================
 * Constructor
 * ============================================================ */
LoRaTransport::LoRaTransport(const LoRaTransportConfig& cfg)
    : m_cfg(cfg)
    , m_initialized(false) {
}

/* ============================================================
 * init()
 *
 * 1. lorawan_hal_init() — platform-specific SX1262 hardware
 * 2. lorawan_hal_setup() — LoRaMAC stack + credentials
 * ============================================================ */
bool LoRaTransport::init() {
    /* Platform-specific SX1262 hardware init */
    if (!lorawan_hal_init()) {
        return false;
    }

    /* LoRaMAC stack init + OTAA credentials */
    LoRaWanHalParams params;
    params.adr_enable     = m_cfg.adr_enable;
    params.datarate       = m_cfg.datarate;
    params.public_network = m_cfg.public_network;
    params.join_trials    = m_cfg.join_trials;
    params.tx_power       = m_cfg.tx_power;

    if (!lorawan_hal_setup(params, m_cfg.dev_eui, m_cfg.app_eui, m_cfg.app_key)) {
        return false;
    }

    m_initialized = true;
    return true;
}

/* ============================================================
 * connect() — Blocking OTAA join
 * ============================================================ */
bool LoRaTransport::connect(uint32_t timeout_ms) {
    if (!m_initialized) {
        return false;
    }

    return lorawan_hal_join(timeout_ms);
}

/* ============================================================
 * isConnected()
 * ============================================================ */
bool LoRaTransport::isConnected() const {
    return lorawan_hal_is_joined();
}

/* ============================================================
 * send() — Unconfirmed uplink
 * ============================================================ */
bool LoRaTransport::send(const uint8_t* data, uint8_t len, uint8_t port) {
    if (!m_initialized || !lorawan_hal_is_joined()) {
        return false;
    }

    return lorawan_hal_send(port, data, len, false);
}
