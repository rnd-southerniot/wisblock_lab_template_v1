/**
 * @file gate_runner.cpp
 * @brief Gate 9 — Persistence Reboot Test v1 Runner
 * @version 1.0
 * @date 2026-02-25
 *
 * Two-phase reboot test:
 *   Phase 1 (first boot):
 *     - SystemManager init (join, modbus, scheduler)
 *     - storage_hal_init() — verify returns true
 *     - Write test values (interval=15000, slave=42)
 *     - Read-back verify (immediate)
 *     - Write phase marker → reboot
 *
 *   Phase 2 (after reboot):
 *     - storage_hal_init()
 *     - Read phase marker — must == GATE9_PHASE_2
 *     - Read persisted values — must match test values
 *     - SystemManager init — verify auto-loads persisted values
 *     - Clean up phase marker
 *     - Print PASS / FAIL
 *
 * Uses Serial.print() throughout (works on both nRF52840 and ESP32-S3).
 * Platform reboot via NVIC_SystemReset() / ESP.restart() — same as Gate 8.
 */

#include <Arduino.h>
#include "gate_config.h"
#include "storage_hal.h"
#include "system_manager.h"

/* ============================================================
 * Helper: FAIL and halt
 * ============================================================ */
static void gate_fail(const char* msg, uint8_t code) {
    Serial.print(GATE_LOG_TAG); Serial.print(" FAIL — "); Serial.println(msg);
    Serial.print(GATE_LOG_TAG); Serial.print(" Fail code: "); Serial.println(code);
    Serial.print(GATE_LOG_TAG); Serial.println(" === GATE COMPLETE ===");
}

/* ============================================================
 * Helper: Platform reboot
 * ============================================================ */
static void platform_reboot(void) {
#ifdef CORE_RAK4631
    NVIC_SystemReset();
#else
    ESP.restart();
#endif
}

/* ============================================================
 * Phase 1 — Pre-reboot: Write values, verify, store marker, reboot
 * ============================================================ */
static void run_phase_1(void) {
    Serial.println();
    Serial.print(GATE_LOG_TAG); Serial.println(" ===== PHASE 1: Pre-Reboot =====");

    /* STEP 1: SystemManager init */
    Serial.println();
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 1: SystemManager init");

    SystemManager sys;
    bool init_ok = sys.init(GATE9_JOIN_TIMEOUT_MS);

    if (!init_ok) {
        gate_fail("SystemManager init failed", GATE_FAIL_INIT);
        return;
    }

    Serial.print(GATE_LOG_TAG); Serial.print(" Transport: "); Serial.println(sys.transport().name());
    Serial.print(GATE_LOG_TAG); Serial.print(" Peripheral: "); Serial.println(sys.peripheral().name());
    Serial.print(GATE_LOG_TAG); Serial.print(" Connected: "); Serial.println(sys.transport().isConnected() ? "YES" : "NO");
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 1: PASS");

    /* STEP 2: storage_hal_init() */
    Serial.println();
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 2: storage_hal_init()");

    bool storage_ok = storage_hal_init();
    if (!storage_ok) {
        gate_fail("storage_hal_init() failed", GATE_FAIL_STORAGE);
        return;
    }
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 2: PASS — storage init OK");

    /* STEP 3: Write test values */
    Serial.println();
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 3: Write test values");

    bool w1 = storage_hal_write_u32(STORAGE_KEY_INTERVAL, GATE9_TEST_INTERVAL_MS);
    bool w2 = storage_hal_write_u8(STORAGE_KEY_SLAVE_ADDR, GATE9_TEST_SLAVE_ADDR);

    Serial.print(GATE_LOG_TAG); Serial.print(" Write interval ("); Serial.print(GATE9_TEST_INTERVAL_MS);
    Serial.print(" ms): "); Serial.println(w1 ? "OK" : "FAIL");
    Serial.print(GATE_LOG_TAG); Serial.print(" Write slave addr ("); Serial.print(GATE9_TEST_SLAVE_ADDR);
    Serial.print("): "); Serial.println(w2 ? "OK" : "FAIL");

    if (!w1 || !w2) {
        gate_fail("Storage write failed", GATE_FAIL_WRITE);
        return;
    }
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 3: PASS — values written");

    /* STEP 4: Read-back verify (immediate) */
    Serial.println();
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 4: Immediate read-back verify");

    uint32_t read_interval = 0;
    uint8_t  read_slave = 0;
    bool r1 = storage_hal_read_u32(STORAGE_KEY_INTERVAL, &read_interval);
    bool r2 = storage_hal_read_u8(STORAGE_KEY_SLAVE_ADDR, &read_slave);

    Serial.print(GATE_LOG_TAG); Serial.print(" Read interval: ");
    if (r1) { Serial.print(read_interval); Serial.println(" ms"); }
    else     { Serial.println("FAIL (key not found)"); }

    Serial.print(GATE_LOG_TAG); Serial.print(" Read slave addr: ");
    if (r2) { Serial.println(read_slave); }
    else     { Serial.println("FAIL (key not found)"); }

    if (!r1 || !r2 || read_interval != GATE9_TEST_INTERVAL_MS || read_slave != GATE9_TEST_SLAVE_ADDR) {
        gate_fail("Immediate read-back mismatch", GATE_FAIL_READBACK);
        return;
    }
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 4: PASS — read-back matches");

    /* STEP 5: Write phase marker */
    Serial.println();
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 5: Write phase marker");

    bool w3 = storage_hal_write_u8(STORAGE_KEY_GATE9_PHASE, GATE9_PHASE_2);
    if (!w3) {
        gate_fail("Phase marker write failed", GATE_FAIL_WRITE);
        return;
    }
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 5: PASS — phase marker written");

    /* STEP 6: Reboot */
    Serial.println();
    Serial.print(GATE_LOG_TAG); Serial.println(" PHASE 1 COMPLETE — rebooting in 2s");
    Serial.flush();
    delay(2000);
    platform_reboot();
}

