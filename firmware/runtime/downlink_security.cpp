/**
 * @file downlink_security.cpp
 * @brief Secure Downlink Protocol v2 — Full Implementation
 * @version 2.1
 * @date 2026-02-26
 *
 * Validates and applies authenticated downlink commands.
 *
 * Validation pipeline:
 *   1. Length guard
 *   2. Magic check (0xD1)
 *   3. Version check (0x02)
 *   4. Nonce replay check (must be > last_valid_nonce)
 *   5. HMAC-SHA256 verification (truncated to 4 bytes)
 *   6. Command dispatch via SystemManager
 *   7. Nonce persist to flash
 *
 * Part of firmware/runtime/ production abstraction layer.
 */

#include "downlink_security.h"
#include "crypto_sha256.h"
#include "system_manager.h"
#include <string.h>
#include <stdio.h>

/* ============================================================
 * Utility: Big-endian uint32 extraction
 * ============================================================ */
uint32_t sdl_read_u32_be(const uint8_t* buf) {
    return ((uint32_t)buf[0] << 24) |
           ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] <<  8) |
           ((uint32_t)buf[3]);
}

/* ============================================================
 * Helper: Build a rejection result
 * ============================================================ */
static SecureDlResult make_reject(bool auth, bool replay, bool version,
                                   const char* fmt, ...) {
    SecureDlResult r;
    memset(&r, 0, sizeof(r));
    r.valid        = false;
    r.auth_fail    = auth;
    r.replay_fail  = replay;
    r.version_fail = version;
    r.cmd_id       = 0x00;

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(r.msg, sizeof(r.msg), fmt, ap);
    va_end(ap);

    return r;
}

/* ============================================================
 * Helper: Build a success result
 * ============================================================ */
static SecureDlResult make_accept(uint8_t cmd_id, const char* fmt, ...) {
    SecureDlResult r;
    memset(&r, 0, sizeof(r));
    r.valid     = true;
    r.cmd_id    = cmd_id;

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(r.msg, sizeof(r.msg), fmt, ap);
    va_end(ap);

    return r;
}

/* ============================================================
 * Validate and Apply — Full Pipeline
 * ============================================================ */
SecureDlResult secure_dl_validate_and_apply(
    const uint8_t* buf,
    uint8_t        len,
    SystemManager& sys,
    const uint8_t  key[32]
) {
    /* ---- Step 1: Length guard ---- */
    if (len < SDL_FRAME_MIN_LEN) {
        return make_reject(false, false, true,
                           "REJECT: frame too short (%d < %d)",
                           len, SDL_FRAME_MIN_LEN);
    }
    if (len > SDL_FRAME_MAX_LEN) {
        return make_reject(false, false, true,
                           "REJECT: frame too long (%d > %d)",
                           len, SDL_FRAME_MAX_LEN);
    }

    /* ---- Step 2: Magic check ---- */
    if (buf[SDL_OFF_MAGIC] != SDL_FRAME_MAGIC) {
        return make_reject(false, false, true,
                           "REJECT: magic 0x%02X != 0x%02X",
                           buf[SDL_OFF_MAGIC], SDL_FRAME_MAGIC);
    }

    /* ---- Step 3: Version check ---- */
    if (buf[SDL_OFF_VERSION] != SDL_PROTOCOL_VERSION) {
        return make_reject(false, false, true,
                           "REJECT: version 0x%02X != 0x%02X",
                           buf[SDL_OFF_VERSION], SDL_PROTOCOL_VERSION);
    }

    /* ---- Step 4: Nonce extraction and replay check ---- */
    uint32_t nonce = sdl_read_u32_be(&buf[SDL_OFF_NONCE]);
    uint32_t last  = storage_hal_read_u32(SDL_NONCE_STORAGE_KEY, SDL_NONCE_DEFAULT);

    if (nonce <= last) {
        return make_reject(false, true, false,
                           "REJECT: replay (nonce=%lu <= last=%lu)",
                           (unsigned long)nonce, (unsigned long)last);
    }

    /* ---- Step 5: HMAC verification ---- */
    {
        uint8_t hmac_computed[32];
        uint8_t msg_len = len - SDL_HMAC_TRUNC_LEN;  /* Everything except HMAC field */

        sdl_hmac_sha256(key, SDL_HMAC_KEY_LEN, buf, msg_len, hmac_computed);

        /* Compare first SDL_HMAC_TRUNC_LEN bytes */
        const uint8_t* hmac_received = &buf[len - SDL_HMAC_TRUNC_LEN];

        if (memcmp(hmac_computed, hmac_received, SDL_HMAC_TRUNC_LEN) != 0) {
            return make_reject(true, false, false,
                               "REJECT: HMAC mismatch");
        }
    }

    /* ---- Step 6: Command dispatch ---- */
    uint8_t cmd = buf[SDL_OFF_CMD];
    uint8_t payload_len = len - SDL_FRAME_MIN_LEN;  /* 0 if no payload */
    const uint8_t* payload = &buf[SDL_OFF_PAYLOAD];

    switch (cmd) {

    case SDL_CMD_SET_INTERVAL: {
        /* Payload: uint32_t interval_ms (4 bytes BE) */
        if (payload_len < 4) {
            return make_reject(true, false, false,
                               "REJECT: SET_INTERVAL needs 4-byte payload, got %d",
                               payload_len);
        }
        uint32_t interval_ms = sdl_read_u32_be(payload);

        if (!sys.setUplinkInterval(interval_ms)) {
            return make_reject(true, false, false,
                               "REJECT: SET_INTERVAL(%lu) failed",
                               (unsigned long)interval_ms);
        }

        /* Persist nonce AFTER successful command */
        storage_hal_write_u32(SDL_NONCE_STORAGE_KEY, nonce);

        return make_accept(cmd, "OK: SET_INTERVAL %lums",
                           (unsigned long)interval_ms);
    }

    case SDL_CMD_SET_SLAVE_ADDR: {
        /* Payload: uint8_t addr (1 byte) */
        if (payload_len < 1) {
            return make_reject(true, false, false,
                               "REJECT: SET_SLAVE_ADDR needs 1-byte payload");
        }
        uint8_t addr = payload[0];

        if (!sys.setSlaveAddr(addr)) {
            return make_reject(true, false, false,
                               "REJECT: SET_SLAVE_ADDR(%d) invalid",
                               addr);
        }

        storage_hal_write_u32(SDL_NONCE_STORAGE_KEY, nonce);

        return make_accept(cmd, "OK: SET_SLAVE_ADDR %d", addr);
    }

    case SDL_CMD_REQUEST_STATUS: {
        /* No payload required */
        sys.requestStatusUplink();

        storage_hal_write_u32(SDL_NONCE_STORAGE_KEY, nonce);

        return make_accept(cmd, "OK: REQUEST_STATUS");
    }

    case SDL_CMD_REBOOT: {
        /* Persist nonce BEFORE reboot (won't survive otherwise) */
        storage_hal_write_u32(SDL_NONCE_STORAGE_KEY, nonce);

        /* Note: requestReboot() may not return */
        sys.requestReboot();

        return make_accept(cmd, "OK: REBOOT");
    }

    default:
        return make_reject(true, false, false,
                           "REJECT: unknown cmd 0x%02X", cmd);
    }
}
