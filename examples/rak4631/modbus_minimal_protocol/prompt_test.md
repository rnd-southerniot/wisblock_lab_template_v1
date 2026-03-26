# Prompt Test — RAK4631 Gate 3: Modbus Minimal Protocol

## Purpose

Verify that an AI coding assistant can build, flash, and validate Gate 3 on the RAK4631 using only these prompts.

---

## Prompt 1: Build Verification

> Build the Gate 3 environment (`rak4631_gate3`) and confirm it compiles with zero errors. Report RAM and Flash usage.

**Expected outcome:**
- `rak4631_gate3 SUCCESS`
- RAM ~3.6%, Flash ~7.4%
- Zero errors, zero warnings

---

## Prompt 2: Flash and Capture

> Flash Gate 3 to the RAK4631 board. After flashing, wait 8 seconds for reboot, then open the serial port to capture output. The gate completes in about 1–2 seconds.

**Expected outcome:**
- Upload succeeds via DFU
- Full 3-step output captured
- `[GATE3-R4631] PASS` in output

---

## Prompt 3: Validate 7-Step Parser

> From the Gate 3 serial output, confirm the Modbus response was validated by the 7-step parser. List each step and its result.

**Expected outcome:**
1. Length >= 5: 7 bytes — OK
2. CRC match: calculated matches received — OK
3. Slave match: resp[0] == 1 — OK
4. Exception: NO (normal response)
5. Function match: 0x03 — OK
6. byte_count: declared=2 actual=2 — OK
7. Value extraction: 0x0004 — OK

---

## Prompt 4: Validate Protocol Summary

> From the Gate 3 output, extract the complete protocol validation summary. What register was read and what value was returned?

**Expected outcome:**
- Target Slave: 1
- Baud Rate: 4800
- Frame Config: 8N1
- Function Code: 0x03 (Read Holding Registers)
- Register: 0x0000
- byte_count: 2
- Register Value: 0x0004 (4)
- Response: Normal (not exception)

---

## Prompt 5: Compare with RAK3312 Gate 3

> How does the RAK4631 Gate 3 differ from the RAK3312 Gate 3 in terms of UART HAL and RS485 control?

**Expected outcome:**
- RAK3312: `Serial1.begin(baud, config, rx_pin, tx_pin)` — 4 args (ESP32-S3)
- RAK4631: `Serial1.begin(baud, config)` — 2 args (nRF52840, pins fixed by variant.h)
- RAK3312: Manual DE/RE toggling via WB_IO1 (GPIO 21)
- RAK4631: Auto DE/RE (RAK5802 hardware circuit, WB_IO1 is NC)
- RAK3312: `Serial1.end()` works normally
- RAK4631: `Serial1.end()` hangs — must skip it
- RAK3312: NONE, ODD, EVEN parity
- RAK4631: NONE, EVEN only (no Odd in nRF52840 UARTE)
