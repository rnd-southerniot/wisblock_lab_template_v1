# Gate 6 — Runtime Scheduler Integration: Pass Criteria

**Gate ID:** 6
**Gate Name:** `gate_runtime_scheduler_integration`
**Version:** 1.0
**Date:** 2026-02-24

## Overview

Validates the full runtime stack: cooperative scheduler dispatching periodic
Modbus sensor reads and LoRaWAN uplinks via SystemManager. Transitions from
one-shot gate testing (Gate 5) to runtime firmware patterns.

## Hardware Requirements

- RAK3312 (ESP32-S3 + SX1262) on RAK19007 base board
- RAK5802 RS485 module in I/O Slot
- Modbus RTU sensor (Slave=1, Baud=4800, 8N1) connected via RS485
- LoRaWAN gateway in range (AS923-1)
- ChirpStack v4 with device provisioned (same as Gate 5)

## 10 Pass Criteria

### Step 1 — Create SystemManager
- **PASS:** SystemManager constructed without crash
- **Expected output:** `[GATE6] SystemManager created`

### Step 2 — LoRa Transport Init
- **PASS:** `LoRaTransport::init()` returns true (SX1262 hardware + LoRaMAC initialized)
- **FAIL code 1:** `GATE_FAIL_LORA_INIT`
- **Expected output:** `[GATE6] LoRa Init: OK`

### Step 3 — OTAA Join
- **PASS:** `LoRaTransport::connect()` returns true within 30s timeout
- **FAIL code 2:** `GATE_FAIL_JOIN_TIMEOUT`
- **Expected output:** `[GATE6] Join Accept: OK`

### Step 4 — Modbus Peripheral Init
- **PASS:** `ModbusPeripheral::init()` returns true (RS485 + UART initialized)
- **FAIL code 3:** `GATE_FAIL_MODBUS_INIT`
- **Expected output:** `[GATE6] Modbus Init: OK (Slave=1, Baud=4800)`

### Step 5 — Register Scheduler Tasks
- **PASS:** `scheduler.registerTask()` returns valid index (>= 0)
- **FAIL code 4:** `GATE_FAIL_SCHEDULER_INIT`
- **Expected output:** `[GATE6] Scheduler: 1 task(s) registered`

### Step 6 — Scheduler Tick Validation (dry run)
- **PASS:** `tick()` returns without firing any task (interval not elapsed)
- **Expected output:** `[GATE6] Tick: no tasks due (OK — interval not yet elapsed)`
- **Note:** Non-fatal if task fires unexpectedly

### Step 7 — Force Cycle 1
- **PASS:** `fireNow(0)` triggers sensor read + uplink, cycle count == 1
- **FAIL code 5:** `GATE_FAIL_CYCLE_READ` (Modbus read failed)
- **FAIL code 6:** `GATE_FAIL_CYCLE_UPLINK` (uplink send failed)
- **Expected output:** `[GATE6] Cycle 1: PASS`

### Step 8 — Force Cycle 2
- **PASS:** Same as Step 7, cycle count == 2
- **Expected output:** `[GATE6] Cycle 2: PASS`

### Step 9 — Force Cycle 3
- **PASS:** Same as Step 7, cycle count == 3
- **FAIL code 7:** `GATE_FAIL_CYCLE_COUNT` (count mismatch)
- **Expected output:** `[GATE6] Cycle 3: PASS`

### Step 10 — Gate Summary
- **PASS:** All subsystems operational, 3/3 cycles completed
- **Expected output includes:**
  - `[GATE6]   Scheduler    : OK (1 task)`
  - `[GATE6]   Cycles       : 3/3`
  - `[GATE6] PASS`
  - `[GATE6] === GATE COMPLETE ===`

## Fail Codes

