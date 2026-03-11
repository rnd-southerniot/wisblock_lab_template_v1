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
