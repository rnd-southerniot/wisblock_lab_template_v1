# Terminal Test — RAK4631 Gate 2: RS485 Modbus Autodiscovery

## Prerequisites

- **Hardware:** RAK4631 WisBlock Core + RAK5802 (Slot A) + RAK19007 Base Board
- **Sensor:** Modbus RTU slave connected to RAK5802 A+/B-/GND, externally powered
- **Tools:** PlatformIO CLI, USB cable
- **Connection:** RAK4631 connected via USB (CDC serial)

## 1. Build

```bash
pio run -e rak4631_gate2
```

**Expected:**
```
Environment    Status    Duration
-------------  --------  ------------
rak4631_gate2  SUCCESS   00:00:01.xxx
```

## 2. Flash

```bash
pio run -e rak4631_gate2 -t upload --upload-port /dev/cu.usbmodemXXXXX
```

**Expected:**
```
Activating new firmware
Device programmed.
rak4631_gate2  SUCCESS
```

## 3. Monitor

Wait 8 seconds after flash for board to reboot, then:

**Method A — Python serial reader:**
```bash
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

**Method B — After board has booted, press RESET once with serial port already open.**

## Expected Serial Output (PASS)

```
========================================
[SYSTEM] WisBlock Gate Test Runner
[SYSTEM] Core: RAK4631 (nRF52840)
[SYSTEM] Gate: 2 - RS485 Autodiscovery
========================================

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

## Pass Verification Checklist

- [ ] RAK5802 powered via WB_IO2 (pin 34)
- [ ] DE/RE reported as auto (hardware)
- [ ] Modbus device discovered (slave, baud, parity printed)
- [ ] TX frame printed (8 bytes with valid CRC)
- [ ] RX frame printed (>= 5 bytes)
- [ ] CRC reported as valid
- [ ] `[GATE2-R4631] PASS` printed
- [ ] `[GATE2-R4631] === GATE COMPLETE ===` printed
- [ ] Blue LED blinked once

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| `FAIL — no Modbus device discovered` | Sensor not connected or not powered | Check A+/B-/GND wiring, verify sensor has 12V/24V |
| `FAIL — no Modbus device discovered` | RAK5802 in wrong slot | Must be **Slot A** for Serial1 |
| `FAIL — no Modbus device discovered` | A/B wires swapped | Swap A+ and B- at screw terminal |
| No serial output | USB CDC timing | Wait 8s after flash, then open port to set DTR |
| Port not found | Board in bootloader | Press RESET once (not double-tap) |
| Upload `AssertionError` | ststm32 platform bug | Use `--upload-port /dev/cu.usbmodemXXXXX` explicitly |
| Scan hangs | `Serial1.end()` called | Never call `Serial1.end()` on nRF52 — just call `begin()` again |
