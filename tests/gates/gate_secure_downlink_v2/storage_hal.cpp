/**
 * @file storage_hal.cpp
 * @brief Storage HAL — ESP32 Preferences (NVS) implementation
 * @version 1.0
 * @date 2026-02-26
 *
 * Provides persistent uint32_t storage using ESP32 Preferences library
 * (backed by NVS flash partition). Used by Secure Downlink Protocol v2
 * for nonce persistence across reboots.
 *
 * Namespace: "sdl" (Secure Downlink)
 * Keys: Short strings (max 15 chars per NVS limitation)
 *
 * Also works on nRF52 via Adafruit LittleFS (same Preferences API).
 */

#include <Arduino.h>
#include <Preferences.h>
#include "../../../firmware/runtime/downlink_security.h"

static const char* NVS_NAMESPACE = "sdl";

bool storage_hal_write_u32(const char* key, uint32_t value) {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false /* readWrite */)) {
        return false;
    }
    size_t written = prefs.putUInt(key, value);
    prefs.end();
    return (written == sizeof(uint32_t));
}

uint32_t storage_hal_read_u32(const char* key, uint32_t default_val) {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true /* readOnly */)) {
        return default_val;
    }
    uint32_t val = prefs.getUInt(key, default_val);
    prefs.end();
    return val;
}
