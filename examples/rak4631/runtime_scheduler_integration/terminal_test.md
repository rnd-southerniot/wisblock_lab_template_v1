# Terminal Test — RAK4631 Gate 6: Runtime Scheduler Integration

## Prerequisites

- **Hardware:** RAK4631 + RAK5802 (Slot A) + RAK19007 Base Board
- **Sensor:** Modbus RTU slave at address 1, 4800 baud, 8N1 — externally powered
- **Gateway:** ChirpStack v4 gateway in LoRa range, AS923-1
- **Device provisioned:** DevEUI `88:82:24:44:AE:ED:1E:B2` in ChirpStack
- **Tools:** PlatformIO CLI, USB cable
- **Prerequisite:** Gate 5 PASS (LoRaWAN + Modbus uplink validated)

## 1. Build

```bash
pio run -e rak4631_gate6
```

**Expected:**
```
Environment    Status    Duration
-------------  --------  ------------
rak4631_gate6  SUCCESS   00:00:12.xxx
```

RAM ~6.6%, Flash ~17.7% (runtime layer + SX126x-Arduino library).

## 2. Flash

```bash
pio run -e rak4631_gate6 -t upload --upload-port /dev/cu.usbmodemXXXXX
```

**Expected:**
```
Activating new firmware
Device programmed.
rak4631_gate6  SUCCESS
```

## 3. Monitor

Wait 8 seconds after flash, then:

```bash
python3 -c "
import serial, time, sys
time.sleep(8)
ser = serial.Serial('/dev/cu.usbmodem21101', 115200, timeout=1)
time.sleep(0.2)
end = time.time() + 120  # 30s join + 3 cycles + margin
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

> **Note:** The 120-second timeout allows for the 30-second OTAA join window plus 3 forced cycles with 3-second inter-cycle delays. Total runtime is typically ~45 seconds.

## Expected Serial Output (PASS)

```
========================================
[SYSTEM] WisBlock Gate Test Runner
[SYSTEM] Core: RAK4631 (nRF52840)
[SYSTEM] Gate: 6 - Runtime Scheduler Integration
========================================

