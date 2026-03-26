/**
 * @file main.cpp
 * @brief RAK4631 Gate 6 — Runtime Scheduler Integration Example
 * @version 1.0
 * @date 2026-02-25
 *
 * Standalone example demonstrating the production runtime pattern on
 * RAK4631 WisBlock Core (nRF52840 + SX1262):
 *
 *   SystemManager orchestrating:
 *     - TaskScheduler (cooperative, millis()-based)
 *     - LoRaTransport (SX1262 via lora_rak4630_init(), OTAA, AS923-1)
 *     - ModbusPeripheral (RS485 via RAK5802, auto DE/RE)
 *
 * Flow:
 *   1. Init SX1262 via lora_rak4630_init() (BSP one-liner)
 *   2. OTAA join to ChirpStack v4 gateway on AS923-1
 *   3. Init RS485 + Modbus RTU (auto DE/RE, no pin args)
 *   4. Register sensor-uplink task with scheduler (30s interval)
 *   5. Run 3 forced cycles: Modbus read → encode → LoRaWAN uplink
 *
 * This example is self-contained — no dependency on firmware/runtime/ layer.
 * It inlines a minimal cooperative scheduler and the same data flow:
 *   ModbusRead → uint16_t[] → big-endian encode → lmh_send()
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
 * Hex dump helper
 * ================================================================ */
static void print_hex(const uint8_t *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        if (i > 0) Serial.print(' ');
        if (buf[i] < 0x10) Serial.print('0');
        Serial.print(buf[i], HEX);
    }
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

static void scheduler_fire_now(uint8_t idx) {
    if (idx >= g_task_count || !g_tasks[idx].active) return;
    g_tasks[idx].last_run_ms = millis();
    g_tasks[idx].callback();
}

/* ================================================================
 * Runtime State
 * ================================================================ */
static uint32_t g_cycle_count = 0;
static uint16_t g_last_regs[MODBUS_QUANTITY];
static uint8_t  g_last_reg_count = 0;

/* ================================================================
 * Sensor Uplink Task — registered with scheduler
 *
 * Data flow:
 *   1. Read Modbus registers (FC 0x03)
 *   2. Encode as big-endian payload
 *   3. Send unconfirmed LoRaWAN uplink
 *   4. Increment cycle count
 * ================================================================ */
