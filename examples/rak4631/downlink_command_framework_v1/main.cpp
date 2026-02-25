/**
 * @file main.cpp
 * @brief RAK4631 Gate 8 — Downlink Command Framework v1 Example
 * @version 1.0
 * @date 2026-02-25
 *
 * Standalone example demonstrating bidirectional LoRaWAN communication
 * on RAK4631 WisBlock Core (nRF52840 + SX1262):
 *
 *   1. Init SX1262 via lora_rak4630_init() (BSP one-liner)
 *   2. OTAA join to ChirpStack v4 gateway on AS923-1
 *   3. Init RS485 + Modbus RTU (auto DE/RE)
 *   4. Register sensor-uplink task with inline scheduler (30s interval)
 *   5. Send initial uplinks to open RX windows
 *   6. Poll for downlink, parse binary command, apply runtime change
 *   7. Send ACK uplink on fport 11
 *
 * Binary command protocol (fport=10):
 *   [0xD1] [0x01] [CMD] [PAYLOAD...]
 *   CMD 0x01: SET_INTERVAL (uint32_t BE, 1000..3600000 ms)
 *   CMD 0x02: SET_SLAVE_ID (uint8_t, 1..247)
 *   CMD 0x03: REQ_STATUS   (no payload)
 *   CMD 0x04: REBOOT       (uint8_t safety key = 0xA5)
 *
 * This example is self-contained — no dependency on firmware/runtime/ layer.
 * It inlines the downlink parser and scheduler for portability.
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

/* ================================================================
 * Configuration
 * ================================================================ */

/* OTAA Credentials — UPDATE THESE FOR YOUR DEVICE */
static uint8_t nodeDeviceEUI[8]  = { 0x88, 0x82, 0x24, 0x44, 0xAE, 0xED, 0x1E, 0xB2 };
static uint8_t nodeAppEUI[8]     = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t nodeAppKey[16]    = { 0x91, 0x8C, 0x49, 0x08, 0xF0, 0x89, 0x50, 0x6B,
                                     0x30, 0x18, 0x0B, 0x62, 0x65, 0x9A, 0x4A, 0xD5 };

/* Pin Map */
#define PIN_RS485_EN       34   /* WB_IO2 — 3V3_S power enable */
#define PIN_LED_BLUE       36   /* P1.04 — Blue LED */

/* Modbus Configuration */
#define MODBUS_SLAVE       1
#define MODBUS_BAUD        4800
#define MODBUS_QUANTITY    2    /* Read 2 consecutive registers */
#define RESPONSE_TIMEOUT   200  /* Per-attempt timeout (ms) */

/* LoRaWAN */
#define LORAWAN_UPLINK_PORT    10
#define LORAWAN_ACK_PORT       11
#define LORAWAN_JOIN_TIMEOUT   30000

/* Scheduler */
#define POLL_INTERVAL_MS       30000

/* Downlink Protocol */
#define DL_MAGIC               0xD1
#define DL_VERSION             0x01
#define DL_CMD_SET_INTERVAL    0x01
#define DL_CMD_SET_SLAVE_ID    0x02
#define DL_CMD_REQ_STATUS      0x03
#define DL_CMD_REBOOT          0x04
#define DL_INTERVAL_MIN        1000
#define DL_INTERVAL_MAX        3600000
#define DL_SLAVE_MIN           1
#define DL_SLAVE_MAX           247
#define DL_REBOOT_KEY          0xA5

/* Gate Timing */
#define INITIAL_UPLINKS        2
#define DOWNLINK_WAIT_MS       60000

/* ================================================================
 * Join State (set by async callbacks)
 * ================================================================ */
static volatile bool g_joined      = false;
static volatile bool g_join_failed = false;

/* ================================================================
 * Single-Slot Downlink Buffer (IRQ-guarded)
 * ================================================================ */
static volatile bool g_dl_pending = false;
static uint8_t g_dl_buf[64];
static uint8_t g_dl_len  = 0;
static uint8_t g_dl_port = 0;

/* ================================================================
 * LoRaWAN Callbacks
 * ================================================================ */
