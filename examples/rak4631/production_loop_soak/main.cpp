/**
 * @file main.cpp
 * @brief RAK4631 Gate 7 — Production Loop Soak Example
 * @version 1.0
 * @date 2026-02-25
 *
 * Standalone example demonstrating the production soak pattern on
 * RAK4631 WisBlock Core (nRF52840 + SX1262):
 *
 *   1. Init SX1262 via lora_rak4630_init() (BSP one-liner)
 *   2. OTAA join to ChirpStack v4 gateway on AS923-1
 *   3. Init RS485 + Modbus RTU (auto DE/RE, no pin args)
 *   4. Register sensor-uplink task with inline scheduler (30s interval)
 *   5. Run production soak loop via tick() for 5 minutes
 *      — scheduler fires naturally, no fireNow()
 *   6. Print per-cycle metrics + soak summary
 *
 * This example is self-contained — no dependency on firmware/runtime/ layer.
 * It inlines a minimal cooperative scheduler and the same data flow:
 *   ModbusRead -> uint16_t[] -> big-endian encode -> lmh_send()
 *
 * Hardware:
 *   - RAK4631 WisBlock Core (nRF52840 @ 64 MHz + SX1262)
 *   - RAK5802 WisBlock RS485 Module (TP8485E, auto DE/RE)
 *   - RAK19007 WisBlock Base Board
 *   - External Modbus RTU slave (e.g., RS-FSJT-N01 wind sensor)
 *   - ChirpStack v4 gateway within LoRa range
 *
 * Build flags required: -DRAK4630 -D_VARIANT_RAK4630_
 * Lib deps: beegee-tokyo/SX126x-Arduino@^2.0.32
 */

#include <Arduino.h>
#include <LoRaWan-Arduino.h>
#include "pin_map.h"

/* ================================================================
 * OTAA Credentials — UPDATE THESE FOR YOUR DEVICE
 * ================================================================ */
static uint8_t nodeDeviceEUI[8]  = { 0x88, 0x82, 0x24, 0x44, 0xAE, 0xED, 0x1E, 0xB2 };
static uint8_t nodeAppEUI[8]     = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t nodeAppKey[16]    = { 0x91, 0x8C, 0x49, 0x08, 0xF0, 0x89, 0x50, 0x6B,
                                     0x30, 0x18, 0x0B, 0x62, 0x65, 0x9A, 0x4A, 0xD5 };

/* ================================================================
 * Join State (set by async callbacks)
 * ================================================================ */
static volatile bool g_joined      = false;
static volatile bool g_join_failed = false;

/* ================================================================
 * LoRaWAN Callbacks
 * ================================================================ */
static void lorawan_has_joined(void)    { g_joined = true; }
static void lorawan_join_failed(void)   { g_join_failed = true; }
static void lorawan_rx(lmh_app_data_t *data) { (void)data; }
static void lorawan_class_confirm(DeviceClass_t Class) { (void)Class; }

static lmh_param_t lora_param = {
    LORAWAN_ADR_OFF,       /* ADR */
    DR_2,                  /* Data rate (SF10) */
    true,                  /* Public network */
    3,                     /* Join trials */
    TX_POWER_0,            /* TX power */
    LORAWAN_DUTYCYCLE_OFF  /* Duty cycle */
};

static lmh_callback_t lora_callbacks = {
    BoardGetBatteryLevel,
    BoardGetUniqueId,
    BoardGetRandomSeed,
    lorawan_rx,
    lorawan_has_joined,
    lorawan_class_confirm,
    lorawan_join_failed
};

/* ================================================================
 * CRC-16/MODBUS
 * ================================================================ */
static uint16_t modbus_crc16(const uint8_t *data, uint8_t len) {
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 1) ? ((crc >> 1) ^ 0xA001) : (crc >> 1);
        }
    }
    return crc;
}

/* ================================================================
 * UART read with timeout + inter-byte gap
 * ================================================================ */
