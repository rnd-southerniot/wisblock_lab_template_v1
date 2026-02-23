# Gate 5 — LoRaWAN Join + Uplink — Pass Criteria

## 9 Pass Criteria

| # | Criterion | Validation Method | Expected Evidence |
|---|-----------|-------------------|-------------------|
| 1 | LoRaWAN stack initializes | `lmh_init()` returns 0 | `[GATE5] LoRaWAN Init: OK` |
| 2 | AS923-1 region configured | Region logged | `[GATE5] Region: AS923-1 (Class A, ADR=OFF, DR=2)` |
| 3 | OTAA credentials set | DevEUI/AppEUI/AppKey logged | `[GATE5] OTAA credentials configured` |
| 4 | Join request sent | `lmh_join()` called | `[GATE5] Join Start...` |
| 5 | Join accepted by network | `lorawan_has_joined_handler` callback fires | `[GATE5] Join Accept: OK` |
| 6 | Modbus registers read | Valid FC 0x03 response with 2 register values | `[GATE5] Modbus Read: OK (2 registers)` |
| 7 | 4-byte payload built | 2 register values packed as big-endian bytes | `[GATE5] Payload (4 bytes): XX XX YY YY` |
| 8 | Unconfirmed uplink sent | `lmh_send()` returns 0 | `[GATE5] Uplink TX: OK (port=10, 4 bytes)` |
| 9 | No system crash | Runner reaches GATE COMPLETE | `[GATE5] === GATE COMPLETE ===` |

## Fail Code Table

| Code | Constant | Meaning |
|------|----------|---------|
| 0 | `GATE_PASS` | Join + Modbus read + uplink all succeeded |
| 1 | `GATE_FAIL_RADIO_INIT` | LoRaWAN stack init failed (lmh_init) |
| 2 | `GATE_FAIL_JOIN_TIMEOUT` | OTAA join timed out (callback not received within 30s) |
| 3 | `GATE_FAIL_JOIN_DENIED` | Join failed callback received (network rejected) |
| 4 | `GATE_FAIL_MODBUS_INIT` | RS485/UART initialization failed |
| 5 | `GATE_FAIL_MODBUS_READ` | Modbus read failed (all 3 attempts exhausted) |
| 6 | `GATE_FAIL_UPLINK_TX` | `lmh_send()` returned error |
| 7 | `GATE_FAIL_SYSTEM_CRASH` | Runner did not reach GATE COMPLETE |

## Expected Serial Output (PASS)

```
========================================
[SYSTEM] WisBlock Gate Test Runner
[SYSTEM] Core: RAK3312 (ESP32-S3)
[SYSTEM] Gate: 5 - LoRaWAN Join + Uplink
========================================

[GATE5] === GATE START: gate_lorawan_join_uplink v1.0 ===
[GATE5] STEP 1: LoRaWAN stack init
[GATE5] LoRaWAN Init: OK
[GATE5] STEP 2: Region configuration
[GATE5] Region: AS923-1 (Class A, ADR=OFF, DR=2)
[GATE5] STEP 3: Set OTAA credentials
[GATE5]   DevEUI : XX:XX:XX:XX:XX:XX:XX:XX
[GATE5]   AppEUI : XX:XX:XX:XX:XX:XX:XX:XX
[GATE5]   AppKey : XX:XX:XX:XX:XX:XX:XX:XX:XX:XX:XX:XX:XX:XX:XX:XX
[GATE5] OTAA credentials configured
[GATE5] STEP 4: OTAA join (trials=3, timeout=30s)
[GATE5] Join Start...
[GATE5] STEP 5: Waiting for join accept...
[GATE5] Join Accept: OK
[GATE5] STEP 6: Modbus read (Slave=1, Baud=4800, FC=0x03, Reg=0x0000, Qty=2)
[GATE5]   TX (8 bytes): 01 03 00 00 00 02 C4 0B
[GATE5]   RX (9 bytes): 01 03 04 XX XX YY YY CC CC
[GATE5] Modbus Read: OK (2 registers)
[GATE5]   Register[0] : 0xXXXX (N)
[GATE5]   Register[1] : 0xYYYY (N)
[GATE5] STEP 7: Build uplink payload
[GATE5] Payload (4 bytes): XX XX YY YY
[GATE5]   Port: 10
[GATE5]   Confirmed: false
[GATE5] STEP 8: Send uplink
[GATE5] Uplink TX...
[GATE5] Uplink TX: OK (port=10, 4 bytes)
[GATE5] STEP 9: Gate summary
[GATE5]   Radio        : SX1262
[GATE5]   Region       : AS923-1
[GATE5]   Activation   : OTAA
[GATE5]   Join Status  : OK
[GATE5]   Modbus Slave : 1
[GATE5]   Register[0]  : 0xXXXX (N)
[GATE5]   Register[1]  : 0xYYYY (N)
[GATE5]   Payload Hex  : XX XX YY YY
[GATE5]   Uplink Port  : 10
[GATE5]   Uplink Status: OK
[GATE5] PASS
[GATE5] === GATE COMPLETE ===
```

## Prerequisites

1. ChirpStack v4 device provisioned with matching DevEUI/AppEUI/AppKey
2. Actual OTAA credentials filled in `gate_config.h` (replacing placeholder zeros)
3. LoRaWAN gateway within range, connected to ChirpStack
4. Wind speed sensor (Modbus RTU, Slave=1, Baud=4800, 8N1) connected to RS485 bus

## Verification Checklist

- [ ] Serial log shows all 9 steps executing in order
- [ ] `[GATE5] PASS` printed (not `FAIL`)
- [ ] `[GATE5] === GATE COMPLETE ===` printed (no crash)
- [ ] Uplink appears in ChirpStack v4 device events (port 10, 4 bytes)
- [ ] Payload bytes match Modbus register values (big-endian)
