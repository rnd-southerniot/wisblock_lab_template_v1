# Terminal Test — RAK4631 Gate 3: Modbus Minimal Protocol

## Prerequisites

- **Hardware:** RAK4631 + RAK5802 (Slot A) + RAK19007 Base Board
- **Sensor:** Modbus RTU slave at address 1, 4800 baud, 8N1 — externally powered
- **Tools:** PlatformIO CLI, USB cable
- **Prerequisite:** Gate 2 PASS (device discovered)

## 1. Build

```bash
pio run -e rak4631_gate3
```

**Expected:**
```
Environment    Status    Duration
-------------  --------  ------------
rak4631_gate3  SUCCESS   00:00:05.xxx
```

## 2. Flash

```bash
pio run -e rak4631_gate3 -t upload --upload-port /dev/cu.usbmodemXXXXX
```

**Expected:**
```
Activating new firmware
Device programmed.
rak4631_gate3  SUCCESS
```

## 3. Monitor

Wait 8 seconds after flash, then:

```bash
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

## Expected Serial Output (PASS)

```
========================================
[SYSTEM] WisBlock Gate Test Runner
[SYSTEM] Core: RAK4631 (nRF52840)
[SYSTEM] Gate: 3 - Modbus Minimal Protocol
========================================

[GATE3-R4631] === GATE START: gate_rak4631_modbus_minimal_protocol v1.0 ===
[GATE3-R4631] STEP 1: UART + RS485 init (Slave=1, Baud=4800, Parity=NONE)
[GATE3-R4631]   RS485 EN=HIGH (pin 34, WB_IO2 — 3V3_S)
[GATE3-R4631]   DE/RE: auto (RAK5802 hardware)
[GATE3-R4631]   UART Init: OK (4800 8N1)
[GATE3-R4631]   Serial1 TX=16 RX=15
[GATE3-R4631] STEP 2: Modbus Read Holding Register (FC=0x03, Reg=0x0000, Qty=1)
[GATE3-R4631]   TX (8 bytes): 01 03 00 00 00 01 84 0A
[GATE3-R4631]   RX (7 bytes): 01 03 02 00 04 B9 87
[GATE3-R4631]   Slave=1 Value=0x0004
[GATE3-R4631] STEP 3: Protocol validation summary
[GATE3-R4631]   Target Slave  : 1
[GATE3-R4631]   Baud Rate     : 4800
[GATE3-R4631]   Frame Config  : 8N1
[GATE3-R4631]   Function Code : 0x03 (Read Holding Registers)
[GATE3-R4631]   Register      : 0x0000
[GATE3-R4631]   Protocol      : Modbus RTU
[GATE3-R4631]   Response Type : Normal
[GATE3-R4631]   byte_count    : 2
[GATE3-R4631]   Register Value: 0x0004 (4)
[GATE3-R4631]   Response Len  : 7 bytes
[GATE3-R4631]   Response Hex  : 01 03 02 00 04 B9 87

[GATE3-R4631] PASS
[GATE3-R4631] === GATE COMPLETE ===
```

## Pass Verification Checklist

- [ ] UART initialized at 4800 8N1
- [ ] RS485 powered via WB_IO2 (pin 34)
- [ ] DE/RE reported as auto (hardware)
- [ ] TX frame: `01 03 00 00 00 01 84 0A` (8 bytes, CRC valid)
- [ ] RX frame: 7 bytes, slave=1, FC=0x03
- [ ] byte_count = 2 (consistent with 7 - 5 = 2)
- [ ] Register value extracted and printed
- [ ] `[GATE3-R4631] PASS` printed
- [ ] `[GATE3-R4631] === GATE COMPLETE ===` printed

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| `FAIL (code=3)` No response | Sensor not connected/powered | Check wiring, external 12V/24V |
| `FAIL (code=4)` CRC mismatch | Noise on RS485 bus | Check A+/B-/GND connections |
| `FAIL (code=5)` Addr mismatch | Wrong slave address | Verify sensor is at address 1 |
| `FAIL (code=8)` byte_count | Corrupted frame | Re-run, check wiring |
| No serial output | USB CDC timing | Wait 8s post-flash, open port for DTR |
