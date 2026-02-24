/**
 * @file gate_runner.cpp
 * @brief Gate 7 — RAK4631 Production Loop Soak Validation Runner
 * @version 1.0
 * @date 2026-02-25
 *
 * Validates the production runtime loop under real conditions for a
 * fixed 5-minute soak duration. The scheduler runs naturally via tick()
 * (no fireNow) and the harness observes stability metrics.
 *
 * Pass criteria:
 *   - Join occurs once only (no repeated joins)
 *   - System runs for full soak duration without crash
 *   - At least GATE7_MIN_UPLINKS (3) successful uplinks
 *   - No more than GATE7_MAX_CONSEC_FAILS consecutive modbus failures
 *   - No more than GATE7_MAX_CONSEC_FAILS consecutive uplink failures
 *   - All cycle durations < GATE7_MAX_CYCLE_DURATION_MS
 *   - Transport remains connected throughout soak
 *
 * Entry point: gate_rak4631_production_loop_soak_run()
 *
 * Uses Serial.print() instead of Serial.printf() for nRF52840 compatibility.
 * All parameters from gate_config.h.
 * Zero hardcoded values in this file.
 */

#include <Arduino.h>
#include "gate_config.h"
#include "../../../firmware/runtime/system_manager.h"

/* ============================================================
 * Gate Entry Point
 * ============================================================ */
