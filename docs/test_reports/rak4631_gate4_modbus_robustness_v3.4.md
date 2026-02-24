# Test Report — RAK4631 Gate 4: Modbus Robustness Layer Validation

- **Status:** PASSED
- **Date:** 2026-02-24
- **Gate ID:** 4
- **Gate Name:** gate_rak4631_modbus_robustness_layer
- **Version:** 1.0
- **Hardware Profile:** v3.4 (RAK4631)
- **Build Environment:** `rak4631_gate4`
- **Tag:** `v3.4-gate4-pass-rak4631`

---

## Hardware Under Test

| Component | Model | Details |
|-----------|-------|---------|
| WisBlock Core | RAK4631 | nRF52840 @ 64 MHz + SX1262 |
| RS485 Module | RAK5802 | TP8485E transceiver, auto DE/RE, Slot A |
| Base Board | RAK19007 | USB-C, WisBlock standard |
| Modbus Sensor | RS-FSJT-N01 | Wind speed, slave 1, 4800 baud, 8N1 |

## Build Configuration

| Parameter | Value |
|-----------|-------|
| Platform | nordicnrf52 (10.10.0) |
| Board | wiscore_rak4631 |
| Framework | Arduino (Adafruit nRF52 BSP 1.7.0) |
| Toolchain | GCC ARM 7.2.1 |
| TinyUSB | 3.6.0 (framework built-in, symlinked) |
| RAM | 3.6% (8980 / 248832 bytes) |
| Flash | 7.7% (62776 / 815104 bytes) |
| Upload | nrfutil DFU via USB CDC |

## Pass Criteria Results

| # | Criterion | Result | Evidence |
|---|-----------|--------|----------|
| 1 | UART Init | PASS | `UART Init: OK (4800 8N1)` |
| 2 | Multi-register TX | PASS | `TX (8 bytes): 01 03 00 00 00 02 C4 0B` |
| 3 | Valid response | PASS | `RX (9 bytes): 01 03 04 00 06 00 01 DB F2` (CRC valid) |
| 4 | byte_count == 2×qty | PASS | `byte_count=4, expected=4` |
| 5 | Register values | PASS | `Register[0]: 0x0006`, `Register[1]: 0x0001` |
| 6 | Error classification | PASS | `Error: NONE` logged after attempt |
| 7 | Retry/recovery status | PASS | `SUCCESS on attempt 1 (0 retries, 0 UART recoveries)` |
| 8 | No system crash | PASS | `[GATE4-R4631] === GATE COMPLETE ===` |

## Protocol Trace

| Direction | Frame (hex) | Decoded |
|-----------|-------------|---------|
| TX | `01 03 00 00 00 02 C4 0B` | Slave=1, FC=0x03, Reg=0x0000, Qty=2, CRC=0x0BC4 |
| RX | `01 03 04 00 06 00 01 DB F2` | Slave=1, FC=0x03, Bytes=4, Reg[0]=0x0006, Reg[1]=0x0001, CRC=0xF2DB |

## Multi-Register Read Validation

| Check | Expected | Actual | Result |
|-------|----------|--------|--------|
| Quantity requested | 2 | 2 | PASS |
| Response length | 9 bytes | 9 bytes | PASS |
| byte_count field | 4 | 4 | PASS |
| byte_count == 2 * qty | 4 == 4 | Match | PASS |
| Register[0] extracted | Present | 0x0006 (6) | PASS |
| Register[1] extracted | Present | 0x0001 (1) | PASS |

## Retry Mechanism Validation

| Parameter | Config | Actual |
|-----------|--------|--------|
| Max retries | 3 | 0 used |
| Total attempts | 4 max | 1 |
| Retry delay | 50ms | N/A |
| UART recoveries | 2 consec threshold | 0 |
| Success attempt | 1-4 | 1 (first try) |

## Error Classification

| Attempt | Error | Retryable? | Action |
|---------|-------|------------|--------|
| 1 | NONE | — | Success, stop |

## Serial Log

