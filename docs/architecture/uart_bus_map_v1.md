# UART / RS485 Bus Map v1

**Hardware Profile:** v1.0
**Date:** 2026-02-23
**Status:** FROZEN

---

## Bus Topology

```
RAK3212 (ESP32)
  |
  +--- UART (I/O Slot) ---> RAK5802 RS485 Transceiver
                                |
                                +--- RS485 Bus (A+/B-)
                                      |
                                      +--- Modbus RTU Slave Device(s)
```

## Interface Table

| Slot | Module | Protocol | Direction Control | Comm Mode |
|------|--------|----------|-------------------|-----------|
| I/O | RAK5802 | Modbus RTU | Auto (DE/RE by RAK5802) | Polling |

## Serial Frame Configuration

| Parameter | Value |
|-----------|-------|
| Data Bits | 8 |
| Stop Bits | 1 |
| Parity | Determined by auto-discovery |
| Flow Control | None (RS485 half-duplex) |

## Supported Baud Rates

| Baud Rate | Status |
|-----------|--------|
| 4800 | Scan candidate |
| 9600 | Scan candidate |
| 19200 | Scan candidate |
| 115200 | Scan candidate |

## Supported Parity Modes

| Parity | Config Notation |
|--------|-----------------|
| None | 8N1 |
| Odd | 8O1 |
| Even | 8E1 |

## RAK5802 Pin Mapping (I/O Slot)

| Function | Description |
|----------|-------------|
| TX | ESP32 UART TX to RAK5802 DI |
| RX | ESP32 UART RX from RAK5802 RO |
| DE/RE | Auto-controlled by RAK5802 hardware |

## Notes

- RAK5802 handles RS485 direction control automatically; no GPIO management required from firmware.
- Half-duplex operation: TX and RX are time-multiplexed on the RS485 bus.
- Modbus RTU framing uses 3.5-character silent interval as frame delimiter.
- Actual baud rate and parity are determined at runtime via auto-discovery scan.
- See `rs485_autodiscovery_strategy.md` for scan procedure.

## Constraints

- Single UART channel allocated to RS485 via I/O slot.
- No other UART peripherals in this configuration.
- Modbus RTU response timeout must account for slowest baud rate (4800).
