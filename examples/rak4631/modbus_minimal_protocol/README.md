# RAK4631 Gate 3 — Modbus Minimal Protocol Validation

**Status:** PASSED
**Tag:** `v3.3-gate3-pass-rak4631`
**Date:** 2026-02-24

## Overview

Validates the Modbus RTU protocol layer by sending a single Read Holding
Register (FC 0x03) request to a known slave device and parsing the response
with a strict 7-step validator. Confirms CRC-16, slave matching, byte_count
consistency, and register value extraction.

## Hardware

| Component | Model | Location |
|-----------|-------|----------|
| WisBlock Core | RAK4631 (nRF52840 + SX1262) | Core slot |
| RS485 Module | RAK5802 (TP8485E, auto DE/RE) | Slot A |
| Base Board | RAK19007 | — |
| Modbus Sensor | RS-FSJT-N01 wind speed | External, 12V powered |

## Pin Map

| Signal | GPIO | nRF52840 Pin | Notes |
|--------|------|-------------|-------|
| Serial1 TX | 16 | P0.16 | To RAK5802 DI |
| Serial1 RX | 15 | P0.15 | From RAK5802 RO |
| RS485 EN | 34 | P1.02 | WB_IO2, 3V3_S power |
| WB_IO1 | 17 | P0.17 | NC on RAK5802 |
| LED Blue | 36 | P1.04 | Active HIGH |

## Files

| File | Purpose |
|------|---------|
| `main.cpp` | Standalone example with inline 7-step parser |
| `pin_map.h` | GPIO pin definitions + discovered device params |
| `wiring.md` | Hardware wiring guide |
| `terminal_test.md` | Step-by-step terminal verification |
| `prompt_test.md` | AI assistant verification prompts |
| `README.md` | This file |

## Quick Start

```bash
# Build
pio run -e rak4631_gate3

# Flash
pio run -e rak4631_gate3 -t upload --upload-port /dev/cu.usbmodemXXXXX

# Monitor (wait 8s, gate completes in ~1s)
python3 -c "
import serial, serial.tools.list_ports, time, sys
time.sleep(8)
ports = [p.device for p in serial.tools.list_ports.comports() if 'usbmodem' in p.device]
ser = serial.Serial(ports[0], 115200, timeout=1)
ser.dtr = True; time.sleep(0.1)
end = time.time() + 15
while time.time() < end:
    data = ser.read(ser.in_waiting or 1)
    if data:
        text = data.decode('utf-8', errors='replace')
        sys.stdout.write(text); sys.stdout.flush()
        if 'GATE COMPLETE' in text: break
ser.close()
"
```

## 7-Step Parser Validation Order

| Step | Check | Result |
|------|-------|--------|
| 1 | Frame length >= 5 | 7 bytes ✅ |
| 2 | CRC-16 match | 0x87B9 ✅ |
| 3 | Slave address match | 1 == 1 ✅ |
| 4 | Exception detection | NO (normal) ✅ |
| 5 | Function code match | 0x03 ✅ |
| 6 | byte_count consistency | 2 == (7-5) ✅ |
| 7 | Value extraction | 0x0004 ✅ |

## Validated Protocol

| Parameter | Value |
|-----------|-------|
| Slave Address | 1 |
| Baud Rate | 4800 |
| Frame Config | 8N1 |
| Function Code | 0x03 (Read Holding Registers) |
| Register | 0x0000 |
| byte_count | 2 |
| Register Value | 0x0004 (wind speed × 10 = 0.4 m/s) |
| CRC | Valid |
| Response Type | Normal |