```
========================================
[SYSTEM] WisBlock Gate Test Runner
[SYSTEM] Core: RAK4631 (nRF52840)
[SYSTEM] Gate: 4 - Modbus Robustness Layer
========================================

[GATE4-R4631] === GATE START: gate_rak4631_modbus_robustness_layer v1.0 ===
[GATE4-R4631] STEP 1: UART + RS485 init (Slave=1, Baud=4800, Parity=NONE)
[GATE4-R4631]   RS485 EN=HIGH (pin 34, WB_IO2 — 3V3_S)
[GATE4-R4631]   DE/RE: auto (RAK5802 hardware)
[GATE4-R4631]   UART Init: OK (4800 8N1)
[GATE4-R4631]   Serial1 TX=16 RX=15
[GATE4-R4631] STEP 2: Multi-register read (FC=0x03, Reg=0x0000, Qty=2, MaxRetries=3)
[GATE4-R4631] Attempt 1/4:
[GATE4-R4631]   TX (8 bytes): 01 03 00 00 00 02 C4 0B
[GATE4-R4631]   RX (9 bytes): 01 03 04 00 06 00 01 DB F2
[GATE4-R4631]   Parse: OK (normal, byte_count=4, expected=4)
[GATE4-R4631]   Error: NONE
[GATE4-R4631] Result: SUCCESS on attempt 1 (0 retries, 0 UART recoveries)
[GATE4-R4631] STEP 3: Robustness validation summary
[GATE4-R4631]   Target Slave   : 1
[GATE4-R4631]   Baud Rate      : 4800
[GATE4-R4631]   Frame Config   : 8N1
[GATE4-R4631]   Function Code  : 0x03 (Read Holding Registers)
[GATE4-R4631]   Registers      : 0x0000, Qty=2
[GATE4-R4631]   Attempts       : 1
[GATE4-R4631]   Retries Used   : 0
[GATE4-R4631]   Last Error     : NONE
[GATE4-R4631]   UART Recoveries: 0
[GATE4-R4631]   byte_count     : 4 (expected: 4)
[GATE4-R4631]   Register[0]    : 0x0006 (6)
[GATE4-R4631]   Register[1]    : 0x0001 (1)
[GATE4-R4631]   Response Len   : 9 bytes
[GATE4-R4631]   Response Hex   : 01 03 04 00 06 00 01 DB F2

[GATE4-R4631] PASS
[GATE4-R4631] === GATE COMPLETE ===
```

## RAK4631 vs RAK3312 — Gate 4 Differences

| Aspect | RAK3312 (ESP32-S3) | RAK4631 (nRF52840) |
|--------|--------------------|--------------------|
| Serial1.begin() | 4 args: baud, config, rx, tx | 2 args: baud, config |
| DE/RE control | Manual GPIO (WB_IO1=GPIO 21) | Auto (RAK5802 HW, WB_IO1=NC) |
| Serial1.end() | Works normally | Hangs — must not call |
| UART recovery | end() → delay → begin() | delay → begin() only |
| Serial output | Serial.printf() | Serial.print() / println() |
| Parity | NONE, ODD, EVEN | NONE, EVEN only |
| hardware_profile.h | Required | Not used |

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

## Files — Gate Test

| File | Path |
|------|------|
| gate_config.h | `tests/gates/gate_rak4631_modbus_robustness_layer/gate_config.h` |
| gate_runner.cpp | `tests/gates/gate_rak4631_modbus_robustness_layer/gate_runner.cpp` |
| pass_criteria.md | `tests/gates/gate_rak4631_modbus_robustness_layer/pass_criteria.md` |

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
| main.cpp | `examples/rak4631/modbus_robustness_layer/main.cpp` |
| pin_map.h | `examples/rak4631/modbus_robustness_layer/pin_map.h` |
| wiring.md | `examples/rak4631/modbus_robustness_layer/wiring.md` |
| terminal_test.md | `examples/rak4631/modbus_robustness_layer/terminal_test.md` |
| prompt_test.md | `examples/rak4631/modbus_robustness_layer/prompt_test.md` |
| README.md | `examples/rak4631/modbus_robustness_layer/README.md` |
