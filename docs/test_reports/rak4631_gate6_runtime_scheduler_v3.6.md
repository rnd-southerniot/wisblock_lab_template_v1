# Test Report — RAK4631 Gate 6: Runtime Scheduler Integration

- **Status:** PASSED
- **Date:** 2026-02-25
- **Gate ID:** 6
- **Gate Name:** gate_rak4631_runtime_scheduler_integration
- **Version:** 1.0
- **Hardware Profile:** v3.6 (RAK4631)
- **Build Environment:** `rak4631_gate6`
- **Tag:** `v3.6-gate6-pass-rak4631`

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
| Toolchain | GCC ARM 7.2.1 |
| TinyUSB | 3.6.0 (framework built-in, symlinked) |
| SX126x-Arduino | 2.0.32 (beegee-tokyo) |
| RAM | 6.6% (16364 / 248832 bytes) |
| Flash | 17.7% (143876 / 815104 bytes) |
| Upload | nrfutil DFU via USB CDC |
| Build flags | `-DCORE_RAK4631 -DRAK4630 -D_VARIANT_RAK4630_` |

## Pass Criteria Results

| # | Criterion | Result | Evidence |
|---|-----------|--------|----------|
| 1 | SystemManager created | PASS | `SystemManager created` (no crash) |
| 2 | LoRa transport init | PASS | `LoRa Init: OK` |
| 3 | OTAA join success | PASS | `Join Accept: OK` |
| 4 | Modbus peripheral init | PASS | `Modbus Init: OK (Slave=1, Baud=4800)` |
| 5 | Scheduler task registered | PASS | `Scheduler: 1 task(s) registered` |
| 6 | Tick dry-run (no fire) | PASS | `Tick: no tasks due (OK)` |
| 7 | Force cycle 1 | PASS | `Cycle 1: PASS` — read OK, uplink OK |
| 8 | Force cycle 2 | PASS | `Cycle 2: PASS` — read OK, uplink OK |
| 9 | Force cycle 3 | PASS | `Cycle 3: PASS` — read OK, uplink OK |
| 10 | Gate summary + PASS | PASS | `Cycles: 3/3`, `PASS`, `GATE COMPLETE` |

## Runtime Architecture Validation

| Component | Class | Status |
|-----------|-------|--------|
| Scheduler | TaskScheduler | 1 task registered, tick() verified, fireNow() functional |
| Transport | LoRaTransport | init → joined → 3 uplinks sent → still connected |
| Peripheral | ModbusPeripheral | init → 3 reads → all returned 2 registers |
| Orchestrator | SystemManager | Singleton active, sensorUplinkTask() fired 3 times |

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
| Uplink Port | 10 |
| Confirmed | No (unconfirmed) |
| Duty Cycle | OFF |

## OTAA Credentials

| Parameter | Value | Source |
|-----------|-------|--------|
| DevEUI | `88:82:24:44:AE:ED:1E:B2` | Gate 0.5 FICR read |
| AppEUI | `00:00:00:00:00:00:00:00` | ChirpStack v4 default |
| AppKey | `91:8C:49:08:F0:89:50:6B:30:18:0B:62:65:9A:4A:D5` | ChirpStack v4 provisioning |

## Scheduler Validation

| Check | Expected | Actual | Result |
|-------|----------|--------|--------|
| registerTask() | Index >= 0 | 0 | PASS |
| Task name | "sensor_uplink" | "sensor_uplink" | PASS |
| Task interval | 30000ms | 30000ms | PASS |
| taskCount() | 1 | 1 | PASS |
| tick() dry run | No task fires | Count unchanged | PASS |
| fireNow(0) × 3 | 3 callbacks | 3 callbacks | PASS |

## Multi-Cycle Results

| Cycle | Modbus Read | Reg[0] | Reg[1] | Uplink | Payload |
|-------|-------------|--------|--------|--------|---------|
| 1 | OK (2 regs) | 0x0004 (4) | 0x0001 (1) | OK (port=10, 4B) | `00 04 00 01` |
| 2 | OK (2 regs) | 0x0001 (1) | 0x0000 (0) | OK (port=10, 4B) | `00 01 00 00` |
| 3 | OK (2 regs) | 0x0002 (2) | 0x0000 (0) | OK (port=10, 4B) | `00 02 00 00` |

Register values vary per cycle — confirms live sensor data (not cached).

## Payload Format

4 bytes per uplink, big-endian register encoding:

| Byte | Content | Cycle 1 | Cycle 2 | Cycle 3 |
|------|---------|---------|---------|---------|
| 0 | Reg[0] MSB | 0x00 | 0x00 | 0x00 |
| 1 | Reg[0] LSB | 0x04 | 0x01 | 0x02 |
| 2 | Reg[1] MSB | 0x00 | 0x00 | 0x00 |
| 3 | Reg[1] LSB | 0x01 | 0x00 | 0x00 |

## Serial Log

