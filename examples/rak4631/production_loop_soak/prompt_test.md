# Prompt Test — RAK4631 Gate 7: Production Loop Soak

## Purpose

Verify that an AI coding assistant can build, flash, and validate Gate 7 on the RAK4631 using only these prompts.

---

## Prompt 1: Build Verification

> Build the Gate 7 environment (`rak4631_gate7`) and confirm it compiles with zero errors. Report RAM and Flash usage.

**Expected outcome:**
- `rak4631_gate7 SUCCESS`
- RAM ~6.6%, Flash ~17.7%
- Warning: `#warning USING RAK4630` from `RAK4630_MOD.cpp` (confirms `-DRAK4630` flag)
- Zero errors

---

## Prompt 2: Flash and Soak Capture

> Flash Gate 7 to the RAK4631 board. After flashing, wait 8 seconds for reboot, then capture serial output for the full soak duration (~6 minutes). The OTAA join takes up to 30 seconds, then the soak runs for 5 minutes with cycles every 30 seconds.

**Expected outcome:**
- Upload succeeds via nrfutil DFU
- STEP 1 shows SystemManager init OK
- STEP 2 shows per-cycle lines every ~30s
- STEP 3 shows soak summary
- `[GATE7-R4631] PASS` in output
- ~9 cycles with `[RUNTIME] Cycle N: read=OK` lines

---

## Prompt 3: Explain Production Soak Pattern

> From the Gate 7 serial output, explain how this gate differs from Gate 6. Why is there no `fireNow()`? How does the scheduler fire tasks naturally?

**Expected outcome:**
- Gate 6 uses `fireNow()` to trigger 3 cycles immediately
- Gate 7 uses `tick()` in a continuous loop — scheduler fires when `millis() - last_run >= interval`
- 30-second poll interval means ~10 cycles in 300 seconds
- This validates the production code path: no test intervention
- The gate harness only observes — it never forces task execution

---

## Prompt 4: Validate Soak Stability Metrics

> From the Gate 7 soak summary, what were the stability metrics? How many cycles completed? Were there any failures? What was the worst-case cycle duration?

**Expected outcome:**
- Total cycles: 9 (300s / 30s = 10 intervals, minus 1 for first interval)
- Modbus: 9 OK / 0 FAIL
- Uplink: 9 OK / 0 FAIL
- Max consecutive failures: modbus=0, uplink=0
- Max cycle duration: ~82 ms (well under 1500 ms limit)
- Transport: connected throughout
- Single join attempt: 1

---

## Prompt 5: Validate Pass Criteria

> List all 7 pass criteria for Gate 7 and confirm each was met from the soak output.

**Expected outcome:**
1. Single join — init() called once, `Join Attempts: 1` ✓
2. Full duration — ran 300,007 ms (>= 300,000 ms) ✓
3. Min uplinks — 9 >= 3 required ✓
4. Modbus stability — 0 consecutive failures <= 2 max ✓
5. Uplink stability — 0 consecutive failures <= 2 max ✓
6. Cycle duration — 82 ms max < 1500 ms limit ✓
7. Transport connected — `connected` at end of soak ✓

---

## Prompt 6: Compare Gate 6 vs Gate 7

> How does Gate 7 differ from Gate 6 in terms of what it validates? Why is a production soak important after the scheduler integration gate passes?

**Expected outcome:**
- Gate 6 validates that the runtime *works* (init + 3 forced cycles)
- Gate 7 validates that the runtime is *stable over time* (5-minute soak)
- Gate 6 uses fireNow() — bypasses timing logic
- Gate 7 uses tick() — validates real scheduler interval dispatch
- Gate 7 catches: memory leaks, timing drift, radio state degradation, consecutive failures
- Gate 7 is the final validation before deployment

---

## Prompt 7: Analyze Cycle Timing

> The soak output shows cycle durations of 80-82 ms. What happens during those ~80 ms? Why is the limit set at 1500 ms?

**Expected outcome:**
- Cycle timing includes: Modbus RTU request + response (~50-60 ms at 4800 baud) + LoRaWAN payload encoding + lmh_send() queue (~10-20 ms)
- The 1500 ms limit is a guard rail — ensures the system is non-blocking
- If a cycle exceeds 1500 ms, it could indicate: UART timeout, retries, radio congestion
- 82 ms actual vs 1500 ms limit = 18× margin — healthy system
