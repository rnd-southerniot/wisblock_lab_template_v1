# RAK4631 Gate 2 — RS485 Modbus Autodiscovery

**Status:** PASSED
**Tag:** `v3.2-gate2-pass-rak4631`
**Date:** 2026-02-24

## Overview

Discovers a Modbus RTU slave device on the RS485 bus by scanning baud rates,
parity modes, and slave addresses. Uses the RAK5802 WisBlock RS485 module with
hardware auto-direction control (TP8485E transceiver).

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
| `main.cpp` | Standalone example (self-contained) |
| `pin_map.h` | GPIO pin definitions |
| `wiring.md` | Hardware wiring guide |
| `terminal_test.md` | Step-by-step terminal verification |
| `prompt_test.md` | AI assistant verification prompts |
| `README.md` | This file |

## Quick Start

```bash
# Build
pio run -e rak4631_gate2

# Flash (specify port to avoid ststm32 bug)
pio run -e rak4631_gate2 -t upload --upload-port /dev/cu.usbmodemXXXXX

# Monitor (wait 8s for reboot, then open port)
python3 -c "
import serial, serial.tools.list_ports, time, sys
time.sleep(8)
ports = [p.device for p in serial.tools.list_ports.comports() if 'usbmodem' in p.device]
ser = serial.Serial(ports[0], 115200, timeout=1)
ser.dtr = True; time.sleep(0.1)
end = time.time() + 300
while time.time() < end:
    data = ser.read(ser.in_waiting or 1)
    if data:
        text = data.decode('utf-8', errors='replace')
        sys.stdout.write(text); sys.stdout.flush()
        if 'GATE COMPLETE' in text: break
ser.close()
"
```

## Expected Output

```
[GATE2-R4631] === GATE START: gate_rak4631_rs485_autodiscovery_validation v1.0 ===
[GATE2-R4631] STEP 1: Enable RS485 (RAK5802)
[GATE2-R4631]   EN pin: 34 (WB_IO2 — 3V3_S power)
[GATE2-R4631]   Serial1 TX=16 RX=15
[GATE2-R4631]   DE/RE: auto (RAK5802 hardware auto-direction)
[GATE2-R4631] STEP 2: Autodiscovery scan
[GATE2-R4631]   Bauds: 4800, 9600, 19200, 115200
[GATE2-R4631]   Parity: NONE, EVEN (2 modes — nRF52 has no Odd parity)
[GATE2-R4631]   Slaves: 1..100
[GATE2-R4631]   Max probes: 800
[GATE2-R4631]   Trying baud=4800 parity=NONE...
[GATE2-R4631]   >>> FOUND at probe #1

[GATE2-R4631] STEP 3: Results
[GATE2-R4631]   Probes sent: 1
[GATE2-R4631]   Slave : 1
[GATE2-R4631]   Baud  : 4800
[GATE2-R4631]   Parity: NONE
[GATE2-R4631]   Type  : Normal response
[GATE2-R4631]   TX: 01 03 00 00 00 01 84 0A
[GATE2-R4631]   RX: 01 03 02 00 07 F9 86
[GATE2-R4631]   CRC: 0x86F9 (valid)

[GATE2-R4631] PASS
[GATE2-R4631] === GATE COMPLETE ===
```

## Discovered Device

| Parameter | Value |
|-----------|-------|
| Slave Address | 1 |
| Baud Rate | 4800 |
| Parity | NONE (8N1) |
| Register 0x0000 | 0x0007 (wind speed × 10) |
| Response Type | Normal (FC 0x03) |
| CRC | Valid |

## Key Lessons Learned

1. **RAK5802 auto-direction:** The TP8485E DE/RE is driven automatically by an
   on-board circuit. WB_IO1 (pin 17) is NOT connected to DE/RE. Manual GPIO
   toggling is unnecessary and can interfere with the auto-direction timing.

2. **`Serial1.flush()` required:** After `Serial1.write()`, call `flush()` to
   ensure TX completes before the auto-direction circuit switches to RX mode.

3. **No `Serial1.end()`:** On nRF52840, calling `Serial1.end()` before it has
   been started will hang indefinitely. Just call `begin()` again.

4. **nRF52840 parity limitation:** Only NONE and EVEN parity supported in
   hardware (UARTE peripheral has no Odd parity mode).
