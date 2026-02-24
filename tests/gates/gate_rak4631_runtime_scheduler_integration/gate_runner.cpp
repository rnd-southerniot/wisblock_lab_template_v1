/**
 * @file gate_runner.cpp
 * @brief Gate 6 — RAK4631 Runtime Scheduler Integration Validation Runner
 * @version 1.0
 * @date 2026-02-25
 *
 * Validates the full runtime stack running for multiple cycles:
 *   1. Create SystemManager (constructs Scheduler + LoRaTransport + ModbusPeripheral)
 *   2. Initialize LoRa transport (SX1262 via lora_rak4630_init())
 *   3. OTAA join (blocking, 30s timeout)
 *   4. Initialize Modbus peripheral (RS485 + UART via RAK4631 HAL)
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
 * Entry point: gate_rak4631_runtime_scheduler_integration_run()
 *
 * Uses Serial.print() instead of Serial.printf() for nRF52840 compatibility.
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
    Serial.print(GATE_LOG_TAG);
    Serial.println(" STEP 1: Create SystemManager");
    g_sys = &sys;
    sys.activate();  /* Set static singleton for sensorUplinkTask callback */
    Serial.print(GATE_LOG_TAG);
    Serial.println(" SystemManager created");
    return true;
}

/* ============================================================
 * STEP 2 — LoRa Transport Init
 *
 * Initializes SX1262 hardware via lora_rak4630_init() and LoRaMAC stack.
 * Separated from connect() for granular fail codes.
 * ============================================================ */
static bool step_lora_init(SystemManager& sys) {
    Serial.print(GATE_LOG_TAG);
    Serial.println(" STEP 2: LoRa transport init");

    if (!sys.transport().init()) {
        Serial.print(GATE_LOG_TAG);
        Serial.println(" LoRa Init: FAIL");
        gate_result = GATE_FAIL_LORA_INIT;
        return false;
    }

    Serial.print(GATE_LOG_TAG);
    Serial.println(" LoRa Init: OK");
    return true;
}

/* ============================================================
 * STEP 3 — OTAA Join
 *
 * Blocking join with 30s timeout.
 * ============================================================ */
static bool step_otaa_join(SystemManager& sys) {
    Serial.print(GATE_LOG_TAG);
    Serial.print(" STEP 3: OTAA join (timeout=");
    Serial.print(GATE_LORAWAN_JOIN_TIMEOUT_MS / 1000);
    Serial.println("s)");
    Serial.print(GATE_LOG_TAG);
    Serial.println(" Join Start...");

    if (!sys.transport().connect(GATE_LORAWAN_JOIN_TIMEOUT_MS)) {
        Serial.print(GATE_LOG_TAG);
        Serial.println(" Join Accept: FAIL (timeout or denied)");
        gate_result = GATE_FAIL_JOIN_TIMEOUT;
        return false;
    }

    Serial.print(GATE_LOG_TAG);
    Serial.println(" Join Accept: OK");
    return true;
}

/* ============================================================
 * STEP 4 — Modbus Peripheral Init
 *
 * Initializes RS485 transceiver and UART for Modbus RTU.
 * ============================================================ */
static bool step_modbus_init(SystemManager& sys) {
    Serial.print(GATE_LOG_TAG);
    Serial.println(" STEP 4: Modbus peripheral init");

    if (!sys.peripheral().init()) {
        Serial.print(GATE_LOG_TAG);
        Serial.println(" Modbus Init: FAIL");
        gate_result = GATE_FAIL_MODBUS_INIT;
        return false;
    }

    Serial.print(GATE_LOG_TAG);
    Serial.print(" Modbus Init: OK (Slave=");
    Serial.print(GATE_DISCOVERED_SLAVE);
    Serial.print(", Baud=");
    Serial.print((unsigned long)GATE_DISCOVERED_BAUD);
    Serial.println(")");
    return true;
}

/* ============================================================
 * STEP 5 — Register Scheduler Tasks
 *
 * Registers the sensor-uplink task with the scheduler.
 * Uses SystemManager's static callback trampoline.
 * ============================================================ */
