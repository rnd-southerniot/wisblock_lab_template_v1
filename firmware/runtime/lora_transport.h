/**
 * @file lora_transport.h
 * @brief LoRaTransport class — LoRaWAN uplink via HAL
 * @version 2.0
 * @date 2026-02-25
 *
 * Implements TransportInterface for LoRaWAN uplink.
 * Delegates all radio operations to firmware/hal/lorawan_hal.h.
 *
 * v2.0: Fully decoupled from SX126x-Arduino library.
 *       No <LoRaWan-Arduino.h> include. No lmh_* types.
 *       Callback trampolines and join state moved to HAL.
 *       SX1262 pin fields removed from config (HAL reads gate_config.h directly).
 * v1.0: Direct SX126x-Arduino dependency with static singleton + C trampolines.
 *
 * Part of firmware/runtime/ production abstraction layer.
 */

#pragma once
#include <stdint.h>
#include "transport_interface.h"
#include "lorawan_hal.h"

/**
 * Configuration for LoRaTransport — injectable parameters.
 *
 * v2.0: SX1262 pin fields removed (HAL reads them from gate_config.h).
 *       Only OTAA credentials and LoRaWAN radio parameters remain.
 */
struct LoRaTransportConfig {
    /* OTAA credentials */
    uint8_t dev_eui[8];
    uint8_t app_eui[8];
    uint8_t app_key[16];

    /* LoRaWAN parameters */
    bool    adr_enable;
    int8_t  datarate;
    bool    public_network;
    uint8_t join_trials;
    int8_t  tx_power;
};

class LoRaTransport : public TransportInterface {
public:
    explicit LoRaTransport(const LoRaTransportConfig& cfg);

    bool init() override;
    bool connect(uint32_t timeout_ms) override;
    bool isConnected() const override;
    bool send(const uint8_t* data, uint8_t len, uint8_t port) override;
    const char* name() const override { return "LoRaTransport"; }

private:
    LoRaTransportConfig m_cfg;
    bool m_initialized;
};

/* ============================================================
 * LoRaWAN HAL — Downlink buffer pop
 *
 * Retrieves the most recent buffered downlink. Non-blocking.
 * The downlink buffer is filled by LoRaTransport::on_rx_data()
 * and consumed (cleared) by this function.
 * Returns 0 if no downlink pending.
 *
 * @param buf   Output buffer (caller provides, must be >= 64 bytes)
 * @param port  Output: downlink port number
 * @return Number of bytes copied (0 = no downlink pending)
 * ============================================================ */
uint8_t lorawan_hal_pop_downlink(uint8_t* buf, uint8_t* port);