static uint8_t uart_read(uint8_t *buf, uint8_t max_len, uint32_t timeout_ms) {
    unsigned long start = millis();
    while (!Serial1.available()) {
        if ((millis() - start) >= timeout_ms) return 0;
    }
    uint8_t count = 0;
    unsigned long last_byte = millis();
    while (count < max_len) {
        if (Serial1.available()) {
            buf[count++] = Serial1.read();
            last_byte = millis();
        } else if ((millis() - last_byte) >= 5) {
            break;
        }
    }
    return count;
}

/* ================================================================
 * UART flush
 * ================================================================ */
static void uart_flush(void) {
    Serial1.flush();
    delay(10);
    unsigned long t = millis();
    while (Serial1.available() && (millis() - t) < 100) {
        Serial1.read();
    }
}

/* ================================================================
 * Minimal Cooperative Scheduler (inline)
 *
 * millis()-based periodic task dispatch. This is the same pattern
 * used by firmware/runtime/scheduler.h but self-contained here
 * for standalone example use.
 * ================================================================ */
typedef void (*task_callback_t)(void);

struct ScheduledTask {
    task_callback_t callback;
    uint32_t interval_ms;
    uint32_t last_run_ms;
    bool active;
    const char* name;
};

#define MAX_TASKS 4
static ScheduledTask g_tasks[MAX_TASKS];
static uint8_t g_task_count = 0;

static int8_t scheduler_register(task_callback_t cb, uint32_t interval_ms, const char* name) {
    if (g_task_count >= MAX_TASKS || cb == nullptr) return -1;
    uint8_t idx = g_task_count;
    g_tasks[idx].callback    = cb;
    g_tasks[idx].interval_ms = interval_ms;
    g_tasks[idx].last_run_ms = millis();
    g_tasks[idx].active      = true;
    g_tasks[idx].name        = name;
    g_task_count++;
    return (int8_t)idx;
}

static void scheduler_tick(void) {
    uint32_t now = millis();
    for (uint8_t i = 0; i < g_task_count; i++) {
        if (!g_tasks[i].active || g_tasks[i].callback == nullptr) continue;
        if ((now - g_tasks[i].last_run_ms) >= g_tasks[i].interval_ms) {
            g_tasks[i].last_run_ms = now;
            g_tasks[i].callback();
        }
    }
}

/* ================================================================
 * Runtime State + Soak Metrics
 * ================================================================ */
static uint32_t g_cycle_count   = 0;
static uint32_t g_modbus_ok     = 0;
static uint32_t g_modbus_fail   = 0;
static uint32_t g_uplink_ok     = 0;
static uint32_t g_uplink_fail   = 0;
static uint32_t g_max_cycle_ms  = 0;
static uint32_t g_last_cycle_ms = 0;
static uint8_t  g_consec_modbus_fail     = 0;
static uint8_t  g_consec_uplink_fail     = 0;
static uint8_t  g_max_consec_modbus_fail = 0;
static uint8_t  g_max_consec_uplink_fail = 0;
static bool     g_last_modbus_ok = false;
static bool     g_last_uplink_ok = false;

/* ================================================================
 * Sensor Uplink Task — registered with scheduler
 *
 * Data flow:
 *   1. Read Modbus registers (FC 0x03)
 *   2. Encode as big-endian payload
 *   3. Send unconfirmed LoRaWAN uplink
 *   4. Update soak metrics
 * ================================================================ */
