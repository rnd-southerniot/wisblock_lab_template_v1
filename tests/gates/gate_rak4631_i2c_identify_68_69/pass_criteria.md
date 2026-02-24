# Gate 1B — RAK4631 I2C Identify 0x68/0x69: Pass Criteria

## Gate Identity
- **Name:** gate_rak4631_i2c_identify_68_69
- **ID:** 1B
- **Hardware:** RAK4631 WisBlock Core (nRF52840 + SX1262)

## Pass Criteria

| # | Criterion | Expected |
|---|-----------|----------|
| 1 | Compile | `pio run -e rak4631_gate1b` succeeds |
| 2 | Serial | Banner prints on serial console |
| 3 | Presence | Both 0x68 and 0x69 respond with ACK |
| 4 | WHO_AM_I | At least one register returns stable non-0xFF/0x00 value |
| 5 | LED | Blue LED blinks once |
| 6 | Complete | `[GATE1B] PASS` and `[GATE1B] === GATE COMPLETE ===` |