| Code | Constant | Meaning |
|------|----------|---------|
| 0 | `GATE_PASS` | All subsystems initialized, scheduler ran 3 cycles |
| 1 | `GATE_FAIL_LORA_INIT` | LoRaTransport::init() failed |
| 2 | `GATE_FAIL_JOIN_TIMEOUT` | LoRaTransport::connect() timed out |
| 3 | `GATE_FAIL_MODBUS_INIT` | ModbusPeripheral::init() failed |
| 4 | `GATE_FAIL_SCHEDULER_INIT` | Scheduler task registration failed |
| 5 | `GATE_FAIL_CYCLE_READ` | Modbus read failed during scheduler cycle |
| 6 | `GATE_FAIL_CYCLE_UPLINK` | Uplink send failed during scheduler cycle |
| 7 | `GATE_FAIL_CYCLE_COUNT` | Did not complete required number of cycles |
| 8 | `GATE_FAIL_SYSTEM_CRASH` | Runner did not reach GATE COMPLETE |

## Expected Serial Output (successful run)

```
========================================
[SYSTEM] WisBlock Gate Test Runner
[SYSTEM] Core: RAK3312 (ESP32-S3)
[SYSTEM] Gate: 6 - Runtime Scheduler Integration
========================================

[GATE6] === GATE START: gate_runtime_scheduler_integration v1.0 ===
[GATE6] STEP 1: Create SystemManager
[GATE6] SystemManager created
[GATE6] STEP 2: LoRa transport init
[GATE6] LoRa Init: OK
[GATE6] STEP 3: OTAA join (timeout=30s)
[GATE6] Join Start...
[GATE6] Join Accept: OK
[GATE6] STEP 4: Modbus peripheral init
[GATE6] Modbus Init: OK (Slave=1, Baud=4800)
[GATE6] STEP 5: Register scheduler tasks
[GATE6] Task registered: sensor_uplink (interval=30000ms)
[GATE6] Scheduler: 1 task(s) registered
[GATE6] STEP 6: Scheduler tick validation
[GATE6] Tick: no tasks due (OK — interval not yet elapsed)
[GATE6] STEP 7: Force cycle 1
[RUNTIME] Cycle 1: read=OK (4 bytes), uplink=OK (port=10)
[GATE6]   Modbus Read: OK (2 registers)
[GATE6]   Register[0] : 0xXXXX (N)
[GATE6]   Register[1] : 0xYYYY (N)
[GATE6]   Uplink TX: OK (port=10, 4 bytes)
[GATE6] Cycle 1: PASS
[GATE6]   Inter-cycle delay (3s)...
[GATE6] STEP 8: Force cycle 2
[RUNTIME] Cycle 2: read=OK (4 bytes), uplink=OK (port=10)
[GATE6]   Modbus Read: OK (2 registers)
[GATE6]   Uplink TX: OK (port=10, 4 bytes)
[GATE6] Cycle 2: PASS
[GATE6]   Inter-cycle delay (3s)...
[GATE6] STEP 9: Force cycle 3
[RUNTIME] Cycle 3: read=OK (4 bytes), uplink=OK (port=10)
[GATE6]   Modbus Read: OK (2 registers)
[GATE6]   Uplink TX: OK (port=10, 4 bytes)
[GATE6] Cycle 3: PASS
[GATE6] STEP 10: Gate summary
[GATE6]   Scheduler    : OK (1 task)
[GATE6]   Transport    : LoRaTransport (connected)
[GATE6]   Peripheral   : ModbusPeripheral (Slave=1)
[GATE6]   Cycles       : 3/3
[GATE6]   Last Payload : XX XX YY YY
[GATE6]   Uplink Port  : 10
[GATE6] PASS
[GATE6] === GATE COMPLETE ===
```

## Verification Checklist

1. `pio run -e rak3312_gate6` — clean compile, zero errors
2. OTAA credentials in `gate_config.h` match ChirpStack device
3. Flash: `pio run -e rak3312_gate6 -t upload`
4. Serial monitor: all 10 steps pass
5. ChirpStack: 3 uplinks on port 10 (4 bytes each)
6. Serial: `[GATE6] PASS` and `[GATE6] === GATE COMPLETE ===`
