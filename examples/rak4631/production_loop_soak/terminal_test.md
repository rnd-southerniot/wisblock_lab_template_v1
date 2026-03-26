# Terminal Test — RAK4631 Gate 7: Production Loop Soak

## Prerequisites

- **Hardware:** RAK4631 + RAK5802 (Slot A) + RAK19007 Base Board
- **Sensor:** Modbus RTU slave at address 1, 4800 baud, 8N1 — externally powered
- **Gateway:** ChirpStack v4 gateway in LoRa range, AS923-1
- **Device provisioned:** DevEUI `88:82:24:44:AE:ED:1E:B2` in ChirpStack
- **Tools:** PlatformIO CLI, USB cable
- **Prerequisite:** Gate 6 PASS (Runtime scheduler integration validated)

## 1. Build

```bash
pio run -e rak4631_gate7
```

**Expected:**
```
Environment    Status    Duration
-------------  --------  ------------
rak4631_gate7  SUCCESS   00:00:12.xxx
```

RAM ~6.6%, Flash ~17.7% (runtime layer + SX126x-Arduino library).

## 2. Flash

```bash
pio run -e rak4631_gate7 -t upload --upload-port /dev/cu.usbmodemXXXXX
```

**Expected:**
```
Activating new firmware
Device programmed.
rak4631_gate7  SUCCESS
```

## 3. Monitor

Wait 8 seconds after flash, then capture for the full soak (~6 minutes):

```bash
python3 -c "
import serial, time, sys
time.sleep(8)
ser = serial.Serial('/dev/cu.usbmodem21101', 115200, timeout=1)
time.sleep(0.2)
end = time.time() + 400  # 30s join + 300s soak + 60s margin
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

> **Note:** The 400-second timeout allows for 30-second OTAA join plus 300-second soak plus margin. Total runtime is typically ~330 seconds.

## Expected Serial Output (PASS)

```
========================================
[SYSTEM] WisBlock Gate Test Runner
[SYSTEM] Core: RAK4631 (nRF52840)
[SYSTEM] Gate: 7 - Production Loop Soak
========================================

[GATE7-R4631] === GATE START: gate_rak4631_production_loop_soak v1.0 ===
[GATE7-R4631] Soak duration  : 300000 ms (300 s)
[GATE7-R4631] Min uplinks    : 3
[GATE7-R4631] Max consec fail: 2
[GATE7-R4631] Max cycle dur  : 1500 ms
[GATE7-R4631] Poll interval  : 30000 ms

[GATE7-R4631] STEP 1: SystemManager init (production mode)
[GATE7-R4631] SystemManager init: OK (dur=XXXX ms)
[GATE7-R4631]   Transport  : LoRaTransport (connected=yes)
[GATE7-R4631]   Peripheral : ModbusPeripheral (Slave=1, Baud=4800)
[GATE7-R4631]   Scheduler  : 1 task(s), interval=30000 ms
[GATE7-R4631]   Join count : 1 (single init)

[GATE7-R4631] STEP 2: Production soak (300 seconds)
[RUNTIME] Cycle 1: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE7-R4631] Cycle 1 | modbus=OK | uplink=OK | dur=81 ms | elapsed=30 s
[RUNTIME] Cycle 2: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE7-R4631] Cycle 2 | modbus=OK | uplink=OK | dur=81 ms | elapsed=60 s
...
[RUNTIME] Cycle 9: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE7-R4631] Cycle 9 | modbus=OK | uplink=OK | dur=82 ms | elapsed=270 s

[GATE7-R4631] STEP 3: Soak summary
[GATE7-R4631]   Duration          : 300007 ms (300 s)
[GATE7-R4631]   Total Cycles      : 9
[GATE7-R4631]   Modbus OK / FAIL  : 9 / 0
[GATE7-R4631]   Uplink OK / FAIL  : 9 / 0
[GATE7-R4631]   Max Consec Fail   : modbus=0, uplink=0
[GATE7-R4631]   Max Cycle Duration: 82 ms
[GATE7-R4631]   Transport         : connected
[GATE7-R4631]   Join Attempts     : 1
[GATE7-R4631] PASS
[GATE7-R4631] === GATE COMPLETE ===
```

## 4. ChirpStack Verification

After `[GATE7-R4631] PASS`, verify in ChirpStack v4:

1. Navigate to **Devices → [RAK4631 device] → Events**
2. Confirm **Join** event with DevEUI `88:82:24:44:AE:ED:1E:B2`
3. Confirm **~9 Uplink** events on port 10, each with 4-byte payload
4. Decode payload: `XX XX YY YY` = Reg[0] (wind speed), Reg[1] (direction)
5. Verify uplinks are spaced ~30 seconds apart (natural scheduler interval)

## Pass Verification Checklist

- [ ] `SystemManager init: OK` printed
- [ ] `Transport  : LoRaTransport (connected=yes)` printed
- [ ] `Scheduler  : 1 task(s), interval=30000 ms` printed
- [ ] `Join count : 1 (single init)` — single OTAA join only
- [ ] `[RUNTIME] Cycle N: read=OK` lines appear every ~30 seconds
- [ ] `[GATE7-R4631] Cycle N | modbus=OK | uplink=OK` for each cycle
- [ ] `Total Cycles      : 9` (or close to 10)
- [ ] `Modbus OK / FAIL  : N / 0` — zero modbus failures
- [ ] `Uplink OK / FAIL  : N / 0` — zero uplink failures
- [ ] `Max Consec Fail   : modbus=0, uplink=0` — no consecutive failures
- [ ] `Max Cycle Duration: <1500 ms` — all cycles fast
- [ ] `Transport         : connected` — LoRa still joined
- [ ] `[GATE7-R4631] PASS` printed
- [ ] `[GATE7-R4631] === GATE COMPLETE ===` printed
- [ ] ChirpStack shows ~9 uplink events on port 10 (manual check)

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| `SystemManager init: FAIL` | LoRa/join/Modbus init failure | Check antenna, credentials, sensor wiring |
| `CRITERION FAIL: cycles < min` | Too few cycles in soak | Check soak duration, poll interval |
| `CRITERION FAIL: successful uplinks < min` | LoRaWAN sends failing | Check gateway, frequency plan, antenna |
| `CRITERION FAIL: max consec modbus fail` | Sensor disconnected | Check A+/B-/GND, external power |
| `CRITERION FAIL: max cycle dur` | Cycle taking too long | Check Modbus response timeout |
| `CRITERION FAIL: transport disconnected` | LoRa link lost | Check antenna connection, gateway uptime |
| No serial output | USB CDC timing | Wait 8s post-flash, open port for DTR |
| Only partial output captured | Soak still running | Increase monitor timeout to 400s |
