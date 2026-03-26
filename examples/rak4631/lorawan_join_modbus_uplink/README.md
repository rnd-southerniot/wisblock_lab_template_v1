# RAK4631 Gate 5 — LoRaWAN OTAA Join + Modbus Uplink

**Status:** PASSED
**Tag:** `v3.5-gate5-pass-rak4631`
**Date:** 2026-02-25

## Overview

Validates end-to-end LoRaWAN uplink from sensor to network server:
OTAA join on AS923-1, Modbus RTU multi-register read (qty=2),
4-byte payload encoding, and unconfirmed uplink on port 10.

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
| `main.cpp` | Standalone example: join + read + uplink |
| `pin_map.h` | GPIO pin definitions + LoRaWAN/Modbus config |
| `wiring.md` | Hardware wiring guide + signal path diagram |
| `terminal_test.md` | Step-by-step terminal verification |
| `prompt_test.md` | AI assistant verification prompts |
| `README.md` | This file |

## Quick Start

```bash
# Build
pio run -e rak4631_gate5

# Flash
pio run -e rak4631_gate5 -t upload --upload-port /dev/cu.usbmodemXXXXX

# Monitor (wait 8s, gate completes in ~40s including join)
python3 -c "
import serial, time, sys
time.sleep(8)
ser = serial.Serial('/dev/cu.usbmodem21101', 115200, timeout=1)
time.sleep(0.2)
end = time.time() + 90
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

## Validated Results

| Parameter | Value |
|-----------|-------|
| Join Time | ~5.8 seconds |
| Register[0] | 0x0004 (wind speed 4 = 0.4 m/s) |
| Register[1] | 0x0001 (wind direction = 1 degree) |
| Payload | `00 04 00 01` |
| Uplink | port 10, 4 bytes, unconfirmed |
