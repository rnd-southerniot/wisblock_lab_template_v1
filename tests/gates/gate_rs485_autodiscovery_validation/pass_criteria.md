# Gate 2 — RS485 Modbus Autodiscovery Validation

**Gate ID:** 2
**Gate Name:** `gate_rs485_autodiscovery_validation`
**Version:** 1.0
**Date:** 2026-02-23
**Hardware Profile:** v2.0

---

## Purpose

Validate that the RAK5802 RS485 transceiver on the I/O Slot can successfully autodiscover a Modbus RTU slave device by scanning through all valid combinations of baud rate, parity, and slave address. Confirms RS485 bus integrity, UART functionality, Modbus RTU frame construction, CRC-16 validation, and device communication before any runtime Modbus integration begins.

---

## PASS Criteria

All seven criteria must be met for this gate to PASS:

| # | Criterion | Validation Method | Expected Result |
|---|-----------|-------------------|-----------------|
| 1 | UART initializes successfully | `Serial1.begin()` completes at 9600/8N1 | No UART init error |
| 2 | Scan matrix executes without bus error | All 12 baud/parity combinations configure without fault | Zero bus errors |
| 3 | Valid Modbus response received | At least one probe returns data with valid CRC-16 | CRC match on response |
| 4 | Device discovered | Response slave address and function code validated (or exception accepted) | Slave addr + func code match |
| 5 | Discovery summary printed | Baud rate, parity, slave ID, and response hex logged | Summary log lines present |
| 6 | No system crash | Gate runner reaches `GATE COMPLETE` log line | Log line present |
| 7 | Response frame >= 5 bytes | Received frame length checked before CRC/addr validation | No false positives from short/noise frames |

### Criterion #4 — Exception Handling

A Modbus exception response (`function_code | 0x80`) with valid CRC is explicitly treated as a successful discovery. The exception confirms device presence and correct baud/parity; it only indicates the specific register/function is unsupported by the slave.

### Criterion #7 — Short Frame Rejection

Any received data shorter than 5 bytes (minimum: addr + func + 1 data byte + 2 CRC) is discarded as noise or partial frame. This prevents false positives from bus echo, electrical noise, or incomplete transmissions.

---

## FAIL Codes

| Code | Constant | Meaning |
|------|----------|---------|
| 0 | `GATE_PASS` | All criteria met — device discovered |
| 1 | `GATE_FAIL_UART_INIT` | UART (Serial1) failed to initialize |
| 2 | `GATE_FAIL_NO_DEVICE` | Full scan complete, no valid Modbus response received |
| 3 | `GATE_FAIL_CRC_ERROR` | Response received but CRC validation failed on all attempts |
| 4 | `GATE_FAIL_BUS_ERROR` | UART TX/RX fault or UART reconfiguration failure |
| 5 | `GATE_FAIL_TIMEOUT` | Overall gate timeout exceeded (300 seconds) |
| 6 | `GATE_FAIL_SYSTEM_CRASH` | Runner did not reach GATE COMPLETE |
| 7 | `GATE_FAIL_SHORT_FRAME` | Response received but < 5 bytes (false positive rejected) |

---

## Expected Serial Log Format

### PASS Example (Device Found at 9600/Even/Slave 1)

```
========================================
[SYSTEM] WisBlock Gate Test Runner
[SYSTEM] Core: RAK3312 (ESP32-S3)
[SYSTEM] Gate: 2 - RS485 Modbus Autodiscovery Validation
========================================

[GATE2] === GATE START: gate_rs485_autodiscovery_validation v1.0 ===
[GATE2] STEP 1: UART init (TX=43, RX=44)
[GATE2] UART Init: OK
[GATE2] STEP 2: RS485 autodiscovery scan
[GATE2] Scan: 4 bauds x 3 parity x 100 addrs = 1200 probes
[GATE2] Config [1/12]: 4800 8N1
[GATE2] Config: 4800 8N1 Slave 1 | TX: 01 03 00 00 00 01 84 0A
[GATE2] Config: 4800 8N1 Slave 2 | TX: 02 03 00 00 00 01 84 39
...
[GATE2] Progress: 100/1200 probes (21000 ms)
[GATE2] Config [2/12]: 4800 8O1
...
[GATE2] Progress: 200/1200 probes (42000 ms)
[GATE2] Config [3/12]: 4800 8E1
...
[GATE2] Progress: 300/1200 probes (63000 ms)
[GATE2] Config [4/12]: 9600 8N1
[GATE2] Config: 9600 8N1 Slave 1 | TX: 01 03 00 00 00 01 84 0A
[GATE2] RX (7 bytes): 01 03 02 00 64 B8 44
[GATE2] FOUND: Slave=1 Baud=9600 Parity=NONE
[GATE2] Scan done: 301 probes in 64200 ms
[GATE2] STEP 3: Discovery summary
[GATE2]   Slave Address : 1
[GATE2]   Baud Rate     : 9600
[GATE2]   Parity        : NONE
[GATE2]   Frame Config  : 8N1
[GATE2]   Response Type : Normal
[GATE2]   Response Len  : 7 bytes
[GATE2]   Response Hex  : 01 03 02 00 64 B8 44
[GATE2] PASS
[GATE2] === GATE COMPLETE ===
```

