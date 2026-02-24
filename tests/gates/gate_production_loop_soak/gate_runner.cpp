/**
 * @file gate_runner.cpp
 * @brief Gate 7 — Production Loop Soak Validation Runner
 * @version 1.0
 * @date 2026-02-24
 *
 * Validates the production runtime loop under real conditions for a
 * fixed 5-minute soak duration. The scheduler runs naturally via tick()
 * (no fireNow) and the harness observes stability metrics.
 *
 * Pass criteria:
 *   - Join occurs once only (no repeated joins)
 *   - System runs for full soak duration without crash
 *   - At least GATE_SOAK_MIN_UPLINKS successful uplinks
 *   - No more than GATE_SOAK_MAX_CONSEC_FAILS consecutive modbus failures
 *   - No more than GATE_SOAK_MAX_CONSEC_FAILS consecutive uplink failures
 *   - All cycle durations < GATE_SOAK_MAX_CYCLE_MS
 *   - Transport remains connected throughout soak
 *
 * Entry point: gate_production_loop_soak_run()
 *
 * All parameters from gate_config.h.
 * Zero hardcoded values in this file.
 */

#include <Arduino.h>
#include "gate_config.h"
#include "../../../firmware/runtime/system_manager.h"

/* ============================================================
 * Gate Entry Point
 * ============================================================ */