static void sensor_uplink_task(void) {
    uint32_t cycle_start = millis();

    /* Build Modbus request */
    uint8_t tx[8];
    tx[0] = MODBUS_SLAVE;
    tx[1] = 0x03;
    tx[2] = 0x00; tx[3] = 0x00;  /* Start Reg */
    tx[4] = 0x00; tx[5] = MODBUS_QUANTITY;
    uint16_t crc = modbus_crc16(tx, 6);
    tx[6] = (uint8_t)(crc & 0xFF);
    tx[7] = (uint8_t)((crc >> 8) & 0xFF);

    uart_flush();
    delay(10);
    Serial1.write(tx, 8);
    Serial1.flush();
    delayMicroseconds(500);

    uint8_t rx[32];
    uint8_t rx_len = uart_read(rx, sizeof(rx), RESPONSE_TIMEOUT);

    bool read_ok = false;
    uint16_t regs[MODBUS_QUANTITY];

    if (rx_len >= 5) {
        uint16_t calc = modbus_crc16(rx, rx_len - 2);
        uint16_t recv = (uint16_t)rx[rx_len - 2] | ((uint16_t)rx[rx_len - 1] << 8);
        if (calc == recv && rx[0] == MODBUS_SLAVE && rx[1] == 0x03 &&
            rx[2] == 2 * MODBUS_QUANTITY) {
            for (uint8_t i = 0; i < MODBUS_QUANTITY; i++) {
                regs[i] = ((uint16_t)rx[3 + i * 2] << 8) | (uint16_t)rx[4 + i * 2];
            }
            read_ok = true;
        }
    }

    /* Track modbus result */
    g_last_modbus_ok = read_ok;
    if (read_ok) {
        g_modbus_ok++;
        g_consec_modbus_fail = 0;
    } else {
        g_modbus_fail++;
        g_consec_modbus_fail++;
        if (g_consec_modbus_fail > g_max_consec_modbus_fail)
            g_max_consec_modbus_fail = g_consec_modbus_fail;
    }

    /* Send uplink if modbus read OK */
    bool send_ok = false;
    if (read_ok) {
        uint8_t payload[MODBUS_QUANTITY * 2];
        uint8_t payload_len = 0;
        for (uint8_t i = 0; i < MODBUS_QUANTITY; i++) {
            payload[payload_len++] = (uint8_t)(regs[i] >> 8);
            payload[payload_len++] = (uint8_t)(regs[i] & 0xFF);
        }

        lmh_app_data_t lora_data;
        lora_data.buffer   = payload;
        lora_data.buffsize = payload_len;
        lora_data.port     = LORAWAN_UPLINK_PORT;

        lmh_error_status err = lmh_send(&lora_data, LMH_UNCONFIRMED_MSG);
        send_ok = (err == LMH_SUCCESS);
    }

    /* Track uplink result */
    g_last_uplink_ok = send_ok;
    if (send_ok) {
        g_uplink_ok++;
        g_consec_uplink_fail = 0;
    } else {
        g_uplink_fail++;
        g_consec_uplink_fail++;
        if (g_consec_uplink_fail > g_max_consec_uplink_fail)
            g_max_consec_uplink_fail = g_consec_uplink_fail;
    }

    /* Track cycle timing */
    g_last_cycle_ms = millis() - cycle_start;
    if (g_last_cycle_ms > g_max_cycle_ms)
        g_max_cycle_ms = g_last_cycle_ms;

    g_cycle_count++;

    /* Per-cycle runtime log */
    Serial.print("[RUNTIME] Cycle ");
    Serial.print(g_cycle_count);
    Serial.print(": read=");
    Serial.print(read_ok ? "OK" : "FAIL");
    if (read_ok) {
        Serial.print(" (");
        Serial.print(MODBUS_QUANTITY);
        Serial.print(" regs), uplink=");
        Serial.print(send_ok ? "OK" : "FAIL");
        Serial.print(" (port=");
        Serial.print(LORAWAN_UPLINK_PORT);
        Serial.print(", ");
        Serial.print(MODBUS_QUANTITY * 2);
        Serial.print(" bytes)");
    }
    Serial.println();
}

/* ================================================================
 * Main
 * ================================================================ */