### PASS Example (Exception Response at 19200/Even/Slave 5)

```
[GATE2] Config: 19200 8E1 Slave 5 | TX: 05 03 00 00 00 01 85 8E
[GATE2] RX (5 bytes): 05 83 02 D1 70
[GATE2] FOUND: Slave=5 Baud=19200 Parity=EVEN (exception code=0x02)
[GATE2] Scan done: 705 probes in 148500 ms
[GATE2] STEP 3: Discovery summary
[GATE2]   Slave Address : 5
[GATE2]   Baud Rate     : 19200
[GATE2]   Parity        : EVEN
[GATE2]   Frame Config  : 8E1
[GATE2]   Response Type : Exception
[GATE2]   Exception Code: 0x02
[GATE2]   Response Len  : 5 bytes
[GATE2]   Response Hex  : 05 83 02 D1 70
[GATE2] PASS
[GATE2] === GATE COMPLETE ===
```

### FAIL Example (No Device Found)

```
========================================
[SYSTEM] WisBlock Gate Test Runner
[SYSTEM] Core: RAK3312 (ESP32-S3)
[SYSTEM] Gate: 2 - RS485 Modbus Autodiscovery Validation
========================================

[GATE2] === GATE START: gate_rs485_autodiscovery_validation v1.0 ===
[GATE2] STEP 1: UART init (TX=43, RX=44)
[GATE2] UART Init: OK
[GATE2] STEP 2: RS485 autodiscovery scan
[GATE2] Scan: 4 bauds x 3 parity x 100 addrs = 1200 probes
[GATE2] Config [1/12]: 4800 8N1
[GATE2] Config: 4800 8N1 Slave 1 | TX: 01 03 00 00 00 01 84 0A
...
[GATE2] Progress: 100/1200 probes (21000 ms)
...
[GATE2] Config [12/12]: 115200 8E1
...
[GATE2] Progress: 1200/1200 probes (248500 ms)
[GATE2] Scan done: 1200 probes in 248500 ms
[GATE2] No Modbus device discovered
[GATE2] FAIL (code=2)
[GATE2] === GATE COMPLETE ===
```

### FAIL Example (Short Frame Detected)

```
[GATE2] Config: 9600 8N1 Slave 3 | TX: 03 03 00 00 00 01 85 E8
[GATE2] RX (2 bytes): FF FF
[GATE2] RX short frame (2 < 5), skipping
```

---

## Log Tag Reference

| Tag | Meaning |
|-----|---------|
| `[GATE2]` | Gate identifier prefix |
| `[STEP]` | Step is starting (used in step function logs) |
| `[PASS]` | Step or gate passed |
| `[FAIL]` | Step or gate failed |
| `[INFO]` | Informational (progress, config changes, discovery data) |

---

## Parameters

| Parameter | Value | Source |
|-----------|-------|--------|
| UART TX Pin | GPIO 43 | `hardware_profile.h` -> `HW_UART1_TX_PIN` |
| UART RX Pin | GPIO 44 | `hardware_profile.h` -> `HW_UART1_RX_PIN` |
| Data Bits | 8 | `hardware_profile.h` -> `HW_RS485_DATA_BITS` |
| Stop Bits | 1 | `hardware_profile.h` -> `HW_RS485_STOP_BITS` |
| Baud Rates | 4800, 9600, 19200, 115200 | `hardware_profile.h` -> `HW_RS485_BAUD_RATES` |
| Parity Options | None, Odd, Even | `hardware_profile.h` -> `HW_RS485_PARITY_OPTIONS` |
| Slave Address Range | 1 - 100 | User requirement (HW profile allows 1-247) |
| Modbus Function Code | 0x03 (Read Holding Registers) | Modbus RTU standard |
| Target Register | 0x0000 | Gate requirement |
| Quantity | 1 register | Gate requirement |
| Per-attempt Timeout | 200 ms | User requirement |
| UART Stabilization Delay | 50 ms | After baud/parity reconfiguration |
| Flush Delay | 10 ms | Between flush and next operation |
| Inter-byte Timeout | 5 ms | Modbus RTU frame delimiter detection |
| Overall Gate Timeout | 300,000 ms (5 min) | Calculated: 1200 probes x 200ms + margin |
| CRC Polynomial | 0xA001 (reflected) | CRC-16/MODBUS standard |
| CRC Init Value | 0xFFFF | CRC-16/MODBUS standard |
| Min Response Length | 5 bytes | addr(1) + func(1) + data(1) + crc(2) |

---

## Scope Boundaries

- This gate validates **RS485 bus integrity, UART functionality, and Modbus device discovery only**.
- No full Modbus RTU library is implemented.
- CRC-16/MODBUS is computed bitwise (no lookup table).
- RAK5802 DE/RE direction control is automatic (no GPIO management required).
- No LoRaWAN interaction.
- No I2C interaction.
- No power management logic.
- No runtime integration — gate is self-contained in `tests/gates/`.
