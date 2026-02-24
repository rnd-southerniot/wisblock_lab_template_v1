# Test Report — RAK4631 Gate 1: I2C LIS3DH Validation

- **Status:** PASSED
- **Date:** 2026-02-24
- **Gate ID:** 1
- **Gate Name:** gate_rak4631_i2c_lis3dh_validation
- **Version:** 1.0
- **Hardware Profile:** v3.1 (RAK4631)
- **Build Environment:** `rak4631_gate1`
- **Tag:** `v3.1-gate1-pass-rak4631`

---

## Hardware Under Test

| Component | Model | Details |
|-----------|-------|---------|
| WisBlock Core | RAK4631 | nRF52840 @ 64 MHz + SX1262 |
| Accelerometer | RAK1904 | LIS3DH, I2C addr 0x18 |
| Base Board | RAK19007 | USB-C, WisBlock standard |
| DevEUI | 88:82:24:44:AE:ED:1E:B2 | From FICR registers |

## Build Configuration

| Parameter | Value |
|-----------|-------|
| Platform | nordicnrf52 (10.10.0) |
| Board | wiscore_rak4631 |
| Framework | Arduino (Adafruit nRF52 BSP 1.7.0) |
| Toolchain | GCC ARM 7.2.1 |
| TinyUSB | 3.6.0 (framework built-in, symlinked) |
| RAM | 3.7% (9120 / 248832 bytes) |
| Flash | 7.1% (57696 / 815104 bytes) |
| Upload | nrfutil DFU via USB CDC |

## Pass Criteria Results

| # | Criterion | Result | Evidence |
|---|-----------|--------|----------|
| 1 | Compile | PASS | `rak4631_gate1 SUCCESS 00:00:05.264` |
| 2 | Serial | PASS | Full banner captured via USB CDC |
| 3 | Detect | PASS | `ACK — device present` at 0x18 |
| 4 | WHO_AM_I | PASS | `0x33 (expected 0x33)` — LIS3DH confirmed |
| 5 | Configure | PASS | CTRL_REG1=0x47, CTRL_REG4=0x08 written |
| 6 | Reads | PASS | `100/100 passed, 0 failed` |
| 7 | Data | PASS | Z ~16000 counts (≈1g at ±2g/12-bit) |
| 8 | LED | PASS | Blue LED blinked once (visual) |
| 9 | Complete | PASS | `[GATE1-R4631] PASS` + `=== GATE COMPLETE ===` |

## Serial Log

```
========================================
[SYSTEM] WisBlock Gate Test Runner
[SYSTEM] Core: RAK4631 (nRF52840)
[SYSTEM] Gate: 1 - I2C LIS3DH Validation
========================================

[GATE1-R4631] === GATE START: gate_rak4631_i2c_lis3dh_validation v1.0 ===
[GATE1-R4631] Wire SDA=13 SCL=14
[GATE1-R4631] STEP 1: Detect LIS3DH at 0x18
[GATE1-R4631]   ACK — device present
[GATE1-R4631] STEP 2: Read WHO_AM_I (0x0F)
[GATE1-R4631]   WHO_AM_I = 0x33 (expected 0x33)
[GATE1-R4631]   LIS3DH confirmed
[GATE1-R4631] STEP 3: Configure sensor
[GATE1-R4631]   CTRL_REG1=0x47 (50Hz, XYZ on)
[GATE1-R4631]   CTRL_REG4=0x08 (+/-2g, hi-res)
[GATE1-R4631] STEP 4: 100 consecutive accel reads
[GATE1-R4631]   [0] X=1808 Y=-1280 Z=15696
[GATE1-R4631]   [25] X=1792 Y=-1312 Z=15888
[GATE1-R4631]   [50] X=1680 Y=-1312 Z=16048
[GATE1-R4631]   [75] X=1520 Y=-1440 Z=16000

[GATE1-R4631] Reads: 100/100 passed, 0 failed
[GATE1-R4631] X range: [1520 .. 1984]
[GATE1-R4631] Y range: [-1520 .. -1184]
[GATE1-R4631] Z range: [15696 .. 16304]

[GATE1-R4631] PASS
[GATE1-R4631] === GATE COMPLETE ===
```

## Accelerometer Data Analysis

| Axis | Min | Max | Expected (board flat) |
|------|-----|-----|----------------------|
| X | 1520 | 1984 | ~0 (slight tilt) |
| Y | -1520 | -1184 | ~0 (slight tilt) |
| Z | 15696 | 16304 | ~16384 (1g) |

Z-axis reading of ~16000 counts is consistent with 1g gravity at ±2g full scale
with 12-bit left-justified data in 16-bit register (1g = 16384 LSB nominal).
Small X/Y offsets indicate minor board tilt, within normal range.

## Gate Progression (RAK4631)

| Gate | Name | Status | Tag |
|------|------|--------|-----|
| 0 | Toolchain Validation | PASSED | — |
| 0.5 | DevEUI Read | PASSED | — |
| 1A | I2C Scan | PASSED | — |
| 1B | I2C Identify 0x68/0x69 | PASSED | — |
| 1 | I2C LIS3DH Validation | PASSED | `v3.1-gate1-pass-rak4631` |

## Issues Encountered & Resolved

1. **TinyUSB version mismatch (Gate 0)**
   - Symptom: USB CDC port never appeared after flash
   - Cause: `lib_deps` pulled TinyUSB v3.7.4, conflicting with framework's built-in v3.6.0
   - Fix: Symlink to framework's built-in TinyUSB library

2. **Serial output timing (Gate 0.5)**
   - Symptom: Serial output missed — printed before host opened port
   - Cause: `while (!Serial)` with timeout exited before host connected; data lost on TinyUSB when no DTR
   - Fix: Changed to `while (!Serial) { ; }` (wait indefinitely for DTR)

3. **Green LED not working (Gate 0)**
   - Symptom: Green LED (pin 35 / P1.03) does not blink
   - Cause: LED not populated on this board revision
   - Fix: Switched to blue LED (pin 36 / P1.04) for all gates

4. **PlatformIO upload port auto-detect crash**
   - Symptom: `AssertionError: Missed target configuration for wiscore_rak4631`
   - Cause: ststm32 platform bug in board enumeration during port auto-detect
   - Fix: Use `--upload-port /dev/cu.usbmodemXXXXX` explicitly

## Files — Gate Test

| File | Path |
|------|------|
| gate_config.h | `tests/gates/gate_rak4631_i2c_lis3dh_validation/gate_config.h` |
| gate_runner.cpp | `tests/gates/gate_rak4631_i2c_lis3dh_validation/gate_runner.cpp` |
| pass_criteria.md | `tests/gates/gate_rak4631_i2c_lis3dh_validation/pass_criteria.md` |

## Files — Example

| File | Path |
|------|------|
| main.cpp | `examples/rak4631/i2c_lis3dh_validation/main.cpp` |
| pin_map.h | `examples/rak4631/i2c_lis3dh_validation/pin_map.h` |
| terminal_test.md | `examples/rak4631/i2c_lis3dh_validation/terminal_test.md` |
| prompt_test.md | `examples/rak4631/i2c_lis3dh_validation/prompt_test.md` |
| README.md | `examples/rak4631/i2c_lis3dh_validation/README.md` |
