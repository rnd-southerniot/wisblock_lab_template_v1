/**
 * @file storage_hal_nrf52.cpp
 * @brief Storage HAL — nRF52 implementation (InternalFileSystem / LittleFS)
 * @version 1.0
 * @date 2026-02-25
 *
 * Uses <InternalFileSystem.h> — built-in Adafruit nRF52 BSP, LittleFS on
 * internal flash. No extra lib_deps required.
 *
 * Strategy: One file per key under /wisblock/ directory.
 * File contents = raw bytes (4 or 1 byte).
 * Remove-then-create pattern for writes (LittleFS doesn't support
 * in-place overwrite of different sizes).
 *
 * Guarded by PLATFORM_NRF52 (via platform_select.h).
 */

#include "platform_select.h"
#ifdef PLATFORM_NRF52

#include <InternalFileSystem.h>
#include <string.h>
#include "storage_hal.h"

using namespace Adafruit_LittleFS_Namespace;

#define STORAGE_DIR "/wisblock"

static bool s_init = false;

bool storage_hal_init(void) {
    InternalFS.begin();
    // Create directory if it doesn't exist
    if (!InternalFS.exists(STORAGE_DIR)) {
        InternalFS.mkdir(STORAGE_DIR);
    }
    s_init = true;
    return true;
}

static bool read_file(const char* key, void* buf, size_t len) {
    if (!s_init) return false;
    char path[48];
    snprintf(path, sizeof(path), STORAGE_DIR "/%s", key);
    if (!InternalFS.exists(path)) return false;
    File f = InternalFS.open(path, FILE_O_READ);
    if (!f) return false;
    size_t n = f.read((uint8_t*)buf, len);
    f.close();
    return (n == len);
}

static bool write_file(const char* key, const void* buf, size_t len) {
    if (!s_init) return false;
    char path[48];
    snprintf(path, sizeof(path), STORAGE_DIR "/%s", key);
    // Remove existing (LittleFS overwrite safety)
    if (InternalFS.exists(path)) {
        InternalFS.remove(path);
    }
    File f = InternalFS.open(path, FILE_O_WRITE);
    if (!f) return false;
    size_t n = f.write((const uint8_t*)buf, len);
    f.close();
    return (n == len);
}

bool storage_hal_read_u32(const char* key, uint32_t* out_val) {
    return read_file(key, out_val, sizeof(uint32_t));
}

bool storage_hal_write_u32(const char* key, uint32_t val) {
    return write_file(key, &val, sizeof(uint32_t));
}

bool storage_hal_read_u8(const char* key, uint8_t* out_val) {
    return read_file(key, out_val, sizeof(uint8_t));
}

bool storage_hal_write_u8(const char* key, uint8_t val) {
    return write_file(key, &val, sizeof(uint8_t));
}

bool storage_hal_remove(const char* key) {
    if (!s_init) return false;
    char path[48];
    snprintf(path, sizeof(path), STORAGE_DIR "/%s", key);
    if (InternalFS.exists(path)) {
        InternalFS.remove(path);
    }
    return true;
}

#endif // PLATFORM_NRF52
