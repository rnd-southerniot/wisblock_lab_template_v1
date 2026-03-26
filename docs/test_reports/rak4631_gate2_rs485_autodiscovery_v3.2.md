# Test Report — RAK4631 Gate 2: RS485 Modbus Autodiscovery

- **Status:** PASSED
- **Date:** 2026-02-24
- **Gate ID:** 2
- **Gate Name:** gate_rak4631_rs485_autodiscovery_validation
- **Version:** 1.0
- **Hardware Profile:** v3.2 (RAK4631)
- **Build Environment:** `rak4631_gate2`
- **Tag:** `v3.2-gate2-pass-rak4631`

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
| RAM | 3.6% (8912 / 248832 bytes) |
| Flash | 7.3% (59216 / 815104 bytes) |
| Upload | nrfutil DFU via USB CDC |

## Pass Criteria Results

| # | Criterion | Result | Evidence |
|---|-----------|--------|----------|
| 1 | Compile | PASS | `rak4631_gate2 SUCCESS 00:00:01.872` |
| 2 | Serial | PASS | Full banner captured via USB CDC |
| 3 | RS485 Enable | PASS | `EN pin: 34 (WB_IO2 — 3V3_S power)` |
| 4 | Scan Start | PASS | `Trying baud=4800 parity=NONE...` |
| 5 | Discovery | PASS | `>>> FOUND at probe #1` |
| 6 | Frame Valid | PASS | RX 7 bytes, CRC-16 valid |
| 7 | Slave Match | PASS | Request slave=1, response slave=1 |
| 8 | Parameters | PASS | Slave=1, Baud=4800, Parity=NONE printed |
| 9 | LED | PASS | Blue LED blinked once (visual) |
| 10 | Complete | PASS | `[GATE2-R4631] PASS` + `=== GATE COMPLETE ===` |

## Discovery Result

| Parameter | Value |
|-----------|-------|
| Slave Address | 1 |
| Baud Rate | 4800 |
| Parity | NONE (8N1) |
| Response Type | Normal (FC 0x03) |
| Probes Required | 1 of 800 max |
| Scan Time | ~1.3 seconds |

## Protocol Trace

| Direction | Frame (hex) | Decoded |
|-----------|-------------|---------|
| TX | `01 03 00 00 00 01 84 0A` | Slave=1, FC=0x03, Reg=0x0000, Qty=1, CRC=0x0A84 |
| RX | `01 03 02 00 07 F9 86` | Slave=1, FC=0x03, Bytes=2, Value=0x0007, CRC=0x86F9 |

## Serial Log

```
========================================
[SYSTEM] WisBlock Gate Test Runner
[SYSTEM] Core: RAK4631 (nRF52840)
[SYSTEM] Gate: 2 - RS485 Autodiscovery
========================================

[GATE2-R4631] === GATE START: gate_rak4631_rs485_autodiscovery_validation v1.0 ===
[GATE2-R4631] STEP 1: Enable RS485 (RAK5802)
[GATE2-R4631]   EN pin: 34 (WB_IO2 — 3V3_S power)
[GATE2-R4631]   Serial1 TX=16 RX=15
[GATE2-R4631]   DE/RE: auto (RAK5802 hardware auto-direction)
[GATE2-R4631] STEP 2: Autodiscovery scan
[GATE2-R4631]   Bauds: 4800, 9600, 19200, 115200
[GATE2-R4631]   Parity: NONE, EVEN (2 modes — nRF52 has no Odd parity)
[GATE2-R4631]   Slaves: 1..100
[GATE2-R4631]   Max probes: 800
[GATE2-R4631]   Trying baud=4800 parity=NONE...
[GATE2-R4631]   >>> FOUND at probe #1

[GATE2-R4631] STEP 3: Results
[GATE2-R4631]   Probes sent: 1
[GATE2-R4631]   Slave : 1
[GATE2-R4631]   Baud  : 4800
[GATE2-R4631]   Parity: NONE
[GATE2-R4631]   Type  : Normal response
[GATE2-R4631]   TX: 01 03 00 00 00 01 84 0A
[GATE2-R4631]   RX: 01 03 02 00 07 F9 86
[GATE2-R4631]   CRC: 0x86F9 (valid)

[GATE2-R4631] PASS
[GATE2-R4631] === GATE COMPLETE ===
```

## RS485 Architecture Notes

### RAK5802 Auto-Direction (Critical Finding)

