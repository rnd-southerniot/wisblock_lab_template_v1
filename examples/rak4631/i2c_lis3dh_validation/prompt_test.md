# Prompt Test — RAK4631 Gate 1: I2C LIS3DH Validation

## Purpose

Verify that an AI coding assistant can build, flash, and validate Gate 1 on the RAK4631 using only these prompts.

---

## Prompt 1: Build Verification

> Build the Gate 1 environment (`rak4631_gate1`) and confirm it compiles with zero errors. Report RAM and Flash usage percentages.

**Expected outcome:**
- `rak4631_gate1 SUCCESS`
- RAM ~3.7%, Flash ~7.1%
- Zero errors, zero warnings

---

## Prompt 2: Flash and Capture

> Flash Gate 1 to the RAK4631 board. After flashing, wait 8 seconds for reboot, then open the serial port to capture output. The firmware waits for USB CDC DTR before printing.

**Expected outcome:**
- Upload succeeds via DFU (`Device programmed.`)
- Full gate output captured including banner, WHO_AM_I, and 100 accel reads
- `[GATE1-R4631] PASS` in output

---

## Prompt 3: Validate WHO_AM_I

> Read the Gate 1 serial output and confirm the LIS3DH WHO_AM_I register returns the expected value. What address and register were read?

**Expected outcome:**
- Address: 0x18
- Register: 0x0F (WHO_AM_I)
- Value: 0x33 (LIS3DH confirmed)

---

## Prompt 4: Validate Accelerometer Data

> From the Gate 1 output, confirm that all 100 accelerometer reads succeeded. What is the Z-axis range? Is this consistent with 1g (board lying flat)?

**Expected outcome:**
- 100/100 reads passed, 0 failed
- Z range approximately 15000–17000 counts
- At ±2g full scale (12-bit left-justified in 16-bit): 1g ≈ 16384 counts
- Z ≈ 16000 confirms board is flat with gravity on Z-axis

---

## Prompt 5: Verify Gate Completion

> Confirm the gate completed successfully by checking for the PASS marker and GATE COMPLETE line in the serial output.

**Expected outcome:**
- `[GATE1-R4631] PASS` present
- `[GATE1-R4631] === GATE COMPLETE ===` present
- Blue LED blinked once (visual confirmation)
