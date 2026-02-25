/**
 * @file crypto_sha256.cpp
 * @brief Minimal SHA-256 and HMAC-SHA256 — FIPS 180-4 compliant
 * @version 1.0
 * @date 2026-02-26
 *
 * Full software implementation. No heap, no RTOS, no external libraries.
 * All symbols prefixed with sdl_ to avoid ESP32/nRF52 platform collisions.
 *
 * Part of firmware/runtime/ production abstraction layer.
 */

#include "crypto_sha256.h"
#include <string.h>

/* ============================================================
 * SHA-256 Round Constants (FIPS 180-4 Section 4.2.2)
 * ============================================================ */
static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

/* ============================================================
 * Bit manipulation helpers
 * ============================================================ */
static inline uint32_t rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

static inline uint32_t Ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

static inline uint32_t Maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

static inline uint32_t Sigma0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

static inline uint32_t Sigma1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

static inline uint32_t sigma0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

static inline uint32_t sigma1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

/* ============================================================
 * Load/store big-endian
 * ============================================================ */
static inline uint32_t load_be32(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] <<  8) | ((uint32_t)p[3]);
}

static inline void store_be32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >>  8);
    p[3] = (uint8_t)(v);
}

static inline void store_be64(uint8_t* p, uint64_t v) {
    p[0] = (uint8_t)(v >> 56); p[1] = (uint8_t)(v >> 48);
    p[2] = (uint8_t)(v >> 40); p[3] = (uint8_t)(v >> 32);
    p[4] = (uint8_t)(v >> 24); p[5] = (uint8_t)(v >> 16);
    p[6] = (uint8_t)(v >>  8); p[7] = (uint8_t)(v);
}

/* ============================================================
 * SHA-256 Transform — process one 64-byte block
 * ============================================================ */
static void sha256_transform(uint32_t state[8], const uint8_t block[64]) {
    uint32_t W[64];
    uint32_t a, b, c, d, e, f, g, h;

    for (int i = 0; i < 16; i++) {
        W[i] = load_be32(&block[i * 4]);
    }
    for (int i = 16; i < 64; i++) {
        W[i] = sigma1(W[i - 2]) + W[i - 7] + sigma0(W[i - 15]) + W[i - 16];
    }

    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];

    for (int i = 0; i < 64; i++) {
        uint32_t T1 = h + Sigma1(e) + Ch(e, f, g) + K[i] + W[i];
        uint32_t T2 = Sigma0(a) + Maj(a, b, c);
        h = g; g = f; f = e; e = d + T1;
        d = c; c = b; b = a; a = T1 + T2;
    }

    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

/* ============================================================
 * SHA-256 Init
 * ============================================================ */
void sdl_sha256_init(sdl_sha256_ctx_t* ctx) {
    ctx->state[0] = 0x6a09e667; ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372; ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f; ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab; ctx->state[7] = 0x5be0cd19;
    ctx->bitcount = 0;
    memset(ctx->buffer, 0, 64);
}

/* ============================================================
 * SHA-256 Update
 * ============================================================ */
void sdl_sha256_update(sdl_sha256_ctx_t* ctx, const uint8_t* data, size_t len) {
    size_t buf_used = (size_t)((ctx->bitcount >> 3) & 0x3F);
    ctx->bitcount += (uint64_t)len << 3;

    if (buf_used > 0) {
        size_t buf_free = 64 - buf_used;
        if (len >= buf_free) {
            memcpy(&ctx->buffer[buf_used], data, buf_free);
            sha256_transform(ctx->state, ctx->buffer);
            data += buf_free;
            len  -= buf_free;
            buf_used = 0;
        } else {
            memcpy(&ctx->buffer[buf_used], data, len);
            return;
        }
    }

    while (len >= 64) {
        sha256_transform(ctx->state, data);
        data += 64;
        len  -= 64;
    }

    if (len > 0) {
        memcpy(ctx->buffer, data, len);
    }
}

/* ============================================================
 * SHA-256 Final
 * ============================================================ */
void sdl_sha256_final(sdl_sha256_ctx_t* ctx, uint8_t out[32]) {
    size_t buf_used = (size_t)((ctx->bitcount >> 3) & 0x3F);

    ctx->buffer[buf_used++] = 0x80;

    if (buf_used > 56) {
        memset(&ctx->buffer[buf_used], 0, 64 - buf_used);
        sha256_transform(ctx->state, ctx->buffer);
        memset(ctx->buffer, 0, 56);
    } else {
        memset(&ctx->buffer[buf_used], 0, 56 - buf_used);
    }

    store_be64(&ctx->buffer[56], ctx->bitcount);
    sha256_transform(ctx->state, ctx->buffer);

    for (int i = 0; i < 8; i++) {
        store_be32(&out[i * 4], ctx->state[i]);
    }

    memset(ctx, 0, sizeof(*ctx));
}

/* ============================================================
 * SHA-256 One-Shot
 * ============================================================ */
void sdl_sha256(const uint8_t* data, size_t len, uint8_t out[32]) {
    sdl_sha256_ctx_t ctx;
    sdl_sha256_init(&ctx);
    sdl_sha256_update(&ctx, data, len);
    sdl_sha256_final(&ctx, out);
}

/* ============================================================
 * HMAC-SHA256 — RFC 2104
 *
 * Stack usage: ~300 bytes (two sha256_ctx + key_pad)
 * ============================================================ */
void sdl_hmac_sha256(const uint8_t* key, size_t key_len,
                     const uint8_t* msg, size_t msg_len,
                     uint8_t out[32]) {
    sdl_sha256_ctx_t ctx;
    uint8_t key_pad[64];
    uint8_t inner_hash[32];

    /* Step 1: Derive K' */
    memset(key_pad, 0, 64);
    if (key_len > 64) {
        sdl_sha256(key, key_len, key_pad);
    } else {
        memcpy(key_pad, key, key_len);
    }

    /* Step 2: Inner hash = SHA256((K' XOR ipad) || message) */
    sdl_sha256_init(&ctx);
    {
        uint8_t ipad_block[64];
        for (int i = 0; i < 64; i++) {
            ipad_block[i] = key_pad[i] ^ 0x36;
        }
        sdl_sha256_update(&ctx, ipad_block, 64);
        memset(ipad_block, 0, 64);
    }
    sdl_sha256_update(&ctx, msg, msg_len);
    sdl_sha256_final(&ctx, inner_hash);

    /* Step 3: Outer hash = SHA256((K' XOR opad) || inner_hash) */
    sdl_sha256_init(&ctx);
    {
        uint8_t opad_block[64];
        for (int i = 0; i < 64; i++) {
            opad_block[i] = key_pad[i] ^ 0x5C;
        }
        sdl_sha256_update(&ctx, opad_block, 64);
        memset(opad_block, 0, 64);
    }
    sdl_sha256_update(&ctx, inner_hash, 32);
    sdl_sha256_final(&ctx, out);

    memset(key_pad, 0, 64);
    memset(inner_hash, 0, 32);
}
