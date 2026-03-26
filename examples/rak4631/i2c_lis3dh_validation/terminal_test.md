# Terminal Test — RAK4631 Gate 1: I2C LIS3DH Validation

## Prerequisites

- **Hardware:** RAK4631 WisBlock Core + RAK1904 (LIS3DH) + RAK19007 Base Board
- **Tools:** PlatformIO CLI, USB cable
- **Connection:** RAK4631 connected via USB (CDC serial)

## 1. Build

```bash
pio run -e rak4631_gate1
```

**Expected:**
```
Environment    Status    Duration
-------------  --------  ------------
rak4631_gate1  SUCCESS   00:00:05.xxx
```

## 2. Flash

```bash
pio run -e rak4631_gate1 -t upload --upload-port /dev/cu.usbmodemXXXXX
```

**Expected:**
```
Activating new firmware
Device programmed.
rak4631_gate1  SUCCESS
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
end = time.time() + 20
while time.time() < end:
    data = ser.read(ser.in_waiting or 1)
    if data: sys.stdout.write(data.decode('utf-8', errors='replace')); sys.stdout.flush()
ser.close()
"
```

**Method B — After board has booted, press RESET once with serial port already open.**

## Expected Serial Output (PASS)

```
========================================
[SYSTEM] WisBlock Gate Test Runner
[SYSTEM] Core: RAK4631 (nRF52840)
[SYSTEM] Gate: 1 - I2C LIS3DH Validation
========================================

[GATE1-R4631] === GATE START: gate_rak4631_i2c_lis3dh_validation v1.0 ===
[GATE1-R4631] Wire SDA=13 SCL=14
[GATE1-R4631] STEP 1: Detect LIS3DH at 0x18
[GATE1-R4631]   ACK — device present
[GATE1-R4631] STEP 2: Read WHO_AM_I (0x0F)
[GATE1-R4631]   WHO_AM_I = 0x33 (expected 0x33)
[GATE1-R4631]   LIS3DH confirmed
[GATE1-R4631] STEP 3: Configure sensor
[GATE1-R4631]   CTRL_REG1=0x47 (50Hz, XYZ on)
[GATE1-R4631]   CTRL_REG4=0x08 (+/-2g, hi-res)
[GATE1-R4631] STEP 4: 100 consecutive accel reads
[GATE1-R4631]   [0] X=1808 Y=-1280 Z=15696
[GATE1-R4631]   [25] X=1792 Y=-1312 Z=15888
[GATE1-R4631]   [50] X=1680 Y=-1312 Z=16048
[GATE1-R4631]   [75] X=1520 Y=-1440 Z=16000

[GATE1-R4631] Reads: 100/100 passed, 0 failed
[GATE1-R4631] X range: [1520 .. 1984]
[GATE1-R4631] Y range: [-1520 .. -1184]
[GATE1-R4631] Z range: [15696 .. 16304]

[GATE1-R4631] PASS
[GATE1-R4631] === GATE COMPLETE ===
```

## Pass Verification Checklist

- [ ] `WHO_AM_I = 0x33` — LIS3DH identity confirmed
- [ ] `100/100 passed, 0 failed` — all I2C reads succeeded
- [ ] Z-axis reads ~15000–17000 (≈1g at ±2g/12-bit, board flat)
- [ ] `[GATE1-R4631] PASS` printed
- [ ] `[GATE1-R4631] === GATE COMPLETE ===` printed
- [ ] Blue LED blinked once

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| `No ACK at 0x18` | RAK1904 not inserted or wrong slot | Check sensor module is seated in I2C slot |
| `WHO_AM_I mismatch` | Different sensor at 0x18 | Verify RAK1904 (LIS3DH) is installed |
| `UnknownBoard` error | Board JSON missing | Ensure `boards/wiscore_rak4631.json` exists |
| No serial output | USB CDC timing | Wait 8s after flash, then open port to set DTR |
| Port not found | Board in bootloader | Press RESET once (not double-tap) |
| Upload `AssertionError` | ststm32 platform bug | Use `--upload-port /dev/cu.usbmodemXXXXX` explicitly |
