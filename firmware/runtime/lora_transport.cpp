/**
 * @file lora_transport.cpp
 * @brief LoRaTransport — Implementation
 * @version 1.0
 * @date 2026-02-24
 *
 * Wraps SX126x-Arduino LoRaWAN stack with TransportInterface.
 * Exact hw_config + lora_hardware_init + lmh_init pattern proven in Gate 5.
 *
 * Static singleton bridges C function pointer callbacks to C++ object.
 */

#include <Arduino.h>
#include <LoRaWan-Arduino.h>
#include "lora_transport.h"

/* ============================================================
 * Static Singleton
 * ============================================================ */
LoRaTransport* LoRaTransport::s_instance = nullptr;

/* ============================================================
 * C Callback Trampolines
 * ============================================================ */
void LoRaTransport::on_joined() {
    if (s_instance) s_instance->m_joined = true;
}

void LoRaTransport::on_join_failed() {
    if (s_instance) s_instance->m_join_failed = true;
}

void LoRaTransport::on_rx_data(lmh_app_data_t* data) {
    /* Not used — no downlink handling */
    (void)data;
}

void LoRaTransport::on_confirm_class(DeviceClass_t cls) {
    /* Class A only — no class change handling */
    (void)cls;
}

/* ============================================================
 * Constructor
 * ============================================================ */
LoRaTransport::LoRaTransport(const LoRaTransportConfig& cfg)
    : m_cfg(cfg)
    , m_joined(false)
    , m_join_failed(false)
    , m_initialized(false) {
}

/* ============================================================
 * init() — Exact pattern from Gate 5 (proven on hardware)
 *
 * 1. Build hw_config with SX1262 pins
 * 2. lora_hardware_init()
 * 3. Build lmh_param_t and lmh_callback_t
 * 4. lmh_init()
 * 5. Set OTAA credentials
 * ============================================================ */
bool LoRaTransport::init() {
    s_instance = this;

    /* ---- Configure SX1262 hardware pin mapping ---- */
    hw_config hwConfig;
    hwConfig.CHIP_TYPE           = SX1262_CHIP;
    hwConfig.PIN_LORA_RESET      = m_cfg.pin_reset;
    hwConfig.PIN_LORA_NSS        = m_cfg.pin_nss;
    hwConfig.PIN_LORA_SCLK       = m_cfg.pin_sck;
    hwConfig.PIN_LORA_MISO       = m_cfg.pin_miso;
    hwConfig.PIN_LORA_DIO_1      = m_cfg.pin_dio1;
    hwConfig.PIN_LORA_BUSY       = m_cfg.pin_busy;
    hwConfig.PIN_LORA_MOSI       = m_cfg.pin_mosi;
    hwConfig.RADIO_TXEN          = -1;
    hwConfig.RADIO_RXEN          = m_cfg.pin_ant_sw;
    hwConfig.USE_DIO2_ANT_SWITCH = true;   /* Proven in Gate 5 */
    hwConfig.USE_DIO3_TCXO       = true;   /* Proven in Gate 5 */
    hwConfig.USE_DIO3_ANT_SWITCH = false;
    hwConfig.USE_LDO             = false;
    hwConfig.USE_RXEN_ANT_PWR    = false;

    /* ---- Initialize SX1262 hardware (SPI, timers, IRQ) ---- */
    uint32_t hw_err = lora_hardware_init(hwConfig);
    if (hw_err != 0) {
        return false;
    }

    /* ---- Build LoRaWAN parameters ---- */
    lmh_param_t lora_param;
    lora_param.adr_enable      = m_cfg.adr_enable ? LORAWAN_ADR_ON : LORAWAN_ADR_OFF;
    lora_param.tx_data_rate    = m_cfg.datarate;
    lora_param.enable_public_network = m_cfg.public_network;
    lora_param.nb_trials       = m_cfg.join_trials;
    lora_param.tx_power        = m_cfg.tx_power;
    lora_param.duty_cycle      = LORAWAN_DUTYCYCLE_OFF;

    /* ---- Build callback table ---- */
    lmh_callback_t lora_callbacks;
    lora_callbacks.BoardGetBatteryLevel = BoardGetBatteryLevel;
    lora_callbacks.BoardGetUniqueId     = BoardGetUniqueId;
    lora_callbacks.BoardGetRandomSeed   = BoardGetRandomSeed;
    lora_callbacks.lorawan_rx_handler   = on_rx_data;
    lora_callbacks.lorawan_has_joined_handler  = on_joined;
    lora_callbacks.lorawan_confirm_class_handler = on_confirm_class;
    lora_callbacks.lorawan_join_failed_handler = on_join_failed;

    /* ---- Initialize LoRaMAC handler ---- */
    uint32_t err = lmh_init(&lora_callbacks, lora_param, true,
                             CLASS_A, LORAMAC_REGION_AS923);
    if (err != 0) {
        return false;
    }

    /* ---- Set OTAA credentials ---- */
    lmh_setDevEui(m_cfg.dev_eui);
    lmh_setAppEui(m_cfg.app_eui);
    lmh_setAppKey(m_cfg.app_key);

    m_initialized = true;
    return true;
}

/* ============================================================
 * connect() — Blocking OTAA join (called once during init)
 * ============================================================ */
bool LoRaTransport::connect(uint32_t timeout_ms) {
    if (!m_initialized) {
        return false;
    }

    m_joined = false;
    m_join_failed = false;

    lmh_join();

    /* Poll for join result with timeout */
    uint32_t start = millis();
    while (!m_joined && !m_join_failed) {
        if (millis() - start > timeout_ms) break;
        delay(100);
    }

    return m_joined;
}

/* ============================================================
 * isConnected()
 * ============================================================ */
bool LoRaTransport::isConnected() const {
    return m_joined;
}

/* ============================================================
 * send() — Unconfirmed uplink
 * ============================================================ */
bool LoRaTransport::send(const uint8_t* data, uint8_t len, uint8_t port) {
    if (!m_initialized || !m_joined) {
        return false;
    }

    /* Build app data struct — lmh_send expects non-const buffer */
    static uint8_t tx_buf[64];
    uint8_t copy_len = (len <= sizeof(tx_buf)) ? len : sizeof(tx_buf);
    memcpy(tx_buf, data, copy_len);

    lmh_app_data_t app_data;
    app_data.buffer   = tx_buf;
    app_data.buffsize = copy_len;
    app_data.port     = port;

    lmh_error_status status = lmh_send(&app_data, LMH_UNCONFIRMED_MSG);
    return (status == LMH_SUCCESS);
}
