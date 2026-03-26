# Prompt Test — RAK4631 Gate 2: RS485 Modbus Autodiscovery

## Purpose

Verify that an AI coding assistant can build, flash, and validate Gate 2 on the RAK4631 using only these prompts.

---

## Prompt 1: Build Verification

> Build the Gate 2 environment (`rak4631_gate2`) and confirm it compiles with zero errors. Report RAM and Flash usage percentages.

**Expected outcome:**
- `rak4631_gate2 SUCCESS`
- RAM ~3.6%, Flash ~7.3%
- Zero errors, zero warnings

---

## Prompt 2: Flash and Capture

> Flash Gate 2 to the RAK4631 board. After flashing, wait 8 seconds for reboot, then open the serial port to capture output. The firmware waits for USB CDC DTR before printing. The scan may take up to 3 minutes if the device is at a high slave address.

**Expected outcome:**
- Upload succeeds via DFU (`Device programmed.`)
- Full gate output captured including banner, scan progress, and discovery result
- `[GATE2-R4631] PASS` in output

---

## Prompt 3: Validate RS485 Auto-Direction

> From the Gate 2 serial output, confirm that the RAK5802 auto-direction is being used. Is DE/RE being toggled manually via GPIO?

**Expected outcome:**
- `DE/RE: auto (RAK5802 hardware auto-direction)` in output
- No manual GPIO toggling — WB_IO1 (pin 17) is NC on RAK5802
- The TP8485E transceiver's DE/RE is driven by on-board circuit from UART TX line

---

## Prompt 4: Validate Discovery Result

> From the Gate 2 output, what Modbus device was discovered? Report the slave address, baud rate, parity, and the response frame in hex.

**Expected outcome:**
- Slave: 1
- Baud: 4800
- Parity: NONE
- TX: `01 03 00 00 00 01 84 0A`
- RX: `01 03 02 00 XX XX XX` (7 bytes, CRC valid)
- Response type: Normal (not exception)

---

## Prompt 5: Validate CRC

> Verify the CRC-16/MODBUS on the received response frame. What polynomial and initial value are used?

**Expected outcome:**
- Polynomial: 0xA001 (reflected)
- Initial value: 0xFFFF
- CRC in response matches computed CRC
- `CRC: 0xXXXX (valid)` in output

---

## Prompt 6: Verify Gate Completion

> Confirm the gate completed successfully by checking for the PASS marker and GATE COMPLETE line.

**Expected outcome:**
- `[GATE2-R4631] PASS` present
- `[GATE2-R4631] === GATE COMPLETE ===` present
- Blue LED blinked once (visual confirmation)
