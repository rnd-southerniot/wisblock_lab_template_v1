# Pass Criteria — RAK4631 Gate 5: LoRaWAN OTAA Join + Modbus Uplink

**Gate:** `gate_rak4631_lorawan_join_uplink` v1.0
**Gate ID:** 5
**Date:** 2026-02-24
**Hardware Profile:** RAK4631 (nRF52840 + SX1262)

---

## 1. Overview

Gate 5 validates end-to-end LoRaWAN uplink from sensor to network server:
- OTAA join to ChirpStack v4 gateway on AS923-1
- Modbus RTU read (2 registers from wind speed sensor)
- 4-byte payload encoding (big-endian register values)
- Unconfirmed uplink on port 10

Reuses Gate 3's modbus_frame and hal_uart modules.

### RAK4631-Specific Notes
- SX1262 init via `lora_rak4630_init()` — single call, no hw_config struct
- Requires `-DRAK4630` and `-D_VARIANT_RAK4630_` build flags
- RAK5802 auto DE/RE — no manual GPIO toggling
- Serial1.begin(baud, config) — no pin args (variant.h)
- Do NOT call Serial1.end() — hangs on nRF52840

---

## 2. Pass Criteria

All 7 criteria must be met for this gate to PASS:

| # | Criterion | Validation Method | Expected Evidence |
|---|-----------|-------------------|-------------------|
| 1 | LoRaWAN init success | `lora_rak4630_init()` + `lmh_init()` return 0 | `lora_rak4630_init: OK` and `lmh_init: OK` |
| 2 | OTAA join success | Join accept callback within 30s | `Join Accept: OK (Nms)` |
| 3 | Modbus read success | 2-register FC 0x03 read, CRC valid | `Modbus Read: OK (2 registers)` |
| 4 | Payload encoded correctly | 4 bytes big-endian from register values | `Payload (4 bytes): XX XX YY YY` |
| 5 | Uplink sent successfully | `lmh_send()` returns LMH_SUCCESS | `Uplink TX: OK (port=10, 4 bytes)` |
| 6 | No system crash | Runner reaches GATE COMPLETE | `[GATE5-R4631] === GATE COMPLETE ===` |
| 7 | ChirpStack receives uplink | Check ChirpStack v4 device events | Uplink event with matching payload on port 10 |

**Note:** Criterion 7 requires manual verification in ChirpStack v4 web UI.

---

## 3. LoRaWAN Configuration

| Parameter | Value |
|-----------|-------|
| Region | AS923-1 |
| Activation | OTAA |
| Class | A |
| ADR | OFF |
| Data Rate | DR2 (SF10) |
| TX Power | TX_POWER_0 |
| Join Trials | 3 (LoRaMAC internal) |
| Join Timeout | 30s (polling) |
| Uplink Port | 10 |
| Confirmed | No (unconfirmed) |
| Duty Cycle | OFF |

## 4. OTAA Credentials

| Parameter | Value | Source |
|-----------|-------|--------|
| DevEUI | `88:82:24:44:AE:ED:1E:B2` | Gate 0.5 FICR read |
| AppEUI | `00:00:00:00:00:00:00:00` | ChirpStack v4 default |
| AppKey | `91:8C:49:08:F0:89:50:6B:30:18:0B:62:65:9A:4A:D5` | ChirpStack v4 provisioning |

---

## 5. Modbus Configuration

| Parameter | Value | Source |
|-----------|-------|--------|
| Slave Address | 1 | Gate 2 discovery |
| Baud Rate | 4800 | Gate 2 discovery |
| Parity | NONE (8N1) | Gate 2 discovery |
| Function Code | 0x03 | Modbus RTU standard |
| Start Register | 0x0000 | Configuration |
| Quantity | 2 | Configuration |
| Max Retries | 2 (3 total attempts) | Configuration |

---

## 6. Payload Format

4 bytes, big-endian:

```
Byte 0: Register[0] MSB  (wind speed high byte)
Byte 1: Register[0] LSB  (wind speed low byte)
Byte 2: Register[1] MSB  (wind direction high byte)
Byte 3: Register[1] LSB  (wind direction low byte)
```

