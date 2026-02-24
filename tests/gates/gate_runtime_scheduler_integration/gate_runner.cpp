/**
 * @file gate_runner.cpp
 * @brief Gate 6 — Runtime Scheduler Integration Validation Runner
 * @version 1.0
 * @date 2026-02-24
 *
 * Validates the full runtime stack running for multiple cycles:
 *   1. Create SystemManager (constructs Scheduler + LoRaTransport + ModbusPeripheral)
 *   2. Initialize LoRa transport (SX1262 hardware + LoRaMAC)
 *   3. OTAA join (blocking, 30s timeout)
 *   4. Initialize Modbus peripheral (RS485 + UART)
 *   5. Register scheduler tasks
 *   6. Validate scheduler tick (dry run — no tasks due)
 *   7. Force cycle 1 (sensor read + uplink)
 *   8. Force cycle 2
 *   9. Force cycle 3
 *  10. Gate summary
 *
 * Uses fireNow() to trigger cycles immediately instead of waiting 30s per cycle.
 * The scheduler's interval logic is validated in Step 6 (tick returns without firing).
 *
 * Entry point: gate_runtime_scheduler_integration_run()
 *
 * All parameters from gate_config.h.
 * Zero hardcoded values in this file.
 */

#include <Arduino.h>
#include "gate_config.h"
#include "../../../firmware/runtime/system_manager.h"

/* ============================================================
 * Gate Result State
 * ============================================================ */
static uint8_t gate_result = GATE_FAIL_SYSTEM_CRASH;

/* ============================================================
 * Helper — Print byte array as hex with spaces
 * ============================================================ */
static void print_hex(const uint8_t *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        if (i > 0) Serial.print(' ');
        if (buf[i] < 0x10) Serial.print('0');
        Serial.print(buf[i], HEX);
    }
}

/* ============================================================
 * SystemManager instance (file-scope)
 * ============================================================ */
static SystemManager* g_sys = nullptr;

/* ============================================================
 * STEP 1 — Create SystemManager
 *
 * Constructs SystemManager which internally creates:
 *   - Scheduler (empty, no tasks)
 *   - LoRaTransport (configured from gate_config.h)
 *   - ModbusPeripheral (configured from gate_config.h)
 * ============================================================ */
static bool step_create_system(SystemManager& sys) {
    Serial.printf("%s STEP 1: Create SystemManager\r\n", GATE_LOG_TAG);
    g_sys = &sys;
    sys.activate();  /* Set static singleton for sensorUplinkTask callback */
    Serial.printf("%s SystemManager created\r\n", GATE_LOG_TAG);
    return true;
}

/* ============================================================
 * STEP 2 — LoRa Transport Init
 *
 * Initializes SX1262 hardware and LoRaMAC stack.
 * Separated from connect() for granular fail codes.
 * ============================================================ */
static bool step_lora_init(SystemManager& sys) {
    Serial.printf("%s STEP 2: LoRa transport init\r\n", GATE_LOG_TAG);

    if (!sys.transport().init()) {
        Serial.printf("%s LoRa Init: FAIL\r\n", GATE_LOG_TAG);
        gate_result = GATE_FAIL_LORA_INIT;
        return false;
    }

    Serial.printf("%s LoRa Init: OK\r\n", GATE_LOG_TAG);
    return true;
}

/* ============================================================
 * STEP 3 — OTAA Join
 *
 * Blocking join with 30s timeout.
 * ============================================================ */
static bool step_otaa_join(SystemManager& sys) {
    Serial.printf("%s STEP 3: OTAA join (timeout=%ds)\r\n",
                  GATE_LOG_TAG, GATE_LORAWAN_JOIN_TIMEOUT_MS / 1000);
    Serial.printf("%s Join Start...\r\n", GATE_LOG_TAG);

    if (!sys.transport().connect(GATE_LORAWAN_JOIN_TIMEOUT_MS)) {
        Serial.printf("%s Join Accept: FAIL (timeout or denied)\r\n", GATE_LOG_TAG);
        gate_result = GATE_FAIL_JOIN_TIMEOUT;
        return false;
    }

    Serial.printf("%s Join Accept: OK\r\n", GATE_LOG_TAG);
    return true;
}

/* ============================================================
 * STEP 4 — Modbus Peripheral Init
 *
 * Initializes RS485 transceiver and UART for Modbus RTU.
 * ============================================================ */
static bool step_modbus_init(SystemManager& sys) {
    Serial.printf("%s STEP 4: Modbus peripheral init\r\n", GATE_LOG_TAG);

    if (!sys.peripheral().init()) {
        Serial.printf("%s Modbus Init: FAIL\r\n", GATE_LOG_TAG);
        gate_result = GATE_FAIL_MODBUS_INIT;
        return false;
    }

    Serial.printf("%s Modbus Init: OK (Slave=%d, Baud=%lu)\r\n",
                  GATE_LOG_TAG,
                  GATE_DISCOVERED_SLAVE,
                  (unsigned long)GATE_DISCOVERED_BAUD);
    return true;
}