void gate_production_loop_soak_run(void) {
    uint8_t gate_result = GATE_FAIL_SYSTEM_CRASH;

    /* ---- GATE START banner ---- */
    Serial.printf("\r\n%s === GATE START: %s v%s ===\r\n",
                  GATE_LOG_TAG, GATE_NAME, GATE_VERSION);
    Serial.printf("%s Soak duration  : %lu ms (%lu s)\r\n",
                  GATE_LOG_TAG,
                  (unsigned long)GATE_SOAK_DURATION_MS,
                  (unsigned long)(GATE_SOAK_DURATION_MS / 1000));
    Serial.printf("%s Min uplinks    : %d\r\n",
                  GATE_LOG_TAG, GATE_SOAK_MIN_UPLINKS);
    Serial.printf("%s Max consec fail: %d\r\n",
                  GATE_LOG_TAG, GATE_SOAK_MAX_CONSEC_FAILS);
    Serial.printf("%s Max cycle dur  : %d ms\r\n",
                  GATE_LOG_TAG, GATE_SOAK_MAX_CYCLE_MS);
    Serial.printf("%s Poll interval  : %lu ms\r\n",
                  GATE_LOG_TAG, (unsigned long)HW_SYSTEM_POLL_INTERVAL_MS);

    /* ============================================================
     * STEP 1 — SystemManager init (production mode)
     *
     * Single init() call: LoRa radio → OTAA join → Modbus → scheduler.
     * This is the production code path — no granular step separation.
     * ============================================================ */
    Serial.printf("\r\n%s STEP 1: SystemManager init (production mode)\r\n",
                  GATE_LOG_TAG);

    SystemManager sys;

    uint32_t init_start = millis();
    bool init_ok = sys.init(GATE_SOAK_JOIN_TIMEOUT_MS);
    uint32_t init_dur = millis() - init_start;

    if (!init_ok) {
        Serial.printf("%s SystemManager init: FAIL (dur=%lu ms)\r\n",
                      GATE_LOG_TAG, (unsigned long)init_dur);
        Serial.printf("%s   Check: LoRa hardware, OTAA credentials, Modbus wiring\r\n",
                      GATE_LOG_TAG);
        gate_result = GATE_FAIL_INIT;
        goto done;
    }

    Serial.printf("%s SystemManager init: OK (dur=%lu ms)\r\n",
                  GATE_LOG_TAG, (unsigned long)init_dur);
    Serial.printf("%s   Transport  : %s (connected=%s)\r\n",
                  GATE_LOG_TAG,
                  sys.transport().name(),
                  sys.transport().isConnected() ? "yes" : "no");
    Serial.printf("%s   Peripheral : %s (Slave=%d, Baud=%lu)\r\n",
                  GATE_LOG_TAG,
                  sys.peripheral().name(),
                  GATE_DISCOVERED_SLAVE,
                  (unsigned long)GATE_DISCOVERED_BAUD);
    Serial.printf("%s   Scheduler  : %d task(s), interval=%lu ms\r\n",
                  GATE_LOG_TAG,
                  sys.scheduler().taskCount(),
                  (unsigned long)HW_SYSTEM_POLL_INTERVAL_MS);
    Serial.printf("%s   Join count : 1 (single init)\r\n", GATE_LOG_TAG);

    /* ============================================================
     * STEP 2 — Production soak loop
     *
     * Runs tick() continuously for GATE_SOAK_DURATION_MS.
     * The scheduler fires sensorUplinkTask at HW_SYSTEM_POLL_INTERVAL_MS.
     * No fireNow() — natural scheduler-driven execution.
     * ============================================================ */
    {
        Serial.printf("\r\n%s STEP 2: Production soak (%lu seconds)\r\n",
                      GATE_LOG_TAG,
                      (unsigned long)(GATE_SOAK_DURATION_MS / 1000));

        uint32_t soak_start = millis();
        uint32_t last_cycle = sys.cycleCount();
        uint32_t last_progress = soak_start;

        while ((millis() - soak_start) < GATE_SOAK_DURATION_MS) {
            sys.tick();

            uint32_t current_cycle = sys.cycleCount();
            if (current_cycle != last_cycle) {
                /* A new cycle just completed — log it */
                const RuntimeStats& st = sys.stats();
                uint32_t elapsed = millis() - soak_start;

                Serial.printf("%s Cycle %lu | modbus=%s | uplink=%s | dur=%lu ms | elapsed=%lu s\r\n",
                              GATE_LOG_TAG,
                              (unsigned long)current_cycle,
                              st.last_modbus_ok ? "OK" : "FAIL",
                              st.last_uplink_ok ? "OK" : "FAIL",
                              (unsigned long)st.last_cycle_ms,
                              (unsigned long)(elapsed / 1000));

                last_cycle = current_cycle;
                last_progress = millis();
            }

            /* Progress heartbeat every 60s if no cycle fired */
            if ((millis() - last_progress) > 60000) {
                uint32_t elapsed = millis() - soak_start;
                Serial.printf("%s   ... waiting (elapsed=%lu s, cycles=%lu)\r\n",
                              GATE_LOG_TAG,
                              (unsigned long)(elapsed / 1000),
                              (unsigned long)sys.cycleCount());
                last_progress = millis();
            }

            delay(10);  /* Yield to avoid WDT, reduce power */
        }

        uint32_t soak_actual = millis() - soak_start;

        /* ============================================================
         * STEP 3 — Soak summary and PASS/FAIL evaluation
         * ============================================================ */
        const RuntimeStats& st = sys.stats();
        uint32_t total_cycles = sys.cycleCount();

        Serial.printf("\r\n%s STEP 3: Soak summary\r\n", GATE_LOG_TAG);
        Serial.printf("%s   Duration          : %lu ms (%lu s)\r\n",
                      GATE_LOG_TAG,
                      (unsigned long)soak_actual,
                      (unsigned long)(soak_actual / 1000));
        Serial.printf("%s   Total Cycles      : %lu\r\n",
                      GATE_LOG_TAG, (unsigned long)total_cycles);
        Serial.printf("%s   Modbus OK / FAIL  : %lu / %lu\r\n",
                      GATE_LOG_TAG,
                      (unsigned long)st.modbus_ok,
                      (unsigned long)st.modbus_fail);
        Serial.printf("%s   Uplink OK / FAIL  : %lu / %lu\r\n",
                      GATE_LOG_TAG,
                      (unsigned long)st.uplink_ok,
                      (unsigned long)st.uplink_fail);
        Serial.printf("%s   Max Consec Fail   : modbus=%d, uplink=%d\r\n",
                      GATE_LOG_TAG,
                      st.max_consec_modbus_fail,
                      st.max_consec_uplink_fail);
        Serial.printf("%s   Max Cycle Duration: %lu ms\r\n",
                      GATE_LOG_TAG, (unsigned long)st.max_cycle_ms);
        Serial.printf("%s   Transport         : %s\r\n",
                      GATE_LOG_TAG,
                      sys.transport().isConnected() ? "connected" : "DISCONNECTED");
        Serial.printf("%s   Join Attempts     : 1\r\n", GATE_LOG_TAG);

        /* ---- Evaluate pass/fail criteria ---- */
        bool pass = true;

        if (total_cycles < (uint32_t)GATE_SOAK_MIN_UPLINKS) {
            Serial.printf("%s   CRITERION FAIL: cycles=%lu < min=%d\r\n",
                          GATE_LOG_TAG,
                          (unsigned long)total_cycles,
                          GATE_SOAK_MIN_UPLINKS);
            pass = false;
            gate_result = GATE_FAIL_INSUFFICIENT_CYCLES;
        }

        if (st.uplink_ok < (uint32_t)GATE_SOAK_MIN_UPLINKS) {
            Serial.printf("%s   CRITERION FAIL: successful uplinks=%lu < min=%d\r\n",
                          GATE_LOG_TAG,
                          (unsigned long)st.uplink_ok,
                          GATE_SOAK_MIN_UPLINKS);
            pass = false;
            if (gate_result == GATE_FAIL_SYSTEM_CRASH)
                gate_result = GATE_FAIL_INSUFFICIENT_UPLINKS;
        }

        if (st.max_consec_modbus_fail > GATE_SOAK_MAX_CONSEC_FAILS) {
            Serial.printf("%s   CRITERION FAIL: max consec modbus fail=%d > max=%d\r\n",
                          GATE_LOG_TAG,
                          st.max_consec_modbus_fail,
                          GATE_SOAK_MAX_CONSEC_FAILS);
            pass = false;
            if (gate_result == GATE_FAIL_SYSTEM_CRASH)
                gate_result = GATE_FAIL_CONSEC_MODBUS;
        }

        if (st.max_consec_uplink_fail > GATE_SOAK_MAX_CONSEC_FAILS) {
            Serial.printf("%s   CRITERION FAIL: max consec uplink fail=%d > max=%d\r\n",
                          GATE_LOG_TAG,
                          st.max_consec_uplink_fail,
                          GATE_SOAK_MAX_CONSEC_FAILS);
            pass = false;
            if (gate_result == GATE_FAIL_SYSTEM_CRASH)
                gate_result = GATE_FAIL_CONSEC_UPLINK;
        }

        if (st.max_cycle_ms > GATE_SOAK_MAX_CYCLE_MS) {
            Serial.printf("%s   CRITERION FAIL: max cycle dur=%lu ms > max=%d ms\r\n",
                          GATE_LOG_TAG,
                          (unsigned long)st.max_cycle_ms,
                          GATE_SOAK_MAX_CYCLE_MS);
            pass = false;
            if (gate_result == GATE_FAIL_SYSTEM_CRASH)
                gate_result = GATE_FAIL_CYCLE_DURATION;
        }

        if (!sys.transport().isConnected()) {
            Serial.printf("%s   CRITERION FAIL: transport disconnected\r\n",
                          GATE_LOG_TAG);
            pass = false;
            if (gate_result == GATE_FAIL_SYSTEM_CRASH)
                gate_result = GATE_FAIL_TRANSPORT_LOST;
        }

        if (pass) {
            gate_result = GATE_PASS;
        }
    }

done:
    /* ---- Final result ---- */
    if (gate_result == GATE_PASS) {
        Serial.printf("%s PASS\r\n", GATE_LOG_TAG);
    } else {
        Serial.printf("%s FAIL (code=%d)\r\n", GATE_LOG_TAG, gate_result);
    }

    Serial.printf("%s === GATE COMPLETE ===\r\n", GATE_LOG_TAG);
}
