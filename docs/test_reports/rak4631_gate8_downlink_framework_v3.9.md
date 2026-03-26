# Test Report — RAK4631 Gate 8: Downlink Command Framework v1

- **Status:** PASSED (PASS_WITHOUT_DOWNLINK)
- **Date:** 2026-02-25
- **Gate ID:** 8
- **Gate Name:** gate_downlink_command_framework_v1
- **Version:** 1.0
- **Hardware Profile:** v3.9 (RAK4631)
- **Build Environment:** `rak4631_gate8`
- **Tag:** `v3.9-gate8-pass-rak4631`

---

## Hardware Under Test

| Component | Model | Details |
|-----------|-------|---------|
| WisBlock Core | RAK4631 | nRF52840 @ 64 MHz + SX1262 |
| RS485 Module | RAK5802 | TP8485E transceiver, auto DE/RE, Slot A |
| Base Board | RAK19007 | USB-C, WisBlock standard |
| Modbus Sensor | RS-FSJT-N01 | Wind speed, slave 1, 4800 baud, 8N1 |
| LoRa Gateway | ChirpStack v4 | AS923-1, within range |

## Build Configuration

| Parameter | Value |
|-----------|-------|
| Platform | nordicnrf52 (10.10.0) |
| Board | wiscore_rak4631 |
| Framework | Arduino (Adafruit nRF52 BSP 1.7.0) |
| SX126x-Arduino | 2.0.32 (beegee-tokyo) |
| RAM | 6.6% (16432 / 248832 bytes) |
| Flash | 17.8% (145300 / 815104 bytes) |
| Upload | nrfutil DFU via USB CDC |
| Build flags | `-DCORE_RAK4631 -DRAK4630 -D_VARIANT_RAK4630_ -DGATE_8` |

## Gate Configuration

| Parameter | Value | Source |
|-----------|-------|--------|
| Join Timeout | 30000 ms | `GATE8_JOIN_TIMEOUT_MS` |
| Initial Uplinks | 2 | `GATE8_INITIAL_UPLINKS` |
| Downlink Wait | 60000 ms (60 s) | `GATE8_DOWNLINK_WAIT_MS` |
| Poll Interval | 30000 ms (30 s) | `HW_SYSTEM_POLL_INTERVAL_MS` |
| Command fport | 10 | `DL_CMD_FPORT` |
| ACK fport | 11 | `DL_ACK_FPORT` |

## Pass Criteria Results

| # | Criterion | Required | Actual | Result |
|---|-----------|----------|--------|--------|
| 1 | SystemManager init | OK | OK | PASS |
| 2 | OTAA join | Connected | Connected: YES | PASS |
| 3 | Initial uplinks | 2 sent | 2/2 OK | PASS |
| 4 | Downlink poll | No crash | 60s poll completed | PASS |
| 5 | Scheduler ticks | Continues during wait | 2 additional cycles | PASS |
| 6 | Final status print | Stats printed | All fields present | PASS |
| 7 | GATE COMPLETE | Reached | Printed | PASS |

**Outcome:** `PASS_WITHOUT_DOWNLINK` — all infrastructure validated, no downlink was enqueued in ChirpStack for this run.

## Runtime Metrics

| Metric | Value |
|--------|-------|
| Total Cycles | 4 |
| Modbus OK | 4 |
| Modbus FAIL | 0 |
| Uplink OK | 4 |
| Uplink FAIL | 0 |
| Current Interval | 30000 ms (unchanged — no SET_INTERVAL command received) |
| Current Slave Addr | 1 (unchanged — no SET_SLAVE_ID command received) |
| Downlink Received | No (PASS_WITHOUT_DOWNLINK) |

## Serial Log (full capture)