/* ============================================================
 * Phase 2 — Post-reboot: Verify persisted values + auto-load
 * ============================================================ */
static void run_phase_2(void) {
    Serial.println();
    Serial.print(GATE_LOG_TAG); Serial.println(" ===== PHASE 2: Post-Reboot =====");

    bool all_pass = true;

    /* STEP 1: storage_hal_init() */
    Serial.println();
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 1: storage_hal_init()");

    bool storage_ok = storage_hal_init();
    if (!storage_ok) {
        gate_fail("storage_hal_init() failed (phase 2)", GATE_FAIL_STORAGE);
        return;
    }
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 1: PASS — storage init OK");

    /* STEP 2: Read phase marker */
    Serial.println();
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 2: Read phase marker");

    uint8_t phase_val = 0;
    bool phase_ok = storage_hal_read_u8(STORAGE_KEY_GATE9_PHASE, &phase_val);
    Serial.print(GATE_LOG_TAG); Serial.print(" Phase marker: ");
    if (phase_ok) { Serial.println(phase_val); }
    else          { Serial.println("NOT FOUND"); }

    if (!phase_ok || phase_val != GATE9_PHASE_2) {
        gate_fail("Phase marker mismatch", GATE_FAIL_PERSIST);
        return;
    }
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 2: PASS — phase marker correct");

    /* STEP 3: Read persisted values */
    Serial.println();
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 3: Read persisted values");

    uint32_t read_interval = 0;
    uint8_t  read_slave = 0;
    bool r1 = storage_hal_read_u32(STORAGE_KEY_INTERVAL, &read_interval);
    bool r2 = storage_hal_read_u8(STORAGE_KEY_SLAVE_ADDR, &read_slave);

    Serial.print(GATE_LOG_TAG); Serial.print(" Persisted interval: ");
    if (r1) { Serial.print(read_interval); Serial.println(" ms"); }
    else     { Serial.println("NOT FOUND"); all_pass = false; }

    Serial.print(GATE_LOG_TAG); Serial.print(" Persisted slave addr: ");
    if (r2) { Serial.println(read_slave); }
    else     { Serial.println("NOT FOUND"); all_pass = false; }

    if (!r1 || read_interval != GATE9_TEST_INTERVAL_MS) {
        Serial.print(GATE_LOG_TAG); Serial.print(" FAIL — interval expected ");
        Serial.print(GATE9_TEST_INTERVAL_MS); Serial.print(", got "); Serial.println(read_interval);
        all_pass = false;
    }

    if (!r2 || read_slave != GATE9_TEST_SLAVE_ADDR) {
        Serial.print(GATE_LOG_TAG); Serial.print(" FAIL — slave addr expected ");
        Serial.print(GATE9_TEST_SLAVE_ADDR); Serial.print(", got "); Serial.println(read_slave);
        all_pass = false;
    }

    if (all_pass) {
        Serial.print(GATE_LOG_TAG); Serial.println(" STEP 3: PASS — persisted values correct");
    } else {
        gate_fail("Persisted values mismatch", GATE_FAIL_PERSIST);
        return;
    }

    /* STEP 4: SystemManager init — verify auto-loads persisted values */
    Serial.println();
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 4: SystemManager init (verify auto-load)");

    SystemManager sys;
    bool init_ok = sys.init(GATE9_JOIN_TIMEOUT_MS);

    if (!init_ok) {
        gate_fail("SystemManager init failed (phase 2)", GATE_FAIL_INIT);
        return;
    }

    uint32_t loaded_interval = sys.scheduler().taskInterval(0);
    uint8_t  loaded_slave    = sys.peripheral().slaveAddr();

    Serial.print(GATE_LOG_TAG); Serial.print(" Auto-loaded interval: ");
    Serial.print(loaded_interval); Serial.println(" ms");
    Serial.print(GATE_LOG_TAG); Serial.print(" Auto-loaded slave addr: ");
    Serial.println(loaded_slave);

    if (loaded_interval != GATE9_TEST_INTERVAL_MS) {
        Serial.print(GATE_LOG_TAG); Serial.print(" FAIL — interval expected ");
        Serial.print(GATE9_TEST_INTERVAL_MS); Serial.print(", got "); Serial.println(loaded_interval);
        all_pass = false;
    }

    if (loaded_slave != GATE9_TEST_SLAVE_ADDR) {
        Serial.print(GATE_LOG_TAG); Serial.print(" FAIL — slave addr expected ");
        Serial.print(GATE9_TEST_SLAVE_ADDR); Serial.print(", got "); Serial.println(loaded_slave);
        all_pass = false;
    }

    if (all_pass) {
        Serial.print(GATE_LOG_TAG); Serial.println(" STEP 4: PASS — SystemManager auto-loaded persisted values");
    } else {
        gate_fail("SystemManager did not auto-load persisted values", GATE_FAIL_AUTOLOAD);
        return;
    }

    /* STEP 5: Clean up — remove gate9 phase marker */
    Serial.println();
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 5: Cleanup — remove phase marker");

    storage_hal_remove(STORAGE_KEY_GATE9_PHASE);
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 5: PASS — marker removed");

    /* STEP 6: Final result */
    Serial.println();
    if (all_pass) {
        Serial.print(GATE_LOG_TAG); Serial.println(" GATE 9 PASS — values persisted across reboot");
    } else {
        Serial.print(GATE_LOG_TAG); Serial.println(" GATE 9 FAIL");
    }

    Serial.println();
    Serial.print(GATE_LOG_TAG); Serial.println(" === GATE COMPLETE ===");
}

/* ============================================================
 * Gate Entry Point
 * ============================================================ */
void gate_persistence_reboot_v1_run(void) {

    Serial.println();
    Serial.println("========================================");
    Serial.print(GATE_LOG_TAG); Serial.println(" Gate 9: Persistence Reboot Test v1");
    Serial.print(GATE_LOG_TAG); Serial.print(" Gate ID: "); Serial.println(GATE_ID);
    Serial.println("========================================");

    /* Phase detection: read gate9_phase from storage */
    storage_hal_init();

    uint8_t phase = 0;
    bool has_phase = storage_hal_read_u8(STORAGE_KEY_GATE9_PHASE, &phase);

    if (has_phase && phase == GATE9_PHASE_2) {
        Serial.print(GATE_LOG_TAG); Serial.println(" Phase marker detected: PHASE 2 (post-reboot)");
        run_phase_2();
    } else {
        Serial.print(GATE_LOG_TAG); Serial.println(" No phase marker (or phase 0): running PHASE 1");
        run_phase_1();
    }
}
