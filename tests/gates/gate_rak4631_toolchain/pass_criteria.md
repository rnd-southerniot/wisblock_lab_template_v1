# Gate 0 — RAK4631 Toolchain Validation: Pass Criteria

## Gate Identity
- **Name:** gate_rak4631_toolchain
- **ID:** 0
- **Hardware:** RAK4631 WisBlock Core (nRF52840 + SX1262)

## Pass Criteria

| # | Criterion | Expected |
|---|-----------|----------|
| 1 | Compile | `pio run -e rak4631_gate0` succeeds |
| 2 | Serial | `[GATE0] RAK4631 Toolchain OK` printed |
| 3 | LED | Green LED blinks once |
| 4 | Complete | `[GATE0] PASS` and `[GATE0] === GATE COMPLETE ===` |