```
========================================
[SYSTEM] WisBlock Gate Test Runner
[SYSTEM] Core: RAK4631 (nRF52840)
[SYSTEM] Gate: 6 - Runtime Scheduler Integration
========================================

[GATE6-R4631] === GATE START: gate_rak4631_runtime_scheduler_integration v1.0 ===
[GATE6-R4631] STEP 1: Create SystemManager
[GATE6-R4631] SystemManager created
[GATE6-R4631] STEP 2: LoRa transport init
[GATE6-R4631] LoRa Init: OK
[GATE6-R4631] STEP 3: OTAA join (timeout=30s)
[GATE6-R4631] Join Start...
[GATE6-R4631] Join Accept: OK
[GATE6-R4631] STEP 4: Modbus peripheral init
[GATE6-R4631] Modbus Init: OK (Slave=1, Baud=4800)
[GATE6-R4631] STEP 5: Register scheduler tasks
[GATE6-R4631] Task registered: sensor_uplink (interval=30000ms)
[GATE6-R4631] Scheduler: 1 task(s) registered
[GATE6-R4631] STEP 6: Scheduler tick validation
[GATE6-R4631] Tick: no tasks due (OK — interval not yet elapsed)
[GATE6-R4631] STEP 7: Force cycle 1
[RUNTIME] Cycle 1: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE6-R4631]   Modbus Read: OK (2 registers)
[GATE6-R4631]   Register[0] : 0x0004 (4)
[GATE6-R4631]   Register[1] : 0x0001 (1)
[GATE6-R4631]   Uplink TX: OK (port=10, 4 bytes)
[GATE6-R4631] Cycle 1: PASS
[GATE6-R4631]   Inter-cycle delay (3s)...
[GATE6-R4631] STEP 8: Force cycle 2
[RUNTIME] Cycle 2: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE6-R4631]   Modbus Read: OK (2 registers)
[GATE6-R4631]   Register[0] : 0x0001 (1)
[GATE6-R4631]   Register[1] : 0x0000 (0)
[GATE6-R4631]   Uplink TX: OK (port=10, 4 bytes)
[GATE6-R4631] Cycle 2: PASS
[GATE6-R4631]   Inter-cycle delay (3s)...
[GATE6-R4631] STEP 9: Force cycle 3
[RUNTIME] Cycle 3: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE6-R4631]   Modbus Read: OK (2 registers)
[GATE6-R4631]   Register[0] : 0x0002 (2)
[GATE6-R4631]   Register[1] : 0x0000 (0)
[GATE6-R4631]   Uplink TX: OK (port=10, 4 bytes)
[GATE6-R4631] Cycle 3: PASS
[GATE6-R4631] STEP 10: Gate summary
[GATE6-R4631]   Scheduler    : OK (1 task)
[GATE6-R4631]   Transport    : LoRaTransport (connected)
[GATE6-R4631]   Peripheral   : ModbusPeripheral (Slave=1)
[GATE6-R4631]   Cycles       : 3/3
[GATE6-R4631]   Last Payload : 00 02 00 00
[GATE6-R4631]   Uplink Port  : 10
[GATE6-R4631] PASS
[GATE6-R4631] === GATE COMPLETE ===
```

## RAK4631 vs RAK3312 — Gate 6 Differences

| Aspect | RAK3312 (ESP32-S3) | RAK4631 (nRF52840) |
|--------|--------------------|--------------------|
| SX1262 init | Manual `hw_config` + `lora_hardware_init()` | `lora_rak4630_init()` one-liner |
| SX1262 pins | Explicit in gate_config.h | Hardcoded in BSP function |
| Build flags | `-DCORE_RAK3312` | `-DCORE_RAK4631 -DRAK4630 -D_VARIANT_RAK4630_` |
| hal_uart_init() | 4 args (tx, rx, baud, parity) | 2 args (baud, parity) |
| hal_rs485_enable() | 2 args (en, de) | 1 arg (en) — auto DE/RE |
| Modbus cleanup | `hal_uart_deinit()` + `hal_rs485_disable()` | None (Serial1.end() hangs) |
| Serial output | `Serial.printf()` | `Serial.print()` / `Serial.println()` |
| Log tag | `[GATE6]` | `[GATE6-R4631]` |
| DevEUI | `AC:1F:09:FF:FE:24:02:14` | `88:82:24:44:AE:ED:1E:B2` |
| Scheduler class | `Scheduler` (no conflict) | `TaskScheduler` (BSP collision fix) |
| Flash usage | ~17% (ESP32-S3) | 17.7% (nRF52840) |
| RAM usage | ~5% (ESP32-S3) | 6.6% (nRF52840) |

## Platform Portability — `#ifdef CORE_RAK4631` Guards

Gate 6 validates that the shared `firmware/runtime/` layer works on both platforms
via `#ifdef CORE_RAK4631` guards (3 files modified):

| File | Guard Location | RAK4631 Path | RAK3312 Path |
|------|---------------|--------------|--------------|
| `lora_transport.cpp` | `init()` | `lora_rak4630_init()` | Manual `hw_config` struct |
| `modbus_peripheral.cpp` | includes | `gate_rak4631_modbus_minimal_protocol/` | `gate_modbus_minimal_protocol/` |
| `modbus_peripheral.cpp` | `init()` | 2-arg UART, 1-arg RS485 | 4-arg UART, 2-arg RS485 |
| `modbus_peripheral.cpp` | `deinit()` | Skip (hangs on nRF52840) | `hal_uart_deinit()` + `hal_rs485_disable()` |

