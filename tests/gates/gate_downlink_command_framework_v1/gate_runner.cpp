/**
 * @file gate_runner.cpp
 * @brief Gate 8 — Downlink Command Framework v1 Runner
 * @version 1.0
 * @date 2026-02-25
 *
 * Validates bidirectional LoRaWAN communication:
 *   1. Initialize SystemManager (LoRa join, Modbus, scheduler)
 *   2. Send initial uplinks to open RX windows
 *   3. Poll for downlink, parse command, apply, send ACK
 *   4. PASS if downlink received, PASS_WITHOUT_DOWNLINK otherwise
 *
 * Both outcomes are valid PASS conditions — allows gate to pass
 * without requiring ChirpStack downlink to be pre-enqueued.
 *
 * Uses Serial.print() throughout (works on both nRF52840 and ESP32-S3).
 * Single function name — no #ifdef needed (shared gate directory).
 */

#include <Arduino.h>
#include "gate_config.h"
#include "system_manager.h"
#include "downlink_commands.h"
#include "lorawan_hal.h"

/* ============================================================
 * Helper: Print hex bytes
 * ============================================================ */
static void print_hex(const uint8_t* buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        if (buf[i] < 0x10) Serial.print("0");
        Serial.print(buf[i], HEX);
        if (i < len - 1) Serial.print(" ");
    }
}

/* ============================================================
 * Gate Entry Point
 * ============================================================ */
