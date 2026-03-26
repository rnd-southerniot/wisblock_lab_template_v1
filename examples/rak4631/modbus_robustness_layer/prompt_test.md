# Prompt Test — RAK4631 Gate 4: Modbus Robustness Layer

## Purpose

Verify that an AI coding assistant can build, flash, and validate Gate 4 on the RAK4631 using only these prompts.

---

## Prompt 1: Build Verification

> Build the Gate 4 environment (`rak4631_gate4`) and confirm it compiles with zero errors. Report RAM and Flash usage.

**Expected outcome:**
- `rak4631_gate4 SUCCESS`
- RAM ~3.6%, Flash ~7.7%
- Zero errors, zero warnings (or only unused-function warnings)

---

## Prompt 2: Flash and Capture

> Flash Gate 4 to the RAK4631 board. After flashing, wait 8 seconds for reboot, then open the serial port to capture output. The gate completes in about 1–2 seconds.

**Expected outcome:**
- Upload succeeds via DFU
- Full 3-step output captured
- `[GATE4-R4631] PASS` in output

---

## Prompt 3: Validate Multi-Register Read

> From the Gate 4 serial output, confirm the multi-register read succeeded. What were the two register values? Was the byte_count consistent with the quantity?

**Expected outcome:**
- TX: `01 03 00 00 00 02 C4 0B` (qty=2)
- RX: 9 bytes with byte_count=4 (= 2 * 2)
- Register[0] and Register[1] both extracted
- byte_count == 2 * quantity confirmed

---

## Prompt 4: Validate Error Classification

> From the Gate 4 output, what error classification was logged for each attempt? How many retries were needed?

**Expected outcome:**
- Attempt 1: `Error: NONE` (success on first try)
- 0 retries, 0 UART recoveries
- Result: `SUCCESS on attempt 1`

---

## Prompt 5: Validate Retry Mechanism

> Describe the retry mechanism implemented in Gate 4. What errors are retryable? What triggers UART recovery?

**Expected outcome:**
- Max 3 retries (4 total attempts)
- Retryable: TIMEOUT and CRC_FAIL only
- Non-retryable: LENGTH_FAIL, EXCEPTION, UART_ERROR — abort immediately
- UART recovery: triggered after 2 consecutive retryable failures
- Recovery: call hal_uart_init() again (Serial1.begin()), no Serial1.end()
- 50ms delay between retries

---

## Prompt 6: Compare with RAK3312 Gate 4

> How does the RAK4631 Gate 4 differ from the RAK3312 Gate 4 in terms of UART HAL, RS485 control, and UART recovery?

**Expected outcome:**
- RAK3312: `hal_uart_init(tx, rx, baud, parity)` — 4 args
- RAK4631: `hal_uart_init(baud, parity)` — 2 args (pins fixed by variant.h)
- RAK3312: Manual DE/RE toggling via WB_IO1 (GPIO 21)
- RAK4631: Auto DE/RE (RAK5802 hardware circuit, WB_IO1 is NC)
- RAK3312: UART recovery calls `hal_uart_deinit()` then `hal_uart_init()`
- RAK4631: UART recovery calls `hal_uart_init()` only (Serial1.end() hangs)
- RAK3312: Serial.printf() for formatted output
- RAK4631: Serial.print() / Serial.println() only