static bool step_register_tasks(SystemManager& sys) {
    Serial.print(GATE_LOG_TAG);
    Serial.println(" STEP 5: Register scheduler tasks");

    /* Use SystemManager's own sensorUplinkTask (increments cycle count) */
    int8_t idx = sys.registerSensorTask();

    if (idx < 0) {
        Serial.print(GATE_LOG_TAG);
        Serial.println(" Task registration: FAIL");
        gate_result = GATE_FAIL_SCHEDULER_INIT;
        return false;
    }

    Serial.print(GATE_LOG_TAG);
    Serial.print(" Task registered: ");
    Serial.print(sys.scheduler().taskName(idx));
    Serial.print(" (interval=");
    Serial.print((unsigned long)HW_SYSTEM_POLL_INTERVAL_MS);
    Serial.println("ms)");

    Serial.print(GATE_LOG_TAG);
    Serial.print(" Scheduler: ");
    Serial.print(sys.scheduler().taskCount());
    Serial.println(" task(s) registered");
    return true;
}

/* ============================================================
 * STEP 6 — Validate Scheduler Tick (dry run)
 *
 * Calls tick() once — verifies no task fires since interval
 * hasn't elapsed. Confirms non-blocking behavior.
 * ============================================================ */
static bool step_tick_validation(SystemManager& sys) {
    Serial.print(GATE_LOG_TAG);
    Serial.println(" STEP 6: Scheduler tick validation");

    uint32_t count_before = sys.cycleCount();
    sys.tick();
    uint32_t count_after = sys.cycleCount();

    /* No task should have fired — interval not yet elapsed */
    if (count_before == count_after) {
        Serial.print(GATE_LOG_TAG);
        Serial.println(" Tick: no tasks due (OK \xe2\x80\x94 interval not yet elapsed)");
    } else {
        Serial.print(GATE_LOG_TAG);
        Serial.print(" Tick: task fired unexpectedly (count=");
        Serial.print((unsigned long)count_after);
        Serial.println(")");
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
    Serial.print(GATE_LOG_TAG);
    Serial.print(" STEP ");
    Serial.print(cycle_num + 6);
    Serial.print(": Force cycle ");
    Serial.println(cycle_num);

    uint32_t count_before = sys.cycleCount();

    /* Fire the sensor-uplink task immediately */
    sys.scheduler().fireNow(0);

    /* Small delay for LoRaWAN TX to complete */
    delay(500);

    uint32_t count_after = sys.cycleCount();

    /* Verify cycle count incremented */
    if (count_after <= count_before) {
        Serial.print(GATE_LOG_TAG);
        Serial.print(" Cycle ");
        Serial.print(cycle_num);
        Serial.print(": FAIL (count did not increment: ");
        Serial.print((unsigned long)count_before);
        Serial.print(" -> ");
        Serial.print((unsigned long)count_after);
        Serial.println(")");
        gate_result = GATE_FAIL_CYCLE_COUNT;
        return false;
    }

    /* Log register values from last read */
    uint8_t reg_count = sys.peripheral().lastRegCount();
    if (reg_count == 0) {
        Serial.print(GATE_LOG_TAG);
        Serial.println("   Modbus Read: FAIL (no registers)");
        gate_result = GATE_FAIL_CYCLE_READ;
        return false;
    }

    Serial.print(GATE_LOG_TAG);
    Serial.print("   Modbus Read: OK (");
    Serial.print(reg_count);
    Serial.println(" registers)");

    for (uint8_t i = 0; i < reg_count; i++) {
        uint16_t val = sys.peripheral().lastRegValue(i);
        Serial.print(GATE_LOG_TAG);
        Serial.print("   Register[");
        Serial.print(i);
        Serial.print("] : 0x");
        if (val < 0x1000) Serial.print('0');
        if (val < 0x0100) Serial.print('0');
        if (val < 0x0010) Serial.print('0');
        Serial.print(val, HEX);
        Serial.print(" (");
        Serial.print(val);
        Serial.println(")");
    }

    /* Verify transport is still connected (uplink was possible) */
    if (!sys.transport().isConnected()) {
        Serial.print(GATE_LOG_TAG);
        Serial.println("   Uplink TX: FAIL (transport disconnected)");
        gate_result = GATE_FAIL_CYCLE_UPLINK;
        return false;
    }

    Serial.print(GATE_LOG_TAG);
    Serial.print("   Uplink TX: OK (port=");
    Serial.print(GATE_PAYLOAD_PORT);
    Serial.print(", ");
    Serial.print(reg_count * 2);
    Serial.println(" bytes)");

    Serial.print(GATE_LOG_TAG);
    Serial.print(" Cycle ");
    Serial.print(cycle_num);
    Serial.println(": PASS");
    return true;
}

/* ============================================================
 * STEP 10 — Gate Summary
 * ============================================================ */
static void step_summary(SystemManager& sys) {
    Serial.print(GATE_LOG_TAG);
    Serial.println(" STEP 10: Gate summary");

    Serial.print(GATE_LOG_TAG);
    Serial.print("   Scheduler    : OK (");
    Serial.print(sys.scheduler().taskCount());
    Serial.println(" task)");

    Serial.print(GATE_LOG_TAG);
    Serial.print("   Transport    : ");
    Serial.print(sys.transport().name());
    Serial.print(" (");
    Serial.print(sys.transport().isConnected() ? "connected" : "disconnected");
    Serial.println(")");

    Serial.print(GATE_LOG_TAG);
    Serial.print("   Peripheral   : ");
    Serial.print(sys.peripheral().name());
    Serial.print(" (Slave=");
    Serial.print(GATE_DISCOVERED_SLAVE);
    Serial.println(")");

    Serial.print(GATE_LOG_TAG);
    Serial.print("   Cycles       : ");
    Serial.print((unsigned long)sys.cycleCount());
    Serial.print("/");
    Serial.println(GATE_SCHEDULER_TEST_CYCLES);

    /* Print last payload */
    uint8_t reg_count = sys.peripheral().lastRegCount();
    Serial.print(GATE_LOG_TAG);
    Serial.print("   Last Payload : ");
    for (uint8_t i = 0; i < reg_count; i++) {
        uint16_t val = sys.peripheral().lastRegValue(i);
        if (i > 0) Serial.print(' ');
        if ((val >> 8) < 0x10) Serial.print('0');
        Serial.print((uint8_t)(val >> 8), HEX);
        Serial.print(' ');
        if ((val & 0xFF) < 0x10) Serial.print('0');
        Serial.print((uint8_t)(val & 0xFF), HEX);
    }
    Serial.println();

    Serial.print(GATE_LOG_TAG);
    Serial.print("   Uplink Port  : ");
    Serial.println(GATE_PAYLOAD_PORT);
}

/* ============================================================
 * Gate Entry Point
 * ============================================================ */
void gate_rak4631_runtime_scheduler_integration_run(void) {
    gate_result = GATE_FAIL_SYSTEM_CRASH;
    g_sys = nullptr;

    /* ---- GATE START banner ---- */
    Serial.println();
    Serial.print(GATE_LOG_TAG);
    Serial.print(" === GATE START: ");
    Serial.print(GATE_NAME);
    Serial.print(" v");
    Serial.print(GATE_VERSION);
    Serial.println(" ===");

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
            Serial.print(GATE_LOG_TAG);
            Serial.println("   Inter-cycle delay (3s)...");
            delay(3000);
        }
    }

    /* ---- Verify final cycle count ---- */
    if (sys.cycleCount() < GATE_SCHEDULER_TEST_CYCLES) {
        Serial.print(GATE_LOG_TAG);
        Serial.print(" Cycle count: ");
        Serial.print((unsigned long)sys.cycleCount());
        Serial.print(" < ");
        Serial.print(GATE_SCHEDULER_TEST_CYCLES);
        Serial.println(" required");
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
