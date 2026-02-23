# Gate 3 — Modbus Minimal Protocol Validation

**Gate ID:** 3
**Gate Name:** `gate_modbus_minimal_protocol`
**Version:** 1.1
**Date:** 2026-02-24
**Hardware Profile:** v2.0
**Prerequisite:** Gate 2 PASS (RS485 Modbus Autodiscovery)

---

## Purpose

Validate the Modbus RTU protocol layer by sending a single Read Holding Register (FC 0x03) request to a previously discovered slave device. Confirms frame construction, CRC-16 calculation, response parsing, byte_count consistency, register value extraction, and exception handling. Uses the modbus_frame module as a reusable Modbus frame layer separate from the gate runner.

This gate does NOT re-discover the device — it uses parameters from gate_config.h (`GATE_DISCOVERED_*` defines), populated from Gate 2's successful discovery.

---

## PASS Criteria

All seven criteria must be met for this gate to PASS:

| # | Criterion | Validation Method | Expected Result |
|---|-----------|-------------------|-----------------|
| 1 | UART initializes at discovered params | `Serial1.begin()` with `GATE_DISCOVERED_BAUD`/`GATE_DISCOVERED_PARITY` | No UART init error |
| 2 | Modbus request frame built correctly | 8-byte frame with valid CRC-16 | Frame matches expected hex |
| 3 | Valid response received | Response >= 5 bytes with correct CRC-16 | CRC match |
| 4 | Slave address validated | `resp[0] == GATE_DISCOVERED_SLAVE` | Address match |
| 5 | byte_count consistency | `resp[2] == (frame_len - 5)` for normal responses | Declared == actual |
| 6 | Structured output printed | `[GATE3] Slave=N Value=0xNNNN` or `[GATE3] Exception Code=0xNN` | Log line present |
| 7 | No system crash | Gate runner reaches `GATE COMPLETE` log line | Log line present |

### Criterion #5 — byte_count Consistency

For normal FC 0x03 responses, the frame format is:
```
[addr, func, byte_count, data_0, data_1, ..., crc_lo, crc_hi]
```
Overhead = addr(1) + func(1) + byte_count(1) + crc(2) = 5 bytes.
Therefore `byte_count` (data[2]) must equal `frame_length - 5`.

For 1 register (quantity=1): `byte_count == 2` and `frame_length == 7`.

This check catches corrupted frames where the declared byte_count does not match the actual number of data bytes received.

**Not applicable to exception responses** — exception frames have a fixed structure `[addr, func|0x80, exc_code, crc_lo, crc_hi]` with no byte_count field.

### Exception Handling

A Modbus exception response (`function_code | 0x80`) with valid CRC is accepted as a valid parsed response. The `ModbusResponse.exception` flag is set true and `exc_code` is populated. This is not a gate failure — it confirms the protocol layer correctly identifies exception frames.

### Structured Output Format

The gate prints one of two structured output lines:

**Normal response:**
```
[GATE3] Slave=1 Value=0x0064
```

**Exception response:**
```
[GATE3] Exception Code=0x02
```

---

## Parser Validation Order

The `modbus_parse_response()` function validates in this exact order:

| Step | Check | Fail Behaviour |
|------|-------|----------------|
| 1 | Frame length >= 5 bytes | Return valid=false |
| 2 | CRC-16 match | Return valid=false |
| 3 | Slave address match | Return valid=false |
| 4 | Exception detection (func & 0x80) | Return valid=true, exception=true |
| 5 | Function code match | Return valid=false |
| 6 | byte_count == (len - 5) | Return valid=false |
| 7 | Register value extraction | Return valid=true |

Exception detection (step 4) is evaluated before byte_count (step 6) because exception frames have a different structure with no byte_count field.

---

## FAIL Codes

| Code | Constant | Meaning |
|------|----------|---------|
| 0 | `GATE_PASS` | All criteria met — protocol validated |
| 1 | `GATE_FAIL_UART_INIT` | UART (Serial1) failed to initialize |
| 2 | `GATE_FAIL_TX_ERROR` | hal_uart_write() failed |
| 3 | `GATE_FAIL_NO_RESPONSE` | Timeout — no data received from slave |
| 4 | `GATE_FAIL_CRC_ERROR` | Response received but CRC-16 mismatch |
| 5 | `GATE_FAIL_ADDR_MISMATCH` | Response slave address != expected |
| 6 | `GATE_FAIL_SHORT_FRAME` | Response < 5 bytes |
| 7 | `GATE_FAIL_SYSTEM_CRASH` | Runner did not reach GATE COMPLETE |
| 8 | `GATE_FAIL_BYTE_COUNT` | byte_count field inconsistent with frame length |

