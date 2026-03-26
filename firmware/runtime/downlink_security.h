#pragma once

#include <stdint.h>

class SystemManager;

#define SDL_FRAME_MAGIC        0xD1
#define SDL_PROTOCOL_VERSION   0x02
#define SDL_HMAC_KEY_LEN       32
#define SDL_HMAC_TRUNC_LEN     4
#define SDL_MIN_FRAME_LEN      12
#define SDL_MAX_FRAME_LEN      60
#define SDL_NONCE_STORAGE_KEY  "sdl_nonce"

#define SDL_CMD_SET_INTERVAL   0x01
#define SDL_CMD_SET_SLAVE_ADDR 0x02
#define SDL_CMD_REQUEST_STATUS 0x03
#define SDL_CMD_REBOOT         0x04

struct SecureDlResult {
    bool valid;
    uint8_t cmd_id;
    bool auth_fail;
    bool replay_fail;
    bool version_fail;
    char msg[96];
};

SecureDlResult secure_dl_validate_and_apply(const uint8_t* frame,
                                            uint8_t len,
                                            SystemManager& sys,
                                            const uint8_t key[SDL_HMAC_KEY_LEN]);
/**
 * @file downlink_security.h
 * @brief Secure Downlink Protocol v2 — HMAC-SHA256 authenticated command channel
 * @version 2.1
 * @date 2026-02-26
 *
 * Provides authenticated, replay-protected downlink command processing.
 * All downlink frames are validated via truncated HMAC-SHA256 and a
 * monotonically increasing nonce before any command is applied.
 *
 * Frame Format (Secure Downlink Protocol v2):
 * +--------+---------+--------+-------+-----------+-----------+----------------+
 * | Byte 0 | Byte 1  | Byte 2 | Byte 3| Bytes 4-7 | Bytes 8.. | Last 4 bytes   |
 * | MAGIC  | VERSION | CMD_ID | FLAGS | Nonce(BE) | Payload   | HMAC-trunc(BE) |
 * +--------+---------+--------+-------+-----------+-----------+----------------+
 *
 * - MAGIC:      0xD1 (frame sync / identifier)
 * - VERSION:    0x02 (protocol version)
 * - CMD_ID:     Command identifier (see SDL_CMD_* defines)
 * - FLAGS:      Reserved (must be 0x00 for v2)
 * - Nonce:      uint32_t big-endian, must be > last_valid_nonce
 * - Payload:    Command-specific data (0..48 bytes)
 * - HMAC-trunc: First 4 bytes of HMAC-SHA256(key, magic||ver||cmd||flags||nonce||payload)
 *
 * Minimum frame: 12 bytes (no payload)
 * Maximum frame: 60 bytes (48-byte payload, fits LoRaWAN DR2)
 *
 * HMAC Computation:
 *   input  = frame[0 .. len-5]  (everything except the 4-byte HMAC field)
 *   key    = pre-shared 32-byte secret (supplied by caller)
 *   output = HMAC-SHA256(key, input)
 *   trunc  = output[0..3]  (first 4 bytes / MSB-first truncation)
 *
 * Nonce Persistence:
 *   last_valid_nonce is persisted to flash via storage_hal using key
 *   SDL_NONCE_STORAGE_KEY ("sdl_nonce"). On boot, if no stored value
 *   exists, last_valid_nonce defaults to 0.
 *
 * Part of firmware/runtime/ production abstraction layer.
 */

#pragma once
#include <stdint.h>

/* Forward declaration — avoids pulling in full system_manager.h */
class SystemManager;

/* ============================================================
 * Protocol Constants
 * ============================================================ */

/** Magic byte — first byte of every secure downlink frame */
#define SDL_FRAME_MAGIC             0xD1

/** Protocol version byte */
#define SDL_PROTOCOL_VERSION        0x02

/** Minimum valid frame: magic(1)+ver(1)+cmd(1)+flags(1)+nonce(4)+hmac(4) = 12 */
#define SDL_FRAME_MIN_LEN           12

/** Maximum valid frame: min(12) + max_payload(48) = 60 */
#define SDL_FRAME_MAX_LEN           60

/** HMAC truncation length in bytes (first N bytes of HMAC-SHA256 output) */
#define SDL_HMAC_TRUNC_LEN          4

/** HMAC key length in bytes (HMAC-SHA256 key) */
#define SDL_HMAC_KEY_LEN            32

/** Nonce field length in bytes */
#define SDL_NONCE_LEN               4

/** Storage key for persisting last valid nonce via storage_hal */
#define SDL_NONCE_STORAGE_KEY       "sdl_nonce"

