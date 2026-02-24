# RAK4631 Gate 7 — Production Loop Soak

**Status:** PASSED
**Tag:** `v3.7-gate7-pass-rak4631`
**Date:** 2026-02-25

## Overview

Validates the production runtime loop under real conditions for a 5-minute
soak duration. The scheduler runs naturally via `tick()` — no `fireNow()`
intervention. The gate harness observes stability, uplink consistency, and
bounded cycle timing.

Transitions from forced-cycle validation (Gate 6) to the actual production
execution pattern used in deployment firmware.

## Hardware

| Component | Model | Location |
|-----------|-------|----------|
| WisBlock Core | RAK4631 (nRF52840 + SX1262) | Core slot |
| RS485 Module | RAK5802 (TP8485E, auto DE/RE) | Slot A |
| Base Board | RAK19007 | — |
| Modbus Sensor | RS-FSJT-N01 wind speed | External, 12V powered |
| LoRa Gateway | ChirpStack v4 | Within LoRa range |

## Pin Map

| Signal | GPIO | nRF52840 Pin | Notes |
|--------|------|-------------|-------|
| Serial1 TX | 16 | P0.16 | To RAK5802 DI |
| Serial1 RX | 15 | P0.15 | From RAK5802 RO |
| RS485 EN | 34 | P1.02 | WB_IO2, 3V3_S power |
| WB_IO1 | 17 | P0.17 | NC on RAK5802 |
| LED Blue | 36 | P1.04 | Active HIGH |
| SX1262 | — | On-board | Handled by lora_rak4630_init() |

## Files

| File | Purpose |
|------|---------|
| `main.cpp` | Standalone example: init + 5-minute soak loop + summary |
| `pin_map.h` | GPIO pin definitions + LoRaWAN/Modbus/Soak config |
| `wiring.md` | Hardware wiring guide + signal path diagram |
| `terminal_test.md` | Step-by-step terminal verification |
| `prompt_test.md` | AI assistant verification prompts |
| `README.md` | This file |

## Quick Start

```bash
# Build
pio run -e rak4631_gate7

# Flash
pio run -e rak4631_gate7 -t upload --upload-port /dev/cu.usbmodemXXXXX

# Monitor (~6 minutes: 30s join + 300s soak + summary)
python3 -c "
import serial, time, sys
time.sleep(8)
ser = serial.Serial('/dev/cu.usbmodem21101', 115200, timeout=1)
time.sleep(0.2)
end = time.time() + 400
buf = ''
while time.time() < end:
    data = ser.read(ser.in_waiting or 1)
    if data:
        text = data.decode('utf-8', errors='replace')
        buf += text
        sys.stdout.write(text); sys.stdout.flush()
        if 'GATE COMPLETE' in buf: break
ser.close()
"
```

## Production Soak Architecture

```
                    SystemManager
                    ┌───────────────────────────────────┐
                    │                                     │
                    │  TaskScheduler (cooperative)         │
                    │    └── sensor_uplink (30s natural)   │
                    │          │                           │
                    │          ├── ModbusPeripheral        │
                    │          │   └── read(SensorFrame)   │
                    │          │                           │
                    │          ├── encode (big-endian)     │
                    │          │                           │
                    │          └── LoRaTransport           │
                    │              └── send(payload)       │
                    │                                     │
                    │  Gate Harness (observe only)         │
                    │    └── track: OK/FAIL, timing,       │
                    │         consec fails, transport      │
                    │                                     │
                    └───────────────────────────────────┘
```

## LoRaWAN Configuration

| Parameter | Value |
|-----------|-------|
| Region | AS923-1 |
| Activation | OTAA |
| Class | A |
| ADR | OFF |
| Data Rate | DR2 (SF10) |
| TX Power | TX_POWER_0 |
| Join Trials | 3 (LoRaMAC internal) |
| Join Timeout | 30s |
| Uplink Port | 10 |
| Confirmed | No (unconfirmed) |

## Modbus Configuration

| Parameter | Value |
|-----------|-------|
| Slave Address | 1 |
| Baud Rate | 4800 |
| Frame Config | 8N1 |
| Function Code | 0x03 (Read Holding Registers) |
| Start Register | 0x0000 |
| Quantity | 2 |
| Retries | 2 (3 total attempts) |

## Soak Configuration

| Parameter | Value |
|-----------|-------|
| Soak Duration | 300000 ms (5 min) |
| Poll Interval | 30000 ms (30s) |
| Expected Cycles | ~10 |
| Min Uplinks | 3 (conservative) |
| Max Consecutive Fails | 2 |
| Max Cycle Duration | 1500 ms |

## Payload Format

4 bytes, big-endian:

| Byte | Content |
|------|---------|
| 0 | Register[0] MSB (wind speed high) |
| 1 | Register[0] LSB (wind speed low) |
| 2 | Register[1] MSB (wind direction high) |
| 3 | Register[1] LSB (wind direction low) |

## Build Flags

```
-DCORE_RAK4631 -DRAK4630 -D_VARIANT_RAK4630_
```

## Validated Soak Results

| Metric | Value |
|--------|-------|
| Soak Duration | 300,007 ms (300 s) |
| Total Cycles | 9 |
| Modbus OK / FAIL | 9 / 0 |
| Uplink OK / FAIL | 9 / 0 |
| Max Consecutive Fail | modbus=0, uplink=0 |
| Max Cycle Duration | 82 ms |
| Transport | connected |
| Join Attempts | 1 |