---

## Expected Serial Log Format

### PASS Example (Normal Response — Register Value)

```
========================================
[SYSTEM] WisBlock Gate Test Runner
[SYSTEM] Core: RAK3312 (ESP32-S3)
[SYSTEM] Gate: 3 - Modbus Minimal Protocol
========================================

[GATE3] === GATE START: gate_modbus_minimal_protocol v1.1 ===
[GATE3] STEP 1: UART + RS485 init (Slave=1, Baud=4800, Parity=NONE)
[GATE3] RS485 EN=HIGH (GPIO14), DE configured (GPIO21)
[GATE3] UART Init: OK (4800 8N1)
[GATE3] STEP 2: Modbus Read Holding Register (FC=0x03, Reg=0x0000, Qty=1)
[GATE3] TX (8 bytes): 01 03 00 00 00 01 84 0A
[GATE3] RX (7 bytes): 01 03 02 00 00 B8 44
[GATE3] Slave=1 Value=0x0000
[GATE3] STEP 3: Protocol validation summary
[GATE3]   Target Slave  : 1
[GATE3]   Baud Rate     : 4800
[GATE3]   Frame Config  : 8N1
[GATE3]   Function Code : 0x03 (Read Holding Registers)
[GATE3]   Register      : 0x0000
[GATE3]   Protocol      : Modbus RTU
[GATE3]   Response Type : Normal
[GATE3]   byte_count    : 2
[GATE3]   Register Value: 0x0000 (0)
[GATE3]   Response Len  : 7 bytes
[GATE3]   Response Hex  : 01 03 02 00 00 B8 44
[GATE3] PASS
[GATE3] === GATE COMPLETE ===
```

### PASS Example (Exception Response)

```
[GATE3] === GATE START: gate_modbus_minimal_protocol v1.1 ===
[GATE3] STEP 1: UART + RS485 init (Slave=1, Baud=4800, Parity=NONE)
[GATE3] RS485 EN=HIGH (GPIO14), DE configured (GPIO21)
[GATE3] UART Init: OK (4800 8N1)
[GATE3] STEP 2: Modbus Read Holding Register (FC=0x03, Reg=0x0000, Qty=1)
[GATE3] TX (8 bytes): 01 03 00 00 00 01 84 0A
[GATE3] RX (5 bytes): 01 83 02 C0 F1
[GATE3] Exception Code=0x02
[GATE3] STEP 3: Protocol validation summary
[GATE3]   Target Slave  : 1
[GATE3]   Baud Rate     : 4800
[GATE3]   Frame Config  : 8N1
[GATE3]   Function Code : 0x03 (Read Holding Registers)
[GATE3]   Register      : 0x0000
[GATE3]   Protocol      : Modbus RTU
[GATE3]   Response Type : Exception
[GATE3]   Exception Code: 0x02
[GATE3]   Response Len  : 5 bytes
[GATE3]   Response Hex  : 01 83 02 C0 F1
[GATE3] PASS
[GATE3] === GATE COMPLETE ===
```

### FAIL Example (No Response)

```
[GATE3] === GATE START: gate_modbus_minimal_protocol v1.1 ===
[GATE3] STEP 1: UART + RS485 init (Slave=1, Baud=4800, Parity=NONE)
[GATE3] RS485 EN=HIGH (GPIO14), DE configured (GPIO21)
[GATE3] UART Init: OK (4800 8N1)
[GATE3] STEP 2: Modbus Read Holding Register (FC=0x03, Reg=0x0000, Qty=1)
[GATE3] TX (8 bytes): 01 03 00 00 00 01 84 0A
[GATE3] RX: timeout (no response in 200 ms)
[GATE3] FAIL (code=3)
[GATE3] === GATE COMPLETE ===
```

### FAIL Example (byte_count Mismatch)

```
[GATE3] STEP 2: Modbus Read Holding Register (FC=0x03, Reg=0x0000, Qty=1)
[GATE3] TX (8 bytes): 01 03 00 00 00 01 84 0A
[GATE3] RX (7 bytes): 01 03 04 00 64 00 00
[GATE3] byte_count mismatch: declared=4 actual=2
[GATE3] FAIL (code=8)
[GATE3] === GATE COMPLETE ===
```

### FAIL Example (CRC Mismatch)

