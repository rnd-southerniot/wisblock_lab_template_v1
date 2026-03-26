/**
 * @file lorawan_hal.h
 * @brief Platform-agnostic LoRaWAN HAL interface
 * @version 2.0
 * @date 2026-02-25
 *
 * Full LoRaWAN HAL — encapsulates SX126x-Arduino library entirely.
 * Runtime code (lora_transport.cpp) calls only lorawan_hal_* functions
 * and never includes <LoRaWan-Arduino.h> or calls lmh_* directly.
 *
 * Architecture:
 *   - lorawan_hal_init()       — platform-specific SX1262 hw init (nrf52/esp32 files)
 *   - lorawan_hal_setup()      — LoRaMAC stack init + credentials (lorawan_hal_common.cpp)
 *   - lorawan_hal_join()       — OTAA join with bounded timeout (lorawan_hal_common.cpp)
 *   - lorawan_hal_send()       — Uplink (lorawan_hal_common.cpp)
 *   - lorawan_hal_is_joined()  — Join state query (lorawan_hal_common.cpp)
 *
 * Platform files:
 *   - lorawan_hal_nrf52.cpp    — lora_rak4630_init()
 *   - lorawan_hal_esp32.cpp    — hw_config + lora_hardware_init()
 *   - lorawan_hal_common.cpp   — lmh_init, lmh_join, lmh_send, callbacks
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 * LoRaWAN Compatibility Constants
 *
 * These mirror the SX126x-Arduino library's DR_* and TX_POWER_*
 * defines so that gate_config.h can use symbolic names without
 * pulling in <LoRaWan-Arduino.h>.
 *
 * Guarded by #ifndef — no conflict when the library header is
 * also included (e.g., in lorawan_hal_common.cpp).
 * ============================================================ */

/* Datarate indices (LoRaMac-definitions.h) */
#ifndef DR_0
#define DR_0   0
#define DR_1   1
#define DR_2   2
#define DR_3   3
#define DR_4   4
#define DR_5   5
#define DR_6   6
#define DR_7   7
#endif

/* TX power indices (Region.h) */
#ifndef TX_POWER_0
#define TX_POWER_0   0
#define TX_POWER_1   1
#define TX_POWER_2   2
#define TX_POWER_3   3
#define TX_POWER_4   4
#define TX_POWER_5   5
#define TX_POWER_6   6
#define TX_POWER_7   7
#define TX_POWER_8   8
#define TX_POWER_9   9
#define TX_POWER_10  10
#define TX_POWER_11  11
#define TX_POWER_12  12
#define TX_POWER_13  13
#define TX_POWER_14  14
#define TX_POWER_15  15
#endif

/**
 * LoRaWAN parameters passed to lorawan_hal_setup().
 * Mirrors the subset of lmh_param_t that the runtime needs to configure.
 * No library types exposed.
 */
struct LoRaWanHalParams {
    bool    adr_enable;
    int8_t  datarate;
    bool    public_network;
    uint8_t join_trials;
    int8_t  tx_power;
};

/**
 * @brief Initialize SX1262 radio hardware for the current platform.
 *
 *        nRF52 (RAK4631): Calls lora_rak4630_init().
 *        ESP32 (RAK3312):  Builds hw_config, calls lora_hardware_init().
 *
 * @return true on success
 */
bool lorawan_hal_init(void);

/**
 * @brief Initialize LoRaMAC stack, set region to AS923-1, register callbacks.
 *        Must be called after lorawan_hal_init().
 *
 *        Internally builds lmh_param_t and lmh_callback_t with static lifetime,
 *        calls lmh_init(), then sets OTAA credentials.
 *
 * @param params    LoRaWAN radio parameters (ADR, datarate, TX power, etc.)
 * @param dev_eui   8-byte Device EUI
 * @param app_eui   8-byte Application EUI
 * @param app_key   16-byte Application Key
 * @return true on success (lmh_init returned 0)
 */
bool lorawan_hal_setup(const LoRaWanHalParams& params,
                       const uint8_t dev_eui[8],
                       const uint8_t app_eui[8],
                       const uint8_t app_key[16]);

/**
 * @brief Start OTAA join and poll for result with bounded timeout.
 *
 *        Calls lmh_join(), then polls joined/failed flags at 100ms intervals.
 *        Returns when joined, join failed, or timeout expires.
 *
 * @param timeout_ms  Maximum wait time in milliseconds
 * @return true if join succeeded
 */
bool lorawan_hal_join(uint32_t timeout_ms);

/**
 * @brief Send uplink payload.
 *
 *        Builds lmh_app_data_t, calls lmh_send().
 *        Copies data into internal static buffer (lmh_send expects non-const).
 *
 * @param port       Application port (1-223)
 * @param data       Payload bytes
 * @param len        Payload length (max 64 bytes)
 * @param confirmed  true for confirmed uplink, false for unconfirmed
 * @return true if send succeeded (LMH_SUCCESS)
 */
bool lorawan_hal_send(uint8_t port, const uint8_t* data, uint8_t len, bool confirmed);

/**
 * @brief Check if device is currently joined to the network.
 * @return true if OTAA join completed successfully
 */
bool lorawan_hal_is_joined(void);

/**
 * @brief Pop a received downlink from the internal buffer.
 *
 *        Single-slot buffer (v1). Returns false if no downlink pending.
 *        Clears the buffer after pop — subsequent calls return false
 *        until the next downlink arrives.
 *
 * @param out_buf   Output buffer (caller must provide >= 64 bytes)
 * @param out_len   Output — payload length
 * @param out_port  Output — fport
 * @return true if a downlink was available
 */
bool lorawan_hal_pop_downlink(uint8_t* out_buf, uint8_t* out_len, uint8_t* out_port);
