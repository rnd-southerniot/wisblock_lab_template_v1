# Gate 1 — RAK4631 I2C LIS3DH Validation: Pass Criteria

## Gate Identity
- **Name:** gate_rak4631_i2c_lis3dh_validation
- **ID:** 1
- **Hardware:** RAK4631 WisBlock Core (nRF52840 + SX1262)
- **Sensor:** LIS3DH (0x18)

## Pass Criteria

| # | Criterion | Expected |
|---|-----------|----------|
| 1 | Compile | `pio run -e rak4631_gate1` succeeds |
| 2 | Serial | Banner prints on serial console |
| 3 | Detect | Device ACK at 0x18 |
| 4 | WHO_AM_I | Register 0x0F returns 0x33 |
| 5 | Configure | CTRL_REG1/REG4 writes succeed |
| 6 | Reads | 100/100 consecutive accel reads succeed |
| 7 | Data | X/Y/Z values are non-zero, show expected range |
| 8 | LED | Blue LED blinks once |
| 9 | Complete | `[GATE1-R4631] PASS` and `[GATE1-R4631] === GATE COMPLETE ===` |
