# Gate 1A — RAK4631 I2C Bus Scan: Pass Criteria

## Gate Identity
- **Name:** gate_rak4631_i2c_scan
- **ID:** 1A
- **Hardware:** RAK4631 WisBlock Core (nRF52840 + SX1262)

## Pass Criteria

| # | Criterion | Expected |
|---|-----------|----------|
| 1 | Compile | `pio run -e rak4631_gate1a` succeeds |
| 2 | Serial | Banner prints on serial console |
| 3 | Pin Report | Wire SDA/SCL pin numbers printed |
| 4 | Scan | Address range 0x08..0x77 scanned |
| 5 | Devices | At least one I2C device found |
| 6 | LED | Blue LED blinks once |
| 7 | Complete | `[GATE1A] PASS` and `[GATE1A] === GATE COMPLETE ===` |
