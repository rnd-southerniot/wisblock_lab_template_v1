/**
 * @file lora_transport.h
 * @brief LoRaTransport class — wraps SX126x-Arduino LoRaWAN stack
 * @version 1.0
 * @date 2026-02-24
 *
 * Implements TransportInterface for LoRaWAN uplink via SX1262.
 * Uses static singleton + C callback trampolines for SX126x-Arduino
 * library compatibility (requires C function pointers).
 *
 * Single transport instance assumed (realistic for LoRaWAN devices).
 *
 * Part of firmware/runtime/ production abstraction layer.
 */

#pragma once
#include <stdint.h>
#include <LoRaWan-Arduino.h>
#include "transport_interface.h"

/**
 * Configuration for LoRaTransport — all parameters injectable.
 */
struct LoRaTransportConfig {
    /* SX1262 pin mapping */
    int pin_nss, pin_sck, pin_mosi, pin_miso;
    int pin_reset, pin_dio1, pin_busy, pin_ant_sw;

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
    volatile bool m_joined;
    volatile bool m_join_failed;
    bool m_initialized;

    /* Static singleton for C callback trampolines */
    static LoRaTransport* s_instance;

    /* C-compatible callback trampolines */
    static void on_joined();
    static void on_join_failed();
    static void on_rx_data(lmh_app_data_t* data);
    static void on_confirm_class(DeviceClass_t cls);
};
