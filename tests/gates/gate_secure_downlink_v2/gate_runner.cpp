/**
 * @file gate_runner.cpp
 * @brief Gate 10 — Secure Downlink Protocol v2 Validation Runner
 * @version 2.0
 * @date 2026-02-26
 *
 * Phase A: Local synthetic frame validation (unit tests)
 *   STEP 1: SystemManager init
 *   STEP 2: Reset nonce storage
 *   STEP 3: Valid frame — must accept
 *   STEP 4: Replay frame — must reject with replay_fail
 *   STEP 5: Bad magic — must reject with version_fail
 *   STEP 6: Bad HMAC — must reject with auth_fail
 *   STEP 7: Nonce persistence check
 *
 * Phase B: Live downlink validation
 *   STEP 8: Wait for live downlink from ChirpStack
 *   STEP 9: Re-send same live frame — confirm replay rejection
 *
 * All parameters from gate_config.h.
 * Zero hardcoded values in this file.
 *
 * Entry point: gate_secure_downlink_v2_run()
 */

#include <Arduino.h>
#include "gate_config.h"
#include "downlink_security.h"
#include "lorawan_hal.h"
#include "storage_hal.h"
#include "system_manager.h"

namespace {

uint32_t read_u32_or_default(const char* key, uint32_t fallback) {
    uint32_t value = fallback;
    if (storage_hal_read_u32(key, &value)) {
        return value;
    }
    return fallback;
}

}  // namespace

