# RAK4631 Gate 4 — Modbus Robustness Layer Validation

**Status:** PASSED
**Tag:** `v3.4-gate4-pass-rak4631`
**Date:** 2026-02-24

## Overview

Validates the Modbus RTU robustness layer by performing a multi-register read
(FC 0x03, qty=2) with retry support, error classification per attempt, and
UART recovery after consecutive failures. Extends Gate 3's single-register
protocol validation with production-ready error handling.

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
| `main.cpp` | Standalone example with inline retry loop + error classification |
| `pin_map.h` | GPIO pin definitions + discovered device params + retry config |
| `wiring.md` | Hardware wiring guide |
| `terminal_test.md` | Step-by-step terminal verification |
| `prompt_test.md` | AI assistant verification prompts |
| `README.md` | This file |

## Quick Start

```bash
# Build
pio run -e rak4631_gate4

# Flash
pio run -e rak4631_gate4 -t upload --upload-port /dev/cu.usbmodemXXXXX

# Monitor (wait 8s, gate completes in ~1s)
python3 -c "
import serial, serial.tools.list_ports, time, sys
time.sleep(8)
ports = [p.device for p in serial.tools.list_ports.comports() if 'usbmodem' in p.device]
ser = serial.Serial(ports[0], 115200, timeout=1)
ser.dtr = True; time.sleep(0.1)
end = time.time() + 20
while time.time() < end:
    data = ser.read(ser.in_waiting or 1)
    if data:
        text = data.decode('utf-8', errors='replace')
        sys.stdout.write(text); sys.stdout.flush()
        if 'GATE COMPLETE' in text: break
ser.close()
"
```

## Robustness Features

| Feature | Configuration |
|---------|---------------|
| Multi-register read | qty=2, byte_count=4 |
| Max retries | 3 (4 total attempts) |
| Retry delay | 50ms |
| Retryable errors | TIMEOUT, CRC_FAIL |
| Non-retryable errors | LENGTH_FAIL, EXCEPTION, UART_ERROR |
| UART recovery threshold | 2 consecutive retryable failures |
| UART recovery action | Re-call Serial1.begin() (no end()) |

## Error Classification

| Error | Value | Retryable? |
|-------|-------|------------|
| NONE | 0 | N/A (success) |
| TIMEOUT | 1 | Yes |
| CRC_FAIL | 2 | Yes |
| LENGTH_FAIL | 3 | No |
| EXCEPTION | 4 | No |
| UART_ERROR | 5 | No |

## Validated Protocol

| Parameter | Value |
|-----------|-------|
| Slave Address | 1 |
| Baud Rate | 4800 |
| Frame Config | 8N1 |
| Function Code | 0x03 (Read Holding Registers) |
| Start Register | 0x0000 |
| Quantity | 2 |
| byte_count | 4 (= 2 * 2) |
| Register[0] | 0x0006 (wind speed × 10 = 0.6 m/s) |
| Register[1] | 0x0001 (wind direction = 1°) |
| CRC | Valid |
| Response Type | Normal |
| Retries Used | 0 |
