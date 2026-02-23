# Modbus Minimal Protocol Example

**Gate:** 3 — Modbus Minimal Protocol Validation
**Status:** PASSED
**Tag:** `v2.3-gate3-pass-rak3312`
**Date:** 2026-02-24

## Overview

Demonstrates a minimal Modbus RTU Read Holding Register (FC 0x03) transaction over RS485 using the RAK5802 transceiver on a RAK3312 (ESP32-S3) WisBlock system.

This example is derived from the Gate 3 validation test, which confirmed:
- CRC-16/MODBUS frame construction and verification
- Modbus RTU request/response round-trip
- Response parsing with byte_count consistency validation
- Exception response handling

## Hardware

| Component | Model | Location |
|-----------|-------|----------|
| Core | RAK3312 (ESP32-S3 + SX1262) | CPU Slot |
| Base Board | RAK19007 | — |
| RS485 | RAK5802 | IO Slot |
| Slave Device | Modbus RTU slave (e.g. RS-FSJT-N01) | External |

## Pin Map

| Signal | GPIO | WisBlock Pin |
|--------|------|-------------|
| UART1 TX | 43 | — |
| UART1 RX | 44 | — |
| RS485 EN | 14 | WB_IO2 |
| RS485 DE | 21 | WB_IO1 |

See `pin_map.h` for defines.

## Files

| File | Purpose |
|------|---------|
| `main.cpp` | Self-contained Modbus RTU read example |
| `pin_map.h` | GPIO pin definitions |
| `wiring.md` | Physical wiring diagram and notes |
| `terminal_test.md` | Build, flash, monitor instructions |
| `prompt_test.md` | AI-assisted test prompts |
| `README.md` | This file |

## Quick Start

```bash
# Build (gate environment)
pio run -e rak3312_gate3

# Flash
pio run -e rak3312_gate3 -t upload

# Monitor
pio device monitor -e rak3312_gate3
```

## Expected Output

```
[GATE3] TX (8 bytes): 01 03 00 00 00 01 84 0A
[GATE3] RX (7 bytes): 01 03 02 00 06 38 46
[GATE3] Slave=1 Value=0x0006
[GATE3] PASS
[GATE3] === GATE COMPLETE ===
```

## Validated Parameters

| Parameter | Value |
|-----------|-------|
| Slave Address | 1 |
| Baud Rate | 4800 |
| Parity | None (8N1) |
| Register | 0x0000 |
| Quantity | 1 |
| Response Timeout | 200 ms |
