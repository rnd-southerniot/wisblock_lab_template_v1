# Gate 0.5 — RAK4631 DevEUI Read: Pass Criteria

## Gate Identity
- **Name:** gate_rak4631_deveui_read
- **ID:** 0.5
- **Hardware:** RAK4631 WisBlock Core (nRF52840 + SX1262)

## Pass Criteria

| # | Criterion | Expected |
|---|-----------|----------|
| 1 | Compile | `pio run -e rak4631_gate0_5` succeeds |
| 2 | Serial | Banner prints on serial console |
| 3 | FICR | Raw FICR ID0 and ID1 values printed |
| 4 | DevEUI | 8-byte non-zero colon-separated value printed |
| 5 | Raw Hex | Hex array format printed |
| 6 | LED | Blue LED blinks once |
| 7 | Complete | `[GATE0.5] PASS` and `[GATE0.5] === GATE COMPLETE ===` |
