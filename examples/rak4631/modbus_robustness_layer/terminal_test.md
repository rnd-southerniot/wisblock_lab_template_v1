# Terminal Test — RAK4631 Gate 4: Modbus Robustness Layer

## Prerequisites

- **Hardware:** RAK4631 + RAK5802 (Slot A) + RAK19007 Base Board
- **Sensor:** Modbus RTU slave at address 1, 4800 baud, 8N1 — externally powered
- **Tools:** PlatformIO CLI, USB cable
- **Prerequisite:** Gate 3 PASS (protocol validated)

## 1. Build

```bash
pio run -e rak4631_gate4
```

**Expected:**
```
Environment    Status    Duration
-------------  --------  ------------
rak4631_gate4  SUCCESS   00:00:05.xxx
```

## 2. Flash

```bash
pio run -e rak4631_gate4 -t upload --upload-port /dev/cu.usbmodemXXXXX
```

**Expected:**
```
Activating new firmware
Device programmed.
rak4631_gate4  SUCCESS
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

## Expected Serial Output (PASS)

```
========================================
[SYSTEM] WisBlock Gate Test Runner
[SYSTEM] Core: RAK4631 (nRF52840)
[SYSTEM] Gate: 4 - Modbus Robustness Layer
========================================

[GATE4-R4631] === GATE START: gate_rak4631_modbus_robustness_layer v1.0 ===
[GATE4-R4631] STEP 1: UART + RS485 init (Slave=1, Baud=4800, Parity=NONE)
[GATE4-R4631]   RS485 EN=HIGH (pin 34, WB_IO2 — 3V3_S)
[GATE4-R4631]   DE/RE: auto (RAK5802 hardware)
[GATE4-R4631]   UART Init: OK (4800 8N1)
[GATE4-R4631]   Serial1 TX=16 RX=15
[GATE4-R4631] STEP 2: Multi-register read (FC=0x03, Reg=0x0000, Qty=2, MaxRetries=3)
[GATE4-R4631] Attempt 1/4:
[GATE4-R4631]   TX (8 bytes): 01 03 00 00 00 02 C4 0B
[GATE4-R4631]   RX (9 bytes): 01 03 04 00 06 00 01 DB F2
[GATE4-R4631]   Parse: OK (normal, byte_count=4, expected=4)
[GATE4-R4631]   Error: NONE
[GATE4-R4631] Result: SUCCESS on attempt 1 (0 retries, 0 UART recoveries)
[GATE4-R4631] STEP 3: Robustness validation summary
[GATE4-R4631]   Target Slave   : 1
[GATE4-R4631]   Baud Rate      : 4800
[GATE4-R4631]   Frame Config   : 8N1
[GATE4-R4631]   Function Code  : 0x03 (Read Holding Registers)
[GATE4-R4631]   Registers      : 0x0000, Qty=2
[GATE4-R4631]   Attempts       : 1
[GATE4-R4631]   Retries Used   : 0
[GATE4-R4631]   Last Error     : NONE
[GATE4-R4631]   UART Recoveries: 0
[GATE4-R4631]   byte_count     : 4 (expected: 4)
[GATE4-R4631]   Register[0]    : 0x0006 (6)
[GATE4-R4631]   Register[1]    : 0x0001 (1)
[GATE4-R4631]   Response Len   : 9 bytes
[GATE4-R4631]   Response Hex   : 01 03 04 00 06 00 01 DB F2

[GATE4-R4631] PASS
[GATE4-R4631] === GATE COMPLETE ===
```

## Pass Verification Checklist

- [ ] UART initialized at 4800 8N1
- [ ] RS485 powered via WB_IO2 (pin 34)
- [ ] DE/RE reported as auto (hardware)
- [ ] TX frame: `01 03 00 00 00 02 C4 0B` (8 bytes, CRC valid)
- [ ] RX frame: 9 bytes, slave=1, FC=0x03, byte_count=4
- [ ] byte_count == 2 * qty (4 == 2 * 2)
- [ ] Both Register[0] and Register[1] extracted and printed
- [ ] Error classification logged: `Error: NONE`
- [ ] Result line: `SUCCESS on attempt N (M retries, K UART recoveries)`
- [ ] `[GATE4-R4631] PASS` printed
- [ ] `[GATE4-R4631] === GATE COMPLETE ===` printed

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| `FAIL (code=3)` All timeouts | Sensor not connected/powered | Check wiring, external 12V/24V |
| `FAIL (code=4)` CRC errors | Noise on RS485 bus | Check A+/B-/GND connections |
| `FAIL (code=5)` Addr mismatch | Wrong slave address | Verify sensor is at address 1 |
| `FAIL (code=8)` byte_count | Sensor doesn't support reg 0x0001 | Check sensor register map |
| `FAIL (code=10)` UART recovery | nRF52840 UARTE issue | Power cycle board |
| `FAIL (code=11)` Exception | Sensor returned Modbus exception | Check register validity |
| No serial output | USB CDC timing | Wait 8s post-flash, open port for DTR |
