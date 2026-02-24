# Terminal Test — RAK4631 Gate 5: LoRaWAN OTAA Join + Modbus Uplink

## Prerequisites

- **Hardware:** RAK4631 + RAK5802 (Slot A) + RAK19007 Base Board
- **Sensor:** Modbus RTU slave at address 1, 4800 baud, 8N1 — externally powered
- **Gateway:** ChirpStack v4 gateway in LoRa range, AS923-1
- **Device provisioned:** DevEUI `88:82:24:44:AE:ED:1E:B2` in ChirpStack
- **Tools:** PlatformIO CLI, USB cable
- **Prerequisite:** Gate 4 PASS (Modbus robustness validated)

## 1. Build

```bash
pio run -e rak4631_gate5
```

**Expected:**
```
Environment    Status    Duration
-------------  --------  ------------
rak4631_gate5  SUCCESS   00:00:12.xxx
```

RAM ~6.6%, Flash ~17.6% (SX126x-Arduino library adds ~80 KB).

## 2. Flash

```bash
pio run -e rak4631_gate5 -t upload --upload-port /dev/cu.usbmodemXXXXX
```

**Expected:**
```
Activating new firmware
Device programmed.
rak4631_gate5  SUCCESS
```

## 3. Monitor

Wait 8 seconds after flash, then:

```bash
python3 -c "
import serial, time, sys
time.sleep(8)
ser = serial.Serial('/dev/cu.usbmodem21101', 115200, timeout=1)
time.sleep(0.2)
end = time.time() + 90  # 30s join + margin
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

> **Note:** The 90-second timeout allows for the 30-second OTAA join window plus Modbus read and uplink.

## Expected Serial Output (PASS)

```
========================================
[SYSTEM] WisBlock Gate Test Runner
[SYSTEM] Core: RAK4631 (nRF52840)
[SYSTEM] Gate: 5 - LoRaWAN Join + Uplink
========================================

[GATE5-R4631] === GATE START: gate_rak4631_lorawan_join_uplink v1.0 ===
[GATE5-R4631] STEP 1: LoRaWAN stack init
[GATE5-R4631]   lora_rak4630_init: OK
[GATE5-R4631]   lmh_init: OK
[GATE5-R4631]   LoRaWAN Init: OK
[GATE5-R4631] STEP 2: Region configuration
[GATE5-R4631]   Region: AS923-1 (Class A, ADR=OFF, DR=2)
[GATE5-R4631] STEP 3: Set OTAA credentials
[GATE5-R4631]   DevEUI : 88:82:24:44:AE:ED:1E:B2
[GATE5-R4631]   AppEUI : 00:00:00:00:00:00:00:00
[GATE5-R4631]   AppKey : 91:8C:49:08:F0:89:50:6B:30:18:0B:62:65:9A:4A:D5
[GATE5-R4631]   OTAA credentials configured
[GATE5-R4631] STEP 4: OTAA join (trials=3, timeout=30s)
[GATE5-R4631]   Join Start...
[GATE5-R4631] STEP 5: Waiting for join accept...
[GATE5-R4631]   Join Accept: OK (XXXXms)
[GATE5-R4631] STEP 6: Modbus read (Slave=1, Baud=4800, FC=0x03, Reg=0x0000, Qty=2)
[GATE5-R4631]   RS485 EN=HIGH (auto DE/RE)
[GATE5-R4631]   TX (8 bytes): 01 03 00 00 00 02 C4 0B
[GATE5-R4631]   RX (9 bytes): 01 03 04 XX XX YY YY CC CC
[GATE5-R4631]   Modbus Read: OK (2 registers)
[GATE5-R4631]   Register[0] : 0xXXXX (N)
[GATE5-R4631]   Register[1] : 0xYYYY (M)
[GATE5-R4631] STEP 7: Build uplink payload
[GATE5-R4631]   Payload (4 bytes): XX XX YY YY
[GATE5-R4631]   Port: 10
[GATE5-R4631]   Confirmed: false
[GATE5-R4631] STEP 8: Send uplink
[GATE5-R4631]   Uplink TX...
[GATE5-R4631]   Uplink TX: OK (port=10, 4 bytes)
[GATE5-R4631] STEP 9: Gate summary
[GATE5-R4631]   Radio        : SX1262 (RAK4630 built-in)
[GATE5-R4631]   Region       : AS923-1
[GATE5-R4631]   Activation   : OTAA
[GATE5-R4631]   Join Status  : OK
[GATE5-R4631]   Modbus Slave : 1
[GATE5-R4631]   Register[0]  : 0xXXXX (N)
[GATE5-R4631]   Register[1]  : 0xYYYY (M)
[GATE5-R4631]   Payload Hex  : XX XX YY YY
[GATE5-R4631]   Uplink Port  : 10
[GATE5-R4631]   Uplink Status: OK

[GATE5-R4631] PASS
[GATE5-R4631] === GATE COMPLETE ===
```

## 4. ChirpStack Verification

After `[GATE5-R4631] PASS`, verify in ChirpStack v4:

1. Navigate to **Devices → [RAK4631 device] → Events**
2. Confirm **Join** event with DevEUI `88:82:24:44:AE:ED:1E:B2`
3. Confirm **Uplink** event on port 10 with 4-byte payload
4. Decode payload: `XX XX YY YY` = Reg[0] (wind speed), Reg[1] (direction)

## Pass Verification Checklist

- [ ] `lora_rak4630_init: OK` printed
- [ ] `lmh_init: OK` printed
- [ ] DevEUI matches `88:82:24:44:AE:ED:1E:B2`
- [ ] Join Accept received within 30s
- [ ] TX frame: `01 03 00 00 00 02 C4 0B` (8 bytes)
- [ ] RX frame: 9 bytes, CRC valid, byte_count=4
- [ ] Both Register[0] and Register[1] extracted
- [ ] Payload encoded as 4 bytes big-endian
- [ ] `Uplink TX: OK (port=10, 4 bytes)`
- [ ] `[GATE5-R4631] PASS` printed
- [ ] `[GATE5-R4631] === GATE COMPLETE ===` printed
- [ ] ChirpStack shows uplink event on port 10 (manual check)

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| `lora_rak4630_init: FAIL` | Missing `-DRAK4630` flag | Add `-DRAK4630 -D_VARIANT_RAK4630_` to build_flags |
| `Join TIMEOUT` | Gateway not reachable | Check antenna, gateway power, AS923-1 freq plan |
| `Join DENIED` | Credentials mismatch | Verify DevEUI/AppEUI/AppKey in ChirpStack |
| Modbus `RX: timeout` | Sensor not powered | Check external 12V/24V, A+/B-/GND wiring |
| `Uplink TX: FAIL` | LoRaMAC busy / duty cycle | Wait and retry |
| No serial output | USB CDC timing | Wait 8s post-flash, open port for DTR |
