# Gate 3 — RAK4631 Modbus Minimal Protocol Validation

**Gate ID:** 3
**Gate Name:** `gate_rak4631_modbus_minimal_protocol`
**Version:** 1.0
**Date:** 2026-02-24
**Hardware:** RAK4631 WisBlock Core (nRF52840 + SX1262)
**Prerequisite:** Gate 2 PASS (RS485 Modbus Autodiscovery)

---

## Purpose

Validate the Modbus RTU protocol layer by sending a single Read Holding Register
(FC 0x03) request to the previously discovered slave device. Confirms frame
construction, CRC-16 calculation, 7-step response parsing, byte_count consistency,
register value extraction, and exception handling.

Uses discovered parameters from Gate 2: Slave=1, Baud=4800, Parity=NONE (8N1).

---

## PASS Criteria

| # | Criterion | Expected |
|---|-----------|----------|
| 1 | Compile | `pio run -e rak4631_gate3` succeeds |
| 2 | UART Init | Serial1 at 4800 8N1, no init error |
| 3 | TX Frame | 8-byte Modbus request with valid CRC-16 |
| 4 | RX Response | Response >= 5 bytes with correct CRC-16 |
| 5 | Slave Match | `resp[0] == 1` |
| 6 | byte_count | `resp[2] == (frame_len - 5)` for normal response |
| 7 | Structured Output | `[GATE3-R4631] Slave=1 Value=0xNNNN` printed |
| 8 | Complete | `[GATE3-R4631] PASS` and `=== GATE COMPLETE ===` |

---

## Parser Validation Order (7-step)

| Step | Check | Fail Behaviour |
|------|-------|----------------|
| 1 | Frame length >= 5 bytes | Return valid=false |
| 2 | CRC-16 match | Return valid=false |
| 3 | Slave address match | Return valid=false |
| 4 | Exception detection (func & 0x80) | Return valid=true, exception=true |
| 5 | Function code match | Return valid=false |
| 6 | byte_count == (len - 5) | Return valid=false |
| 7 | Register value extraction | Return valid=true |

---

## FAIL Codes

| Code | Constant | Meaning |
|------|----------|---------|
| 0 | `GATE_PASS` | All criteria met |
| 1 | `GATE_FAIL_UART_INIT` | Serial1 failed to initialize |
| 2 | `GATE_FAIL_TX_ERROR` | hal_uart_write() failed |
| 3 | `GATE_FAIL_NO_RESPONSE` | Timeout — no data received |
| 4 | `GATE_FAIL_CRC_ERROR` | CRC-16 mismatch |
| 5 | `GATE_FAIL_ADDR_MISMATCH` | Slave address mismatch |
| 6 | `GATE_FAIL_SHORT_FRAME` | Response < 5 bytes |
| 7 | `GATE_FAIL_SYSTEM_CRASH` | Did not reach GATE COMPLETE |
| 8 | `GATE_FAIL_BYTE_COUNT` | byte_count inconsistent |

---

## Parameters

| Parameter | Value | Source |
|-----------|-------|--------|
| Serial1 TX | pin 16 (P0.16) | variant.h |
| Serial1 RX | pin 15 (P0.15) | variant.h |
| RS485 EN | pin 34 (WB_IO2) | gate_config.h |
| DE/RE | Auto (RAK5802 hardware) | — |
| Slave | 1 | Gate 2 discovery |
| Baud | 4800 | Gate 2 discovery |
| Parity | NONE (8N1) | Gate 2 discovery |
| Function | 0x03 | Read Holding Registers |
| Register | 0x0000 | gate_config.h |
| Quantity | 1 | gate_config.h |
| Timeout | 200 ms | gate_config.h |

---

## nRF52840 / RAK5802 Notes

- `Serial1.begin(baud, config)` — no pin args on nRF52
- Do NOT call `Serial1.end()` — hangs if not previously started
- `Serial1.flush()` works correctly — blocks until TX DMA complete
- RAK5802 auto DE/RE — no manual GPIO toggling on WB_IO1 (pin 17, NC)
- nRF52840 UARTE: NONE and EVEN parity only (no Odd)
