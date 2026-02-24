# Test Report — RAK4631 Gate 5: LoRaWAN OTAA Join + Modbus Uplink

- **Status:** PASSED
- **Date:** 2026-02-25
- **Gate ID:** 5
- **Gate Name:** gate_rak4631_lorawan_join_uplink
- **Version:** 1.0
- **Hardware Profile:** v3.5 (RAK4631)
- **Build Environment:** `rak4631_gate5`
- **Tag:** `v3.5-gate5-pass-rak4631`

---

## Hardware Under Test

| Component | Model | Details |
|-----------|-------|---------|
| WisBlock Core | RAK4631 | nRF52840 @ 64 MHz + SX1262 |
| RS485 Module | RAK5802 | TP8485E transceiver, auto DE/RE, Slot A |
| Base Board | RAK19007 | USB-C, WisBlock standard |
| Modbus Sensor | RS-FSJT-N01 | Wind speed, slave 1, 4800 baud, 8N1 |
| LoRa Gateway | ChirpStack v4 | AS923-1, within range |

## Build Configuration

| Parameter | Value |
|-----------|-------|
| Platform | nordicnrf52 (10.10.0) |
| Board | wiscore_rak4631 |
| Framework | Arduino (Adafruit nRF52 BSP 1.7.0) |
| Toolchain | GCC ARM 7.2.1 |
| TinyUSB | 3.6.0 (framework built-in, symlinked) |
| SX126x-Arduino | 2.0.32 (beegee-tokyo) |
| RAM | 6.6% (16336 / 248832 bytes) |
| Flash | 17.6% (143484 / 815104 bytes) |
| Upload | nrfutil DFU via USB CDC |
| Build flags | `-DCORE_RAK4631 -DRAK4630 -D_VARIANT_RAK4630_` |

## Pass Criteria Results

| # | Criterion | Result | Evidence |
|---|-----------|--------|----------|
| 1 | LoRaWAN init success | PASS | `lora_rak4630_init: OK` and `lmh_init: OK` |
| 2 | OTAA join success | PASS | `Join Accept: OK (5777ms)` |
| 3 | Modbus read success | PASS | `RX (9 bytes): 01 03 04 00 04 00 01 7A 32` (CRC valid) |
| 4 | Payload encoded correctly | PASS | `Payload (4 bytes): 00 04 00 01` |
| 5 | Uplink sent successfully | PASS | `Uplink TX: OK (port=10, 4 bytes)` |
| 6 | No system crash | PASS | `[GATE5-R4631] === GATE COMPLETE ===` |
| 7 | ChirpStack receives uplink | PENDING | Manual verification required in ChirpStack v4 web UI |

## LoRaWAN Configuration

| Parameter | Value |
|-----------|-------|
| Region | AS923-1 (LORAMAC_REGION_AS923) |
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

## OTAA Credentials

| Parameter | Value | Source |
|-----------|-------|--------|
| DevEUI | `88:82:24:44:AE:ED:1E:B2` | Gate 0.5 FICR read |
| AppEUI | `00:00:00:00:00:00:00:00` | ChirpStack v4 default |
| AppKey | `91:8C:49:08:F0:89:50:6B:30:18:0B:62:65:9A:4A:D5` | ChirpStack v4 provisioning |

## LoRaWAN Join Validation

| Parameter | Expected | Actual | Result |
|-----------|----------|--------|--------|
| lora_rak4630_init() | Return 0 | 0 | PASS |
| lmh_init() | Return 0 | 0 | PASS |
| Join Accept | Within 30s | 5777ms | PASS |
| Join callback | g_joined == true | true | PASS |

## Modbus Protocol Trace

| Direction | Frame (hex) | Decoded |
|-----------|-------------|---------|
| TX | `01 03 00 00 00 02 C4 0B` | Slave=1, FC=0x03, Reg=0x0000, Qty=2, CRC=0x0BC4 |
| RX | `01 03 04 00 04 00 01 7A 32` | Slave=1, FC=0x03, Bytes=4, Reg[0]=0x0004, Reg[1]=0x0001, CRC=0x327A |

## Multi-Register Read Validation

| Check | Expected | Actual | Result |
|-------|----------|--------|--------|
| Quantity requested | 2 | 2 | PASS |
| Response length | 9 bytes | 9 bytes | PASS |
| byte_count field | 4 | 4 | PASS |
| byte_count == 2 * qty | 4 == 4 | Match | PASS |
| Register[0] extracted | Present | 0x0004 (4) | PASS |
| Register[1] extracted | Present | 0x0001 (1) | PASS |

## Payload Encoding Validation

| Byte | Content | Expected | Actual | Result |
|------|---------|----------|--------|--------|
| 0 | Reg[0] MSB | 0x00 | 0x00 | PASS |
| 1 | Reg[0] LSB | 0x04 | 0x04 | PASS |
| 2 | Reg[1] MSB | 0x00 | 0x00 | PASS |
| 3 | Reg[1] LSB | 0x01 | 0x01 | PASS |

