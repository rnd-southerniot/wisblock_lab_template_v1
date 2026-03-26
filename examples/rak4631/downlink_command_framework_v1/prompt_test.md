# Prompt Test — RAK4631 Gate 8: Downlink Command Framework v1

## Purpose

Verify that an AI coding assistant can build, flash, and validate Gate 8 on the RAK4631 using only these prompts.

---

## Prompt 1: Build Verification

> Build the Gate 8 environment (`rak4631_gate8`) and confirm it compiles with zero errors. Report RAM and Flash usage. Also verify Gate 7 still compiles (regression check).

**Expected outcome:**
- `rak4631_gate8 SUCCESS`
- `rak4631_gate7 SUCCESS` (regression)
- RAM ~6.6%, Flash ~17.8%
- Zero errors

---

## Prompt 2: Flash and Monitor

> Flash Gate 8 to the RAK4631 board. After flashing, capture serial output until `GATE COMPLETE` appears. No downlink has been enqueued in ChirpStack.

**Expected outcome:**
- Upload succeeds via nrfutil DFU
- STEP 1: SystemManager init OK, Connected: YES
- STEP 2: 2 initial uplinks with Modbus read OK + uplink OK
- STEP 3: 60-second downlink poll with scheduler ticks
- `PASS_WITHOUT_DOWNLINK` printed
- `=== GATE COMPLETE ===` printed

---

## Prompt 3: Explain the Downlink Protocol

> Describe the binary command protocol used in Gate 8. What is the frame format? What commands are supported? How does the device respond?

**Expected outcome:**
- Frame: `[0xD1] [0x01] [CMD] [PAYLOAD...]` on fport 10
- 4 commands: SET_INTERVAL, SET_SLAVE_ID, REQ_STATUS, REBOOT
- ACK response on fport 11: `[0xD1] [0x01] [CMD] [STATUS] [VALUE...]`
- STATUS 0x01 = ACK, 0x00 = NACK
- Rejection rules: silent ignore for bad magic/version/short frame, NACK for invalid params

---

## Prompt 4: Explain PASS_WITHOUT_DOWNLINK

> Why does Gate 8 have a PASS_WITHOUT_DOWNLINK outcome? How does this differ from a full PASS?

**Expected outcome:**
- PASS_WITHOUT_DOWNLINK: all infrastructure works (join, uplinks, scheduler, downlink polling) but no downlink was queued in ChirpStack
- Full PASS: downlink received, parsed, applied, ACK sent
- Both are valid pass conditions — allows CI without ChirpStack dependency
- To get full PASS: enqueue downlink in ChirpStack before running the gate

---

## Prompt 5: Explain IRQ-Guarded Downlink Buffer

> How is the downlink buffer protected against race conditions? What could go wrong without the guards?

**Expected outcome:**
- `on_rx_data()` callback fires from radio ISR context
- `lorawan_hal_pop_downlink()` reads from main loop context
- Both use `noInterrupts()`/`interrupts()` to create critical sections
- Without guards: torn read — `pop` could read partially-written buffer if ISR fires mid-copy
- `s_dl_pending` flag is volatile — visible across contexts

---

## Prompt 6: Validate HAL v2 Layering

> Does the downlink command module (`downlink_commands.cpp`) include `<LoRaWan-Arduino.h>`? How does the runtime stay decoupled from the radio library?

**Expected outcome:**
- `downlink_commands.cpp` does NOT include `<LoRaWan-Arduino.h>`
- It includes `system_manager.h` → `lora_transport.h` → `lorawan_hal.h` (no library types)
- The HAL layer (`lorawan_hal_common.cpp`) is the only file that includes `<LoRaWan-Arduino.h>`
- Downlink buffer lives in the HAL; runtime code calls `lorawan_hal_pop_downlink()` — a clean C function with no library types in its signature

---

## Prompt 7: Describe What Would Happen with a SET_INTERVAL Downlink

> If you enqueue `D1 01 01 00 00 27 10` on fport 10 in ChirpStack, what happens on the device? Walk through the entire flow.

**Expected outcome:**
1. Device sends uplink → RX1/RX2 windows open
2. Gateway delivers queued downlink in RX window
3. `on_rx_data()` ISR fires → copies 7 bytes into `s_dl_buf`, sets `s_dl_pending`
4. Main loop calls `lorawan_hal_pop_downlink()` → gets `D1 01 01 00 00 27 10`, port=10
5. `dl_parse_and_apply()`: validates MAGIC=0xD1, VERSION=0x01, CMD=0x01 (SET_INTERVAL)
6. Decodes 4-byte BE payload: 0x00002710 = 10000 ms
7. Validates range: 10000 ∈ [1000, 3600000] → OK
8. Calls `sys.scheduler().setInterval(0, 10000)` → changes task 0 interval
9. Builds ACK: `D1 01 01 01 00 00 27 10` (8 bytes)
10. Sends ACK uplink on fport 11
11. Scheduler now fires sensor task every 10 seconds instead of 30