```
[GATE3] STEP 2: Modbus Read Holding Register (FC=0x03, Reg=0x0000, Qty=1)
[GATE3] TX (8 bytes): 01 03 00 00 00 01 84 0A
[GATE3] RX (7 bytes): 01 03 02 00 64 FF FF
[GATE3] CRC mismatch: calc=0xB844 recv=0xFFFF
[GATE3] FAIL (code=4)
[GATE3] === GATE COMPLETE ===
```

---

## Log Tag Reference

| Tag | Meaning |
|-----|---------|
| `[GATE3]` | Gate identifier prefix |
| `[STEP]` | Step is starting (embedded in step function logs) |
| `[PASS]` | Gate passed |
| `[FAIL]` | Gate failed |
| `[INFO]` | Informational (data, diagnostics) |

---

## Parameters

All device-specific parameters are defined in `gate_config.h` as `GATE_DISCOVERED_*` defines. Gate runner contains zero hardcoded device values.

| Parameter | Value | Source |
|-----------|-------|--------|
| UART TX Pin | GPIO 43 | `gate_config.h` -> `HW_UART1_TX_PIN` |
| UART RX Pin | GPIO 44 | `gate_config.h` -> `HW_UART1_RX_PIN` |
| RS485 EN Pin | GPIO 14 | `gate_config.h` -> `HW_WB_IO2_PIN` |
| RS485 DE Pin | GPIO 21 | `gate_config.h` -> `HW_WB_IO1_PIN` |
| Slave Address | `GATE_DISCOVERED_SLAVE` | `gate_config.h` (from Gate 2) |
| Baud Rate | `GATE_DISCOVERED_BAUD` | `gate_config.h` (from Gate 2) |
| Parity | `GATE_DISCOVERED_PARITY` | `gate_config.h` (from Gate 2) |
| Function Code | 0x03 | `gate_config.h` -> `GATE_MODBUS_FUNC_READ_HOLD` |
| Target Register | 0x0000 | `gate_config.h` -> `GATE_MODBUS_TARGET_REG_*` |
| Quantity | 1 register | `gate_config.h` -> `GATE_MODBUS_QUANTITY_*` |
| Response Timeout | 200 ms | `gate_config.h` -> `GATE_RESPONSE_TIMEOUT_MS` |
| UART Stabilization | 50 ms | `gate_config.h` -> `GATE_UART_STABILIZE_MS` |
| Inter-byte Timeout | 5 ms | `gate_config.h` -> `GATE_INTER_BYTE_TIMEOUT_MS` |
| Min Response Length | 5 bytes | `gate_config.h` -> `GATE_MODBUS_RESP_MIN_LEN` |
| CRC Polynomial | 0xA001 | `gate_config.h` -> `GATE_CRC_POLY` |
| CRC Init Value | 0xFFFF | `gate_config.h` -> `GATE_CRC_INIT` |

---

## Architecture Notes

### modbus_frame Module

Gate 3 introduces a **reusable modbus_frame module** (`modbus_frame.h`/`.cpp`) that separates the Modbus protocol layer from the gate runner logic:

| Component | File | Purpose |
|-----------|------|---------|
| `ModbusResponse` | modbus_frame.h | Structured result: valid, exception, slave, function, byte_count, value |
| `modbus_crc16()` | modbus_frame.cpp | Bitwise CRC-16/MODBUS calculation |
| `modbus_build_read_holding()` | modbus_frame.cpp | FC 0x03 request frame builder |
| `modbus_parse_response()` | modbus_frame.cpp | 7-step response validator + parser |

This module can be reused by future gates without modification.

### Configuration Separation

All discovered device parameters live exclusively in `gate_config.h`:
- `GATE_DISCOVERED_SLAVE` — slave address
- `GATE_DISCOVERED_BAUD` — baud rate
- `GATE_DISCOVERED_PARITY` — parity mode

Gate runner and modbus_frame reference only these `#define` macros. Changing the target device requires editing only `gate_config.h`.

### HAL Dependency

Gate 3 reuses the UART HAL from Gate 2 (`hal_uart.h`/`hal_uart.cpp`). The build environment must include Gate 2's HAL source files. During implementation, the `platformio.ini` build configuration will resolve this dependency.

---

## Scope Boundaries

- This gate validates **Modbus RTU protocol parsing only** using a known device.
- No bus scanning — uses `GATE_DISCOVERED_*` defines from gate_config.h.
- No retry logic — single request/response cycle.
- No register map exploration — reads only the configured register.
- CRC-16/MODBUS is computed bitwise (no lookup table).
- No LoRaWAN interaction.
- No I2C interaction.
- No power management logic.
- No runtime integration — gate is self-contained in `tests/gates/`.
- Does NOT modify `runtime/`, LoRa stack, or peripheral manager.
