/**
 * @file lorawan_hal_common.cpp
 * @brief LoRaWAN HAL — Cross-platform LoRaMAC encapsulation
 * @version 1.0
 * @date 2026-02-25
 *
 * Encapsulates ALL SX126x-Arduino lmh_* calls behind lorawan_hal.h API.
 * Runtime code never includes <LoRaWan-Arduino.h> or touches lmh_* directly.
 *
 * This file handles:
 *   - lmh_init() with static-lifetime param and callback structs
 *   - lmh_setDevEui/AppEui/AppKey
 *   - lmh_join() with bounded polling
 *   - lmh_send() with internal static TX buffer
 *   - C callback trampolines for join/fail/rx/class
 *
 * Not guarded by PLATFORM_* — this code is identical on both platforms.
 * Platform-specific SX1262 init remains in lorawan_hal_nrf52.cpp / esp32.cpp.
 */

#include <Arduino.h>
#include <LoRaWan-Arduino.h>
#include "lorawan_hal.h"

/* ============================================================
 * Join State — volatile for ISR/callback safety
 * ============================================================ */
static volatile bool s_joined      = false;
static volatile bool s_join_failed = false;

/* ============================================================
 * C Callback Trampolines
 * SX126x-Arduino requires plain C function pointers.
 * These update the join state flags above.
 * ============================================================ */
static void on_joined(void) {
    s_joined = true;
}

static void on_join_failed(void) {
    s_join_failed = true;
}

/* ============================================================
 * Single-Slot Downlink Buffer
 *
 * on_rx_data() captures incoming downlink into static buffer.
 * lorawan_hal_pop_downlink() consumes it (one-shot).
 * s_dl_pending set LAST — consumer checks this flag.
 * ============================================================ */
static volatile bool s_dl_pending = false;
static uint8_t s_dl_buf[64];
static uint8_t s_dl_len  = 0;
static uint8_t s_dl_port = 0;

static void on_rx_data(lmh_app_data_t* data) {
    if (data && data->buffsize > 0 && data->buffsize <= sizeof(s_dl_buf)) {
        noInterrupts();
        memcpy(s_dl_buf, data->buffer, data->buffsize);
        s_dl_len  = data->buffsize;
        s_dl_port = data->port;
        s_dl_pending = true;
        interrupts();
    }
}

static void on_confirm_class(DeviceClass_t cls) {
    /* Class A only — no class change handling */
    (void)cls;
}

/* ============================================================
 * lorawan_hal_setup()
 *
 * Builds lmh_param_t and lmh_callback_t with static lifetime
 * (lmh_init stores pointers to these structs internally).
 * Then sets OTAA credentials.
 * ============================================================ */
bool lorawan_hal_setup(const LoRaWanHalParams& params,
                       const uint8_t dev_eui[8],
                       const uint8_t app_eui[8],
                       const uint8_t app_key[16]) {

    /* ---- Build LoRaWAN parameters (static — lmh_init stores pointer) ---- */
    static lmh_param_t lora_param;
    memset(&lora_param, 0, sizeof(lora_param));
    lora_param.adr_enable          = params.adr_enable ? LORAWAN_ADR_ON : LORAWAN_ADR_OFF;
    lora_param.tx_data_rate        = params.datarate;
    lora_param.enable_public_network = params.public_network;
    lora_param.nb_trials           = params.join_trials;
    lora_param.tx_power            = params.tx_power;
    lora_param.duty_cycle          = LORAWAN_DUTYCYCLE_OFF;

    /* ---- Build callback table (static — lmh_init stores pointer) ---- */
    static lmh_callback_t lora_callbacks;
    memset(&lora_callbacks, 0, sizeof(lora_callbacks));
    lora_callbacks.BoardGetBatteryLevel  = BoardGetBatteryLevel;
    lora_callbacks.BoardGetUniqueId      = BoardGetUniqueId;
    lora_callbacks.BoardGetRandomSeed    = BoardGetRandomSeed;
    lora_callbacks.lmh_RxData            = on_rx_data;
    lora_callbacks.lmh_has_joined        = on_joined;
    lora_callbacks.lmh_ConfirmClass      = on_confirm_class;
    lora_callbacks.lmh_has_joined_failed = on_join_failed;

    /* ---- Initialize LoRaMAC handler ---- */
    uint32_t err = lmh_init(&lora_callbacks, lora_param, true,
                             CLASS_A, LORAMAC_REGION_AS923);
    if (err != 0) {
        return false;
    }

    /* ---- Set OTAA credentials ---- */
    lmh_setDevEui(const_cast<uint8_t*>(dev_eui));
    lmh_setAppEui(const_cast<uint8_t*>(app_eui));
    lmh_setAppKey(const_cast<uint8_t*>(app_key));

    return true;
}

/* ============================================================
 * lorawan_hal_join()
 *
 * Starts OTAA join and polls for result with bounded timeout.
 * Identical logic to LoRaTransport::connect() — moved here.
 * ============================================================ */
bool lorawan_hal_join(uint32_t timeout_ms) {
    s_joined = false;
    s_join_failed = false;

    lmh_join();

    /* Poll for join result with timeout */
    uint32_t start = millis();
    while (!s_joined && !s_join_failed) {
        if (millis() - start > timeout_ms) break;
        delay(100);
    }

    return s_joined;
}

/* ============================================================
 * lorawan_hal_send()
 *
 * Builds lmh_app_data_t and sends uplink.
 * Uses internal static buffer (lmh_send expects non-const).
 * ============================================================ */
bool lorawan_hal_send(uint8_t port, const uint8_t* data, uint8_t len, bool confirmed) {
    if (!s_joined) {
        return false;
    }

    /* Static TX buffer — lmh_send expects non-const, mutable buffer */
    static uint8_t tx_buf[64];
    uint8_t copy_len = (len <= sizeof(tx_buf)) ? len : (uint8_t)sizeof(tx_buf);
    memcpy(tx_buf, data, copy_len);

    lmh_app_data_t app_data;
    app_data.buffer   = tx_buf;
    app_data.buffsize = copy_len;
    app_data.port     = port;

    lmh_error_status status = lmh_send(&app_data,
                                        confirmed ? LMH_CONFIRMED_MSG : LMH_UNCONFIRMED_MSG);
    return (status == LMH_SUCCESS);
}

/* ============================================================
 * lorawan_hal_is_joined()
 * ============================================================ */
bool lorawan_hal_is_joined(void) {
    return s_joined;
}

/* ============================================================
 * lorawan_hal_pop_downlink()
 *
 * Single-slot consumer. Returns false if no downlink pending.
 * Clears pending flag after pop — one-shot consumption.
 * IRQ-guarded to prevent torn reads if on_rx_data fires mid-pop.
 * ============================================================ */
bool lorawan_hal_pop_downlink(uint8_t* out_buf, uint8_t* out_len, uint8_t* out_port) {
    noInterrupts();
    if (!s_dl_pending) {
        interrupts();
        return false;
    }

    uint8_t len = s_dl_len;
    memcpy(out_buf, s_dl_buf, len);
    *out_len  = len;
    *out_port = s_dl_port;
    s_dl_pending = false;
    interrupts();

    return true;
}