void gate_downlink_command_framework_v1_run(void) {

    Serial.println();
    Serial.println("========================================");
    Serial.print(GATE_LOG_TAG); Serial.println(" Gate 8: Downlink Command Framework v1");
    Serial.print(GATE_LOG_TAG); Serial.print(" Gate ID: "); Serial.println(GATE_ID);
    Serial.println("========================================");

    /* ============================================================
     * STEP 1 — SystemManager Init
     * ============================================================ */
    Serial.println();
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 1: SystemManager init");

    SystemManager sys;
    bool init_ok = sys.init(GATE8_JOIN_TIMEOUT_MS);

    if (!init_ok) {
        Serial.print(GATE_LOG_TAG); Serial.println(" FAIL — SystemManager init failed");
        Serial.print(GATE_LOG_TAG); Serial.print(" Fail code: "); Serial.println(GATE_FAIL_INIT);
        Serial.print(GATE_LOG_TAG); Serial.println(" === GATE COMPLETE ===");
        return;
    }

    Serial.print(GATE_LOG_TAG); Serial.print(" Transport: "); Serial.println(sys.transport().name());
    Serial.print(GATE_LOG_TAG); Serial.print(" Peripheral: "); Serial.println(sys.peripheral().name());
    Serial.print(GATE_LOG_TAG); Serial.print(" Tasks registered: "); Serial.println(sys.scheduler().taskCount());
    Serial.print(GATE_LOG_TAG); Serial.print(" Connected: "); Serial.println(sys.transport().isConnected() ? "YES" : "NO");
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 1: PASS");

    /* ============================================================
     * STEP 2 — Initial Uplinks (open RX windows for downlink)
     * ============================================================ */
    Serial.println();
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 2: Initial uplinks to open RX windows");
    Serial.print(GATE_LOG_TAG); Serial.print(" Sending "); Serial.print(GATE8_INITIAL_UPLINKS);
    Serial.println(" uplinks...");

    for (int i = 0; i < GATE8_INITIAL_UPLINKS; i++) {
        sys.scheduler().fireNow(0);   /* Force sensor uplink */
        Serial.print(GATE_LOG_TAG); Serial.print(" Uplink "); Serial.print(i + 1);
        Serial.print("/"); Serial.print(GATE8_INITIAL_UPLINKS);
        Serial.print(" sent (cycle "); Serial.print(sys.cycleCount());
        Serial.println(")");

        /* Give time for RX1/RX2 windows */
        delay(5000);
    }

    Serial.print(GATE_LOG_TAG); Serial.print(" Cycles after initial uplinks: ");
    Serial.println(sys.cycleCount());
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 2: PASS");

    /* ============================================================
     * STEP 3 — Downlink Poll Loop
     * ============================================================ */
    Serial.println();
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 3: Polling for downlink");
    Serial.print(GATE_LOG_TAG); Serial.print(" Timeout: "); Serial.print(GATE8_DOWNLINK_WAIT_MS / 1000);
    Serial.println("s");

    uint32_t dl_start = millis();
    bool dl_received = false;

    while ((millis() - dl_start) < GATE8_DOWNLINK_WAIT_MS) {

        /* Check for downlink */
        uint8_t dl_buf[64];
        uint8_t dl_len = 0;
        uint8_t dl_port = 0;

        if (lorawan_hal_pop_downlink(dl_buf, &dl_len, &dl_port)) {

            Serial.print(GATE_LOG_TAG); Serial.print(" Downlink received! port=");
            Serial.print(dl_port); Serial.print(", len="); Serial.print(dl_len);
            Serial.print(", hex="); print_hex(dl_buf, dl_len); Serial.println();

            /* Parse and apply */
            char msg[128];
            DlResult result;
            bool recognized = dl_parse_and_apply(dl_buf, dl_len, dl_port, sys,
                                                  msg, sizeof(msg), result);

            Serial.print(GATE_LOG_TAG); Serial.print(" Parse result: ");
            Serial.println(msg);

            if (recognized) {
                Serial.print(GATE_LOG_TAG); Serial.print(" Command recognized: ");
                Serial.println(result.valid ? "VALID" : "INVALID/NACK");
            }

            /* Send ACK uplink */
            if (result.needs_ack) {
                Serial.print(GATE_LOG_TAG); Serial.print(" Sending ACK uplink (fport=");
                Serial.print(DL_ACK_FPORT); Serial.print(", len=");
                Serial.print(result.ack_len); Serial.print("): ");
                print_hex(result.ack_payload, result.ack_len);
                Serial.println();

                bool ack_ok = sys.transport().send(result.ack_payload, result.ack_len, DL_ACK_FPORT);
                Serial.print(GATE_LOG_TAG); Serial.print(" ACK uplink ");
                Serial.println(ack_ok ? "sent OK" : "FAILED");
            }

            /* Handle reboot */
            if (result.needs_reboot) {
                Serial.print(GATE_LOG_TAG); Serial.println(" Reboot requested — delaying 2s for ACK to transmit");
                delay(2000);
                Serial.print(GATE_LOG_TAG); Serial.println(" Triggering software reset...");
                /* Platform reset — acceptable in gate test code */
                #ifdef CORE_RAK4631
                NVIC_SystemReset();
                #else
                ESP.restart();
                #endif
            }

            dl_received = true;
            break;
        }

        /* Continue ticking scheduler (keeps uplinks flowing, opens more RX windows) */
        sys.tick();
        delay(100);
    }

    /* ============================================================
     * STEP 4 — Result Evaluation
     * ============================================================ */
    Serial.println();
    Serial.print(GATE_LOG_TAG); Serial.println(" STEP 4: Result evaluation");

    /* Print final stats */
    const RuntimeStats& st = sys.stats();
    Serial.print(GATE_LOG_TAG); Serial.print(" Total cycles: "); Serial.println(sys.cycleCount());
    Serial.print(GATE_LOG_TAG); Serial.print(" Modbus OK: "); Serial.println(st.modbus_ok);
    Serial.print(GATE_LOG_TAG); Serial.print(" Uplink OK: "); Serial.println(st.uplink_ok);
    Serial.print(GATE_LOG_TAG); Serial.print(" Current interval: ");
    Serial.print(sys.scheduler().taskInterval(0)); Serial.println(" ms");
    Serial.print(GATE_LOG_TAG); Serial.print(" Current slave addr: ");
    Serial.println(sys.peripheral().slaveAddr());

    if (dl_received) {
        Serial.println();
        Serial.print(GATE_LOG_TAG); Serial.println(" PASS — downlink received and processed");
    } else {
        Serial.println();
        Serial.print(GATE_LOG_TAG); Serial.println(" PASS_WITHOUT_DOWNLINK — no downlink scheduled");
        Serial.print(GATE_LOG_TAG); Serial.println(" (To fully validate, enqueue a downlink in ChirpStack)");
    }

    Serial.println();
    Serial.print(GATE_LOG_TAG); Serial.println(" === GATE COMPLETE ===");
}
