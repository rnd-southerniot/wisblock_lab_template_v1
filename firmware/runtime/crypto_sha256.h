#pragma once

#include <stddef.h>
#include <stdint.h>

struct CryptoSha256Ctx {
    uint8_t data[64];
    uint32_t datalen;
    uint32_t state[8];
    uint64_t bitlen;
};

void crypto_sha256_init(CryptoSha256Ctx* ctx);
void crypto_sha256_update(CryptoSha256Ctx* ctx, const uint8_t* data, size_t len);
void crypto_sha256_final(CryptoSha256Ctx* ctx, uint8_t hash[32]);
/**
 * @file crypto_sha256.h
 * @brief Minimal SHA-256 and HMAC-SHA256 for embedded targets
 * @version 1.0
 * @date 2026-02-26
 *
 * FIPS 180-4 compliant SHA-256 implementation.
 * No heap allocation. No RTOS dependencies. No external libraries.
 *
 * All symbols prefixed with sdl_ to avoid collisions with
 * platform crypto libraries (e.g. ESP32 wpa_supplicant).
 *
 * Memory budget:
 *   Stack:  ~120 bytes (sdl_sha256_ctx) per hash operation
 *   Flash:  ~2.5 KB (K constants + transform)
 *   Heap:   0 bytes
 *
 * Part of firmware/runtime/ production abstraction layer.
 */

#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * SHA-256 Context (internal state)
 * ============================================================ */
typedef struct {
    uint32_t state[8];      /* Hash state (A-H) */
    uint64_t bitcount;      /* Total bits processed */
    uint8_t  buffer[64];    /* Partial block buffer */
} sdl_sha256_ctx_t;

/* ============================================================
 * SHA-256 Streaming API
 * ============================================================ */

/** Initialize SHA-256 context with IV */
void sdl_sha256_init(sdl_sha256_ctx_t* ctx);

/** Feed data into hash. Can be called multiple times. */
void sdl_sha256_update(sdl_sha256_ctx_t* ctx, const uint8_t* data, size_t len);

/** Finalize hash, write 32-byte digest to out. Context is consumed. */
void sdl_sha256_final(sdl_sha256_ctx_t* ctx, uint8_t out[32]);

/* ============================================================
 * SHA-256 One-Shot API
 * ============================================================ */

/**
 * Compute SHA-256 of data in one call.
 * @param data  Input bytes
 * @param len   Input length
 * @param out   Output buffer (32 bytes)
 */
void sdl_sha256(const uint8_t* data, size_t len, uint8_t out[32]);

/* ============================================================
 * HMAC-SHA256 One-Shot API
 * ============================================================ */

/**
 * Compute HMAC-SHA256.
 * @param key      HMAC key
 * @param key_len  Key length (if > 64, key is pre-hashed)
 * @param msg      Message data
 * @param msg_len  Message length
 * @param out      Output buffer (32 bytes)
 */
void sdl_hmac_sha256(const uint8_t* key, size_t key_len,
                     const uint8_t* msg, size_t msg_len,
                     uint8_t out[32]);

#ifdef __cplusplus
}
#endif
