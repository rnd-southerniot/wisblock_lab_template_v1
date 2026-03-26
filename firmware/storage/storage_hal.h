/**
 * @file storage_hal.h
 * @brief Storage HAL — Platform-agnostic persistent key-value store
 * @version 1.0
 * @date 2026-02-25
 *
 * C-style API matching existing HAL pattern (lorawan_hal.h).
 * No platform types exposed.
 *
 * ESP32:  Preferences (NVS flash) — namespace "wisblock"
 * nRF52:  InternalFileSystem (LittleFS) — directory /wisblock/
 *
 * Implementations: storage_hal_esp32.cpp, storage_hal_nrf52.cpp
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>

/* Storage keys — string-based for both NVS and LittleFS */
#define STORAGE_KEY_INTERVAL   "poll_interval"
#define STORAGE_KEY_SLAVE_ADDR "slave_addr"

/* Gate 9 test key */
#define STORAGE_KEY_GATE9_PHASE "gate9_phase"

/**
 * @brief Initialize storage subsystem.
 *        ESP32: Opens Preferences namespace "wisblock".
 *        nRF52: Calls InternalFS.begin().
 * @return true on success
 */
bool storage_hal_init(void);

/**
 * @brief Read a uint32_t value.
 * @param key       Null-terminated key string
 * @param out_val   Output value
 * @return true if key existed and was read
 */
bool storage_hal_read_u32(const char* key, uint32_t* out_val);

/**
 * @brief Write a uint32_t value.
 * @param key   Null-terminated key string
 * @param val   Value to store
 * @return true on success
 */
bool storage_hal_write_u32(const char* key, uint32_t val);

/**
 * @brief Read a uint8_t value.
 * @param key       Null-terminated key string
 * @param out_val   Output value
 * @return true if key existed and was read
 */
bool storage_hal_read_u8(const char* key, uint8_t* out_val);

/**
 * @brief Write a uint8_t value.
 * @param key   Null-terminated key string
 * @param val   Value to store
 * @return true on success
 */
bool storage_hal_write_u8(const char* key, uint8_t val);

/**
 * @brief Remove a key from storage.
 * @param key   Null-terminated key string
 * @return true if key was removed (or didn't exist)
 */
bool storage_hal_remove(const char* key);