void gate_secure_downlink_v2_run(void) {
    uint8_t gate_result = GATE_FAIL_SYSTEM_CRASH;
    const uint8_t test_key[SDL_HMAC_KEY_LEN] = GATE_SDL_TEST_KEY;

    Serial.printf("\r\n%s === GATE START: %s v%s ===\r\n",
                  GATE_LOG_TAG, GATE_NAME, GATE_VERSION);
#ifdef CORE_RAK4631
    Serial.printf("%s Platform        : RAK4631 (nRF52840)\r\n", GATE_LOG_TAG);
#else
    Serial.printf("%s Platform        : RAK3312 (ESP32-S3)\r\n", GATE_LOG_TAG);
#endif
    Serial.printf("%s Frame format    : MAGIC+VER+CMD+FLAGS+NONCE(4)+PAYLOAD+HMAC(4)\r\n",
                  GATE_LOG_TAG);
    Serial.printf("%s HMAC truncation : %d bytes (MSB of SHA-256)\r\n",
                  GATE_LOG_TAG, SDL_HMAC_TRUNC_LEN);
    Serial.printf("%s Nonce key       : \"%s\"\r\n", GATE_LOG_TAG, SDL_NONCE_STORAGE_KEY);
    Serial.printf("%s Test frame len  : %d bytes\r\n", GATE_LOG_TAG, GATE_SDL_TEST_FRAME_LEN);

    Serial.printf("\r\n%s STEP 1: SystemManager init\r\n", GATE_LOG_TAG);
    SystemManager sys;
    uint32_t init_start = millis();
    bool init_ok = sys.init(GATE_SDL_JOIN_TIMEOUT_MS);
    uint32_t init_dur = millis() - init_start;
    if (!init_ok) {
        Serial.printf("%s   FAIL: SystemManager init failed (dur=%lu ms)\r\n",
#include "../../../firmware/runtime/system_manager.h"
#include "../../../firmware/runtime/downlink_security.h"
#include "../../../firmware/runtime/lora_transport.h"

/* ============================================================
 * Gate Entry Point
 * ============================================================ */
void gate_secure_downlink_v2_run(void) {
    uint8_t gate_result = GATE_FAIL_SYSTEM_CRASH;

    /* Load test key from config */
    const uint8_t test_key[] = GATE_SDL_TEST_KEY;

    /* ---- GATE START banner ---- */
    Serial.printf("\r\n%s === GATE START: %s v%s ===\r\n",
                  GATE_LOG_TAG, GATE_NAME, GATE_VERSION);
    Serial.printf("%s Frame: MAGIC(0xD1)+VER(0x02)+CMD+FLAGS+NONCE(4)+PAYLOAD+HMAC(4)\r\n",
                  GATE_LOG_TAG);
    Serial.printf("%s HMAC truncation  : %d bytes (MSB of SHA-256)\r\n",
                  GATE_LOG_TAG, SDL_HMAC_TRUNC_LEN);
    Serial.printf("%s Nonce storage key: \"%s\"\r\n",
                  GATE_LOG_TAG, SDL_NONCE_STORAGE_KEY);
    Serial.printf("%s Test frame len   : %d bytes\r\n",
                  GATE_LOG_TAG, GATE_SDL_TEST_FRAME_LEN);

    /* ============================================================
     * STEP 1 — SystemManager init
     * ============================================================ */
    Serial.printf("\r\n%s STEP 1: SystemManager init\r\n", GATE_LOG_TAG);

    SystemManager sys;

    uint32_t init_start = millis();
    bool init_ok = sys.init(GATE_SDL_JOIN_TIMEOUT_MS);
    uint32_t init_dur = millis() - init_start;

    if (!init_ok) {
        Serial.printf("%s SystemManager init: FAIL (dur=%lu ms)\r\n",
                      GATE_LOG_TAG, (unsigned long)init_dur);
        gate_result = GATE_FAIL_INIT;
        goto done;
    }
    Serial.printf("%s   Transport  : %s (connected=%s, dur=%lu ms)\r\n",
                  GATE_LOG_TAG,
                  sys.transport().name(),
                  sys.transport().isConnected() ? "yes" : "no",
                  (unsigned long)init_dur);
    Serial.printf("%s   Peripheral : %s\r\n", GATE_LOG_TAG, sys.peripheral().name());
    Serial.printf("%s   PASS: SystemManager init OK\r\n", GATE_LOG_TAG);

    Serial.printf("\r\n%s STEP 2: Reset nonce storage to %lu\r\n",
                  GATE_LOG_TAG, (unsigned long)GATE_SDL_INITIAL_NONCE);
    if (!storage_hal_write_u32(GATE_SDL_NONCE_KEY, GATE_SDL_INITIAL_NONCE)) {
        Serial.printf("%s   FAIL: Nonce reset write failed\r\n", GATE_LOG_TAG);
        gate_result = GATE_FAIL_NONCE_PERSIST;
        goto done;
    }
    {
        uint32_t readback = read_u32_or_default(GATE_SDL_NONCE_KEY, 0xFFFFFFFFUL);
        Serial.printf("%s   Readback    : %lu (expected %lu)\r\n",
                      GATE_LOG_TAG,
                      (unsigned long)readback,
                      (unsigned long)GATE_SDL_INITIAL_NONCE);
        if (readback != GATE_SDL_INITIAL_NONCE) {
            Serial.printf("%s   FAIL: Nonce reset failed — storage HAL not working\r\n",
                          GATE_LOG_TAG);
            gate_result = GATE_FAIL_NONCE_PERSIST;
            goto done;
        }
    }
    Serial.printf("%s   PASS: Nonce storage reset OK\r\n", GATE_LOG_TAG);

    {
        Serial.printf("\r\n%s STEP 3: Submit valid frame (CMD_SET_INTERVAL, nonce=1, 10000ms)\r\n",
                      GATE_LOG_TAG);
        uint8_t frame[] = GATE_SDL_TEST_FRAME;
        SecureDlResult r = secure_dl_validate_and_apply(frame, GATE_SDL_TEST_FRAME_LEN, sys, test_key);
        Serial.printf("%s   valid=%s  cmd_id=0x%02X  msg=\"%s\"\r\n",
                      GATE_LOG_TAG, r.valid ? "true" : "false", r.cmd_id, r.msg);
        if (!r.valid) {
            Serial.printf("%s   CRITERION FAIL: valid frame was rejected\r\n", GATE_LOG_TAG);
            Serial.printf("%s   auth=%s replay=%s version=%s\r\n",
                          GATE_LOG_TAG,
                          r.auth_fail ? "FAIL" : "ok",
                          r.replay_fail ? "FAIL" : "ok",
                          r.version_fail ? "FAIL" : "ok");
            gate_result = GATE_FAIL_VALID_REJECTED;
            goto done;
        }
        if (r.cmd_id != GATE_SDL_TEST_EXPECTED_CMD) {
            Serial.printf("%s   CRITERION FAIL: cmd_id=0x%02X expected 0x%02X\r\n",

    Serial.printf("%s SystemManager init: OK (dur=%lu ms)\r\n",
                  GATE_LOG_TAG, (unsigned long)init_dur);
    Serial.printf("%s   Transport  : %s (connected=%s)\r\n",
                  GATE_LOG_TAG,
                  sys.transport().name(),
                  sys.transport().isConnected() ? "yes" : "no");

    /* ============================================================
     * STEP 2 — Reset nonce storage
     * ============================================================ */
    Serial.printf("\r\n%s STEP 2: Reset nonce storage to %lu\r\n",
                  GATE_LOG_TAG, (unsigned long)GATE_SDL_INITIAL_NONCE);

    storage_hal_write_u32(GATE_SDL_NONCE_KEY, GATE_SDL_INITIAL_NONCE);

    {
        uint32_t readback = storage_hal_read_u32(GATE_SDL_NONCE_KEY, 0xFFFFFFFF);
        Serial.printf("%s   Nonce readback: %lu (expected %lu)\r\n",
                      GATE_LOG_TAG,
                      (unsigned long)readback,
                      (unsigned long)GATE_SDL_INITIAL_NONCE);
    }

    /* ============================================================
     * STEP 3 — Valid frame: CMD_SET_INTERVAL, nonce=1, 10000ms
     * ============================================================ */
    {
        Serial.printf("\r\n%s STEP 3: Submit valid frame (SET_INTERVAL, nonce=1, 10000ms)\r\n",
                      GATE_LOG_TAG);

        uint8_t frame[] = GATE_SDL_TEST_FRAME;
        SecureDlResult r = secure_dl_validate_and_apply(frame,
                                                         GATE_SDL_TEST_FRAME_LEN,
                                                         sys, test_key);

        Serial.printf("%s   valid=%s auth_fail=%s replay_fail=%s version_fail=%s\r\n",
                      GATE_LOG_TAG,
                      r.valid        ? "true" : "false",
                      r.auth_fail    ? "true" : "false",
                      r.replay_fail  ? "true" : "false",
                      r.version_fail ? "true" : "false");
        Serial.printf("%s   cmd_id=0x%02X msg=\"%s\"\r\n",
                      GATE_LOG_TAG, r.cmd_id, r.msg);

        if (!r.valid) {
            Serial.printf("%s   CRITERION FAIL: valid frame was rejected\r\n",
                          GATE_LOG_TAG);
            gate_result = GATE_FAIL_VALID_REJECTED;
            goto done;
        }

        if (r.cmd_id != GATE_SDL_TEST_EXPECTED_CMD) {
            Serial.printf("%s   CRITERION FAIL: cmd_id=0x%02X, expected 0x%02X\r\n",
                          GATE_LOG_TAG, r.cmd_id, GATE_SDL_TEST_EXPECTED_CMD);
            gate_result = GATE_FAIL_WRONG_CMD;
            goto done;
        }
        Serial.printf("%s   PASS: Valid frame accepted, cmd=0x%02X, interval=%d ms\r\n",
                      GATE_LOG_TAG, r.cmd_id, GATE_SDL_TEST_EXPECTED_INTERVAL);
    }

    {
        Serial.printf("\r\n%s STEP 4: Replay same frame (nonce=1, expect reject)\r\n",
                      GATE_LOG_TAG);
        uint8_t frame[] = GATE_SDL_TEST_FRAME;
        SecureDlResult r = secure_dl_validate_and_apply(frame, GATE_SDL_TEST_FRAME_LEN, sys, test_key);
        Serial.printf("%s   valid=%s  replay_fail=%s  msg=\"%s\"\r\n",
                      GATE_LOG_TAG,
                      r.valid ? "true" : "false",
                      r.replay_fail ? "true" : "false",
                      r.msg);
        if (r.valid) {
            Serial.printf("%s   CRITERION FAIL: replay frame was accepted\r\n", GATE_LOG_TAG);
            gate_result = GATE_FAIL_REPLAY_ACCEPTED;
            goto done;
        }
        if (!r.replay_fail) {
            Serial.printf("%s   CRITERION FAIL: rejected but replay_fail not set\r\n", GATE_LOG_TAG);
            gate_result = GATE_FAIL_REPLAY_FLAG;
            goto done;
        }
        Serial.printf("%s   PASS: Replay correctly rejected (replay_fail=true)\r\n", GATE_LOG_TAG);
    }

    {
        Serial.printf("\r\n%s STEP 5: Bad magic frame (0xFF, expect version_fail)\r\n",
                      GATE_LOG_TAG);
        uint8_t frame[] = GATE_SDL_BAD_MAGIC_FRAME;
        SecureDlResult r = secure_dl_validate_and_apply(frame, sizeof(frame), sys, test_key);
        Serial.printf("%s   valid=%s  version_fail=%s  msg=\"%s\"\r\n",
                      GATE_LOG_TAG,
                      r.valid ? "true" : "false",
                      r.version_fail ? "true" : "false",
                      r.msg);
        if (r.valid) {
            Serial.printf("%s   CRITERION FAIL: bad-magic frame was accepted\r\n", GATE_LOG_TAG);
            gate_result = GATE_FAIL_BAD_VERSION_ACCEPTED;
            goto done;
        }
        if (!r.version_fail) {
            Serial.printf("%s   CRITERION FAIL: rejected but version_fail not set\r\n", GATE_LOG_TAG);
            gate_result = GATE_FAIL_VERSION_FLAG;
            goto done;
        }
        Serial.printf("%s   PASS: Bad magic correctly rejected (version_fail=true)\r\n", GATE_LOG_TAG);
    }

    {
        Serial.printf("\r\n%s STEP 6: Tampered HMAC frame (zeroed, expect auth_fail)\r\n",
                      GATE_LOG_TAG);
        uint8_t frame[] = GATE_SDL_BAD_HMAC_FRAME;
        SecureDlResult r = secure_dl_validate_and_apply(frame, sizeof(frame), sys, test_key);
        Serial.printf("%s   valid=%s  auth_fail=%s  msg=\"%s\"\r\n",
                      GATE_LOG_TAG,
                      r.valid ? "true" : "false",
                      r.auth_fail ? "true" : "false",
                      r.msg);
        if (r.valid) {
            Serial.printf("%s   CRITERION FAIL: tampered HMAC frame was accepted\r\n", GATE_LOG_TAG);
            gate_result = GATE_FAIL_BAD_HMAC_ACCEPTED;
            goto done;
        }
        if (!r.auth_fail) {
            Serial.printf("%s   CRITERION FAIL: rejected but auth_fail not set\r\n", GATE_LOG_TAG);
            gate_result = GATE_FAIL_AUTH_FLAG;
            goto done;
        }
        Serial.printf("%s   PASS: Bad HMAC correctly rejected (auth_fail=true)\r\n", GATE_LOG_TAG);
    }

    {
        Serial.printf("\r\n%s STEP 7: Verify nonce persistence\r\n", GATE_LOG_TAG);
        uint32_t stored = read_u32_or_default(GATE_SDL_NONCE_KEY, 0xFFFFFFFFUL);
        Serial.printf("%s   Stored nonce : %lu (expected %lu)\r\n",
                      GATE_LOG_TAG,
                      (unsigned long)stored,
                      (unsigned long)GATE_SDL_TEST_EXPECTED_NONCE);
        if (stored != GATE_SDL_TEST_EXPECTED_NONCE) {
            Serial.printf("%s   CRITERION FAIL: nonce=%lu expected=%lu\r\n",
                          GATE_LOG_TAG,
                          (unsigned long)stored,
                          (unsigned long)GATE_SDL_TEST_EXPECTED_NONCE);
            gate_result = GATE_FAIL_NONCE_PERSIST;
            goto done;
        }
        Serial.printf("%s   PASS: Nonce persisted correctly (=%lu)\r\n",
                      GATE_LOG_TAG, (unsigned long)stored);
    }

    {
        Serial.printf("\r\n%s STEP 8: Waiting for live downlink (timeout=%lu ms)\r\n",
                      GATE_LOG_TAG, (unsigned long)GATE_SDL_DOWNLINK_TIMEOUT_MS);
        Serial.printf("%s   --> Queue a valid SDL v2 frame in ChirpStack now <--\r\n", GATE_LOG_TAG);
        Serial.printf("%s   Frame must use nonce > %lu\r\n",
                      GATE_LOG_TAG, (unsigned long)GATE_SDL_TEST_EXPECTED_NONCE);

        uint8_t dl_buf[64];
        uint8_t dl_port = 0;
        uint8_t dl_len = 0;

        Serial.printf("%s   PASS: Valid frame accepted, cmd=0x%02X\r\n",
                      GATE_LOG_TAG, r.cmd_id);
    }

    /* ============================================================
     * STEP 4 — Replay attack: same frame again (same nonce=1)
     * ============================================================ */
    {
        Serial.printf("\r\n%s STEP 4: Submit replay frame (same nonce=1)\r\n",
                      GATE_LOG_TAG);

        uint8_t frame[] = GATE_SDL_REPLAY_FRAME;
        SecureDlResult r = secure_dl_validate_and_apply(frame,
                                                         GATE_SDL_REPLAY_FRAME_LEN,
                                                         sys, test_key);

        Serial.printf("%s   valid=%s replay_fail=%s\r\n",
                      GATE_LOG_TAG,
                      r.valid       ? "true" : "false",
                      r.replay_fail ? "true" : "false");
        Serial.printf("%s   msg=\"%s\"\r\n", GATE_LOG_TAG, r.msg);

        if (r.valid) {
            Serial.printf("%s   CRITERION FAIL: replay frame was accepted\r\n",
                          GATE_LOG_TAG);
            gate_result = GATE_FAIL_REPLAY_ACCEPTED;
            goto done;
        }

        if (!r.replay_fail) {
            Serial.printf("%s   CRITERION FAIL: rejected but replay_fail not set\r\n",
                          GATE_LOG_TAG);
            gate_result = GATE_FAIL_REPLAY_FLAG;
            goto done;
        }

        Serial.printf("%s   PASS: Replay correctly rejected\r\n", GATE_LOG_TAG);
    }

    /* ============================================================
     * STEP 5 — Bad magic: 0xAA instead of 0xD1
     * ============================================================ */
    {
        Serial.printf("\r\n%s STEP 5: Submit bad-magic frame (magic=0xAA)\r\n",
                      GATE_LOG_TAG);

        uint8_t frame[] = GATE_SDL_BAD_VERSION_FRAME;
        SecureDlResult r = secure_dl_validate_and_apply(frame,
                                                         GATE_SDL_BAD_VERSION_FRAME_LEN,
                                                         sys, test_key);

        Serial.printf("%s   valid=%s version_fail=%s\r\n",
                      GATE_LOG_TAG,
                      r.valid        ? "true" : "false",
                      r.version_fail ? "true" : "false");
        Serial.printf("%s   msg=\"%s\"\r\n", GATE_LOG_TAG, r.msg);

        if (r.valid) {
            Serial.printf("%s   CRITERION FAIL: bad-magic frame was accepted\r\n",
                          GATE_LOG_TAG);
            gate_result = GATE_FAIL_BAD_VERSION_ACCEPTED;
            goto done;
        }

        if (!r.version_fail) {
            Serial.printf("%s   CRITERION FAIL: rejected but version_fail not set\r\n",
                          GATE_LOG_TAG);
            gate_result = GATE_FAIL_VERSION_FLAG;
            goto done;
        }

        Serial.printf("%s   PASS: Bad magic correctly rejected\r\n", GATE_LOG_TAG);
    }

    /* ============================================================
     * STEP 6 — Tampered HMAC: valid structure, zeroed HMAC
     * ============================================================ */
    {
        Serial.printf("\r\n%s STEP 6: Submit bad-HMAC frame (zeroed HMAC)\r\n",
                      GATE_LOG_TAG);

        uint8_t frame[] = GATE_SDL_BAD_HMAC_FRAME;
        SecureDlResult r = secure_dl_validate_and_apply(frame,
                                                         GATE_SDL_BAD_HMAC_FRAME_LEN,
                                                         sys, test_key);

        Serial.printf("%s   valid=%s auth_fail=%s\r\n",
                      GATE_LOG_TAG,
                      r.valid     ? "true" : "false",
                      r.auth_fail ? "true" : "false");
        Serial.printf("%s   msg=\"%s\"\r\n", GATE_LOG_TAG, r.msg);

        if (r.valid) {
            Serial.printf("%s   CRITERION FAIL: bad-HMAC frame was accepted\r\n",
                          GATE_LOG_TAG);
            gate_result = GATE_FAIL_BAD_HMAC_ACCEPTED;
            goto done;
        }

        if (!r.auth_fail) {
            Serial.printf("%s   CRITERION FAIL: rejected but auth_fail not set\r\n",
                          GATE_LOG_TAG);
            gate_result = GATE_FAIL_AUTH_FLAG;
            goto done;
        }

        Serial.printf("%s   PASS: Bad HMAC correctly rejected\r\n", GATE_LOG_TAG);
    }

    /* ============================================================
     * STEP 7 — Verify nonce persistence
     * ============================================================ */
    {
        Serial.printf("\r\n%s STEP 7: Verify nonce persistence\r\n", GATE_LOG_TAG);

        uint32_t stored = storage_hal_read_u32(GATE_SDL_NONCE_KEY, 0xFFFFFFFF);

        Serial.printf("%s   Stored nonce: %lu (expected %lu)\r\n",
                      GATE_LOG_TAG,
                      (unsigned long)stored,
                      (unsigned long)GATE_SDL_TEST_EXPECTED_NONCE);

        if (stored != GATE_SDL_TEST_EXPECTED_NONCE) {
            Serial.printf("%s   CRITERION FAIL: nonce not persisted correctly\r\n",
                          GATE_LOG_TAG);
            gate_result = GATE_FAIL_NONCE_PERSIST;
            goto done;
        }

        Serial.printf("%s   PASS: Nonce persisted correctly\r\n", GATE_LOG_TAG);
    }

    /* ============================================================
     * STEP 8 — Wait for live downlink from ChirpStack
     *
     * Operator sends a secure downlink via ChirpStack queue.
     * Device polls lorawan_hal_pop_downlink() + tick() for delivery.
     * ============================================================ */
    {
        Serial.printf("\r\n%s STEP 8: Waiting for live downlink (timeout=%lu ms)\r\n",
                      GATE_LOG_TAG, (unsigned long)GATE_SDL_DOWNLINK_TIMEOUT_MS);
        Serial.printf("%s   Send downlink via ChirpStack now...\r\n", GATE_LOG_TAG);

        uint8_t dl_buf[64];
        uint8_t dl_port = 0;
        uint8_t dl_len  = 0;
        uint32_t wait_start = millis();
        bool got_dl = false;

        while ((millis() - wait_start) < GATE_SDL_DOWNLINK_TIMEOUT_MS) {
            sys.tick();
            if (lorawan_hal_pop_downlink(dl_buf, &dl_len, &dl_port)) {
            sys.tick();  /* Process LoRa stack events */

            dl_len = lorawan_hal_pop_downlink(dl_buf, &dl_port);
            if (dl_len > 0) {
                got_dl = true;
                break;
            }
            delay(GATE_SDL_POLL_MS);
        }

        if (!got_dl) {
            Serial.printf("%s   TIMEOUT: No downlink in %lu ms\r\n",
            Serial.printf("%s   TIMEOUT: No downlink received in %lu ms\r\n",
                          GATE_LOG_TAG, (unsigned long)GATE_SDL_DOWNLINK_TIMEOUT_MS);
            gate_result = GATE_FAIL_DL_TIMEOUT;
            goto done;
        }

        Serial.printf("%s   Downlink received: port=%d len=%d\r\n", GATE_LOG_TAG, dl_port, dl_len);
        Serial.printf("%s   Hex: ", GATE_LOG_TAG);
        for (uint8_t i = 0; i < dl_len; ++i) {
        Serial.printf("%s   Downlink received: port=%d, len=%d\r\n",
                      GATE_LOG_TAG, dl_port, dl_len);
        Serial.printf("%s   Hex: ", GATE_LOG_TAG);
        for (uint8_t i = 0; i < dl_len; i++) {
            Serial.printf("%02X ", dl_buf[i]);
        }
        Serial.printf("\r\n");

        SecureDlResult r = secure_dl_validate_and_apply(dl_buf, dl_len, sys, test_key);
        Serial.printf("%s   valid=%s  cmd_id=0x%02X  msg=\"%s\"\r\n",
                      GATE_LOG_TAG, r.valid ? "true" : "false", r.cmd_id, r.msg);
        if (!r.valid) {
            Serial.printf("%s   CRITERION FAIL: live downlink rejected\r\n", GATE_LOG_TAG);
            Serial.printf("%s   auth=%s replay=%s version=%s\r\n",
                          GATE_LOG_TAG,
                          r.auth_fail ? "FAIL" : "ok",
                          r.replay_fail ? "FAIL" : "ok",
        /* Validate the live frame */
        SecureDlResult r = secure_dl_validate_and_apply(dl_buf, dl_len,
                                                         sys, test_key);

        Serial.printf("%s   valid=%s cmd_id=0x%02X\r\n",
                      GATE_LOG_TAG,
                      r.valid   ? "true" : "false",
                      r.cmd_id);
        Serial.printf("%s   msg=\"%s\"\r\n", GATE_LOG_TAG, r.msg);

        if (!r.valid) {
            Serial.printf("%s   CRITERION FAIL: live downlink failed validation\r\n",
                          GATE_LOG_TAG);
            Serial.printf("%s   auth=%s replay=%s version=%s\r\n",
                          GATE_LOG_TAG,
                          r.auth_fail    ? "FAIL" : "ok",
                          r.replay_fail  ? "FAIL" : "ok",
                          r.version_fail ? "FAIL" : "ok");
            gate_result = GATE_FAIL_DL_INVALID;
            goto done;
        }
        Serial.printf("%s   PASS: Live downlink validated and applied\r\n", GATE_LOG_TAG);

        Serial.printf("\r\n%s STEP 9: Replay live downlink (same nonce, expect reject)\r\n",
                      GATE_LOG_TAG);
        SecureDlResult r2 = secure_dl_validate_and_apply(dl_buf, dl_len, sys, test_key);
        Serial.printf("%s   valid=%s  replay_fail=%s  msg=\"%s\"\r\n",
                      GATE_LOG_TAG,
                      r2.valid ? "true" : "false",
                      r2.replay_fail ? "true" : "false",
                      r2.msg);
        if (r2.valid) {
            Serial.printf("%s   CRITERION FAIL: live replay was accepted\r\n", GATE_LOG_TAG);
            gate_result = GATE_FAIL_DL_REPLAY_ACCEPTED;
            goto done;
        }
        if (!r2.replay_fail) {
            Serial.printf("%s   CRITERION FAIL: replay rejected but replay_fail not set\r\n", GATE_LOG_TAG);
            gate_result = GATE_FAIL_REPLAY_FLAG;
            goto done;
        }
        Serial.printf("%s   PASS: Live replay correctly rejected\r\n", GATE_LOG_TAG);
    }

    gate_result = GATE_PASS;

done:

        Serial.printf("%s   PASS: Live downlink validated and applied\r\n",
                      GATE_LOG_TAG);

        /* ============================================================
         * STEP 9 — Replay the live frame
         * ============================================================ */
        Serial.printf("\r\n%s STEP 9: Replay live downlink (same nonce)\r\n",
                      GATE_LOG_TAG);

        SecureDlResult r2 = secure_dl_validate_and_apply(dl_buf, dl_len,
                                                          sys, test_key);

        Serial.printf("%s   valid=%s replay_fail=%s\r\n",
                      GATE_LOG_TAG,
                      r2.valid       ? "true" : "false",
                      r2.replay_fail ? "true" : "false");
        Serial.printf("%s   msg=\"%s\"\r\n", GATE_LOG_TAG, r2.msg);

        if (r2.valid) {
            Serial.printf("%s   CRITERION FAIL: live replay was accepted\r\n",
                          GATE_LOG_TAG);
            gate_result = GATE_FAIL_DL_REPLAY_ACCEPTED;
            goto done;
        }

        Serial.printf("%s   PASS: Live replay correctly rejected\r\n",
                      GATE_LOG_TAG);
    }

    /* All steps passed */
    gate_result = GATE_PASS;

done:
    /* ---- Final result ---- */
    Serial.printf("\r\n");
    if (gate_result == GATE_PASS) {
        Serial.printf("%s PASS\r\n", GATE_LOG_TAG);
    } else {
        Serial.printf("%s FAIL (code=%d)\r\n", GATE_LOG_TAG, gate_result);
    }

    Serial.printf("%s === GATE COMPLETE ===\r\n", GATE_LOG_TAG);
}
