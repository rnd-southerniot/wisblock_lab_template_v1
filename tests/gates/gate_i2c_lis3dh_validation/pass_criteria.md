# Gate 1 — I2C LIS3DH Validation

**Gate ID:** 1
**Gate Name:** `gate_i2c_lis3dh_validation`
**Version:** 1.0
**Date:** 2026-02-23
**Hardware Profile:** v1.0

---

## Purpose

Validate that the RAK1904 (LIS3DH) accelerometer on Slot B is physically present, correctly wired, and reliably communicable over the shared I2C bus before any driver implementation begins.

---

## PASS Criteria

All six criteria must be met for this gate to PASS:

| # | Criterion | Validation Method | Expected Result |
|---|-----------|-------------------|-----------------|
| 1 | I2C initializes successfully | `Wire.begin()` returns, bus responds to transmission | No I2C init error |
| 2 | Device detected at 0x18 | `Wire.endTransmission()` returns 0 (ACK) | ACK from 0x18 |
| 3 | WHO_AM_I matches LIS3DH | Read register `0x0F` | Returns `0x33` |
| 4 | 100 consecutive reads without error | Read `OUT_X_L` (0x28) 100 times at 10ms intervals | 100/100 success |
| 5 | No bus timeout | All transactions complete within 50ms each | Zero timeouts |
| 6 | No system crash | Gate runner reaches `GATE COMPLETE` log line | Log line present |

---

## FAIL Codes

| Code | Constant | Meaning |
|------|----------|---------|
| 0 | `GATE_PASS` | All criteria met |
| 1 | `GATE_FAIL_I2C_INIT` | I2C bus failed to initialize |
| 2 | `GATE_FAIL_DEVICE_NOT_FOUND` | No ACK at address 0x18 |
| 3 | `GATE_FAIL_WHO_AM_I` | WHO_AM_I register mismatch |
| 4 | `GATE_FAIL_READ_ERROR` | Read failure during consecutive reads |
| 5 | `GATE_FAIL_BUS_TIMEOUT` | I2C transaction timeout |
| 6 | `GATE_FAIL_SYSTEM_CRASH` | Runner did not reach GATE COMPLETE |

---

## Expected Serial Log Format

### PASS Example

```
========================================
[GATE1] [INFO] GATE START: gate_i2c_lis3dh_validation v1.0
========================================
[GATE1] [STEP] STEP 1: I2C bus init (SDA=4, SCL=5, 100000 Hz)
[GATE1] [PASS] STEP 1: I2C bus initialized OK
[GATE1] [STEP] STEP 2: Scanning for device at 0x18
[GATE1] [PASS] STEP 2: Device ACK at 0x18
[GATE1] [STEP] STEP 3: Reading WHO_AM_I (reg=0x0F, expect=0x33)
[GATE1] [PASS] STEP 3: WHO_AM_I = 0x33 (match)
[GATE1] [STEP] STEP 4: Starting 100 consecutive reads (interval=10 ms)
[GATE1] [INFO] STEP 4: Progress 25/100 OK
[GATE1] [INFO] STEP 4: Progress 50/100 OK
[GATE1] [INFO] STEP 4: Progress 75/100 OK
[GATE1] [INFO] STEP 4: Progress 100/100 OK
[GATE1] [PASS] STEP 4: 100/100 reads OK, 0 failures
========================================
[GATE1] [PASS] ALL STEPS PASSED (1250 ms)
[GATE1] [INFO] RESULT: reads_ok=100, reads_fail=0
[GATE1] [INFO] GATE COMPLETE
========================================
```

### FAIL Example (Device Not Found)

```
========================================
[GATE1] [INFO] GATE START: gate_i2c_lis3dh_validation v1.0
========================================
[GATE1] [STEP] STEP 1: I2C bus init (SDA=4, SCL=5, 100000 Hz)
[GATE1] [PASS] STEP 1: I2C bus initialized OK
[GATE1] [STEP] STEP 2: Scanning for device at 0x18
[GATE1] [FAIL] STEP 2: No device at 0x18 (error=2)
========================================
[GATE1] [FAIL] GATE FAILED (code=2, 15 ms)
[GATE1] [INFO] RESULT: reads_ok=0, reads_fail=0
[GATE1] [INFO] GATE COMPLETE
========================================
```

---

## Log Tag Reference

| Tag | Meaning |
|-----|---------|
| `[GATE1]` | Gate identifier prefix |
| `[STEP]` | Step is starting |
| `[PASS]` | Step or gate passed |
| `[FAIL]` | Step or gate failed |
| `[INFO]` | Informational (progress, results) |

---

## Parameters

| Parameter | Value | Source |
|-----------|-------|--------|
| I2C Address | `0x18` | `hardware_profile.h` → `HW_I2C_RAK1904_ADDR` |
| WHO_AM_I Register | `0x0F` | LIS3DH datasheet |
| WHO_AM_I Expected | `0x33` | LIS3DH datasheet |
| Consecutive Reads | 100 | Gate requirement |
| Read Interval | 10 ms | `gate_config.h` |
| Per-transaction Timeout | 50 ms | `gate_config.h` |
| Overall Gate Timeout | 10,000 ms | `gate_config.h` |

---

## Scope Boundaries

- This gate validates **hardware presence and bus integrity only**.
- No full accelerometer driver is implemented.
- No LoRaWAN interaction.
- No RS485 interaction.
- No power management logic.
