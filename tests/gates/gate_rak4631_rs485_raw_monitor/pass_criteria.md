# Gate 2-PROBE — RAK4631 RS485 Single Modbus Probe

## Purpose
Single-probe RS485 diagnostic. Sends ONE Modbus RTU request
(slave 1, read holding reg 0x0000, qty 1) at 4800 8N1,
then listens for response. Minimal round-trip test.

## Pass Criteria

| # | Criterion | Expected |
|---|-----------|----------|
| 1 | Compile | `pio run -e rak4631_gate2_raw` succeeds |
| 2 | Banner | `[GATE2-PROBE] === GATE START` prints |
| 3 | TX | 8-byte Modbus frame printed and sent |
| 4 | RX length | Response >= 5 bytes |
| 5 | Slave match | Response[0] == 0x01 |
| 6 | CRC valid | CRC-16/MODBUS matches |
| 7 | Verdict | `[GATE2-PROBE] PASS` printed |

## Hardware Setup
- RAK5802 in **Slot A** on RAK19007 baseboard
- Modbus sensor connected to A+/B-/GND screw terminal
- Sensor powered externally (12V/24V)
- Sensor configured at slave address 1, 4800 baud, 8N1
