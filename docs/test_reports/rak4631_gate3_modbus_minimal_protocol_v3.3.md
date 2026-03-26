# Test Report — RAK4631 Gate 3: Modbus Minimal Protocol Validation

- **Status:** PASSED
- **Date:** 2026-02-24
- **Gate ID:** 3
- **Gate Name:** gate_rak4631_modbus_minimal_protocol
- **Version:** 1.0
- **Hardware Profile:** v3.3 (RAK4631)
- **Build Environment:** `rak4631_gate3`
- **Tag:** `v3.3-gate3-pass-rak4631`

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
| RAM | 3.6% (8960 / 248832 bytes) |
| Flash | 7.4% (60448 / 815104 bytes) |
| Upload | nrfutil DFU via USB CDC |

## Pass Criteria Results

| # | Criterion | Result | Evidence |
|---|-----------|--------|----------|
| 1 | Compile | PASS | `rak4631_gate3 SUCCESS 00:00:05.390` |
| 2 | UART Init | PASS | `UART Init: OK (4800 8N1)` |
| 3 | TX Frame | PASS | `01 03 00 00 00 01 84 0A` (8 bytes, CRC valid) |
| 4 | RX Response | PASS | `01 03 02 00 04 B9 87` (7 bytes, CRC valid) |
| 5 | Slave Match | PASS | resp[0] == 1 |
| 6 | byte_count | PASS | declared=2, actual=(7-5)=2 — consistent |
| 7 | Structured Output | PASS | `Slave=1 Value=0x0004` |
| 8 | Complete | PASS | `[GATE3-R4631] PASS` + `=== GATE COMPLETE ===` |

## 7-Step Parser Validation

| Step | Check | Input | Result |
|------|-------|-------|--------|
| 1 | Frame length >= 5 | len=7 | PASS |
| 2 | CRC-16 match | calc=0x87B9, rx=0x87B9 | PASS |
| 3 | Slave address match | resp[0]=1, expected=1 | PASS |
| 4 | Exception detection | resp[1]=0x03 (no 0x80 bit) | Normal |
| 5 | Function code match | resp[1]=0x03, expected=0x03 | PASS |
| 6 | byte_count consistency | resp[2]=2, (7-5)=2 | PASS |
| 7 | Value extraction | (0x00 << 8) \| 0x04 = 0x0004 | PASS |

## Protocol Trace

| Direction | Frame (hex) | Decoded |
|-----------|-------------|---------|
| TX | `01 03 00 00 00 01 84 0A` | Slave=1, FC=0x03, Reg=0x0000, Qty=1, CRC=0x0A84 |
| RX | `01 03 02 00 04 B9 87` | Slave=1, FC=0x03, Bytes=2, Value=0x0004, CRC=0x87B9 |

## Serial Log

```
========================================
[SYSTEM] WisBlock Gate Test Runner
[SYSTEM] Core: RAK4631 (nRF52840)
[SYSTEM] Gate: 3 - Modbus Minimal Protocol
========================================

[GATE3-R4631] === GATE START: gate_rak4631_modbus_minimal_protocol v1.0 ===
[GATE3-R4631] STEP 1: UART + RS485 init (Slave=1, Baud=4800, Parity=NONE)
[GATE3-R4631]   RS485 EN=HIGH (pin 34, WB_IO2 — 3V3_S)
[GATE3-R4631]   DE/RE: auto (RAK5802 hardware)
[GATE3-R4631]   UART Init: OK (4800 8N1)
[GATE3-R4631]   Serial1 TX=16 RX=15
[GATE3-R4631] STEP 2: Modbus Read Holding Register (FC=0x03, Reg=0x0000, Qty=1)
[GATE3-R4631]   TX (8 bytes): 01 03 00 00 00 01 84 0A
[GATE3-R4631]   RX (7 bytes): 01 03 02 00 04 B9 87
[GATE3-R4631]   Slave=1 Value=0x0004
[GATE3-R4631] STEP 3: Protocol validation summary
[GATE3-R4631]   Target Slave  : 1
[GATE3-R4631]   Baud Rate     : 4800
[GATE3-R4631]   Frame Config  : 8N1
[GATE3-R4631]   Function Code : 0x03 (Read Holding Registers)
[GATE3-R4631]   Register      : 0x0000
[GATE3-R4631]   Protocol      : Modbus RTU
[GATE3-R4631]   Response Type : Normal
[GATE3-R4631]   byte_count    : 2
[GATE3-R4631]   Register Value: 0x0004 (4)
[GATE3-R4631]   Response Len  : 7 bytes
[GATE3-R4631]   Response Hex  : 01 03 02 00 04 B9 87

[GATE3-R4631] PASS
[GATE3-R4631] === GATE COMPLETE ===
```

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

## Files — Gate Test

| File | Path |
|------|------|
| gate_config.h | `tests/gates/gate_rak4631_modbus_minimal_protocol/gate_config.h` |
| modbus_frame.h | `tests/gates/gate_rak4631_modbus_minimal_protocol/modbus_frame.h` |
| modbus_frame.cpp | `tests/gates/gate_rak4631_modbus_minimal_protocol/modbus_frame.cpp` |
| hal_uart.h | `tests/gates/gate_rak4631_modbus_minimal_protocol/hal_uart.h` |
| hal_uart.cpp | `tests/gates/gate_rak4631_modbus_minimal_protocol/hal_uart.cpp` |
| gate_runner.cpp | `tests/gates/gate_rak4631_modbus_minimal_protocol/gate_runner.cpp` |
| pass_criteria.md | `tests/gates/gate_rak4631_modbus_minimal_protocol/pass_criteria.md` |

## Files — Example

| File | Path |
|------|------|
| main.cpp | `examples/rak4631/modbus_minimal_protocol/main.cpp` |
| pin_map.h | `examples/rak4631/modbus_minimal_protocol/pin_map.h` |
| wiring.md | `examples/rak4631/modbus_minimal_protocol/wiring.md` |
| terminal_test.md | `examples/rak4631/modbus_minimal_protocol/terminal_test.md` |
| prompt_test.md | `examples/rak4631/modbus_minimal_protocol/prompt_test.md` |
| README.md | `examples/rak4631/modbus_minimal_protocol/README.md` |
