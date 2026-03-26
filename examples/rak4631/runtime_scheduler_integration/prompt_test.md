# Prompt Test — RAK4631 Gate 6: Runtime Scheduler Integration

## Purpose

Verify that an AI coding assistant can build, flash, and validate Gate 6 on the RAK4631 using only these prompts.

---

## Prompt 1: Build Verification

> Build the Gate 6 environment (`rak4631_gate6`) and confirm it compiles with zero errors. Report RAM and Flash usage. Does the RAK3312 Gate 6 environment still compile (regression check)?

**Expected outcome:**
- `rak4631_gate6 SUCCESS`
- RAM ~6.6%, Flash ~17.7%
- Warning: `#warning USING RAK4630` from `RAK4630_MOD.cpp` (confirms `-DRAK4630` flag)
- `rak3312_gate6 SUCCESS` (regression check — runtime layer unchanged for RAK3312)
- Zero errors on both environments

---

## Prompt 2: Flash and Capture

> Flash Gate 6 to the RAK4631 board. After flashing, wait 8 seconds for reboot, then capture serial output. The OTAA join takes up to 30 seconds, then 3 forced cycles with 3-second delays between them. Total runtime is about 45 seconds.

**Expected outcome:**
- Upload succeeds via nrfutil DFU
- Full 10-step output captured
- `[GATE6-R4631] PASS` in output
- 3 cycles with `[RUNTIME] Cycle N: read=OK` lines

---

## Prompt 3: Validate Runtime Architecture

> From the Gate 6 serial output, identify the three runtime subsystems (scheduler, transport, peripheral). How do they cooperate? What does the `[RUNTIME]` log line tell us about data flow?

**Expected outcome:**
- TaskScheduler: cooperative, millis()-based, 1 task registered at 30s interval
- LoRaTransport: SX1262 via `lora_rak4630_init()`, OTAA joined, sends uplinks
- ModbusPeripheral: RS485 via RAK4631 HAL (auto DE/RE), reads 2 registers
- Data flow: ModbusPeripheral → SensorFrame → encode → LoRaTransport
- `[RUNTIME]` line comes from SystemManager::sensorUplinkTask() (static callback)

---

## Prompt 4: Validate Scheduler Behavior

> From the Gate 6 output, explain the difference between Step 6 (tick validation) and Steps 7-9 (forced cycles). Why does tick() not fire any task in Step 6? What is fireNow() doing?

**Expected outcome:**
- Step 6: `tick()` checks all tasks, finds interval not elapsed (registered moments ago)
- Steps 7-9: `fireNow(0)` bypasses interval check, directly calls callback
- `fireNow()` resets `last_run_ms` and invokes callback immediately
- This validates both the interval-based dispatch logic AND the callback execution
- In production, only `tick()` would be called from `loop()`

---

## Prompt 5: Validate Multi-Cycle Reliability

> From the Gate 6 output, confirm all 3 cycles completed successfully. What register values were read in each cycle? Are the uplink payloads correct?

**Expected outcome:**
- Cycle 1, 2, 3 all show `read=OK (2 regs), uplink=OK`
- Register values vary per cycle (live sensor data)
- Each payload is 4 bytes: Reg[0] MSB, Reg[0] LSB, Reg[1] MSB, Reg[1] LSB
- Transport remains connected after all 3 uplinks
- Final cycle count: 3/3

---

## Prompt 6: Compare Gate 5 vs Gate 6

> How does Gate 6 differ from Gate 5 in terms of architecture? What new components does Gate 6 introduce? Why is this considered a "runtime" gate?

**Expected outcome:**
- Gate 5: One-shot linear flow (init → read → uplink → done)
- Gate 6: Runtime pattern (SystemManager → TaskScheduler → periodic callbacks)
- New components: TaskScheduler, SystemManager, SensorFrame, RuntimeStats
- Abstraction layers: TransportInterface, PeripheralInterface
- "Runtime" = production firmware pattern vs "gate test" = validation harness
- Gate 6 proves the runtime layer works on RAK4631 with `#ifdef CORE_RAK4631` guards

---

## Prompt 7: Explain the Scheduler→TaskScheduler Rename

> During Gate 6 development, a `Scheduler` class name collision was discovered. What was the conflict? How was it resolved? Which files were affected?

**Expected outcome:**
- Adafruit nRF52 BSP's `rtos.h` declares `extern SchedulerRTOS Scheduler;`
- This global variable shadowed our `class Scheduler` declaration
- Error: `'Scheduler' does not name a type` in system_manager.h
- Resolution: Renamed `Scheduler` → `TaskScheduler` in 4 files:
  - `firmware/runtime/scheduler.h` (class + constructor)
  - `firmware/runtime/scheduler.cpp` (all method prefixes)
  - `firmware/runtime/system_manager.h` (member type + return type)
  - `firmware/runtime/system_manager.cpp` (return type)
- The rename is purely cosmetic — no behavior change
- RAK3312 builds unaffected (no `Scheduler` global in ESP32 BSP)