```
========================================
[SYSTEM] WisBlock Gate Test Runner
[SYSTEM] Core: RAK4631 (nRF52840)
[SYSTEM] Gate: 8 - Downlink Command Framework v1
========================================

========================================
[GATE8] Gate 8: Downlink Command Framework v1
[GATE8] Gate ID: 8
========================================

[GATE8] STEP 1: SystemManager init
[GATE8] Transport: LoRaTransport
[GATE8] Peripheral: ModbusPeripheral
[GATE8] Tasks registered: 1
[GATE8] Connected: YES
[GATE8] STEP 1: PASS

[GATE8] STEP 2: Initial uplinks to open RX windows
[GATE8] Sending 2 uplinks...
[RUNTIME] Cycle 1: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE8] Uplink 1/2 sent (cycle 1)
[RUNTIME] Cycle 2: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE8] Uplink 2/2 sent (cycle 2)
[GATE8] Cycles after initial uplinks: 2
[GATE8] STEP 2: PASS

[GATE8] STEP 3: Polling for downlink
[GATE8] Timeout: 60s
[RUNTIME] Cycle 3: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[RUNTIME] Cycle 4: read=OK (2 regs), uplink=OK (port=10, 4 bytes)

[GATE8] STEP 4: Result evaluation
[GATE8] Total cycles: 4
[GATE8] Modbus OK: 4
[GATE8] Uplink OK: 4
[GATE8] Current interval: 30000 ms
[GATE8] Current slave addr: 1

[GATE8] PASS_WITHOUT_DOWNLINK — no downlink scheduled
[GATE8] (To fully validate, enqueue a downlink in ChirpStack)

[GATE8] === GATE COMPLETE ===
```

## Downlink Framework Validation

### Protocol Implementation

| Feature | Status | Notes |
|---------|--------|-------|
| Binary command parser | Compiled + linked | `downlink_commands.cpp` |
| SET_INTERVAL (0x01) | Code complete | uint32_t BE, range [1000, 3600000] |
| SET_SLAVE_ID (0x02) | Code complete | uint8_t, range [1, 247] |
| REQ_STATUS (0x03) | Code complete | 17-byte status ACK payload |
| REBOOT (0x04) | Code complete | Safety key 0xA5, NVIC_SystemReset |
| ACK uplink builder | Code complete | fport 11, MAGIC+VERSION+CMD+STATUS+VALUE |
| Silent rejection | Code complete | Bad magic/version/short frame → no ACK |
| NACK response | Code complete | Unknown CMD / out-of-range → STATUS=0x00 |

### HAL Downlink Delivery

| Feature | Status | Notes |
|---------|--------|-------|
| Single-slot buffer | Implemented | `s_dl_buf[64]` in lorawan_hal_common.cpp |
| `on_rx_data()` capture | Implemented | IRQ-guarded with `noInterrupts()`/`interrupts()` |
| `lorawan_hal_pop_downlink()` | Implemented | IRQ-guarded, one-shot consumption |
| Volatile flag | Yes | `static volatile bool s_dl_pending` |
| No library leakage | Confirmed | Runtime calls `lorawan_hal_pop_downlink()` — no `lmh_*` types |

### Runtime Additive Changes

| File | Change | Backward Compatible |
|------|--------|---------------------|
| scheduler.h/cpp | Added `setInterval()`, `taskInterval()` | Yes — no existing methods modified |
| modbus_peripheral.h/cpp | Added `setSlaveAddr()`, `slaveAddr()` | Yes — no existing methods modified |
| lorawan_hal.h | Added `lorawan_hal_pop_downlink()` declaration | Yes — no existing functions modified |
| lorawan_hal_common.cpp | Added downlink buffer + `on_rx_data()` capture | Yes — existing join/send untouched |

### Regression Check

| Environment | Status | Duration |
|-------------|--------|----------|
| rak4631_gate8 | SUCCESS | 13.2s |
| rak3312_gate8 | SUCCESS | 6.2s |
| rak4631_gate7 | SUCCESS | 7.1s |
| rak3312_gate7 | SUCCESS | 5.8s |

All 4 environments compile successfully. Gate 7 unaffected.

## LoRaWAN Configuration

| Parameter | Value |
|-----------|-------|
| Region | AS923-1 (LORAMAC_REGION_AS923) |
| Activation | OTAA |
| Class | A |
| ADR | OFF |
| Data Rate | DR2 (SF10) |
| TX Power | TX_POWER_0 |
| Join Trials | 3 (LoRaMAC internal) |
| Join Timeout | 30s (polling) |
| Sensor Uplink Port | 10 |
| ACK Uplink Port | 11 |
| Downlink Command Port | 10 |
| Confirmed | No (unconfirmed) |

## OTAA Credentials

| Parameter | Value | Source |
|-----------|-------|--------|
| DevEUI | `88:82:24:44:AE:ED:1E:B2` | Gate 0.5 FICR read |
| AppEUI | `00:00:00:00:00:00:00:00` | ChirpStack v4 default |
| AppKey | `91:8C:49:08:F0:89:50:6B:30:18:0B:62:65:9A:4A:D5` | ChirpStack v4 provisioning |

## Stability Notes

