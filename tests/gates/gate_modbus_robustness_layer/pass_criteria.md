# Pass Criteria — Gate 4: Modbus Robustness Layer Validation

**Gate:** `gate_modbus_robustness_layer` v1.0
**Gate ID:** 4
**Date:** 2026-02-24

---

## 1. Overview

Gate 4 validates Modbus RTU robustness features over the RS485 bus:
- Multi-register read (FC 0x03, quantity = 2 registers)
- Retry mechanism (max 3 retries after initial attempt)
- Error classification per attempt
- UART recovery (flush + re-init after 2 consecutive retryable failures)

Reuses Gate 3's modbus_frame (CRC, frame builder, 7-step parser) and hal_uart (Serial1 + RAK5802 RS485 control) without duplication.

---

## 2. Pass Criteria

All 8 criteria must be met for this gate to PASS:

| # | Criterion | Validation Method | Expected Evidence |
|---|-----------|-------------------|-------------------|
| 1 | UART initializes successfully | Serial1.begin() with discovered params | `[GATE4] UART Init: OK (4800 8N1)` |
| 2 | Multi-register request built correctly | FC 0x03, qty=2, 8-byte frame with valid CRC | `TX (8 bytes): 01 03 00 00 00 02 C4 0B` |
| 3 | Valid response received | Normal FC 0x03 response with valid CRC (first try or after retries) | `RX (9 bytes):` with CRC valid |
| 4 | byte_count == 2 * quantity | byte_count field matches 2 * GATE_MODBUS_QUANTITY | `byte_count=4, expected=4` |
| 5 | All register values extracted | Both Register[0] and Register[1] printed | `Register[0] : 0xNNNN` and `Register[1] : 0xNNNN` |
| 6 | Error classification logged per attempt | Error enum value printed after each attempt | `Error: NONE` (or TIMEOUT, CRC_FAIL, etc.) |
| 7 | Retry/recovery status logged | Final result line with attempt count, retry count, recovery count | `Result: SUCCESS on attempt N (M retries, K UART recoveries)` |
| 8 | No system crash | Runner reaches GATE COMPLETE | `[GATE4] === GATE COMPLETE ===` |

---

## 3. Error Classification

Each attempt is classified into exactly one error category:

| Error | Value | Meaning | Retryable? |
|-------|-------|---------|------------|
| `GATE_ERR_NONE` | 0 | Success | N/A |
| `GATE_ERR_TIMEOUT` | 1 | No response within 200ms | **Yes** |
| `GATE_ERR_CRC_FAIL` | 2 | Response CRC-16 mismatch | **Yes** |
| `GATE_ERR_LENGTH_FAIL` | 3 | Frame < 5 bytes or byte_count mismatch | No |
| `GATE_ERR_EXCEPTION` | 4 | Modbus exception response | No |
| `GATE_ERR_UART_ERROR` | 5 | UART write or re-init failed | No |

**Retry policy:** Only TIMEOUT and CRC_FAIL trigger retries. All other errors abort immediately.

---

## 4. Retry Mechanism

| Parameter | Value |
|-----------|-------|
| Max retries | 3 (after initial attempt = 4 total attempts) |
| Retry delay | 50ms |
| Flush between retries | Yes (hal_uart_flush) |
| UART recovery threshold | 2 consecutive retryable failures |
| UART recovery action | hal_uart_deinit → 50ms delay → hal_uart_init → flush → 50ms stabilize |

### Retry Flow

```
Attempt 1 (initial):
  TX → RX → classify → if retryable error: continue
                        if non-retryable: ABORT
                        if success: DONE

Retry 1 (attempt 2):
  delay 50ms → flush → TX → RX → classify → ...

Retry 2 (attempt 3):
  delay 50ms → flush
  [if 2 consecutive fails: UART re-init + reset counter]
  TX → RX → classify → ...

Retry 3 (attempt 4):
  delay 50ms → flush → TX → RX → classify → ...
```

---

## 5. Multi-Register Read

| Parameter | Value |
|-----------|-------|
| Function Code | 0x03 (Read Holding Registers) |
| Start Register | 0x0000 |
| Quantity | 2 registers |
| Expected byte_count | 4 (= 2 * 2) |
| Expected response length | 9 bytes: addr(1) + func(1) + byte_count(1) + data(4) + crc(2) |

### Response Frame Structure (qty=2)