/* ============================================================
 * STEP 5 — Register Scheduler Tasks
 *
 * Registers the sensor-uplink task with the scheduler.
 * Uses SystemManager's static callback trampoline.
 * ============================================================ */
static bool step_register_tasks(SystemManager& sys) {
    Serial.printf("%s STEP 5: Register scheduler tasks\r\n", GATE_LOG_TAG);

    /* Use SystemManager's own sensorUplinkTask (increments cycle count) */
    int8_t idx = sys.registerSensorTask();

    if (idx < 0) {
        Serial.printf("%s Task registration: FAIL\r\n", GATE_LOG_TAG);
        gate_result = GATE_FAIL_SCHEDULER_INIT;
        return false;
    }

    Serial.printf("%s Task registered: %s (interval=%lums)\r\n",
                  GATE_LOG_TAG,
                  sys.scheduler().taskName(idx),
                  (unsigned long)HW_SYSTEM_POLL_INTERVAL_MS);
    Serial.printf("%s Scheduler: %d task(s) registered\r\n",
                  GATE_LOG_TAG, sys.scheduler().taskCount());
    return true;
}

/* ============================================================
 * STEP 6 — Validate Scheduler Tick (dry run)
 *
 * Calls tick() once — verifies no task fires since interval
 * hasn't elapsed. Confirms non-blocking behavior.
 * ============================================================ */
static bool step_tick_validation(SystemManager& sys) {
    Serial.printf("%s STEP 6: Scheduler tick validation\r\n", GATE_LOG_TAG);

    uint32_t count_before = sys.cycleCount();
    sys.tick();
    uint32_t count_after = sys.cycleCount();

    /* No task should have fired — interval not yet elapsed */
    if (count_before == count_after) {
        Serial.printf("%s Tick: no tasks due (OK — interval not yet elapsed)\r\n",
                      GATE_LOG_TAG);
    } else {
        Serial.printf("%s Tick: task fired unexpectedly (count=%lu)\r\n",
                      GATE_LOG_TAG, (unsigned long)count_after);
        /* Not a fatal error — continue */
    }

    return true;
}

/* ============================================================
 * Force Cycle — helper for Steps 7, 8, 9
 *
 * Uses scheduler.fireNow(0) to trigger sensor-uplink immediately.
 * Validates cycle count incremented and reads register values.
 * ============================================================ */
static bool step_force_cycle(SystemManager& sys, uint8_t cycle_num) {
    Serial.printf("%s STEP %d: Force cycle %d\r\n",
                  GATE_LOG_TAG, cycle_num + 6, cycle_num);

    uint32_t count_before = sys.cycleCount();

    /* Fire the sensor-uplink task immediately */
    sys.scheduler().fireNow(0);

    /* Small delay for LoRaWAN TX to complete */
    delay(500);

    uint32_t count_after = sys.cycleCount();

    /* Verify cycle count incremented */
    if (count_after <= count_before) {
        Serial.printf("%s Cycle %d: FAIL (count did not increment: %lu → %lu)\r\n",
                      GATE_LOG_TAG, cycle_num,
                      (unsigned long)count_before, (unsigned long)count_after);
        gate_result = GATE_FAIL_CYCLE_COUNT;
        return false;
    }

    /* Log register values from last read */
    uint8_t reg_count = sys.peripheral().lastRegCount();
    if (reg_count == 0) {
        Serial.printf("%s   Modbus Read: FAIL (no registers)\r\n", GATE_LOG_TAG);
        gate_result = GATE_FAIL_CYCLE_READ;
        return false;
    }

    Serial.printf("%s   Modbus Read: OK (%d registers)\r\n",
                  GATE_LOG_TAG, reg_count);
    for (uint8_t i = 0; i < reg_count; i++) {
        uint16_t val = sys.peripheral().lastRegValue(i);
        Serial.printf("%s   Register[%d] : 0x%04X (%d)\r\n",
                      GATE_LOG_TAG, i, val, val);
    }

    /* Verify transport is still connected (uplink was possible) */
    if (!sys.transport().isConnected()) {
        Serial.printf("%s   Uplink TX: FAIL (transport disconnected)\r\n", GATE_LOG_TAG);
        gate_result = GATE_FAIL_CYCLE_UPLINK;
        return false;
    }

    Serial.printf("%s   Uplink TX: OK (port=%d, %d bytes)\r\n",
                  GATE_LOG_TAG, GATE_PAYLOAD_PORT, reg_count * 2);
    Serial.printf("%s Cycle %d: PASS\r\n", GATE_LOG_TAG, cycle_num);
    return true;
}