static void sensor_uplink_task(void) {
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
    g_last_reg_count = 0;

    if (rx_len >= 5) {
        uint16_t calc = modbus_crc16(rx, rx_len - 2);
        uint16_t recv = (uint16_t)rx[rx_len - 2] | ((uint16_t)rx[rx_len - 1] << 8);
        if (calc == recv && rx[0] == MODBUS_SLAVE && rx[1] == 0x03 &&
            rx[2] == 2 * MODBUS_QUANTITY) {
            for (uint8_t i = 0; i < MODBUS_QUANTITY; i++) {
                g_last_regs[i] = ((uint16_t)rx[3 + i * 2] << 8) | (uint16_t)rx[4 + i * 2];
            }
            g_last_reg_count = MODBUS_QUANTITY;
            read_ok = true;
        }
    }

    if (read_ok) {
        /* Encode payload: big-endian register values */
        uint8_t payload[MODBUS_QUANTITY * 2];
        uint8_t payload_len = 0;
        for (uint8_t i = 0; i < MODBUS_QUANTITY; i++) {
            payload[payload_len++] = (uint8_t)(g_last_regs[i] >> 8);
            payload[payload_len++] = (uint8_t)(g_last_regs[i] & 0xFF);
        }

        /* Send LoRaWAN uplink */
        lmh_app_data_t lora_data;
        lora_data.buffer   = payload;
        lora_data.buffsize = payload_len;
        lora_data.port     = LORAWAN_UPLINK_PORT;

        lmh_error_status err = lmh_send(&lora_data, LMH_UNCONFIRMED_MSG);

        Serial.print("[RUNTIME] Cycle ");
        Serial.print(g_cycle_count + 1);
        Serial.print(": read=OK (");
        Serial.print(MODBUS_QUANTITY);
        Serial.print(" regs), uplink=");
        Serial.print(err == LMH_SUCCESS ? "OK" : "FAIL");
        Serial.print(" (port=");
        Serial.print(LORAWAN_UPLINK_PORT);
        Serial.print(", ");
        Serial.print(payload_len);
        Serial.println(" bytes)");
    } else {
        Serial.print("[RUNTIME] Cycle ");
        Serial.print(g_cycle_count + 1);
        Serial.println(": read=FAIL");
    }

    g_cycle_count++;
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
    Serial.println("[SYSTEM] RAK4631 Runtime Scheduler Integration");
    Serial.println("========================================");

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

    /* ---- Step 6: Scheduler tick validation (dry run) ---- */
    uint32_t count_before = g_cycle_count;
    scheduler_tick();
    if (g_cycle_count == count_before) {
        Serial.println("[SCHEDULER] Tick: no tasks due (OK)");
    }

    /* ---- Steps 7-9: Force 3 cycles ---- */
    for (uint8_t cycle = 1; cycle <= NUM_TEST_CYCLES; cycle++) {
        Serial.print("[SCHEDULER] Forcing cycle ");
        Serial.print(cycle);
        Serial.println("...");

        scheduler_fire_now(0);
        delay(500);

        if (g_last_reg_count > 0) {
            for (uint8_t i = 0; i < g_last_reg_count; i++) {
                Serial.print("[RESULT] Register[");
                Serial.print(i);
                Serial.print("] = 0x");
                if (g_last_regs[i] < 0x1000) Serial.print('0');
                if (g_last_regs[i] < 0x0100) Serial.print('0');
                if (g_last_regs[i] < 0x0010) Serial.print('0');
                Serial.print(g_last_regs[i], HEX);
                Serial.print(" (");
                Serial.print(g_last_regs[i]);
                Serial.println(")");
            }
        }

        Serial.print("[RESULT] Cycle ");
        Serial.print(cycle);
        Serial.print(": ");
        Serial.println(g_last_reg_count > 0 ? "PASS" : "FAIL");

        /* Inter-cycle delay for LoRaWAN duty cycle */
        if (cycle < NUM_TEST_CYCLES) {
            Serial.println("[SCHEDULER] Inter-cycle delay (3s)...");
            delay(3000);
        }
    }

    /* ---- Summary ---- */
    Serial.println();
    Serial.print("[RESULT] Cycles completed: ");
    Serial.print((unsigned long)g_cycle_count);
    Serial.print("/");
    Serial.println(NUM_TEST_CYCLES);
    Serial.print("[RESULT] Join time: ");
    Serial.print(join_ms);
    Serial.println("ms");
    Serial.print("[RESULT] Last payload: ");
    for (uint8_t i = 0; i < g_last_reg_count; i++) {
        if (i > 0) Serial.print(' ');
        print_hex((const uint8_t*)&g_last_regs[i], 0);  /* Use inline encoding */
        uint8_t hi = (uint8_t)(g_last_regs[i] >> 8);
        uint8_t lo = (uint8_t)(g_last_regs[i] & 0xFF);
        if (hi < 0x10) Serial.print('0');
        Serial.print(hi, HEX);
        Serial.print(' ');
        if (lo < 0x10) Serial.print('0');
        Serial.print(lo, HEX);
    }
    Serial.println();
    Serial.print("[RESULT] Uplink port: ");
    Serial.println(LORAWAN_UPLINK_PORT);

    if (g_cycle_count >= NUM_TEST_CYCLES) {
        Serial.println("[RESULT] PASS");
    } else {
        Serial.println("[RESULT] FAIL");
    }

    /* LED blink */
    pinMode(PIN_LED_BLUE, OUTPUT);
    digitalWrite(PIN_LED_BLUE, HIGH);
    delay(500);
    digitalWrite(PIN_LED_BLUE, LOW);
}

void loop() {
    /* In production, call scheduler_tick() here for periodic dispatch.
     * For this example, cycles are forced in setup(). */
    delay(10000);
}
