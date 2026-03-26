# Test Report — RAK4631 Gate 7: Production Loop Soak

- **Status:** PASSED
- **Date:** 2026-02-25
- **Gate ID:** 7
- **Gate Name:** gate_rak4631_production_loop_soak
- **Version:** 1.0
- **Hardware Profile:** v3.7 (RAK4631)
- **Build Environment:** `rak4631_gate7`
- **Tag:** `v3.7-gate7-pass-rak4631`

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
| Flash | 17.7% (144388 / 815104 bytes) |
| Upload | nrfutil DFU via USB CDC |
| Build flags | `-DCORE_RAK4631 -DRAK4630 -D_VARIANT_RAK4630_` |

## Soak Configuration

| Parameter | Value | Source |
|-----------|-------|--------|
| Soak Duration | 300000 ms (5 min) | `GATE7_SOAK_DURATION_MS` |
| Poll Interval | 30000 ms (30 s) | `HW_SYSTEM_POLL_INTERVAL_MS` |
| Expected Cycles | ~10 | duration / interval |
| Min Uplinks | 3 | `GATE7_MIN_UPLINKS` (conservative) |
| Max Consec Fails | 2 | `GATE7_MAX_CONSEC_FAILS` |
| Max Cycle Duration | 1500 ms | `GATE7_MAX_CYCLE_DURATION_MS` |
| Join Timeout | 30 s | `GATE7_JOIN_TIMEOUT_MS` |

## Pass Criteria Results

| # | Criterion | Required | Actual | Result |
|---|-----------|----------|--------|--------|
| 1 | Single join | 1 attempt | 1 attempt | PASS |
| 2 | Full duration | >= 300000 ms | 300007 ms | PASS |
| 3 | Min uplinks | >= 3 | 9 | PASS |
| 4 | Modbus stability | <= 2 consec fail | 0 consec fail | PASS |
| 5 | Uplink stability | <= 2 consec fail | 0 consec fail | PASS |
| 6 | Cycle duration | < 1500 ms | 82 ms max | PASS |
| 7 | Transport connected | yes | connected | PASS |

## Soak Metrics Summary

| Metric | Value |
|--------|-------|
| Soak Duration | 300,007 ms (300 s) |
| Total Cycles | 9 |
| Modbus OK | 9 |
| Modbus FAIL | 0 |
| Uplink OK | 9 |
| Uplink FAIL | 0 |
| Max Consecutive Modbus Fail | 0 |
| Max Consecutive Uplink Fail | 0 |
| Max Cycle Duration | 82 ms |
| Min Cycle Duration | 80 ms |
| Typical Cycle Duration | 81 ms |
| Transport at End | connected |
| Join Attempts | 1 (single init) |

## Per-Cycle Detail

| Cycle | Modbus | Uplink | Duration | Elapsed |
|-------|--------|--------|----------|---------|
| 1 | OK | OK | ~81 ms | 30 s |
| 2 | OK | OK | ~81 ms | 60 s |
| 3 | OK | OK | 81 ms | 90 s |
| 4 | OK | OK | 80 ms | 120 s |
| 5 | OK | OK | 81 ms | 150 s |
| 6 | OK | OK | 81 ms | 180 s |
| 7 | OK | OK | 81 ms | 210 s |
| 8 | OK | OK | 82 ms | 240 s |
| 9 | OK | OK | 82 ms | 270 s |

All 9 cycles completed with zero failures. Cycle durations were consistently
80-82 ms, well under the 1500 ms limit (18× margin).

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

## Serial Log (captured cycles 3-9 + summary)