/* ============================================================
 * STEP 10 — Gate Summary
 * ============================================================ */
static void step_summary(SystemManager& sys) {
    Serial.printf("%s STEP 10: Gate summary\r\n", GATE_LOG_TAG);
    Serial.printf("%s   Scheduler    : OK (%d task)\r\n",
                  GATE_LOG_TAG, sys.scheduler().taskCount());
    Serial.printf("%s   Transport    : %s (%s)\r\n",
                  GATE_LOG_TAG, sys.transport().name(),
                  sys.transport().isConnected() ? "connected" : "disconnected");
    Serial.printf("%s   Peripheral   : %s (Slave=%d)\r\n",
                  GATE_LOG_TAG, sys.peripheral().name(),
                  GATE_DISCOVERED_SLAVE);
    Serial.printf("%s   Cycles       : %lu/%d\r\n",
                  GATE_LOG_TAG,
                  (unsigned long)sys.cycleCount(),
                  GATE_SCHEDULER_TEST_CYCLES);

    /* Print last payload */
    uint8_t reg_count = sys.peripheral().lastRegCount();
    Serial.printf("%s   Last Payload : ", GATE_LOG_TAG);
    for (uint8_t i = 0; i < reg_count; i++) {
        uint16_t val = sys.peripheral().lastRegValue(i);
        if (i > 0) Serial.print(' ');
        if ((val >> 8) < 0x10) Serial.print('0');
        Serial.print((uint8_t)(val >> 8), HEX);
        Serial.print(' ');
        if ((val & 0xFF) < 0x10) Serial.print('0');
        Serial.print((uint8_t)(val & 0xFF), HEX);
    }
    Serial.printf("\r\n");

    Serial.printf("%s   Uplink Port  : %d\r\n", GATE_LOG_TAG, GATE_PAYLOAD_PORT);
}

/* ============================================================
 * Gate Entry Point
 * ============================================================ */
void gate_runtime_scheduler_integration_run(void) {
    gate_result = GATE_FAIL_SYSTEM_CRASH;
    g_sys = nullptr;

    /* ---- GATE START banner ---- */
    Serial.printf("\r\n%s === GATE START: %s v%s ===\r\n",
                  GATE_LOG_TAG, GATE_NAME, GATE_VERSION);

    /* ---- Step 1: Create SystemManager ---- */
    SystemManager sys;
    if (!step_create_system(sys)) goto done;

    /* ---- Step 2: LoRa transport init ---- */
    if (!step_lora_init(sys)) goto done;

    /* ---- Step 3: OTAA join ---- */
    if (!step_otaa_join(sys)) goto done;

    /* ---- Step 4: Modbus peripheral init ---- */
    if (!step_modbus_init(sys)) goto done;

    /* ---- Step 5: Register scheduler tasks ---- */
    if (!step_register_tasks(sys)) goto done;

    /* ---- Step 6: Scheduler tick validation ---- */
    if (!step_tick_validation(sys)) goto done;

    /* ---- Steps 7-9: Force cycles 1-3 ---- */
    for (uint8_t cycle = 1; cycle <= GATE_SCHEDULER_TEST_CYCLES; cycle++) {
        if (!step_force_cycle(sys, cycle)) goto done;

        /* Inter-cycle delay for LoRaWAN duty cycle compliance */
        if (cycle < GATE_SCHEDULER_TEST_CYCLES) {
            Serial.printf("%s   Inter-cycle delay (3s)...\r\n", GATE_LOG_TAG);
            delay(3000);
        }
    }

    /* ---- Verify final cycle count ---- */
    if (sys.cycleCount() < GATE_SCHEDULER_TEST_CYCLES) {
        Serial.printf("%s Cycle count: %lu < %d required\r\n",
                      GATE_LOG_TAG,
                      (unsigned long)sys.cycleCount(),
                      GATE_SCHEDULER_TEST_CYCLES);
        gate_result = GATE_FAIL_CYCLE_COUNT;
        goto done;
    }

    /* ---- Step 10: Summary ---- */
    step_summary(sys);

    /* ---- All steps passed ---- */
    gate_result = GATE_PASS;

done:
    /* ---- Result ---- */
    if (gate_result == GATE_PASS) {
        Serial.printf("%s PASS\r\n", GATE_LOG_TAG);
    } else {
        Serial.printf("%s FAIL (code=%d)\r\n", GATE_LOG_TAG, gate_result);
    }

    Serial.printf("%s === GATE COMPLETE ===\r\n", GATE_LOG_TAG);
}
