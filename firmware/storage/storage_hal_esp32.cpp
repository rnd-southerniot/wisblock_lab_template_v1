/**
 * @file storage_hal_esp32.cpp
 * @brief Storage HAL — ESP32 implementation (Preferences / NVS flash)
 * @version 1.0
 * @date 2026-02-25
 *
 * Uses <Preferences.h> — built-in ESP-IDF library, backed by NVS flash.
 * No extra lib_deps required.
 *
 * Guarded by PLATFORM_ESP32 (via platform_select.h).
 */

#include "platform_select.h"
#ifdef PLATFORM_ESP32

#include <Arduino.h>
#include <Preferences.h>
#include "storage_hal.h"

static Preferences s_prefs;
static bool s_init = false;

bool storage_hal_init(void) {
    if (s_init) return true;  // Already initialized — skip re-init
    s_init = s_prefs.begin("wisblock", false);  // RW mode
#ifdef HAS_STORAGE_HAL
    if (s_init) {
        Serial.println("[STORAGE] ESP32 prefs.begin OK");
    } else {
        Serial.println("[STORAGE] ESP32 prefs.begin failed (namespace=wisblock)");
    }
#endif
    return s_init;
}

bool storage_hal_read_u32(const char* key, uint32_t* out_val) {
    if (!s_init) return false;
    if (!s_prefs.isKey(key)) return false;
    *out_val = s_prefs.getUInt(key, 0);
    return true;
}

bool storage_hal_write_u32(const char* key, uint32_t val) {
    if (!s_init) return false;
    return s_prefs.putUInt(key, val) > 0;
}

bool storage_hal_read_u8(const char* key, uint8_t* out_val) {
    if (!s_init) return false;
    if (!s_prefs.isKey(key)) return false;
    *out_val = s_prefs.getUChar(key, 0);
    return true;
}

bool storage_hal_write_u8(const char* key, uint8_t val) {
    if (!s_init) return false;
    return s_prefs.putUChar(key, val) > 0;
}

bool storage_hal_remove(const char* key) {
    if (!s_init) return false;
    s_prefs.remove(key);
    return true;
}

#endif // PLATFORM_ESP32
