#include "downlink_security.h"

#ifdef GATE_10

#include <stdio.h>
#include <string.h>
#include "crypto_sha256.h"
#include "storage_hal.h"
#include "system_manager.h"

namespace {

uint32_t decode_be32(const uint8_t* buf) {
    return ((uint32_t)buf[0] << 24)
         | ((uint32_t)buf[1] << 16)
         | ((uint32_t)buf[2] << 8)
         | (uint32_t)buf[3];
}

void sha256_bytes(const uint8_t* data, size_t len, uint8_t out[32]) {
    CryptoSha256Ctx ctx;
    crypto_sha256_init(&ctx);
    crypto_sha256_update(&ctx, data, len);
    crypto_sha256_final(&ctx, out);
}

void hmac_sha256(const uint8_t* key, size_t key_len,
                 const uint8_t* msg, size_t msg_len,
                 uint8_t out[32]) {
    uint8_t key_block[64];
    uint8_t inner_hash[32];
    uint8_t ipad[64];
    uint8_t opad[64];

    memset(key_block, 0, sizeof(key_block));
    if (key_len > sizeof(key_block)) {
        sha256_bytes(key, key_len, key_block);
    } else {
        memcpy(key_block, key, key_len);
    }

    for (size_t i = 0; i < sizeof(key_block); ++i) {
        ipad[i] = (uint8_t)(key_block[i] ^ 0x36);
        opad[i] = (uint8_t)(key_block[i] ^ 0x5c);
    }

    CryptoSha256Ctx ctx;
    crypto_sha256_init(&ctx);
    crypto_sha256_update(&ctx, ipad, sizeof(ipad));
    crypto_sha256_update(&ctx, msg, msg_len);
    crypto_sha256_final(&ctx, inner_hash);

    crypto_sha256_init(&ctx);
    crypto_sha256_update(&ctx, opad, sizeof(opad));
    crypto_sha256_update(&ctx, inner_hash, sizeof(inner_hash));
    crypto_sha256_final(&ctx, out);
}

uint32_t read_nonce_or_default(uint32_t fallback) {
    uint32_t value = fallback;
    if (!storage_hal_init()) {
        return fallback;
    }
    if (storage_hal_read_u32(SDL_NONCE_STORAGE_KEY, &value)) {
        return value;
    }
    return fallback;
}

bool write_nonce(uint32_t nonce) {
    if (!storage_hal_init()) {
        return false;
    }
    return storage_hal_write_u32(SDL_NONCE_STORAGE_KEY, nonce);
}

bool apply_command(uint8_t cmd,
                   const uint8_t* payload,
                   uint8_t payload_len,
                   SystemManager& sys,
                   char* msg,
                   size_t msg_len) {
    switch (cmd) {
    case SDL_CMD_SET_INTERVAL: {
        if (payload_len != 4) {
            snprintf(msg, msg_len, "SET_INTERVAL payload len=%u", payload_len);
            return false;
        }

        uint32_t interval_ms = decode_be32(payload);
        if (interval_ms < 1000 || interval_ms > 3600000) {
            snprintf(msg, msg_len, "SET_INTERVAL out of range: %lu",
                     (unsigned long)interval_ms);
            return false;
        }

        if (!sys.scheduler().setInterval(0, interval_ms)) {
            snprintf(msg, msg_len, "SET_INTERVAL scheduler update failed");
            return false;
        }

        storage_hal_write_u32(STORAGE_KEY_INTERVAL, interval_ms);
        snprintf(msg, msg_len, "SET_INTERVAL applied: %lu ms",
                 (unsigned long)interval_ms);
        return true;
    }

    case SDL_CMD_SET_SLAVE_ADDR: {
        if (payload_len != 1) {
            snprintf(msg, msg_len, "SET_SLAVE_ADDR payload len=%u", payload_len);
            return false;
        }

        uint8_t slave_addr = payload[0];
        if (slave_addr < 1 || slave_addr > 247) {
            snprintf(msg, msg_len, "SET_SLAVE_ADDR out of range: %u", slave_addr);
            return false;
        }

        sys.peripheral().setSlaveAddr(slave_addr);
        storage_hal_write_u8(STORAGE_KEY_SLAVE_ADDR, slave_addr);
        snprintf(msg, msg_len, "SET_SLAVE_ADDR applied: %u", slave_addr);
        return true;
    }

    case SDL_CMD_REQUEST_STATUS: {
        const RuntimeStats& st = sys.stats();
        snprintf(msg, msg_len, "REQUEST_STATUS ok: uplink_ok=%lu modbus_ok=%lu",
                 (unsigned long)st.uplink_ok,
                 (unsigned long)st.modbus_ok);
        return true;
    }

    case SDL_CMD_REBOOT:
        snprintf(msg, msg_len, "REBOOT accepted");
        return true;

    default:
        snprintf(msg, msg_len, "Unknown cmd: 0x%02X", cmd);
        return false;
    }
}

}  // namespace

SecureDlResult secure_dl_validate_and_apply(const uint8_t* frame,
                                            uint8_t len,
                                            SystemManager& sys,
                                            const uint8_t key[SDL_HMAC_KEY_LEN]) {
    SecureDlResult result = {};
    snprintf(result.msg, sizeof(result.msg), "Rejected");

    if (len < SDL_MIN_FRAME_LEN || len > SDL_MAX_FRAME_LEN) {
        snprintf(result.msg, sizeof(result.msg), "Length invalid: %u", len);
        return result;
    }

    if (frame[0] != SDL_FRAME_MAGIC) {
        result.version_fail = true;
        snprintf(result.msg, sizeof(result.msg), "Bad magic: 0x%02X", frame[0]);
        return result;
    }

    if (frame[1] != SDL_PROTOCOL_VERSION) {
        result.version_fail = true;
        snprintf(result.msg, sizeof(result.msg), "Bad version: 0x%02X", frame[1]);
        return result;
    }

    uint32_t nonce = decode_be32(&frame[4]);
    uint32_t last_nonce = read_nonce_or_default(0);
    if (nonce <= last_nonce) {
        result.replay_fail = true;
        snprintf(result.msg, sizeof(result.msg), "Replay nonce=%lu last=%lu",
                 (unsigned long)nonce, (unsigned long)last_nonce);
        return result;
    }

    uint8_t full_hmac[32];
    hmac_sha256(key, SDL_HMAC_KEY_LEN, frame, len - SDL_HMAC_TRUNC_LEN, full_hmac);
    if (memcmp(full_hmac, &frame[len - SDL_HMAC_TRUNC_LEN], SDL_HMAC_TRUNC_LEN) != 0) {
        result.auth_fail = true;
        snprintf(result.msg, sizeof(result.msg), "HMAC mismatch");
        return result;
    }

    uint8_t cmd_id = frame[2];
    const uint8_t* payload = &frame[8];
    uint8_t payload_len = (uint8_t)(len - SDL_MIN_FRAME_LEN);
    if (!apply_command(cmd_id, payload, payload_len, sys, result.msg, sizeof(result.msg))) {
        return result;
    }

    if (!write_nonce(nonce)) {
        snprintf(result.msg, sizeof(result.msg), "Nonce persist failed");
        return result;
    }

    result.valid = true;
    result.cmd_id = cmd_id;
    return result;
}

#endif
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
