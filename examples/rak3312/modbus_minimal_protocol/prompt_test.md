# Prompt Test — Modbus Minimal Protocol

## Purpose

Verify Gate 3 functionality using AI-assisted testing prompts. Each prompt can be given to an AI assistant with access to the PlatformIO CLI to validate a specific aspect of the gate.

---

## Prompt 1: Build Verification

> Build the Gate 3 environment (`rak3312_gate3`) and confirm it compiles with zero errors. Report RAM and Flash usage percentages.

**Expected outcome:**
- `SUCCESS` with 0 errors
- RAM ~5.8%, Flash ~8.5%

---

## Prompt 2: Flash and Capture

> Flash the `rak3312_gate3` firmware to the RAK3312 and capture the serial output. Wait for `GATE COMPLETE` or 10 seconds, whichever comes first.

**Expected outcome:**
- Upload succeeds to `/dev/cu.usbmodem211401`
- Serial output contains `[GATE3] === GATE START` through `[GATE3] === GATE COMPLETE ===`

---

## Prompt 3: Parse Validation

> From the captured serial output, verify:
> 1. TX frame is exactly `01 03 00 00 00 01 84 0A`
> 2. RX frame has 7 bytes
> 3. RX CRC is valid (compute CRC-16/MODBUS on bytes 0-4, compare with bytes 5-6)
> 4. byte_count field (byte 2 of response) equals frame_length - 5
> 5. The structured output line matches `[GATE3] Slave=1 Value=0xNNNN`

**Expected outcome:**
- TX frame matches (CRC of `01 03 00 00 00 01` = `0x0A84`, sent as `84 0A`)
- RX CRC validates
- byte_count = 2, frame_length - 5 = 2 (consistent)
- Structured output present

---

## Prompt 4: Exception Path Verification

> Modify `gate_config.h` to set `GATE_MODBUS_TARGET_REG_HI` to `0xFF` and `GATE_MODBUS_TARGET_REG_LO` to `0xFF` (invalid register). Build, flash, and capture output. Verify the gate still PASSES with an exception response.

**Expected outcome:**
- Response shows `Exception Code=0x02` (illegal data address)
- Gate still prints `[GATE3] PASS` (exceptions are valid responses)

**Cleanup:** Restore register to `0x00`/`0x00` after test.

---

## Prompt 5: No-Device Failure

> Disconnect the RS485 slave device. Flash and capture output. Verify the gate reports FAIL code 3 (no response).

**Expected outcome:**
- `[GATE3] RX: timeout (no response in 200 ms)`
- `[GATE3] FAIL (code=3)`
- `[GATE3] === GATE COMPLETE ===` (no crash)

---

## Prompt 6: Full PASS Criteria Audit

> Review the captured serial output against all 7 pass criteria in `tests/gates/gate_modbus_minimal_protocol/pass_criteria.md`. For each criterion, quote the evidence line from the serial log.

**Expected outcome:**

| # | Criterion | Evidence |
|---|-----------|----------|
| 1 | UART initializes | `UART Init: OK (4800 8N1)` |
| 2 | Frame built correctly | `TX (8 bytes): 01 03 00 00 00 01 84 0A` |
| 3 | Valid response received | `RX (7 bytes): 01 03 02 ...` with valid CRC |
| 4 | Slave address validated | Response byte 0 = `01` matches slave 1 |
| 5 | byte_count consistency | `byte_count : 2` and frame len 7 - 5 = 2 |
| 6 | Structured output printed | `Slave=1 Value=0xNNNN` |
| 7 | No system crash | `=== GATE COMPLETE ===` |
