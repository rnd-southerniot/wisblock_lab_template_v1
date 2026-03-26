# RAK4631 Gate 8 — Downlink Command Framework v1

**Status:** PASSED
**Tag:** `v3.9-gate8-pass-rak4631`
**Date:** 2026-02-25

## Overview

Validates bidirectional LoRaWAN communication: receive a ChirpStack downlink,
parse a binary command, apply a safe runtime change (poll interval or slave ID),
and confirm with an ACK uplink on fport 11.

Transitions from uplink-only operation (Gates 1–7) to full bidirectional
communication — the foundation for remote device management.

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
| SX1262 | — | On-board | Handled by lora_rak4630_init() |

## Files

| File | Purpose |
|------|---------|
| `main.cpp` | Standalone example: init + uplinks + downlink poll + ACK |
| `command_schema.md` | Binary command protocol specification |
| `chirpstack_enqueue.md` | How to enqueue test downlinks in ChirpStack |
| `terminal_test.md` | Step-by-step terminal verification |
| `prompt_test.md` | AI assistant verification prompts |
| `README.md` | This file |

## Quick Start

```bash
# Build
pio run -e rak4631_gate8

# Flash + Monitor
python3 -c "
import serial, time, subprocess, sys, glob
subprocess.run(['pio', 'run', '-e', 'rak4631_gate8', '-t', 'upload'],
               capture_output=True, text=True, timeout=60)
time.sleep(5)
ports = glob.glob('/dev/cu.usbmodem*')
ser = serial.Serial(ports[0], 115200, timeout=1)
start = time.time()
while time.time() - start < 180:
    line = ser.readline()
    if line:
        text = line.decode('utf-8', errors='replace').rstrip()
        print(text, flush=True)
        if 'GATE COMPLETE' in text: break
ser.close()
"
```

## Architecture

```
                    SystemManager
                    ┌──────────────────────────────────────────┐
                    │                                            │
                    │  TaskScheduler (cooperative, 30s default)  │
                    │    └── sensor_uplink                       │
                    │          ├── ModbusPeripheral::read()      │
                    │          ├── encode (big-endian)           │
                    │          └── LoRaTransport::send()         │
                    │                                            │
                    │  Downlink Path (new in Gate 8)             │
                    │    ├── lorawan_hal_pop_downlink()          │
                    │    │   └── IRQ-guarded single-slot buffer  │
                    │    ├── dl_parse_and_apply()                │
                    │    │   └── SET_INTERVAL / SET_SLAVE_ID /   │
                    │    │       REQ_STATUS / REBOOT              │
                    │    └── LoRaTransport::send(ACK, fport=11)  │
                    │                                            │
                    └──────────────────────────────────────────┘
```

## Binary Command Protocol

| CMD  | Name         | Payload        | ACK Size |
|------|--------------|----------------|----------|
| 0x01 | SET_INTERVAL | uint32_t BE ms | 8 bytes  |
| 0x02 | SET_SLAVE_ID | uint8_t addr   | 5 bytes  |
| 0x03 | REQ_STATUS   | (none)         | 17 bytes |
| 0x04 | REBOOT       | 0xA5 safety    | 5 bytes  |

See `command_schema.md` for complete specification.

## LoRaWAN Configuration

| Parameter | Value |
|-----------|-------|
| Region | AS923-1 |
| Activation | OTAA |
| Class | A |
| ADR | OFF |
| Data Rate | DR2 (SF10) |
| Uplink Port | 10 (sensor data) |
| ACK Port | 11 (command responses) |
| Downlink Port | 10 (commands) |

## Modbus Configuration

| Parameter | Value |
|-----------|-------|
| Slave Address | 1 (changeable via SET_SLAVE_ID) |
| Baud Rate | 4800 |
| Frame Config | 8N1 |
| Registers | 0x0000, qty=2 |

## Build Flags

```
-DCORE_RAK4631 -DRAK4630 -D_VARIANT_RAK4630_ -DGATE_8
```

## Validated Results

| Metric | Value |
|--------|-------|
| OTAA Join | OK |
| Initial Uplinks | 2/2 OK |
| Total Cycles | 4 |
| Modbus OK | 4/4 |
| Uplink OK | 4/4 |
| Downlink Parse | Framework validated (PASS_WITHOUT_DOWNLINK) |
| HAL Layering | No `<LoRaWan-Arduino.h>` in runtime |
| IRQ Safety | `noInterrupts()`/`interrupts()` on downlink buffer |
| Gate 7 Regression | PASS (unchanged) |
