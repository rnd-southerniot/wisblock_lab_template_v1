# RAK4631 Gate 1 — I2C LIS3DH Validation

**Status:** PASSED
**Tag:** `v3.1-gate1-pass-rak4631`
**Date:** 2026-02-24

## Overview

Validates the LIS3DH 3-axis accelerometer on the RAK4631 WisBlock Core via I2C.
Confirms device detection, WHO_AM_I identity, sensor configuration, and 100 consecutive
error-free accelerometer reads.

## Hardware

| Component | Model | Location |
|-----------|-------|----------|
| WisBlock Core | RAK4631 (nRF52840 + SX1262) | Core slot |
| Accelerometer | RAK1904 (LIS3DH) | Sensor slot (I2C) |
| Base Board | RAK19007 | — |

## Pin Map

| Signal | GPIO | nRF52840 Pin | Notes |
|--------|------|-------------|-------|
| I2C SDA | 13 | P0.13 | Wire bus |
| I2C SCL | 14 | P0.14 | Wire bus |
| LED Blue | 36 | P1.04 | Active HIGH |
| WB_IO1 | 17 | P0.17 | — |
| WB_IO2 | 34 | P1.02 | — |

## Files

| File | Purpose |
|------|---------|
| `main.cpp` | Standalone example (self-contained) |
| `pin_map.h` | GPIO pin definitions |
| `terminal_test.md` | Step-by-step terminal verification |
| `prompt_test.md` | AI assistant verification prompts |
| `README.md` | This file |

## Quick Start

```bash
# Build
pio run -e rak4631_gate1

# Flash (specify port to avoid ststm32 bug)
pio run -e rak4631_gate1 -t upload --upload-port /dev/cu.usbmodemXXXXX

# Monitor (wait 8s for reboot, then open port)
python3 -c "
import serial, serial.tools.list_ports, time, sys
time.sleep(8)
ports = [p.device for p in serial.tools.list_ports.comports() if 'usbmodem' in p.device]
ser = serial.Serial(ports[0], 115200, timeout=1)
ser.dtr = True; time.sleep(0.1)
end = time.time() + 20
while time.time() < end:
    data = ser.read(ser.in_waiting or 1)
    if data: sys.stdout.write(data.decode('utf-8', errors='replace')); sys.stdout.flush()
ser.close()
"
```

## Expected Output

```
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

## Validated Parameters

| Parameter | Value |
|-----------|-------|
| I2C Address | 0x18 |
| WHO_AM_I | 0x33 (LIS3DH) |
| ODR | 50 Hz |
| Full Scale | ±2g |
| Resolution | High (12-bit) |
| Read Success Rate | 100/100 (0% failure) |
| Z-axis @ 1g | ~16000 counts |