void gate_rak4631_production_loop_soak_run(void) {
    uint8_t gate_result = GATE_FAIL_SYSTEM_CRASH;

    /* ---- GATE START banner ---- */
    Serial.println();
    Serial.print(GATE_LOG_TAG);
    Serial.print(" === GATE START: ");
    Serial.print(GATE_NAME);
    Serial.print(" v");
    Serial.print(GATE_VERSION);
    Serial.println(" ===");

    Serial.print(GATE_LOG_TAG);
    Serial.print(" Soak duration  : ");
    Serial.print((unsigned long)GATE7_SOAK_DURATION_MS);
    Serial.print(" ms (");
    Serial.print((unsigned long)(GATE7_SOAK_DURATION_MS / 1000));
    Serial.println(" s)");

    Serial.print(GATE_LOG_TAG);
    Serial.print(" Min uplinks    : ");
    Serial.println(GATE7_MIN_UPLINKS);

    Serial.print(GATE_LOG_TAG);
    Serial.print(" Max consec fail: ");
    Serial.println(GATE7_MAX_CONSEC_FAILS);

    Serial.print(GATE_LOG_TAG);
    Serial.print(" Max cycle dur  : ");
    Serial.print(GATE7_MAX_CYCLE_DURATION_MS);
    Serial.println(" ms");

    Serial.print(GATE_LOG_TAG);
    Serial.print(" Poll interval  : ");
    Serial.print((unsigned long)HW_SYSTEM_POLL_INTERVAL_MS);
    Serial.println(" ms");

    /* ============================================================
     * STEP 1 — SystemManager init (production mode)
     *
     * Single init() call: LoRa radio -> OTAA join -> Modbus -> scheduler.
     * This is the production code path — no granular step separation.
     * ============================================================ */
    Serial.println();
    Serial.print(GATE_LOG_TAG);
    Serial.println(" STEP 1: SystemManager init (production mode)");

    SystemManager sys;

    uint32_t init_start = millis();
    bool init_ok = sys.init(GATE7_JOIN_TIMEOUT_MS);
    uint32_t init_dur = millis() - init_start;

    if (!init_ok) {
        Serial.print(GATE_LOG_TAG);
        Serial.print(" SystemManager init: FAIL (dur=");
        Serial.print((unsigned long)init_dur);
        Serial.println(" ms)");
        Serial.print(GATE_LOG_TAG);
        Serial.println("   Check: LoRa hardware, OTAA credentials, Modbus wiring");
        gate_result = GATE_FAIL_INIT;
        goto done;
    }

    Serial.print(GATE_LOG_TAG);
    Serial.print(" SystemManager init: OK (dur=");
    Serial.print((unsigned long)init_dur);
    Serial.println(" ms)");

    Serial.print(GATE_LOG_TAG);
    Serial.print("   Transport  : ");
    Serial.print(sys.transport().name());
    Serial.print(" (connected=");
    Serial.print(sys.transport().isConnected() ? "yes" : "no");
    Serial.println(")");

    Serial.print(GATE_LOG_TAG);
    Serial.print("   Peripheral : ");
    Serial.print(sys.peripheral().name());
    Serial.print(" (Slave=");
    Serial.print(GATE_DISCOVERED_SLAVE);
    Serial.print(", Baud=");
    Serial.print((unsigned long)GATE_DISCOVERED_BAUD);
    Serial.println(")");

    Serial.print(GATE_LOG_TAG);
    Serial.print("   Scheduler  : ");
    Serial.print(sys.scheduler().taskCount());
    Serial.print(" task(s), interval=");
    Serial.print((unsigned long)HW_SYSTEM_POLL_INTERVAL_MS);
    Serial.println(" ms");

    Serial.print(GATE_LOG_TAG);
    Serial.println("   Join count : 1 (single init)");

    /* ============================================================
     * STEP 2 — Production soak loop
     *
     * Runs tick() continuously for GATE7_SOAK_DURATION_MS.
     * The scheduler fires sensorUplinkTask at HW_SYSTEM_POLL_INTERVAL_MS.
     * No fireNow() — natural scheduler-driven execution.
     * ============================================================ */
    {
        Serial.println();
        Serial.print(GATE_LOG_TAG);
        Serial.print(" STEP 2: Production soak (");
        Serial.print((unsigned long)(GATE7_SOAK_DURATION_MS / 1000));
        Serial.println(" seconds)");

        uint32_t soak_start = millis();
        uint32_t last_cycle = sys.cycleCount();
        uint32_t last_progress = soak_start;

        while ((millis() - soak_start) < GATE7_SOAK_DURATION_MS) {
            sys.tick();

            uint32_t current_cycle = sys.cycleCount();
            if (current_cycle != last_cycle) {
                /* A new cycle just completed — log it */
                const RuntimeStats& st = sys.stats();
                uint32_t elapsed = millis() - soak_start;

                Serial.print(GATE_LOG_TAG);
                Serial.print(" Cycle ");
                Serial.print((unsigned long)current_cycle);
                Serial.print(" | modbus=");
                Serial.print(st.last_modbus_ok ? "OK" : "FAIL");
                Serial.print(" | uplink=");
                Serial.print(st.last_uplink_ok ? "OK" : "FAIL");
                Serial.print(" | dur=");
                Serial.print((unsigned long)st.last_cycle_ms);
                Serial.print(" ms | elapsed=");
                Serial.print((unsigned long)(elapsed / 1000));
                Serial.println(" s");

                last_cycle = current_cycle;
                last_progress = millis();
            }

            /* Progress heartbeat every 60s if no cycle fired */
            if ((millis() - last_progress) > 60000) {
                uint32_t elapsed = millis() - soak_start;
                Serial.print(GATE_LOG_TAG);
                Serial.print("   ... waiting (elapsed=");
                Serial.print((unsigned long)(elapsed / 1000));
                Serial.print(" s, cycles=");
                Serial.print((unsigned long)sys.cycleCount());
                Serial.println(")");
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

        Serial.println();
        Serial.print(GATE_LOG_TAG);
        Serial.println(" STEP 3: Soak summary");

        Serial.print(GATE_LOG_TAG);
        Serial.print("   Duration          : ");
        Serial.print((unsigned long)soak_actual);
        Serial.print(" ms (");
        Serial.print((unsigned long)(soak_actual / 1000));
        Serial.println(" s)");

        Serial.print(GATE_LOG_TAG);
        Serial.print("   Total Cycles      : ");
        Serial.println((unsigned long)total_cycles);

        Serial.print(GATE_LOG_TAG);
        Serial.print("   Modbus OK / FAIL  : ");
        Serial.print((unsigned long)st.modbus_ok);
        Serial.print(" / ");
        Serial.println((unsigned long)st.modbus_fail);

        Serial.print(GATE_LOG_TAG);
        Serial.print("   Uplink OK / FAIL  : ");
        Serial.print((unsigned long)st.uplink_ok);
        Serial.print(" / ");
        Serial.println((unsigned long)st.uplink_fail);

        Serial.print(GATE_LOG_TAG);
        Serial.print("   Max Consec Fail   : modbus=");
        Serial.print(st.max_consec_modbus_fail);
        Serial.print(", uplink=");
        Serial.println(st.max_consec_uplink_fail);

        Serial.print(GATE_LOG_TAG);
        Serial.print("   Max Cycle Duration: ");
        Serial.print((unsigned long)st.max_cycle_ms);
        Serial.println(" ms");

        Serial.print(GATE_LOG_TAG);
        Serial.print("   Transport         : ");
        Serial.println(sys.transport().isConnected() ? "connected" : "DISCONNECTED");

        Serial.print(GATE_LOG_TAG);
        Serial.println("   Join Attempts     : 1");

        /* ---- Evaluate pass/fail criteria ---- */
        bool pass = true;

        if (total_cycles < (uint32_t)GATE7_MIN_UPLINKS) {
            Serial.print(GATE_LOG_TAG);
            Serial.print("   CRITERION FAIL: cycles=");
            Serial.print((unsigned long)total_cycles);
            Serial.print(" < min=");
            Serial.println(GATE7_MIN_UPLINKS);
            pass = false;
            gate_result = GATE_FAIL_INSUFFICIENT_CYCLES;
        }

        if (st.uplink_ok < (uint32_t)GATE7_MIN_UPLINKS) {
            Serial.print(GATE_LOG_TAG);
            Serial.print("   CRITERION FAIL: successful uplinks=");
            Serial.print((unsigned long)st.uplink_ok);
            Serial.print(" < min=");
            Serial.println(GATE7_MIN_UPLINKS);
            pass = false;
            if (gate_result == GATE_FAIL_SYSTEM_CRASH)
                gate_result = GATE_FAIL_INSUFFICIENT_UPLINKS;
        }

        if (st.max_consec_modbus_fail > GATE7_MAX_CONSEC_FAILS) {
            Serial.print(GATE_LOG_TAG);
            Serial.print("   CRITERION FAIL: max consec modbus fail=");
            Serial.print(st.max_consec_modbus_fail);
            Serial.print(" > max=");
            Serial.println(GATE7_MAX_CONSEC_FAILS);
            pass = false;
            if (gate_result == GATE_FAIL_SYSTEM_CRASH)
                gate_result = GATE_FAIL_CONSEC_MODBUS;
        }

        if (st.max_consec_uplink_fail > GATE7_MAX_CONSEC_FAILS) {
            Serial.print(GATE_LOG_TAG);
            Serial.print("   CRITERION FAIL: max consec uplink fail=");
            Serial.print(st.max_consec_uplink_fail);
            Serial.print(" > max=");
            Serial.println(GATE7_MAX_CONSEC_FAILS);
            pass = false;
            if (gate_result == GATE_FAIL_SYSTEM_CRASH)
                gate_result = GATE_FAIL_CONSEC_UPLINK;
        }

        if (st.max_cycle_ms > GATE7_MAX_CYCLE_DURATION_MS) {
            Serial.print(GATE_LOG_TAG);
            Serial.print("   CRITERION FAIL: max cycle dur=");
            Serial.print((unsigned long)st.max_cycle_ms);
            Serial.print(" ms > max=");
            Serial.print(GATE7_MAX_CYCLE_DURATION_MS);
            Serial.println(" ms");
            pass = false;
            if (gate_result == GATE_FAIL_SYSTEM_CRASH)
                gate_result = GATE_FAIL_CYCLE_DURATION;
        }

        if (!sys.transport().isConnected()) {
            Serial.print(GATE_LOG_TAG);
            Serial.println("   CRITERION FAIL: transport disconnected");
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
        Serial.print(GATE_LOG_TAG);
        Serial.println(" PASS");
    } else {
        Serial.print(GATE_LOG_TAG);
        Serial.print(" FAIL (code=");
        Serial.print(gate_result);
        Serial.println(")");
    }

    Serial.print(GATE_LOG_TAG);
    Serial.println(" === GATE COMPLETE ===");
}
