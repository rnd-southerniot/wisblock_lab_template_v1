# UART / RS485 Bus Map v2

**Hardware Profile:** v2.0 (RAK3312)
**Date:** 2026-02-23
**Status:** FROZEN
**Replaces:** uart_bus_map_v1 (RAK3212)

---

## Bus Topology

```
RAK3312 (ESP32-S3)
  |
  +--- UART1 (TX=GPIO43, RX=GPIO44) ---> RAK5802 RS485 Transceiver
                                              |
                                              +--- RS485 Bus (A+/B-)
                                                    |
                                                    +--- Modbus RTU Slave Device(s)
```

## Interface Table

| Slot | Module | Protocol | Direction Control | Comm Mode |
|------|--------|----------|-------------------|-----------|
| I/O | RAK5802 | Modbus RTU | Auto (DE/RE by RAK5802) | Polling |

## UART Pin Assignments

| Port | TX | RX | Usage |
|------|----|----|-------|
| UART1 | GPIO 43 | GPIO 44 | RS485 via RAK5802 (I/O Slot) |

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
| TX | ESP32-S3 UART1 TX (GPIO 43) to RAK5802 DI |
| RX | ESP32-S3 UART1 RX (GPIO 44) from RAK5802 RO |
| DE/RE | Auto-controlled by RAK5802 hardware |

## Pin Change from v1.0

| Function | v1.0 (RAK3212) | v2.0 (RAK3312) |
|----------|-----------------|-----------------|
| UART TX | Not specified | GPIO 43 |
| UART RX | Not specified | GPIO 44 |

## Notes

- RAK5802 handles RS485 direction control automatically.
- Half-duplex operation: TX and RX time-multiplexed on the RS485 bus.
- Modbus RTU framing uses 3.5-character silent interval as frame delimiter.
- See `rs485_autodiscovery_strategy.md` for scan procedure (unchanged from v1.0).

## Constraints

- Single UART channel allocated to RS485 via I/O slot.
- No other UART peripherals in this configuration.
- Modbus RTU response timeout must account for slowest baud rate (4800).