/** Default nonce value when no stored nonce exists */
#define SDL_NONCE_DEFAULT           0

/* ============================================================
 * Frame Byte Offsets
 * ============================================================ */
#define SDL_OFF_MAGIC               0
#define SDL_OFF_VERSION             1
#define SDL_OFF_CMD                 2
#define SDL_OFF_FLAGS               3
#define SDL_OFF_NONCE               4   /* 4 bytes, big-endian */
#define SDL_OFF_PAYLOAD             8   /* Variable length, 0..48 bytes */
/* HMAC offset = len - SDL_HMAC_TRUNC_LEN (last 4 bytes of frame) */

/* ============================================================
 * Command IDs
 * ============================================================ */

/** Set uplink interval (payload: uint32_t interval_ms, 4 bytes BE) */
#define SDL_CMD_SET_INTERVAL        0x01

/** Set Modbus slave address (payload: uint8_t addr, 1 byte) */
#define SDL_CMD_SET_SLAVE_ADDR      0x02

/** Request status uplink (payload: none) */
#define SDL_CMD_REQUEST_STATUS      0x03

/** Request device reboot (payload: none) */
#define SDL_CMD_REBOOT              0x04

/* ============================================================
 * Validation Result
 * ============================================================ */

/**
 * Result of secure downlink validation.
 *
 * On success (valid == true):
 *   - cmd_id contains the authenticated command
 *   - msg contains human-readable description
 *   - auth_fail, replay_fail, version_fail are all false
 *
 * On failure (valid == false):
 *   - Exactly one of auth_fail, replay_fail, version_fail is true
 *   - msg contains the rejection reason
 *   - cmd_id is 0x00
 */
struct SecureDlResult {
    bool     valid;           /**< true if frame passed all checks and was applied */
    bool     auth_fail;       /**< HMAC mismatch — tampered or wrong key */
    bool     replay_fail;     /**< Nonce <= last_valid_nonce — replay detected */
    bool     version_fail;    /**< Protocol version or magic mismatch */
    uint8_t  cmd_id;          /**< Authenticated command ID (0x00 if invalid) */
    char     msg[64];         /**< Human-readable result description */
};

/* ============================================================
 * Storage HAL — Required Platform Functions
 *
 * These must be provided by the platform (ESP32 NVS, EEPROM, etc.)
 * before secure_dl_validate_and_apply() can persist nonces.
 * ============================================================ */

/**
 * Write a uint32_t value to persistent storage.
 * @param key   Null-terminated key string
 * @param value Value to store
 * @return true if write succeeded
 */
extern bool storage_hal_write_u32(const char* key, uint32_t value);

/**
 * Read a uint32_t value from persistent storage.
 * @param key         Null-terminated key string
 * @param default_val Value to return if key does not exist
 * @return Stored value, or default_val if not found
 */
extern uint32_t storage_hal_read_u32(const char* key, uint32_t default_val);

/* ============================================================
 * Public API
 * ============================================================ */

/**
 * Validate and apply a Secure Downlink Protocol v2 frame.
 *
 * Validation order:
 *   1. Length check (SDL_FRAME_MIN_LEN <= len <= SDL_FRAME_MAX_LEN)
 *   2. Magic check (buf[0] == SDL_FRAME_MAGIC)
 *   3. Version check (buf[1] == SDL_PROTOCOL_VERSION)
 *   4. Nonce check (extracted nonce > last_valid_nonce from storage)
 *   5. HMAC check (computed HMAC-SHA256 truncated match)
 *   6. Command dispatch (apply command to SystemManager)
 *   7. Nonce persist (write new last_valid_nonce to storage)
 *
 * HMAC is checked AFTER nonce to avoid wasting CPU on replayed frames.
 * Nonce is persisted AFTER successful command application.
 *
 * @param buf   Raw downlink frame bytes
 * @param len   Frame length in bytes
 * @param sys   Reference to SystemManager for command application
 * @param key   32-byte HMAC key
 * @return SecureDlResult with validation outcome
 */
SecureDlResult secure_dl_validate_and_apply(
    const uint8_t* buf,
    uint8_t        len,
    SystemManager& sys,
    const uint8_t  key[32]
);

/* ============================================================
 * Internal Helpers (exposed for unit testing)
 * ============================================================ */

/**
 * Extract uint32_t from big-endian buffer.
 * @param buf  Pointer to 4 bytes in big-endian order
 * @return Decoded uint32_t value
 */
uint32_t sdl_read_u32_be(const uint8_t* buf);