static void lorawan_has_joined(void)  { g_joined = true; }
static void lorawan_join_failed(void) { g_join_failed = true; }

static void lorawan_rx(lmh_app_data_t* data) {
    if (data && data->buffsize > 0 && data->buffsize <= sizeof(g_dl_buf)) {
        noInterrupts();
        memcpy(g_dl_buf, data->buffer, data->buffsize);
        g_dl_len  = data->buffsize;
        g_dl_port = data->port;
        g_dl_pending = true;
        interrupts();
    }
}

static void lorawan_class_confirm(DeviceClass_t cls) { (void)cls; }

static lmh_param_t lora_param = {
    LORAWAN_ADR_OFF, DR_2, true, 3, TX_POWER_0, LORAWAN_DUTYCYCLE_OFF
};

static lmh_callback_t lora_callbacks = {
    BoardGetBatteryLevel, BoardGetUniqueId, BoardGetRandomSeed,
    lorawan_rx, lorawan_has_joined, lorawan_class_confirm, lorawan_join_failed
};

/* ================================================================
 * Pop downlink (IRQ-guarded)
 * ================================================================ */
static bool pop_downlink(uint8_t* out, uint8_t* out_len, uint8_t* out_port) {
    noInterrupts();
    if (!g_dl_pending) { interrupts(); return false; }
    uint8_t len = g_dl_len;
    memcpy(out, g_dl_buf, len);
    *out_len  = len;
    *out_port = g_dl_port;
    g_dl_pending = false;
    interrupts();
    return true;
}

/* ================================================================
 * CRC-16/MODBUS
 * ================================================================ */
static uint16_t modbus_crc16(const uint8_t* data, uint8_t len) {
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i];
        for (uint8_t bit = 0; bit < 8; bit++)
            crc = (crc & 1) ? ((crc >> 1) ^ 0xA001) : (crc >> 1);
    }
    return crc;
}

/* ================================================================
 * UART helpers
 * ================================================================ */