```
01 03 04 XX XX YY YY CC CC
│  │  │  │     │     │
│  │  │  │     │     └── CRC-16 (2 bytes, LSB first)
│  │  │  │     └── Register[1] value (2 bytes, MSB first)
│  │  │  └── Register[0] value (2 bytes, MSB first)
│  │  └── byte_count = 4 (= 2 registers * 2 bytes)
│  └── Function Code 0x03 (echo)
└── Slave Address 1 (echo)
```

### Validation

1. Parser step 6: `data[2] == (len - 5)` → `4 == 9 - 5 = 4` (consistent)
2. Runner check: `byte_count == 2 * GATE_MODBUS_QUANTITY` → `4 == 2 * 2 = 4` (consistent)
3. Value extraction: `Register[0] = raw[3..4]`, `Register[1] = raw[5..6]`

---

## 6. CRC Re-Check for Error Classification

When the 7-step parser returns `valid=false`, the gate runner must determine the specific cause. Since `frame length >= 5` is checked first:

1. **If frame >= 5 bytes:** Re-compute CRC-16 on `rx_buf[0..len-3]`, compare with `rx_buf[len-2..len-1]`
   - Mismatch → `GATE_ERR_CRC_FAIL` (retryable)
   - Match → other validation failure → `GATE_ERR_LENGTH_FAIL` (non-retryable)

2. **If frame < 5 bytes:** Classified as `GATE_ERR_LENGTH_FAIL` without CRC check (non-retryable)

This ensures CRC re-check is only attempted when the frame is long enough to contain a CRC.

---

## 7. Pass/Fail Codes

| Code | Constant | Meaning |
|------|----------|---------|
| 0 | `GATE_PASS` | Multi-register read succeeded |
| 1 | `GATE_FAIL_UART_INIT` | Initial Serial1.begin() failed |
| 2 | `GATE_FAIL_TX_ERROR` | hal_uart_write() failed |
| 3 | `GATE_FAIL_NO_RESPONSE` | All retries exhausted — all timeouts |
| 4 | `GATE_FAIL_CRC_ERROR` | All retries exhausted — all CRC failures |
| 5 | `GATE_FAIL_ADDR_MISMATCH` | Slave address mismatch (non-retryable) |
| 6 | `GATE_FAIL_SHORT_FRAME` | Response < 5 bytes or validation error |
| 7 | `GATE_FAIL_SYSTEM_CRASH` | GATE COMPLETE not reached |
| 8 | `GATE_FAIL_BYTE_COUNT` | byte_count != 2 * quantity |
| 9 | `GATE_FAIL_RETRIES_EXHAUSTED` | Mixed retryable errors across attempts |
| 10 | `GATE_FAIL_UART_RECOVERY` | UART re-init failed during recovery |
| 11 | `GATE_FAIL_EXCEPTION` | Modbus exception (non-retryable) |

---

## 8. Expected Serial Output

### PASS — First-Try Success

```
========================================
[SYSTEM] WisBlock Gate Test Runner
[SYSTEM] Core: RAK3312 (ESP32-S3)
[SYSTEM] Gate: 4 - Modbus Robustness Layer
========================================

[GATE4] === GATE START: gate_modbus_robustness_layer v1.0 ===
[GATE4] STEP 1: UART + RS485 init (Slave=1, Baud=4800, Parity=NONE)
[GATE4] RS485 EN=HIGH (GPIO14), DE configured (GPIO21)
[GATE4] UART Init: OK (4800 8N1)
[GATE4] STEP 2: Multi-register read (FC=0x03, Reg=0x0000, Qty=2, MaxRetries=3)
[GATE4] Attempt 1/4:
[GATE4]   TX (8 bytes): 01 03 00 00 00 02 C4 0B
[GATE4]   RX (9 bytes): 01 03 04 00 06 00 00 XX XX
[GATE4]   Parse: OK (normal, byte_count=4, expected=4)
[GATE4]   Error: NONE
[GATE4] Result: SUCCESS on attempt 1 (0 retries, 0 UART recoveries)
[GATE4] STEP 3: Robustness validation summary
[GATE4]   Target Slave   : 1
[GATE4]   Baud Rate      : 4800
[GATE4]   Frame Config   : 8N1
[GATE4]   Function Code  : 0x03 (Read Holding Registers)
[GATE4]   Registers      : 0x0000, Qty=2
[GATE4]   Attempts       : 1
[GATE4]   Retries Used   : 0
[GATE4]   Last Error     : NONE
[GATE4]   UART Recoveries: 0
[GATE4]   byte_count     : 4 (expected: 4)
[GATE4]   Register[0]    : 0x0006 (6)
[GATE4]   Register[1]    : 0x0000 (0)
[GATE4]   Response Len   : 9 bytes
[GATE4]   Response Hex   : 01 03 04 00 06 00 00 XX XX
[GATE4] PASS
[GATE4] === GATE COMPLETE ===
```