The RAK5802 module uses a TP8485E RS-485 transceiver with an **on-board
auto-direction circuit** (RC delay + TCON logic tied to the UART TX line).
This circuit automatically drives DE HIGH when the TX line is active and
switches to RX mode when TX goes idle.

**WB_IO1 (pin 17) is NOT connected to the TP8485E's DE/RE pins on the RAK5802.**
It is marked NC (not connected) on the schematic. The variant.h labels it as
"EPD DC" (E-Paper Display Data/Command), which is its purpose with other modules.

**Implication:** Firmware must NOT manually toggle pin 17 for DE/RE control.
Doing so is harmless on boards where R63 is depopulated (NC), but could interfere
with the auto-direction timing on boards where R63 is populated.

### HAL Requirements (nRF52840 + RAK5802)

1. **Power:** `pinMode(34, OUTPUT); digitalWrite(34, HIGH);` — enables 3V3_S rail
2. **Init:** `Serial1.begin(baud, config);` — no pin args on nRF52
3. **Write:** `Serial1.write(data, len); Serial1.flush();` — flush required
4. **Read:** Poll `Serial1.available()` with timeout
5. **No `Serial1.end()`:** Hangs on nRF52 if not previously started
6. **No manual DE/RE:** Auto-direction handles everything

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

## Issues Encountered & Resolved

1. **Manual DE/RE toggling on NC pin (Gate 2 initial attempt)**
   - Symptom: 800 probes sent, zero responses received
   - Cause: Code toggled WB_IO1 (pin 17) as DE/RE, but pin 17 is NC on RAK5802.
     The `Serial1.flush()` was also replaced with a busy-wait loop.
   - Fix: Removed all manual DE/RE GPIO toggling. Restored `Serial1.flush()`.
     RAK5802 auto-direction circuit handles DE/RE from TX line.

2. **`Serial1.flush()` replaced with busy-wait (Gate 2 initial attempt)**
   - Symptom: TX data potentially not fully transmitted before auto-direction switchback
   - Cause: Concern about `Serial1.flush()` hanging on nRF52 (it doesn't)
   - Fix: Restored `Serial1.flush()` — it works correctly and blocks until TX DMA completes

3. **`Serial1.end()` hangs on nRF52 (Gate 2 early debugging)**
   - Symptom: Firmware hung during baud rate changes
   - Cause: `Serial1.end()` blocks indefinitely if Serial1 was never started
   - Fix: Removed `Serial1.end()` call — just call `Serial1.begin()` again

4. **New hardware required (Gate 2 retry)**
   - Symptom: Scan consistently failed on original hardware even after firmware fix
   - Cause: Original RAK4631 or RAK5802 module may have been defective
   - Fix: Replaced both RAK4631 and RAK5802 with new modules — immediately passed

## Files — Gate Test

| File | Path |
|------|------|
| gate_config.h | `tests/gates/gate_rak4631_rs485_autodiscovery_validation/gate_config.h` |
| gate_runner.cpp | `tests/gates/gate_rak4631_rs485_autodiscovery_validation/gate_runner.cpp` |
| hal_uart.h | `tests/gates/gate_rak4631_rs485_autodiscovery_validation/hal_uart.h` |
| hal_uart.cpp | `tests/gates/gate_rak4631_rs485_autodiscovery_validation/hal_uart.cpp` |
| pass_criteria.md | `tests/gates/gate_rak4631_rs485_autodiscovery_validation/pass_criteria.md` |

## Files — Diagnostic Gate

| File | Path |
|------|------|
| gate_runner.cpp | `tests/gates/gate_rak4631_rs485_raw_monitor/gate_runner.cpp` |
| pass_criteria.md | `tests/gates/gate_rak4631_rs485_raw_monitor/pass_criteria.md` |

## Files — Example

| File | Path |
|------|------|
| main.cpp | `examples/rak4631/rs485_autodiscovery/main.cpp` |
| pin_map.h | `examples/rak4631/rs485_autodiscovery/pin_map.h` |
| wiring.md | `examples/rak4631/rs485_autodiscovery/wiring.md` |
| terminal_test.md | `examples/rak4631/rs485_autodiscovery/terminal_test.md` |
| prompt_test.md | `examples/rak4631/rs485_autodiscovery/prompt_test.md` |
| README.md | `examples/rak4631/rs485_autodiscovery/README.md` |
