# Prompt Test — RAK4631 Gate 5: LoRaWAN OTAA Join + Modbus Uplink

## Purpose

Verify that an AI coding assistant can build, flash, and validate Gate 5 on the RAK4631 using only these prompts.

---

## Prompt 1: Build Verification

> Build the Gate 5 environment (`rak4631_gate5`) and confirm it compiles with zero errors. Report RAM and Flash usage. What warning do you expect from the SX126x-Arduino library?

**Expected outcome:**
- `rak4631_gate5 SUCCESS`
- RAM ~6.6%, Flash ~17.6%
- Warning: `#warning USING RAK4630` from `RAK4630_MOD.cpp` (confirms `-DRAK4630` flag)
- Zero errors

---

## Prompt 2: Flash and Capture

> Flash Gate 5 to the RAK4631 board. After flashing, wait 8 seconds for reboot, then capture serial output. The OTAA join may take up to 30 seconds, and the full gate completes in about 40 seconds.

**Expected outcome:**
- Upload succeeds via nrfutil DFU
- Full 9-step output captured
- `[GATE5-R4631] PASS` in output
- Join Accept time reported in milliseconds

---

## Prompt 3: Validate LoRaWAN Join

> From the Gate 5 serial output, confirm the OTAA join succeeded. What was the join time? Which DevEUI was used? What region and data rate?

**Expected outcome:**
- `lora_rak4630_init: OK` and `lmh_init: OK`
- DevEUI: `88:82:24:44:AE:ED:1E:B2`
- Region: AS923-1, Class A, ADR=OFF, DR=2 (SF10)
- Join Accept within 30s (typically 3–10 seconds)

---

## Prompt 4: Validate Modbus Read

> From the Gate 5 output, confirm the Modbus multi-register read succeeded. What were the register values? What was the TX/RX protocol trace?

**Expected outcome:**
- TX: `01 03 00 00 00 02 C4 0B` (Read 2 Holding Regs from 0x0000)
- RX: 9 bytes, CRC valid, byte_count=4
- Register[0] and Register[1] extracted
- byte_count == 2 * quantity confirmed

---

## Prompt 5: Validate Uplink

> From the Gate 5 output, confirm the uplink was sent successfully. What payload was sent, on which port, and was it confirmed or unconfirmed?

**Expected outcome:**
- Payload: 4 bytes big-endian from register values
- Port: 10
- Confirmed: false (unconfirmed)
- `Uplink TX: OK (port=10, 4 bytes)`

---

## Prompt 6: ChirpStack Verification

> After the gate passes, how should I verify the uplink arrived in ChirpStack? What should I look for in the device events?

**Expected outcome:**
- Navigate to Devices → [RAK4631 device] → Events
- Look for Join event with DevEUI `88:82:24:44:AE:ED:1E:B2`
- Look for Uplink event on port 10
- Decode 4-byte payload: Reg[0] MSB, Reg[0] LSB, Reg[1] MSB, Reg[1] LSB

---

## Prompt 7: Compare RAK4631 vs RAK3312 Gate 5

> How does the RAK4631 Gate 5 differ from the RAK3312 Gate 5 in terms of LoRaWAN init, Modbus HAL, and build configuration?

**Expected outcome:**
- RAK4631: `lora_rak4630_init()` one-liner vs RAK3312: manual `hw_config` struct
- RAK4631: `hal_uart_init(baud, parity)` 2 args vs RAK3312: 4 args
- RAK4631: Auto DE/RE vs RAK3312: Manual GPIO toggling
- RAK4631: `-DRAK4630 -D_VARIANT_RAK4630_` flags vs RAK3312: `-DCORE_RAK3312`
- RAK4631: `Serial.print()` vs RAK3312: `Serial.printf()`
- RAK4631: No `Serial1.end()` vs RAK3312: `hal_uart_deinit()` available
- Same library: `beegee-tokyo/SX126x-Arduino@^2.0.32`
- Same `lmh_*` API: `lmh_init()`, `lmh_join()`, `lmh_send()`