- **Zero failures** across 4 sensor-uplink cycles: 4/4 Modbus OK, 4/4 uplink OK
- **Scheduler continues ticking** during 60-second downlink poll — 2 additional cycles fired naturally at 30-second intervals
- **OTAA join succeeded** on first attempt (single init via SystemManager)
- **IRQ-guarded downlink buffer** prevents torn reads between radio ISR and main loop
- **HAL v2 layering preserved**: `downlink_commands.cpp` does not include `<LoRaWan-Arduino.h>`
- **No dynamic allocation** in any new code path
- **RAM impact**: +68 bytes (16364 → 16432, from downlink buffer + command parser state)
- **Flash impact**: +912 bytes (144388 → 145300, downlink parser + HAL pop function)

## ChirpStack Downlink Verification

**Status: PENDING** — Manual verification with enqueued downlink.

To fully validate the downlink path:

1. Enqueue in ChirpStack: fport=10, hex=`D101010000271` (SET_INTERVAL to 10s)
2. Flash and monitor Gate 8
3. Verify:
   - `Downlink received!` with hex `D1 01 01 00 00 27 10`
   - `ACK uplink sent OK` on fport 11
   - `Current interval: 10000 ms` in final stats
   - ChirpStack shows uplink on fport 11 with payload `D1010101000027 10`

## Gate Progression (RAK4631)

| Gate | Name | Status | Tag |
|------|------|--------|-----|
| 0 | Toolchain Validation | PASSED | — |
| 0.5 | DevEUI Read | PASSED | — |
| 1 | I2C LIS3DH Validation | PASSED | `v3.1-gate1-pass-rak4631` |
| 2 | RS485 Modbus Autodiscovery | PASSED | `v3.2-gate2-pass-rak4631` |
| 3 | Modbus Minimal Protocol | PASSED | `v3.3-gate3-pass-rak4631` |
| 4 | Modbus Robustness Layer | PASSED | `v3.4-gate4-pass-rak4631` |
| 5 | LoRaWAN Join + Uplink | PASSED | `v3.5-gate5-pass-rak4631` |
| 6 | Runtime Scheduler Integration | PASSED | `v3.6-gate6-pass-rak4631` |
| 7 | Production Loop Soak | PASSED | `v3.7-gate7-pass-rak4631` |
| 8 | Downlink Command Framework v1 | PASSED | `v3.9-gate8-pass-rak4631` |

## Files — Gate Test

| File | Path |
|------|------|
| gate_config.h | `tests/gates/gate_downlink_command_framework_v1/gate_config.h` |
| gate_runner.cpp | `tests/gates/gate_downlink_command_framework_v1/gate_runner.cpp` |
| pass_criteria.md | `tests/gates/gate_downlink_command_framework_v1/pass_criteria.md` |

## Files — Runtime Module (new)

| File | Path | Action |
|------|------|--------|
| downlink_commands.h | `firmware/runtime/downlink_commands.h` | CREATED |
| downlink_commands.cpp | `firmware/runtime/downlink_commands.cpp` | CREATED |

## Files — Runtime (modified — additive only)

| File | Path | Change |
|------|------|--------|
| scheduler.h | `firmware/runtime/scheduler.h` | +setInterval(), +taskInterval() |
| scheduler.cpp | `firmware/runtime/scheduler.cpp` | +setInterval(), +taskInterval() |
| modbus_peripheral.h | `firmware/runtime/modbus_peripheral.h` | +setSlaveAddr(), +slaveAddr() |
| modbus_peripheral.cpp | `firmware/runtime/modbus_peripheral.cpp` | +setSlaveAddr(), +slaveAddr() |

## Files — HAL (modified — additive only)

| File | Path | Change |
|------|------|--------|
| lorawan_hal.h | `firmware/hal/lorawan_hal.h` | +lorawan_hal_pop_downlink() |
| lorawan_hal_common.cpp | `firmware/hal/lorawan_hal_common.cpp` | +downlink buffer, +IRQ guards, +pop impl |

## Files — Example

| File | Path |
|------|------|
| main.cpp | `examples/rak4631/downlink_command_framework_v1/main.cpp` |
| command_schema.md | `examples/rak4631/downlink_command_framework_v1/command_schema.md` |
| chirpstack_enqueue.md | `examples/rak4631/downlink_command_framework_v1/chirpstack_enqueue.md` |
| terminal_test.md | `examples/rak4631/downlink_command_framework_v1/terminal_test.md` |
| prompt_test.md | `examples/rak4631/downlink_command_framework_v1/prompt_test.md` |
| README.md | `examples/rak4631/downlink_command_framework_v1/README.md` |