## Scheduler→TaskScheduler Rename

During Gate 6 development, a class name collision was discovered:

- **Conflict:** Adafruit nRF52 BSP's `rtos.h` declares `extern SchedulerRTOS Scheduler;`
- **Symptom:** `'Scheduler' does not name a type` in `system_manager.h`
- **Resolution:** Renamed `class Scheduler` → `class TaskScheduler` in 4 files
- **Files affected:**
  - `firmware/runtime/scheduler.h` (class + constructor)
  - `firmware/runtime/scheduler.cpp` (all method prefixes)
  - `firmware/runtime/system_manager.h` (member type + return type)
  - `firmware/runtime/system_manager.cpp` (return type)
- **Impact:** No behavior change. RAK3312 builds unaffected.

## Gate Progression (RAK4631)

| Gate | Name | Status | Tag |
|------|------|--------|-----|
| 0 | Toolchain Validation | PASSED | — |
| 0.5 | DevEUI Read | PASSED | — |
| 1A | I2C Scan | PASSED | — |
| 1B | I2C Identify 0x68/0x69 | PASSED | — |
| 1 | I2C LIS3DH Validation | PASSED | `v3.1-gate1-pass-rak4631` |
| 2-PROBE | RS485 Single Probe | PASSED | — |
| 2 | RS485 Modbus Autodiscovery | PASSED | `v3.2-gate2-pass-rak4631` |
| 3 | Modbus Minimal Protocol | PASSED | `v3.3-gate3-pass-rak4631` |
| 4 | Modbus Robustness Layer | PASSED | `v3.4-gate4-pass-rak4631` |
| 5 | LoRaWAN Join + Uplink | PASSED | `v3.5-gate5-pass-rak4631` |
| 6 | Runtime Scheduler Integration | PASSED | `v3.6-gate6-pass-rak4631` |

## Files — Gate Test

| File | Path |
|------|------|
| gate_config.h | `tests/gates/gate_rak4631_runtime_scheduler_integration/gate_config.h` |
| gate_runner.cpp | `tests/gates/gate_rak4631_runtime_scheduler_integration/gate_runner.cpp` |
| pass_criteria.md | `tests/gates/gate_rak4631_runtime_scheduler_integration/pass_criteria.md` |

## Files — Shared Runtime Layer

| File | Path |
|------|------|
| scheduler.h | `firmware/runtime/scheduler.h` |
| scheduler.cpp | `firmware/runtime/scheduler.cpp` |
| system_manager.h | `firmware/runtime/system_manager.h` |
| system_manager.cpp | `firmware/runtime/system_manager.cpp` |
| lora_transport.h | `firmware/runtime/lora_transport.h` |
| lora_transport.cpp | `firmware/runtime/lora_transport.cpp` |
| modbus_peripheral.h | `firmware/runtime/modbus_peripheral.h` |
| modbus_peripheral.cpp | `firmware/runtime/modbus_peripheral.cpp` |

## Files — Shared from Gate 3

| File | Path |
|------|------|
| modbus_frame.h | `tests/gates/gate_rak4631_modbus_minimal_protocol/modbus_frame.h` |
| modbus_frame.cpp | `tests/gates/gate_rak4631_modbus_minimal_protocol/modbus_frame.cpp` |
| hal_uart.h | `tests/gates/gate_rak4631_modbus_minimal_protocol/hal_uart.h` |
| hal_uart.cpp | `tests/gates/gate_rak4631_modbus_minimal_protocol/hal_uart.cpp` |

## Files — Example

| File | Path |
|------|------|
| main.cpp | `examples/rak4631/runtime_scheduler_integration/main.cpp` |
| pin_map.h | `examples/rak4631/runtime_scheduler_integration/pin_map.h` |
| wiring.md | `examples/rak4631/runtime_scheduler_integration/wiring.md` |
| terminal_test.md | `examples/rak4631/runtime_scheduler_integration/terminal_test.md` |
| prompt_test.md | `examples/rak4631/runtime_scheduler_integration/prompt_test.md` |
| README.md | `examples/rak4631/runtime_scheduler_integration/README.md` |

## ChirpStack Verification

**Status: PENDING** — Manual verification required.

After Gate 6 PASS, verify in ChirpStack v4:

1. Navigate to **Devices → [RAK4631 device] → Events**
2. Confirm **Join** event with DevEUI `88:82:24:44:AE:ED:1E:B2`
3. Confirm **3 Uplink** events on port 10, each with 4-byte payload
4. Uplink payloads (from serial log):
   - Cycle 1: `00 04 00 01` — wind speed 4, direction 1
   - Cycle 2: `00 01 00 00` — wind speed 1, direction 0
   - Cycle 3: `00 02 00 00` — wind speed 2, direction 0
