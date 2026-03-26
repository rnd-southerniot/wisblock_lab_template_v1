# Gate 2 — RAK4631 RS485 Modbus Autodiscovery: Pass Criteria

## Gate Identity
- **Name:** gate_rak4631_rs485_autodiscovery_validation
- **ID:** 2
- **Hardware:** RAK4631 WisBlock Core (nRF52840 + SX1262)
- **Transceiver:** RAK5802 RS485 (auto DE/RE)

## Pass Criteria

| # | Criterion | Expected |
|---|-----------|----------|
| 1 | Compile | `pio run -e rak4631_gate2` succeeds |
| 2 | Serial | Banner prints on serial console |
| 3 | RS485 Enable | RAK5802 module powered via WB_IO2 (pin 34) |
| 4 | Scan Start | Baud/parity/slave scan begins |
| 5 | Discovery | At least one Modbus device found |
| 6 | Frame Valid | Response frame length >= 5, CRC-16 valid |
| 7 | Slave Match | Response slave address matches probe |
| 8 | Parameters | Discovered Slave, Baud, Parity printed |
| 9 | LED | Blue LED blinks once |
| 10 | Complete | `[GATE2-R4631] PASS` and `[GATE2-R4631] === GATE COMPLETE ===` |

## Scan Parameters

| Parameter | Value |
|-----------|-------|
| Baud rates | 4800, 9600, 19200, 115200 |
| Parity modes | None, Odd, Even |
| Slave range | 1..100 |
| Max probes | 1200 |
| Response timeout | 200 ms |
| Inter-byte gap | 5 ms |

## Response Validation Order

1. Frame length >= 5 bytes
2. CRC-16/MODBUS match (poly=0xA001, init=0xFFFF)
3. Slave address matches request
4. Exception response (func | 0x80) accepted as valid discovery
5. Normal response function code match

## UART HAL Notes (nRF52840)

- `Serial1.begin(baud, config)` — no pin args (pins fixed by variant.h)
- TX = pin 16 (P0.16), RX = pin 15 (P0.15)
- RAK5802 auto DE/RE — no manual direction control
- Flush and drain between baud/parity changes
