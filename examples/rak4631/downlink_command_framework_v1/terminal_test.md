# Terminal Test — RAK4631 Gate 8: Downlink Command Framework v1

## Prerequisites

- **Hardware:** RAK4631 + RAK5802 (Slot A) + RAK19007 Base Board
- **Sensor:** Modbus RTU slave at address 1, 4800 baud, 8N1 — externally powered
- **Gateway:** ChirpStack v4 gateway in LoRa range, AS923-1
- **Device provisioned:** DevEUI `88:82:24:44:AE:ED:1E:B2` in ChirpStack
- **Tools:** PlatformIO CLI, USB cable
- **Prerequisite:** Gate 7 PASS (Production loop soak validated)

## 1. Build

```bash
pio run -e rak4631_gate8
```

**Expected:**
```
Environment    Status    Duration
-------------  --------  ------------
rak4631_gate8  SUCCESS   00:00:13.xxx
```

RAM ~6.6%, Flash ~17.8%.

## 2. Flash

```bash
pio run -e rak4631_gate8 -t upload
```

**Expected:**
```
Activating new firmware
Device programmed.
rak4631_gate8  SUCCESS
```

## 3. Monitor (PASS_WITHOUT_DOWNLINK)

Upload and immediately capture serial (~2 minutes):

```bash
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

**Expected (no downlink enqueued):**
```
[SYSTEM] Gate: 8 - Downlink Command Framework v1
[GATE8] STEP 1: SystemManager init
[GATE8] Connected: YES
[GATE8] STEP 1: PASS
[GATE8] STEP 2: Initial uplinks to open RX windows
[RUNTIME] Cycle 1: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[RUNTIME] Cycle 2: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE8] STEP 2: PASS
[GATE8] STEP 3: Polling for downlink
[RUNTIME] Cycle 3: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[RUNTIME] Cycle 4: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE8] PASS_WITHOUT_DOWNLINK — no downlink scheduled
[GATE8] === GATE COMPLETE ===
```

## 4. Monitor (PASS with Downlink)

**Before flashing**, enqueue a downlink in ChirpStack:
1. Open **Devices → Queue** tab
2. Enqueue: fport=10, hex=`D101010000271` (SET_INTERVAL to 10000 ms)
3. Then flash + monitor as in step 3

**Expected (downlink enqueued):**
```
[GATE8] STEP 3: Polling for downlink
[GATE8] Downlink received! port=10, len=7, hex=D1 01 01 00 00 27 10
[GATE8] Parse result: ACK SET_INTERVAL: 10000 ms
[GATE8] Sending ACK uplink (fport=11, len=8): D1 01 01 01 00 00 27 10
[GATE8] ACK uplink sent OK
[GATE8] Current interval: 10000 ms
[GATE8] PASS — downlink received and processed
[GATE8] === GATE COMPLETE ===
```

## Pass Verification Checklist

### PASS_WITHOUT_DOWNLINK (minimum)
- [ ] `SystemManager init` succeeds, `Connected: YES`
- [ ] `STEP 1: PASS` printed
- [ ] 2 initial uplinks sent with `read=OK`, `uplink=OK`
- [ ] `STEP 2: PASS` printed
- [ ] 60-second downlink poll completes
- [ ] `PASS_WITHOUT_DOWNLINK` printed
- [ ] `=== GATE COMPLETE ===` printed

### PASS (with downlink)
- [ ] All PASS_WITHOUT_DOWNLINK checks above
- [ ] `Downlink received!` with correct hex payload
- [ ] `Parse result:` shows ACK message
- [ ] `Sending ACK uplink (fport=11)` with correct ACK hex
- [ ] `ACK uplink sent OK`
- [ ] `Current interval:` shows new value (if SET_INTERVAL)
- [ ] `PASS — downlink received and processed` printed

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| `SystemManager init` fails | LoRa/join/Modbus failure | Check antenna, credentials, sensor |
| No downlink received | Not enqueued or no RX window | Enqueue before flash; uplinks open windows |
| `NACK` on valid command | Payload formatting error | Check hex encoding matches schema |
| ACK uplink FAIL | Radio busy or duty cycle | Wait for next TX window |
| No serial output | USB CDC timing | Wait 5s post-flash |
