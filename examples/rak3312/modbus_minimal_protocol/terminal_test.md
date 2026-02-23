# Terminal Test — Modbus Minimal Protocol

## Prerequisites

- PlatformIO CLI installed
- RAK3312 connected via USB (`/dev/cu.usbmodem211401`)
- RS485 slave device wired to RAK5802 (see `wiring.md`)

## Build

```bash
cd wisblock_lab_template_v1
pio run -e rak3312_gate3
```

Expected output:
```
Environment    Status    Duration
-------------  --------  ------------
rak3312_gate3  SUCCESS   00:00:03.xxx
```

## Flash

```bash
pio run -e rak3312_gate3 -t upload
```

Expected output:
```
Writing at 0x00010000... (100 %)
Wrote 284784 bytes (160093 compressed) at 0x00010000
Hard resetting via RTS pin...
========================= [SUCCESS] Took 4.xx seconds =========================
```

## Monitor

```bash
pio device monitor -e rak3312_gate3
```

Or with Python serial:
```bash
python3 -c "
import serial, time
ser = serial.Serial('/dev/cu.usbmodem211401', 115200, timeout=1)
time.sleep(0.5)
ser.setDTR(False); time.sleep(0.1); ser.setDTR(True)
time.sleep(0.5); ser.reset_input_buffer()
start = time.time()
while (time.time() - start) < 10:
    line = ser.readline()
    if line:
        print(line.decode('utf-8', errors='replace').rstrip())
    if b'GATE COMPLETE' in line:
        break
ser.close()
"
```

## Expected Serial Output (PASS)

```
========================================
[SYSTEM] WisBlock Gate Test Runner
[SYSTEM] Core: RAK3312 (ESP32-S3)
[SYSTEM] Gate: 3 - Modbus Minimal Protocol
========================================

[GATE3] === GATE START: gate_modbus_minimal_protocol v1.1 ===
[GATE3] STEP 1: UART + RS485 init (Slave=1, Baud=4800, Parity=NONE)
[GATE3] RS485 EN=HIGH (GPIO14), DE configured (GPIO21)
[GATE3] UART Init: OK (4800 8N1)
[GATE3] STEP 2: Modbus Read Holding Register (FC=0x03, Reg=0x0000, Qty=1)
[GATE3] TX (8 bytes): 01 03 00 00 00 01 84 0A
[GATE3] RX (7 bytes): 01 03 02 00 06 38 46
[GATE3] Slave=1 Value=0x0006
[GATE3] STEP 3: Protocol validation summary
[GATE3]   Target Slave  : 1
[GATE3]   Baud Rate     : 4800
[GATE3]   Frame Config  : 8N1
[GATE3]   Function Code : 0x03 (Read Holding Registers)
[GATE3]   Register      : 0x0000
[GATE3]   Protocol      : Modbus RTU
[GATE3]   Response Type : Normal
[GATE3]   byte_count    : 2
[GATE3]   Register Value: 0x0006 (6)
[GATE3]   Response Len  : 7 bytes
[GATE3]   Response Hex  : 01 03 02 00 06 38 46
[GATE3] PASS
[GATE3] === GATE COMPLETE ===
```

## Pass Verification Checklist

- [ ] `[GATE3] UART Init: OK` — UART initialized
- [ ] `[GATE3] TX (8 bytes):` — Request frame sent
- [ ] `[GATE3] RX (7 bytes):` — Response received (7 bytes for 1 register)
- [ ] `[GATE3] Slave=N Value=0xNNNN` — Structured output printed
- [ ] `[GATE3] PASS` — All criteria met
- [ ] `[GATE3] === GATE COMPLETE ===` — No crash

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| No serial output | USB CDC not enabled | Verify `-DARDUINO_USB_CDC_ON_BOOT=1` in build_flags |
| `RX: timeout` | No slave response | Check wiring, power, A/B polarity |
| `CRC mismatch` | Baud/parity wrong | Verify slave is 4800 8N1 |
| `Short frame` | Electrical noise | Check GND connection, cable shielding |
| `FAIL (code=1)` | UART init failed | Check GPIO 43/44 not conflicting |