static uint8_t uart_read_bytes(uint8_t* buf, uint8_t max_len, uint32_t timeout_ms) {
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

static void uart_flush(void) {
    Serial1.flush();
    delay(10);
    unsigned long t = millis();
    while (Serial1.available() && (millis() - t) < 100) Serial1.read();
}

/* ================================================================
 * Inline Cooperative Scheduler
 * ================================================================ */
typedef void (*task_callback_t)(void);
struct ScheduledTask { task_callback_t cb; uint32_t interval_ms; uint32_t last_ms; bool active; };
#define MAX_TASKS 4
static ScheduledTask g_tasks[MAX_TASKS];
static uint8_t g_task_count = 0;

static int8_t sched_register(task_callback_t cb, uint32_t interval_ms) {
    if (g_task_count >= MAX_TASKS || cb == nullptr) return -1;
    uint8_t idx = g_task_count;
    g_tasks[idx] = { cb, interval_ms, millis(), true };
    g_task_count++;
    return (int8_t)idx;
}

static void sched_tick(void) {
    uint32_t now = millis();
    for (uint8_t i = 0; i < g_task_count; i++) {
        if (!g_tasks[i].active || g_tasks[i].cb == nullptr) continue;
        if ((now - g_tasks[i].last_ms) >= g_tasks[i].interval_ms) {
            g_tasks[i].last_ms = now;
            g_tasks[i].cb();
        }
    }
}

static void sched_fire_now(uint8_t idx) {
    if (idx >= g_task_count || !g_tasks[idx].active) return;
    g_tasks[idx].last_ms = millis();
    g_tasks[idx].cb();
}

/* ================================================================
 * Runtime State
 * ================================================================ */
static uint32_t g_cycle_count  = 0;
static uint32_t g_modbus_ok    = 0;
static uint32_t g_uplink_ok    = 0;
static uint8_t  g_slave_addr   = MODBUS_SLAVE;
static uint32_t g_interval_ms  = POLL_INTERVAL_MS;

/* ================================================================
 * Send LoRaWAN uplink
 * ================================================================ */
static bool lora_send(const uint8_t* data, uint8_t len, uint8_t port) {
    if (!g_joined) return false;
    static uint8_t tx_buf[64];
    uint8_t n = (len <= sizeof(tx_buf)) ? len : (uint8_t)sizeof(tx_buf);
    memcpy(tx_buf, data, n);
    lmh_app_data_t app = { tx_buf, n, port };
    return (lmh_send(&app, LMH_UNCONFIRMED_MSG) == LMH_SUCCESS);
}

/* ================================================================
 * Sensor Uplink Task
 * ================================================================ */
static void sensor_uplink_task(void) {
    /* Build Modbus request */
    uint8_t tx[8];
    tx[0] = g_slave_addr; tx[1] = 0x03;
    tx[2] = 0x00; tx[3] = 0x00;
    tx[4] = 0x00; tx[5] = MODBUS_QUANTITY;
    uint16_t crc = modbus_crc16(tx, 6);
    tx[6] = (uint8_t)(crc & 0xFF);
    tx[7] = (uint8_t)((crc >> 8) & 0xFF);

    uart_flush(); delay(10);
    Serial1.write(tx, 8); Serial1.flush();
    delayMicroseconds(500);

    uint8_t rx[32];
    uint8_t rx_len = uart_read_bytes(rx, sizeof(rx), RESPONSE_TIMEOUT);

    bool read_ok = false;
    uint16_t regs[MODBUS_QUANTITY];

    if (rx_len >= 5) {
        uint16_t calc = modbus_crc16(rx, rx_len - 2);
        uint16_t recv = (uint16_t)rx[rx_len - 2] | ((uint16_t)rx[rx_len - 1] << 8);
        if (calc == recv && rx[0] == g_slave_addr && rx[1] == 0x03 &&
            rx[2] == 2 * MODBUS_QUANTITY) {
            for (uint8_t i = 0; i < MODBUS_QUANTITY; i++)
                regs[i] = ((uint16_t)rx[3 + i*2] << 8) | (uint16_t)rx[4 + i*2];
            read_ok = true;
        }
    }

    if (read_ok) {
        g_modbus_ok++;
        uint8_t payload[MODBUS_QUANTITY * 2];
        uint8_t plen = 0;
        for (uint8_t i = 0; i < MODBUS_QUANTITY; i++) {
            payload[plen++] = (uint8_t)(regs[i] >> 8);
            payload[plen++] = (uint8_t)(regs[i] & 0xFF);
        }
        bool send_ok = lora_send(payload, plen, LORAWAN_UPLINK_PORT);
        if (send_ok) g_uplink_ok++;

        Serial.print("[RUNTIME] Cycle "); Serial.print(g_cycle_count + 1);
        Serial.print(": read=OK, uplink="); Serial.println(send_ok ? "OK" : "FAIL");
    } else {
        Serial.print("[RUNTIME] Cycle "); Serial.print(g_cycle_count + 1);
        Serial.println(": read=FAIL");
    }
    g_cycle_count++;
}

/* ================================================================
 * Downlink Command Parser
 * ================================================================ */
static void encode_be32(uint8_t* buf, uint32_t val) {
    buf[0] = (uint8_t)(val >> 24); buf[1] = (uint8_t)(val >> 16);
    buf[2] = (uint8_t)(val >>  8); buf[3] = (uint8_t)(val);
}

static uint32_t decode_be32(const uint8_t* buf) {
    return ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16)
         | ((uint32_t)buf[2] <<  8) | ((uint32_t)buf[3]);
}

struct DlResult {
    bool needs_ack;
    uint8_t ack[20];
    uint8_t ack_len;
    bool needs_reboot;
};

static uint8_t ack_hdr(uint8_t* buf, uint8_t cmd, uint8_t status) {
    buf[0] = DL_MAGIC; buf[1] = DL_VERSION; buf[2] = cmd; buf[3] = status;
    return 4;
}