Payload: `00 04 00 01` — wind speed 4 (0.4 m/s), wind direction 1 degree.

## Uplink Validation

| Parameter | Expected | Actual | Result |
|-----------|----------|--------|--------|
| lmh_send() | LMH_SUCCESS | LMH_SUCCESS | PASS |
| Port | 10 | 10 | PASS |
| Payload size | 4 bytes | 4 bytes | PASS |
| Confirmed | false | false | PASS |

## Serial Log

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
[GATE5-R4631]   Join Accept: OK (5777ms)
[GATE5-R4631] STEP 6: Modbus read (Slave=1, Baud=4800, FC=0x03, Reg=0x0000, Qty=2)
[GATE5-R4631]   RS485 EN=HIGH (auto DE/RE)
[GATE5-R4631]   TX (8 bytes): 01 03 00 00 00 02 C4 0B
[GATE5-R4631]   RX (9 bytes): 01 03 04 00 04 00 01 7A 32
[GATE5-R4631]   Modbus Read: OK (2 registers)
[GATE5-R4631]   Register[0] : 0x0004 (4)
[GATE5-R4631]   Register[1] : 0x0001 (1)
[GATE5-R4631] STEP 7: Build uplink payload
[GATE5-R4631]   Payload (4 bytes): 00 04 00 01
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
[GATE5-R4631]   Register[0]  : 0x0004 (4)
[GATE5-R4631]   Register[1]  : 0x0001 (1)
[GATE5-R4631]   Payload Hex  : 00 04 00 01
[GATE5-R4631]   Uplink Port  : 10
[GATE5-R4631]   Uplink Status: OK

[GATE5-R4631] PASS
[GATE5-R4631] === GATE COMPLETE ===
```

## RAK4631 vs RAK3312 — Gate 5 Differences

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
| Flash usage | ~17% (ESP32-S3) | 17.6% (nRF52840) |
| RAM usage | ~5% (ESP32-S3) | 6.6% (nRF52840) |

## Gate Progression (RAK4631)

| Gate | Name | Status | Tag |
|------|------|--------|-----|
| 0 | Toolchain Validation | PASSED | — |
| 0.5 | DevEUI Read | PASSED | — |
| 1A | I2C Scan | PASSED | — |
| 1B | I2C Identify 0x68/0x69 | PASSED | — |
| 1 | I2C LIS3DH Validation | PASSED | `v3.1-gate1-pass-rak4631` |
| 2-PROBE | RS485 Single Probe | PASSED | — |
| 2 | RS485 Modbus Autodiscovery | PASSED | `v3.2-gate2-pass-rak4631` |
| 3 | Modbus Minimal Protocol | PASSED | `v3.3-gate3-pass-rak4631` |
| 4 | Modbus Robustness Layer | PASSED | `v3.4-gate4-pass-rak4631` |
| 5 | LoRaWAN Join + Uplink | PASSED | `v3.5-gate5-pass-rak4631` |

## Files — Gate Test

| File | Path |
|------|------|
| gate_config.h | `tests/gates/gate_rak4631_lorawan_join_uplink/gate_config.h` |
| gate_runner.cpp | `tests/gates/gate_rak4631_lorawan_join_uplink/gate_runner.cpp` |
| pass_criteria.md | `tests/gates/gate_rak4631_lorawan_join_uplink/pass_criteria.md` |

## Files — Shared from Gate 3

| File | Path |
|------|------|
| modbus_frame.h | `tests/gates/gate_rak4631_modbus_minimal_protocol/modbus_frame.h` |
| modbus_frame.cpp | `tests/gates/gate_rak4631_modbus_minimal_protocol/modbus_frame.cpp` |
| hal_uart.h | `tests/gates/gate_rak4631_modbus_minimal_protocol/hal_uart.h` |
| hal_uart.cpp | `tests/gates/gate_rak4631_modbus_minimal_protocol/hal_uart.cpp` |

## Files — Example

| File | Path |
|------|------|
| main.cpp | `examples/rak4631/lorawan_join_modbus_uplink/main.cpp` |
| pin_map.h | `examples/rak4631/lorawan_join_modbus_uplink/pin_map.h` |
| wiring.md | `examples/rak4631/lorawan_join_modbus_uplink/wiring.md` |
| terminal_test.md | `examples/rak4631/lorawan_join_modbus_uplink/terminal_test.md` |
| prompt_test.md | `examples/rak4631/lorawan_join_modbus_uplink/prompt_test.md` |
| README.md | `examples/rak4631/lorawan_join_modbus_uplink/README.md` |

## ChirpStack Verification

**Status: PENDING** — Manual verification required.

After Gate 5 PASS, verify in ChirpStack v4:

1. Navigate to **Devices → [RAK4631 device] → Events**
2. Confirm **Join** event with DevEUI `88:82:24:44:AE:ED:1E:B2`
3. Confirm **Uplink** event on port 10 with 4-byte payload
4. Decode payload: `00 04 00 01` = wind speed 4 (0.4 m/s), direction 1 degree