Example: Reg[0]=0x0006, Reg[1]=0x0001 → Payload: `00 06 00 01`

---

## 7. Pass/Fail Codes

| Code | Constant | Meaning |
|------|----------|---------|
| 0 | `GATE_PASS` | Join + Modbus read + uplink all succeeded |
| 1 | `GATE_FAIL_RADIO_INIT` | lora_rak4630_init() or lmh_init() failed |
| 2 | `GATE_FAIL_JOIN_TIMEOUT` | OTAA join timed out (30s) |
| 3 | `GATE_FAIL_JOIN_DENIED` | Join failed callback received |
| 4 | `GATE_FAIL_MODBUS_INIT` | RS485/UART initialization failed |
| 5 | `GATE_FAIL_MODBUS_READ` | Modbus read failed (all retries exhausted) |
| 6 | `GATE_FAIL_UPLINK_TX` | lmh_send() returned error |
| 7 | `GATE_FAIL_SYSTEM_CRASH` | GATE COMPLETE not reached |

---

## 8. Expected Serial Output (PASS)

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
[GATE5-R4631]   RX (9 bytes): 01 03 04 00 06 00 01 DB F2
[GATE5-R4631]   Modbus Read: OK (2 registers)
[GATE5-R4631]   Register[0] : 0x0006 (6)
[GATE5-R4631]   Register[1] : 0x0001 (1)
[GATE5-R4631] STEP 7: Build uplink payload
[GATE5-R4631]   Payload (4 bytes): 00 06 00 01
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
[GATE5-R4631]   Register[0]  : 0x0006 (6)
[GATE5-R4631]   Register[1]  : 0x0001 (1)
[GATE5-R4631]   Payload Hex  : 00 06 00 01
[GATE5-R4631]   Uplink Port  : 10
[GATE5-R4631]   Uplink Status: OK

[GATE5-R4631] PASS
[GATE5-R4631] === GATE COMPLETE ===
```

---

## 9. RAK4631 vs RAK3312 — Gate 5 Differences

| Aspect | RAK3312 (ESP32-S3) | RAK4631 (nRF52840) |
|--------|--------------------|--------------------|
| SX1262 init | Manual `hw_config` + `lora_hardware_init()` | `lora_rak4630_init()` one-liner |
| SX1262 pins | Explicit in gate_config.h | Hardcoded in BSP function |
| Build flags | `-DCORE_RAK3312` | `-DCORE_RAK4631 -DRAK4630 -D_VARIANT_RAK4630_` |
| hal_uart_init() | 4 args (tx, rx, baud, parity) | 2 args (baud, parity) |
| hal_rs485_enable() | 2 args (en, de) | 1 arg (en) — auto DE/RE |
| Modbus cleanup | `hal_uart_deinit()` + `hal_rs485_disable()` | None (Serial1.end() hangs) |
| Serial output | `Serial.printf()` | `Serial.print()` / `Serial.println()` |
| Log tag | `[GATE5]` | `[GATE5-R4631]` |
| DevEUI | `AC:1F:09:FF:FE:24:02:14` | `88:82:24:44:AE:ED:1E:B2` |

---

## 10. ChirpStack Verification

After Gate 5 PASS, verify in ChirpStack v4:

1. Navigate to **Devices → [RAK4631 device] → Events**
2. Confirm **Join** event with DevEUI `88:82:24:44:AE:ED:1E:B2`
3. Confirm **Uplink** event on port 10 with 4-byte payload
4. Decode payload: bytes `00 06 00 01` = wind speed 6 (0.6 m/s), direction 1°

---

## 11. Scope

### In Scope
- OTAA join on AS923-1
- Modbus FC 0x03 multi-register read (qty=2)
- 4-byte payload encoding
- Unconfirmed uplink on port 10

### Out of Scope
- Downlink handling
- Confirmed uplinks
- ADR negotiation
- Multiple uplinks / periodic TX
- Production runtime scheduler