[GATE6-R4631] === GATE START: gate_rak4631_runtime_scheduler_integration v1.0 ===
[GATE6-R4631] STEP 1: Create SystemManager
[GATE6-R4631] SystemManager created
[GATE6-R4631] STEP 2: LoRa transport init
[GATE6-R4631] LoRa Init: OK
[GATE6-R4631] STEP 3: OTAA join (timeout=30s)
[GATE6-R4631] Join Start...
[GATE6-R4631] Join Accept: OK
[GATE6-R4631] STEP 4: Modbus peripheral init
[GATE6-R4631] Modbus Init: OK (Slave=1, Baud=4800)
[GATE6-R4631] STEP 5: Register scheduler tasks
[GATE6-R4631] Task registered: sensor_uplink (interval=30000ms)
[GATE6-R4631] Scheduler: 1 task(s) registered
[GATE6-R4631] STEP 6: Scheduler tick validation
[GATE6-R4631] Tick: no tasks due (OK — interval not yet elapsed)
[GATE6-R4631] STEP 7: Force cycle 1
[RUNTIME] Cycle 1: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE6-R4631]   Modbus Read: OK (2 registers)
[GATE6-R4631]   Register[0] : 0xXXXX (N)
[GATE6-R4631]   Register[1] : 0xYYYY (M)
[GATE6-R4631]   Uplink TX: OK (port=10, 4 bytes)
[GATE6-R4631] Cycle 1: PASS
[GATE6-R4631]   Inter-cycle delay (3s)...
[GATE6-R4631] STEP 8: Force cycle 2
[RUNTIME] Cycle 2: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE6-R4631]   Modbus Read: OK (2 registers)
[GATE6-R4631]   Register[0] : 0xXXXX (N)
[GATE6-R4631]   Register[1] : 0xYYYY (M)
[GATE6-R4631]   Uplink TX: OK (port=10, 4 bytes)
[GATE6-R4631] Cycle 2: PASS
[GATE6-R4631]   Inter-cycle delay (3s)...
[GATE6-R4631] STEP 9: Force cycle 3
[RUNTIME] Cycle 3: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE6-R4631]   Modbus Read: OK (2 registers)
[GATE6-R4631]   Register[0] : 0xXXXX (N)
[GATE6-R4631]   Register[1] : 0xYYYY (M)
[GATE6-R4631]   Uplink TX: OK (port=10, 4 bytes)
[GATE6-R4631] Cycle 3: PASS
[GATE6-R4631] STEP 10: Gate summary
[GATE6-R4631]   Scheduler    : OK (1 task)
[GATE6-R4631]   Transport    : LoRaTransport (connected)
[GATE6-R4631]   Peripheral   : ModbusPeripheral (Slave=1)
[GATE6-R4631]   Cycles       : 3/3
[GATE6-R4631]   Last Payload : XX XX YY YY
[GATE6-R4631]   Uplink Port  : 10
[GATE6-R4631] PASS
[GATE6-R4631] === GATE COMPLETE ===
```

## 4. ChirpStack Verification

After `[GATE6-R4631] PASS`, verify in ChirpStack v4:

1. Navigate to **Devices → [RAK4631 device] → Events**
2. Confirm **Join** event with DevEUI `88:82:24:44:AE:ED:1E:B2`
3. Confirm **3 Uplink** events on port 10, each with 4-byte payload
4. Decode payload: `XX XX YY YY` = Reg[0] (wind speed), Reg[1] (direction)
5. Verify uplinks are spaced ~3 seconds apart (forced cycle delay)

## Pass Verification Checklist

- [ ] `LoRa Init: OK` printed
- [ ] `Join Accept: OK` printed within 30s
- [ ] `Modbus Init: OK (Slave=1, Baud=4800)` printed
- [ ] `Task registered: sensor_uplink (interval=30000ms)` printed
- [ ] `Scheduler: 1 task(s) registered` printed
- [ ] `Tick: no tasks due` — dry run passed
- [ ] `[RUNTIME] Cycle 1: read=OK` — SystemManager sensorUplinkTask fired
- [ ] `Cycle 1: PASS`, `Cycle 2: PASS`, `Cycle 3: PASS` — all 3 cycles
- [ ] `Cycles       : 3/3` — final count matches
- [ ] `Transport    : LoRaTransport (connected)` — still connected after 3 uplinks
- [ ] `[GATE6-R4631] PASS` printed
- [ ] `[GATE6-R4631] === GATE COMPLETE ===` printed
- [ ] ChirpStack shows 3 uplink events on port 10 (manual check)

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| `LoRa Init: FAIL` | Missing `-DRAK4630` flag | Add `-DRAK4630 -D_VARIANT_RAK4630_` to build_flags |
| `Join TIMEOUT` | Gateway not reachable | Check antenna, gateway power, AS923-1 freq plan |
| `Join DENIED` | Credentials mismatch | Verify DevEUI/AppEUI/AppKey in ChirpStack |
| `Modbus Init: FAIL` | RS485 not powered | Check WB_IO2 (pin 34) HIGH, RAK5802 in Slot A |
| `Cycle N: FAIL (count)` | sensorUplinkTask didn't execute | Check SystemManager::activate() was called |
| `Modbus Read: FAIL` | Sensor not powered | Check external 12V/24V, A+/B-/GND wiring |
| `Uplink TX: FAIL` | LoRaMAC busy / duty cycle | Increase inter-cycle delay |
| No serial output | USB CDC timing | Wait 8s post-flash, open port for DTR |
| `Scheduler` compile error | BSP symbol collision | Ensure `Scheduler` renamed to `TaskScheduler` in runtime/ |
