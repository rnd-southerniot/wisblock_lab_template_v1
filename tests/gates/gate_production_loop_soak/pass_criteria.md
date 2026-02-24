# Gate 7 — Production Loop Soak: Pass Criteria

## Gate Identity
- **Name:** `gate_production_loop_soak`
- **ID:** 7
- **Version:** 1.0
- **Hardware:** RAK3312 (ESP32-S3 + SX1262) on RAK19007

## Objective
Validate the production runtime loop in real conditions for a fixed 5-minute
duration. Verify stability, consistent uplinks, and bounded cycle timing.

## Configuration
| Parameter | Value | Source |
|-----------|-------|--------|
| Soak Duration | 300000 ms (5 min) | `GATE_SOAK_DURATION_MS` |
| Poll Interval | 30000 ms (30 s) | `HW_SYSTEM_POLL_INTERVAL_MS` |
| Expected Cycles | ~10 | duration / interval |
| Min Uplinks | 9 | `GATE_SOAK_MIN_UPLINKS` = floor(300000/30000) - 1 |
| Max Consec Fails | 2 | `GATE_SOAK_MAX_CONSEC_FAILS` |
| Max Cycle Duration | 1500 ms | `GATE_SOAK_MAX_CYCLE_MS` |
| Uplink Port | 10 | `GATE_PAYLOAD_PORT` |
| Join Timeout | 30 s | `GATE_SOAK_JOIN_TIMEOUT_MS` |

## Pass Criteria (all must be met)

1. **Single Join** — OTAA join occurs once only during `SystemManager::init()`.
   No repeated joins during the soak loop.

2. **Full Duration** — System runs for the entire `GATE_SOAK_DURATION_MS`
   without crash, reboot, or watchdog reset.

3. **Minimum Uplinks** — At least `GATE_SOAK_MIN_UPLINKS` (9) successful
   uplinks sent via LoRaWAN transport.

4. **Modbus Stability** — No more than `GATE_SOAK_MAX_CONSEC_FAILS` (2)
   consecutive Modbus read failures.

5. **Uplink Stability** — No more than `GATE_SOAK_MAX_CONSEC_FAILS` (2)
   consecutive uplink send failures.

6. **Cycle Duration** — Every cycle completes within `GATE_SOAK_MAX_CYCLE_MS`
   (1500 ms). This bounds the non-blocking behavior of the scheduler.

7. **Transport Connected** — LoRa transport remains connected (joined)
   throughout the entire soak duration.

## Fail Codes

| Code | Constant | Meaning |
|------|----------|---------|
| 0 | `GATE_PASS` | All criteria met |
| 1 | `GATE_FAIL_INIT` | SystemManager::init() failed (LoRa/join/Modbus) |
| 2 | `GATE_FAIL_INSUFFICIENT_CYCLES` | Total cycles < MIN_UPLINKS |
| 3 | `GATE_FAIL_INSUFFICIENT_UPLINKS` | Successful uplinks < MIN_UPLINKS |
| 4 | `GATE_FAIL_CONSEC_MODBUS` | Consecutive Modbus failures > MAX_CONSEC_FAILS |
| 5 | `GATE_FAIL_CONSEC_UPLINK` | Consecutive uplink failures > MAX_CONSEC_FAILS |
| 6 | `GATE_FAIL_CYCLE_DURATION` | Cycle duration > MAX_CYCLE_MS |
| 7 | `GATE_FAIL_TRANSPORT_LOST` | Transport disconnected during soak |
| 8 | `GATE_FAIL_SYSTEM_CRASH` | Runner did not reach GATE COMPLETE |

## Expected Serial Output

### Init Phase
```
[GATE7] === GATE START: gate_production_loop_soak v1.0 ===
[GATE7] Soak duration  : 300000 ms (300 s)
[GATE7] Min uplinks    : 9
[GATE7] Max consec fail: 2
[GATE7] Max cycle dur  : 1500 ms
[GATE7] Poll interval  : 30000 ms

[GATE7] STEP 1: SystemManager init (production mode)
[GATE7] SystemManager init: OK (dur=XXXX ms)
[GATE7]   Transport  : LoRaTransport (connected=yes)
[GATE7]   Peripheral : ModbusPeripheral (Slave=1, Baud=4800)
[GATE7]   Scheduler  : 1 task(s), interval=30000 ms
[GATE7]   Join count : 1 (single init)
```

### Soak Phase (repeats every ~30s)
```
[GATE7] STEP 2: Production soak (300 seconds)
[RUNTIME] Cycle 1: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE7] Cycle 1 | modbus=OK | uplink=OK | dur=XXX ms | elapsed=30 s
[RUNTIME] Cycle 2: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE7] Cycle 2 | modbus=OK | uplink=OK | dur=XXX ms | elapsed=60 s
...
[RUNTIME] Cycle 10: read=OK (2 regs), uplink=OK (port=10, 4 bytes)
[GATE7] Cycle 10 | modbus=OK | uplink=OK | dur=XXX ms | elapsed=300 s
```

### Summary Phase
```
[GATE7] STEP 3: Soak summary
[GATE7]   Duration          : 300XXX ms (300 s)
[GATE7]   Total Cycles      : 10
[GATE7]   Modbus OK / FAIL  : 10 / 0
[GATE7]   Uplink OK / FAIL  : 10 / 0
[GATE7]   Max Consec Fail   : modbus=0, uplink=0
[GATE7]   Max Cycle Duration: XXX ms
[GATE7]   Transport         : connected
[GATE7]   Join Attempts     : 1
[GATE7] PASS
[GATE7] === GATE COMPLETE ===
```

## Verification
1. Serial output shows `[GATE7] PASS` and `[GATE7] === GATE COMPLETE ===`
2. ChirpStack device events show at least 9 uplinks on port 10
3. No watchdog resets or Guru Meditation errors during soak
4. All cycle durations < 1500 ms
