# RAK4631 Gate 6 — Runtime Scheduler Integration

**Status:** PASSED
**Tag:** `v3.6-gate6-pass-rak4631`
**Date:** 2026-02-25

## Overview

Validates the production runtime architecture on RAK4631 (nRF52840):
SystemManager orchestrating a cooperative TaskScheduler, LoRaTransport,
and ModbusPeripheral for 3 forced sensor-uplink cycles.

Transitions from one-shot gate testing (Gate 5) to the cyclic runtime pattern
used in production firmware.

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
| `main.cpp` | Standalone example: scheduler + join + modbus + 3 uplinks |
| `pin_map.h` | GPIO pin definitions + LoRaWAN/Modbus/Scheduler config |
| `wiring.md` | Hardware wiring guide + signal path diagram |
| `terminal_test.md` | Step-by-step terminal verification |
| `prompt_test.md` | AI assistant verification prompts |
| `README.md` | This file |

## Quick Start

```bash
# Build
pio run -e rak4631_gate6

# Flash
pio run -e rak4631_gate6 -t upload --upload-port /dev/cu.usbmodemXXXXX

# Monitor (wait 8s, gate completes in ~45s including join + 3 cycles)
python3 -c "
import serial, time, sys
time.sleep(8)
ser = serial.Serial('/dev/cu.usbmodem21101', 115200, timeout=1)
time.sleep(0.2)
end = time.time() + 120
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

## Runtime Architecture

```
                    SystemManager
                    ┌─────────────────────────────────┐
                    │                                   │
                    │  TaskScheduler (cooperative)       │
                    │    └── sensor_uplink (30s)         │
                    │          │                         │
                    │          ├── ModbusPeripheral      │
                    │          │   └── read(SensorFrame) │
                    │          │                         │
                    │          ├── encode (big-endian)   │
                    │          │                         │
                    │          └── LoRaTransport         │
                    │              └── send(payload)     │
                    │                                   │
                    └─────────────────────────────────┘
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

## Scheduler Configuration

| Parameter | Value |
|-----------|-------|
| Poll Interval | 30000ms (30s) |
| Task Name | `sensor_uplink` |
| Test Cycles | 3 (forced via fireNow()) |
| Inter-cycle Delay | 3s (LoRaWAN duty cycle) |

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

The `-DRAK4630` and `-D_VARIANT_RAK4630_` flags are **critical** — they enable `SPI_LORA` instantiation in `RAK4630_MOD.cpp` of the SX126x-Arduino library.

`-DCORE_RAK4631` activates `#ifdef` guards in the shared runtime layer:
- `lora_transport.cpp` → uses `lora_rak4630_init()` instead of manual hw_config
- `modbus_peripheral.cpp` → uses RAK4631 HAL (2-arg init, 1-arg rs485), skips deinit()

## Validated Results

| Parameter | Value |
|-----------|-------|
| Cycle 1 Reg[0] | 0x0004 (wind speed 4) |
| Cycle 1 Reg[1] | 0x0001 (wind direction 1) |
| Cycle 2 Reg[0] | 0x0001 |
| Cycle 2 Reg[1] | 0x0000 |
| Cycle 3 Reg[0] | 0x0002 |
| Cycle 3 Reg[1] | 0x0000 |
| Cycles | 3/3 |
| Uplinks | 3 on port 10, 4 bytes each |
| Last Payload | `00 02 00 00` |