```
[RUNTIME] Cycle 3: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE7-R4631] Cycle 3 | modbus=OK | uplink=OK | dur=81 ms | elapsed=90 s
[RUNTIME] Cycle 4: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE7-R4631] Cycle 4 | modbus=OK | uplink=OK | dur=80 ms | elapsed=120 s
[RUNTIME] Cycle 5: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE7-R4631] Cycle 5 | modbus=OK | uplink=OK | dur=81 ms | elapsed=150 s
[RUNTIME] Cycle 6: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE7-R4631] Cycle 6 | modbus=OK | uplink=OK | dur=81 ms | elapsed=180 s
[RUNTIME] Cycle 7: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE7-R4631] Cycle 7 | modbus=OK | uplink=OK | dur=81 ms | elapsed=210 s
[RUNTIME] Cycle 8: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE7-R4631] Cycle 8 | modbus=OK | uplink=OK | dur=82 ms | elapsed=240 s
[RUNTIME] Cycle 9: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE7-R4631] Cycle 9 | modbus=OK | uplink=OK | dur=82 ms | elapsed=270 s

[GATE7-R4631] STEP 3: Soak summary
[GATE7-R4631]   Duration          : 300007 ms (300 s)
[GATE7-R4631]   Total Cycles      : 9
[GATE7-R4631]   Modbus OK / FAIL  : 9 / 0
[GATE7-R4631]   Uplink OK / FAIL  : 9 / 0
[GATE7-R4631]   Max Consec Fail   : modbus=0, uplink=0
[GATE7-R4631]   Max Cycle Duration: 82 ms
[GATE7-R4631]   Transport         : connected
[GATE7-R4631]   Join Attempts     : 1
[GATE7-R4631] PASS
[GATE7-R4631] === GATE COMPLETE ===
```

> **Note:** Serial capture connected after device boot. Cycles 1-2 and the init
> banner were output before the serial monitor attached. The init phase and
> cycles 1-2 are confirmed successful by the final summary (9 total cycles,
> 9 modbus OK, 9 uplink OK).

## RAK4631 vs RAK3312 — Gate 7 Differences

| Aspect | RAK3312 (ESP32-S3) | RAK4631 (nRF52840) |
|--------|--------------------|--------------------|
| SX1262 init | Manual `hw_config` + `lora_hardware_init()` | `lora_rak4630_init()` one-liner |
| SX1262 pins | Explicit in gate_config.h | Hardcoded in BSP function |
| Build flags | `-DCORE_RAK3312` | `-DCORE_RAK4631 -DRAK4630 -D_VARIANT_RAK4630_` |
| hal_uart_init() | 4 args (tx, rx, baud, parity) | 2 args (baud, parity) |
| hal_rs485_enable() | 2 args (en, de) | 1 arg (en) — auto DE/RE |
| Serial output | `Serial.printf()` | `Serial.print()` / `Serial.println()` |
| Log tag | `[GATE7]` | `[GATE7-R4631]` |
| DevEUI | `AC:1F:09:FF:FE:24:02:14` | `88:82:24:44:AE:ED:1E:B2` |
| hardware_profile.h | Yes — includes HW_* defines | No — defines inline in gate_config.h |
| Min uplinks | 9 (formula-based) | 3 (conservative, user-specified) |
| Soak define prefix | `GATE_SOAK_*` | `GATE7_*` |

## Payload Format

4 bytes per uplink, big-endian register encoding:

| Byte | Content |
|------|---------|
| 0 | Register[0] MSB (wind speed high) |
| 1 | Register[0] LSB (wind speed low) |
| 2 | Register[1] MSB (wind direction high) |
| 3 | Register[1] LSB (wind direction low) |

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
| 7 | Production Loop Soak | PASSED | `v3.7-gate7-pass-rak4631` |

## Files — Gate Test

| File | Path |
|------|------|
| gate_config.h | `tests/gates/gate_rak4631_production_loop_soak/gate_config.h` |
| gate_runner.cpp | `tests/gates/gate_rak4631_production_loop_soak/gate_runner.cpp` |
| pass_criteria.md | `tests/gates/gate_rak4631_production_loop_soak/pass_criteria.md` |

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
| main.cpp | `examples/rak4631/production_loop_soak/main.cpp` |
| pin_map.h | `examples/rak4631/production_loop_soak/pin_map.h` |
| wiring.md | `examples/rak4631/production_loop_soak/wiring.md` |
| terminal_test.md | `examples/rak4631/production_loop_soak/terminal_test.md` |
| prompt_test.md | `examples/rak4631/production_loop_soak/prompt_test.md` |
| README.md | `examples/rak4631/production_loop_soak/README.md` |

## ChirpStack Verification

**Status: PENDING** — Manual verification required.

After Gate 7 PASS, verify in ChirpStack v4:

1. Navigate to **Devices → [RAK4631 device] → Events**
2. Confirm **Join** event with DevEUI `88:82:24:44:AE:ED:1E:B2`
3. Confirm **~9 Uplink** events on port 10, each with 4-byte payload
4. Verify uplinks are spaced ~30 seconds apart (natural scheduler interval)
5. No repeated join events during the 5-minute soak period