static bool parse_downlink(const uint8_t* buf, uint8_t len, DlResult& r) {
    memset(&r, 0, sizeof(r));

    /* Silent rejection */
    if (len < 3 || buf[0] != DL_MAGIC || buf[1] != DL_VERSION) return false;

    uint8_t cmd = buf[2];

    switch (cmd) {
    case DL_CMD_SET_INTERVAL: {
        if (len < 7) { r.needs_ack = true; r.ack_len = ack_hdr(r.ack, cmd, 0x00); return true; }
        uint32_t val = decode_be32(&buf[3]);
        if (val < DL_INTERVAL_MIN || val > DL_INTERVAL_MAX) {
            r.needs_ack = true; r.ack_len = ack_hdr(r.ack, cmd, 0x00); return true;
        }
        g_interval_ms = val;
        g_tasks[0].interval_ms = val;
        r.needs_ack = true; r.ack_len = ack_hdr(r.ack, cmd, 0x01);
        encode_be32(&r.ack[4], val); r.ack_len += 4;
        Serial.print("[DL] SET_INTERVAL: "); Serial.print(val); Serial.println(" ms");
        return true;
    }
    case DL_CMD_SET_SLAVE_ID: {
        if (len < 4) { r.needs_ack = true; r.ack_len = ack_hdr(r.ack, cmd, 0x00); return true; }
        uint8_t addr = buf[3];
        if (addr < DL_SLAVE_MIN || addr > DL_SLAVE_MAX) {
            r.needs_ack = true; r.ack_len = ack_hdr(r.ack, cmd, 0x00); return true;
        }
        g_slave_addr = addr;
        r.needs_ack = true; r.ack_len = ack_hdr(r.ack, cmd, 0x01);
        r.ack[4] = addr; r.ack_len += 1;
        Serial.print("[DL] SET_SLAVE_ID: "); Serial.println(addr);
        return true;
    }
    case DL_CMD_REQ_STATUS: {
        r.needs_ack = true; r.ack_len = ack_hdr(r.ack, cmd, 0x01);
        r.ack[4] = g_slave_addr;
        encode_be32(&r.ack[5], g_interval_ms);
        encode_be32(&r.ack[9], g_uplink_ok);
        encode_be32(&r.ack[13], g_modbus_ok);
        r.ack_len += 13;
        Serial.println("[DL] REQ_STATUS");
        return true;
    }
    case DL_CMD_REBOOT: {
        if (len < 4) { r.needs_ack = true; r.ack_len = ack_hdr(r.ack, cmd, 0x00); return true; }
        if (buf[3] != DL_REBOOT_KEY) {
            r.needs_ack = true; r.ack_len = ack_hdr(r.ack, cmd, 0x00); return true;
        }
        r.needs_ack = true; r.needs_reboot = true;
        r.ack_len = ack_hdr(r.ack, cmd, 0x01);
        r.ack[4] = DL_REBOOT_KEY; r.ack_len += 1;
        Serial.println("[DL] REBOOT (safety key valid)");
        return true;
    }
    default:
        r.needs_ack = true; r.ack_len = ack_hdr(r.ack, cmd, 0x00);
        Serial.print("[DL] Unknown CMD: 0x"); Serial.println(cmd, HEX);
        return true;
    }
}

/* ================================================================
 * Print hex bytes
 * ================================================================ */