### PASS — With Retries and UART Recovery

```
[GATE4] Attempt 1/4:
[GATE4]   TX (8 bytes): 01 03 00 00 00 02 C4 0B
[GATE4]   RX: timeout (200ms)
[GATE4]   Error: TIMEOUT
[GATE4] Retry 1/3 (delay=50ms, flush)
[GATE4] Attempt 2/4:
[GATE4]   TX (8 bytes): 01 03 00 00 00 02 C4 0B
[GATE4]   RX: timeout (200ms)
[GATE4]   Error: TIMEOUT
[GATE4] Retry 2/3 (delay=50ms, flush, UART recovery: re-init after 2 consecutive failures)
[GATE4] UART re-init: OK (4800 8N1)
[GATE4] Attempt 3/4:
[GATE4]   TX (8 bytes): 01 03 00 00 00 02 C4 0B
[GATE4]   RX (9 bytes): 01 03 04 00 06 00 00 XX XX
[GATE4]   Parse: OK (normal, byte_count=4, expected=4)
[GATE4]   Error: NONE
[GATE4] Result: SUCCESS on attempt 3 (2 retries, 1 UART recovery)
```

### FAIL — All Retries Exhausted (Timeout)

```
[GATE4] Attempt 1/4:
[GATE4]   TX (8 bytes): 01 03 00 00 00 02 C4 0B
[GATE4]   RX: timeout (200ms)
[GATE4]   Error: TIMEOUT
[GATE4] Retry 1/3 (delay=50ms, flush)
[GATE4] Attempt 2/4:
[GATE4]   TX (8 bytes): 01 03 00 00 00 02 C4 0B
[GATE4]   RX: timeout (200ms)
[GATE4]   Error: TIMEOUT
[GATE4] Retry 2/3 (delay=50ms, flush, UART recovery: re-init after 2 consecutive failures)
[GATE4] UART re-init: OK (4800 8N1)
[GATE4] Attempt 3/4:
[GATE4]   TX (8 bytes): 01 03 00 00 00 02 C4 0B
[GATE4]   RX: timeout (200ms)
[GATE4]   Error: TIMEOUT
[GATE4] Retry 3/3 (delay=50ms, flush)
[GATE4] Attempt 4/4:
[GATE4]   TX (8 bytes): 01 03 00 00 00 02 C4 0B
[GATE4]   RX: timeout (200ms)
[GATE4]   Error: TIMEOUT
[GATE4] Result: FAILED after 4 attempts (last error: TIMEOUT)
[GATE4] FAIL (code=3)
[GATE4] === GATE COMPLETE ===
```

### FAIL — Modbus Exception (Non-Retryable)

```
[GATE4] Attempt 1/4:
[GATE4]   TX (8 bytes): 01 03 00 00 00 02 C4 0B
[GATE4]   RX (5 bytes): 01 83 02 C0 F1
[GATE4]   Parse: Exception (code=0x02)
[GATE4]   Error: EXCEPTION
[GATE4] FAIL (code=11)
[GATE4] === GATE COMPLETE ===
```

---

## 9. Parameters

| Parameter | Value | Source |
|-----------|-------|--------|
| Slave Address | 1 | Gate 2 discovery |
| Baud Rate | 4800 | Gate 2 discovery |
| Parity | NONE (8N1) | Gate 2 discovery |
| Function Code | 0x03 | Modbus RTU standard |
| Start Register | 0x0000 | Configuration |
| Quantity | 2 | Configuration |
| Response Timeout | 200ms | Configuration |
| Max Retries | 3 | Configuration |
| Retry Delay | 50ms | Configuration |
| UART Recovery Threshold | 2 consecutive failures | Configuration |
| Slave Device | RS-FSJT-N01 wind speed sensor | Hardware setup |

---

## 10. Scope

### In Scope
- Multi-register FC 0x03 read with configurable quantity
- Retry loop with error classification
- UART flush between retries
- UART re-init recovery after consecutive failures
- byte_count == 2 * quantity validation

### Out of Scope
- Full Modbus library implementation
- Write function codes (FC 0x06, 0x10)
- Multi-slave communication
- LoRa stack integration
- Production error handling / watchdog
