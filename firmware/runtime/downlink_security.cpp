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