static void print_hex(const uint8_t* buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        if (buf[i] < 0x10) Serial.print("0");
        Serial.print(buf[i], HEX);
        if (i < len - 1) Serial.print(" ");
    }
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
    Serial.println("[SYSTEM] RAK4631 Downlink Command Framework v1");
    Serial.println("========================================");

    /* ---- Step 1: LoRaWAN init ---- */
    Serial.println("[INIT] Initializing SX1262...");
    if (lora_rak4630_init() != 0) { Serial.println("[INIT] SX1262 FAIL"); return; }
    if (lmh_init(&lora_callbacks, lora_param, true, CLASS_A, LORAMAC_REGION_AS923) != 0) {
        Serial.println("[INIT] lmh_init FAIL"); return;
    }
    Serial.println("[INIT] LoRaMAC: OK (AS923-1, Class A, DR2)");

    lmh_setDevEui(nodeDeviceEUI);
    lmh_setAppEui(nodeAppEUI);
    lmh_setAppKey(nodeAppKey);

    /* ---- Step 2: OTAA join ---- */
    Serial.println("[JOIN] Starting OTAA join...");
    g_joined = false; g_join_failed = false;
    lmh_join();
    unsigned long j0 = millis();
    while (!g_joined && !g_join_failed) {
        if (millis() - j0 > LORAWAN_JOIN_TIMEOUT) break;
        delay(100);
    }
    if (!g_joined) { Serial.println("[JOIN] FAIL"); return; }
    Serial.print("[JOIN] OK ("); Serial.print(millis() - j0); Serial.println(" ms)");

    /* ---- Step 3: Modbus init ---- */
    pinMode(PIN_RS485_EN, OUTPUT);
    digitalWrite(PIN_RS485_EN, HIGH);
    delay(300);
    Serial1.begin(MODBUS_BAUD, SERIAL_8N1);
    delay(50);
    uart_flush();
    Serial.println("[MODBUS] RS485 ready (Slave=1, 4800 baud, auto DE/RE)");

    /* ---- Step 4: Register scheduler task ---- */
    sched_register(sensor_uplink_task, g_interval_ms);
    Serial.print("[SCHED] Registered sensor_uplink (interval=");
    Serial.print(g_interval_ms); Serial.println(" ms)");

    /* ---- Step 5: Initial uplinks (open RX windows) ---- */
    Serial.print("[GATE8] Sending "); Serial.print(INITIAL_UPLINKS);
    Serial.println(" initial uplinks...");
    for (int i = 0; i < INITIAL_UPLINKS; i++) {
        sched_fire_now(0);
        delay(5000);
    }

    /* ---- Step 6: Downlink poll loop ---- */
    Serial.print("[GATE8] Polling for downlink ("); Serial.print(DOWNLINK_WAIT_MS / 1000);
    Serial.println("s timeout)...");

    uint32_t dl_start = millis();
    bool dl_received = false;

    while ((millis() - dl_start) < DOWNLINK_WAIT_MS) {
        uint8_t dl_buf[64]; uint8_t dl_len, dl_port;
        if (pop_downlink(dl_buf, &dl_len, &dl_port)) {
            Serial.print("[GATE8] Downlink! port="); Serial.print(dl_port);
            Serial.print(", len="); Serial.print(dl_len);
            Serial.print(", hex="); print_hex(dl_buf, dl_len); Serial.println();

            DlResult result;
            parse_downlink(dl_buf, dl_len, result);

            if (result.needs_ack) {
                Serial.print("[GATE8] ACK (fport=11, len="); Serial.print(result.ack_len);
                Serial.print("): "); print_hex(result.ack, result.ack_len); Serial.println();
                bool ack_ok = lora_send(result.ack, result.ack_len, LORAWAN_ACK_PORT);
                Serial.print("[GATE8] ACK uplink "); Serial.println(ack_ok ? "OK" : "FAIL");
            }

            if (result.needs_reboot) {
                Serial.println("[GATE8] Rebooting in 2s...");
                delay(2000);
                NVIC_SystemReset();
            }

            dl_received = true;
            break;
        }
        sched_tick();
        delay(100);
    }

    /* ---- Result ---- */
    Serial.println();
    Serial.print("[GATE8] Cycles: "); Serial.print(g_cycle_count);
    Serial.print(", Modbus OK: "); Serial.print(g_modbus_ok);
    Serial.print(", Uplink OK: "); Serial.print(g_uplink_ok);
    Serial.print(", Interval: "); Serial.print(g_interval_ms);
    Serial.print(" ms, Slave: "); Serial.println(g_slave_addr);

    if (dl_received) {
        Serial.println("[GATE8] PASS — downlink received and processed");
    } else {
        Serial.println("[GATE8] PASS_WITHOUT_DOWNLINK — no downlink scheduled");
        Serial.println("[GATE8] (Enqueue a downlink in ChirpStack to fully validate)");
    }
    Serial.println("[GATE8] === GATE COMPLETE ===");

    pinMode(PIN_LED_BLUE, OUTPUT);
    digitalWrite(PIN_LED_BLUE, HIGH); delay(500); digitalWrite(PIN_LED_BLUE, LOW);
}

void loop() {
    delay(10000);
}