void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }
    delay(1000);

    Serial.println();
    Serial.println("========================================");
    Serial.println("[SYSTEM] RAK4631 Production Loop Soak");
    Serial.println("========================================");

    Serial.print("[CONFIG] Soak duration : ");
    Serial.print((unsigned long)SOAK_DURATION_MS);
    Serial.print(" ms (");
    Serial.print((unsigned long)(SOAK_DURATION_MS / 1000));
    Serial.println(" s)");
    Serial.print("[CONFIG] Poll interval : ");
    Serial.print((unsigned long)SCHEDULER_POLL_INTERVAL_MS);
    Serial.println(" ms");
    Serial.print("[CONFIG] Min uplinks   : ");
    Serial.println(SOAK_MIN_UPLINKS);

    /* ---- Step 1: LoRaWAN init ---- */
    Serial.println("[INIT] Initializing SX1262...");
    uint32_t hw_err = lora_rak4630_init();
    if (hw_err != 0) {
        Serial.print("[INIT] lora_rak4630_init FAILED: ");
        Serial.println(hw_err);
        return;
    }
    Serial.println("[INIT] lora_rak4630_init: OK");

    uint32_t lmh_err = lmh_init(&lora_callbacks, lora_param, true,
                                  CLASS_A, LORAMAC_REGION_AS923);
    if (lmh_err != 0) {
        Serial.print("[INIT] lmh_init FAILED: ");
        Serial.println(lmh_err);
        return;
    }
    Serial.println("[INIT] lmh_init: OK");
    Serial.println("[INIT] Region: AS923-1, Class A, DR2, ADR=OFF");

    /* ---- Step 2: Set credentials ---- */
    lmh_setDevEui(nodeDeviceEUI);
    lmh_setAppEui(nodeAppEUI);
    lmh_setAppKey(nodeAppKey);

    Serial.print("[INIT] DevEUI: ");
    for (uint8_t i = 0; i < 8; i++) {
        if (i > 0) Serial.print(':');
        if (nodeDeviceEUI[i] < 0x10) Serial.print('0');
        Serial.print(nodeDeviceEUI[i], HEX);
    }
    Serial.println();

    /* ---- Step 3: OTAA join ---- */
    Serial.println("[JOIN] Starting OTAA join...");
    g_joined = false;
    g_join_failed = false;
    lmh_join();

    unsigned long join_start = millis();
    while (!g_joined && !g_join_failed) {
        if (millis() - join_start > LORAWAN_JOIN_TIMEOUT) break;
        delay(100);
    }

    if (!g_joined) {
        if (g_join_failed) {
            Serial.println("[JOIN] Join DENIED");
        } else {
            Serial.println("[JOIN] Join TIMEOUT (30s)");
        }
        return;
    }

    unsigned long join_ms = millis() - join_start;
    Serial.print("[JOIN] Join Accept: OK (");
    Serial.print(join_ms);
    Serial.println("ms)");

    /* ---- Step 4: Modbus init ---- */
    Serial.println("[MODBUS] Initializing RS485...");
    pinMode(PIN_RS485_EN, OUTPUT);
    digitalWrite(PIN_RS485_EN, HIGH);
    delay(300);
    Serial.println("[MODBUS] RS485 powered (WB_IO2=34 HIGH, auto DE/RE)");

    Serial1.begin(MODBUS_BAUD, SERIAL_8N1);
    delay(50);
    uart_flush();
    Serial.print("[MODBUS] UART ready (Slave=");
    Serial.print(MODBUS_SLAVE);
    Serial.print(", Baud=");
    Serial.print(MODBUS_BAUD);
    Serial.println(")");

    /* ---- Step 5: Register scheduler task ---- */
    int8_t idx = scheduler_register(sensor_uplink_task,
                                     SCHEDULER_POLL_INTERVAL_MS,
                                     "sensor_uplink");
    if (idx < 0) {
        Serial.println("[SCHEDULER] Task registration FAILED");
        return;
    }
    Serial.print("[SCHEDULER] Registered: ");
    Serial.print(g_tasks[idx].name);
    Serial.print(" (interval=");
    Serial.print((unsigned long)SCHEDULER_POLL_INTERVAL_MS);
    Serial.println("ms)");

    /* ---- Step 6: Production soak loop ---- */
    Serial.println();
    Serial.print("[SOAK] Starting production soak (");
    Serial.print((unsigned long)(SOAK_DURATION_MS / 1000));
    Serial.println(" seconds)...");

    uint32_t soak_start = millis();
    uint32_t last_cycle = g_cycle_count;
    uint32_t last_progress = soak_start;

    while ((millis() - soak_start) < SOAK_DURATION_MS) {
        scheduler_tick();

        uint32_t current_cycle = g_cycle_count;
        if (current_cycle != last_cycle) {
            uint32_t elapsed = millis() - soak_start;
            Serial.print("[SOAK] Cycle ");
            Serial.print(current_cycle);
            Serial.print(" | modbus=");
            Serial.print(g_last_modbus_ok ? "OK" : "FAIL");
            Serial.print(" | uplink=");
            Serial.print(g_last_uplink_ok ? "OK" : "FAIL");
            Serial.print(" | dur=");
            Serial.print(g_last_cycle_ms);
            Serial.print(" ms | elapsed=");
            Serial.print((unsigned long)(elapsed / 1000));
            Serial.println(" s");

            last_cycle = current_cycle;
            last_progress = millis();
        }

        /* Progress heartbeat every 60s if no cycle fired */
        if ((millis() - last_progress) > 60000) {
            uint32_t elapsed = millis() - soak_start;
            Serial.print("[SOAK]   ... waiting (elapsed=");
            Serial.print((unsigned long)(elapsed / 1000));
            Serial.print(" s, cycles=");
            Serial.print(g_cycle_count);
            Serial.println(")");
            last_progress = millis();
        }

        delay(10);  /* Yield to avoid WDT, reduce power */
    }

    uint32_t soak_actual = millis() - soak_start;

    /* ---- Soak summary ---- */
    Serial.println();
    Serial.println("[RESULT] === Soak Summary ===");
    Serial.print("[RESULT] Duration          : ");
    Serial.print(soak_actual);
    Serial.print(" ms (");
    Serial.print((unsigned long)(soak_actual / 1000));
    Serial.println(" s)");
    Serial.print("[RESULT] Total Cycles      : ");
    Serial.println(g_cycle_count);
    Serial.print("[RESULT] Modbus OK / FAIL  : ");
    Serial.print(g_modbus_ok);
    Serial.print(" / ");
    Serial.println(g_modbus_fail);
    Serial.print("[RESULT] Uplink OK / FAIL  : ");
    Serial.print(g_uplink_ok);
    Serial.print(" / ");
    Serial.println(g_uplink_fail);
    Serial.print("[RESULT] Max Consec Fail   : modbus=");
    Serial.print(g_max_consec_modbus_fail);
    Serial.print(", uplink=");
    Serial.println(g_max_consec_uplink_fail);
    Serial.print("[RESULT] Max Cycle Duration: ");
    Serial.print(g_max_cycle_ms);
    Serial.println(" ms");
    Serial.print("[RESULT] Join Attempts     : 1");
    Serial.println();

    /* ---- Pass/fail evaluation ---- */
    bool pass = true;
    if (g_uplink_ok < (uint32_t)SOAK_MIN_UPLINKS) {
        Serial.print("[RESULT] FAIL: uplinks=");
        Serial.print(g_uplink_ok);
        Serial.print(" < min=");
        Serial.println(SOAK_MIN_UPLINKS);
        pass = false;
    }
    if (g_max_consec_modbus_fail > SOAK_MAX_CONSEC_FAILS) {
        Serial.print("[RESULT] FAIL: consec modbus fail=");
        Serial.println(g_max_consec_modbus_fail);
        pass = false;
    }
    if (g_max_consec_uplink_fail > SOAK_MAX_CONSEC_FAILS) {
        Serial.print("[RESULT] FAIL: consec uplink fail=");
        Serial.println(g_max_consec_uplink_fail);
        pass = false;
    }
    if (g_max_cycle_ms > SOAK_MAX_CYCLE_MS) {
        Serial.print("[RESULT] FAIL: max cycle dur=");
        Serial.print(g_max_cycle_ms);
        Serial.println(" ms");
        pass = false;
    }

    Serial.println(pass ? "[RESULT] PASS" : "[RESULT] FAIL");
    Serial.println("[RESULT] === SOAK COMPLETE ===");

    /* LED blink */
    pinMode(PIN_LED_BLUE, OUTPUT);
    digitalWrite(PIN_LED_BLUE, HIGH);
    delay(500);
    digitalWrite(PIN_LED_BLUE, LOW);
}

void loop() {
    /* In a real production deployment, you would call scheduler_tick()
     * here instead of running the soak in setup(). The soak pattern
     * runs the full loop in setup() so it can print a summary. */
    delay(10000);
}
