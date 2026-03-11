# Gate 9: Persistence Reboot Test v1 — Pass Criteria

## Two-Phase Reboot Test

Gate 9 validates that storage HAL persists runtime values across a power cycle.

## Pass Criteria

1. **Phase 1: storage_hal_init()** → returns true (OK)
2. **Phase 1: Write + immediate read-back** → interval == 15000, slave addr == 42
3. **Phase 1: Phase marker written** → reboot triggered
4. **Phase 2: storage_hal_init()** → returns true (OK)
5. **Phase 2: Phase marker read** → value == GATE9_PHASE_2 (2)
6. **Phase 2: Persisted interval** → 15000 ms
7. **Phase 2: Persisted slave addr** → 42
8. **Phase 2: SystemManager auto-loaded interval** → 15000 ms
9. **Phase 2: SystemManager auto-loaded slave addr** → 42
10. **Phase 2: Cleanup marker removed** → OK
11. **`=== GATE COMPLETE ===`** printed

## Expected Serial Output

### Phase 1 (first boot)
```
[GATE9] Gate 9: Persistence Reboot Test v1
[GATE9] No phase marker (or phase 0): running PHASE 1
[GATE9] ===== PHASE 1: Pre-Reboot =====
[GATE9] STEP 1: SystemManager init
[GATE9] STEP 1: PASS
[GATE9] STEP 2: storage_hal_init()
[GATE9] STEP 2: PASS — storage init OK
[GATE9] STEP 3: Write test values
[GATE9] STEP 3: PASS — values written
[GATE9] STEP 4: Immediate read-back verify
[GATE9] STEP 4: PASS — read-back matches
[GATE9] STEP 5: Write phase marker
[GATE9] STEP 5: PASS — phase marker written
[GATE9] PHASE 1 COMPLETE — rebooting in 2s
```

### Phase 2 (after automatic reboot)
```
[GATE9] Gate 9: Persistence Reboot Test v1
[GATE9] Phase marker detected: PHASE 2 (post-reboot)
[GATE9] ===== PHASE 2: Post-Reboot =====
[GATE9] STEP 1: storage_hal_init()
[GATE9] STEP 1: PASS — storage init OK
[GATE9] STEP 2: Read phase marker
[GATE9] STEP 2: PASS — phase marker correct
[GATE9] STEP 3: Read persisted values
[GATE9] STEP 3: PASS — persisted values correct
[GATE9] STEP 4: SystemManager init (verify auto-load)
[STORAGE] Loaded interval: 15000 ms
[STORAGE] Loaded slave addr: 42
[GATE9] STEP 4: PASS — SystemManager auto-loaded persisted values
[GATE9] STEP 5: Cleanup — remove phase marker
[GATE9] STEP 5: PASS — marker removed
[GATE9] GATE 9 PASS — values persisted across reboot
[GATE9] === GATE COMPLETE ===
```

## Fail Codes

| Code | Meaning |
|------|---------|
| 0 | GATE_PASS |
| 1 | GATE_FAIL_INIT — SystemManager init failed |
| 2 | GATE_FAIL_JOIN — OTAA join failed |
| 3 | GATE_FAIL_STORAGE — storage_hal_init() failed |
| 4 | GATE_FAIL_WRITE — Storage write failed |
| 5 | GATE_FAIL_READBACK — Immediate read-back mismatch |
| 6 | GATE_FAIL_PERSIST — Post-reboot persistence mismatch |
| 7 | GATE_FAIL_AUTOLOAD — SystemManager didn't auto-load |
