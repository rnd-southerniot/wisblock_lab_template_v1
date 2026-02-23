# I2C Bus Map v1

**Hardware Profile:** v1.0
**Date:** 2026-02-23
**Status:** FROZEN

---

## Bus Topology

```
RAK3212 (ESP32)
  |
  +--- Shared I2C Bus (3.3V) --- WisBlock Connector
          |
          +--- [Slot B] RAK1904 (LIS3DH) @ 0x18
```

## Device Table

| Slot | Module | IC | I2C Address | Voltage | Bus | Mode | Interrupt |
|------|--------|----|-------------|---------|-----|------|-----------|
| B | RAK1904 | LIS3DH | `0x18` | 3.3V | Shared | Polled | No |

## Bus Parameters

| Parameter | Value |
|-----------|-------|
| Bus Type | Shared WisBlock I2C |
| Pull-ups | On-board (RAK19007) |
| Clock Speed | 100 kHz (default) |
| Max Devices | 1 (current) |

## Address Map

| Address | Device | Status |
|---------|--------|--------|
| `0x18` | RAK1904 (LIS3DH) | Occupied |
| `0x19` | RAK1904 (LIS3DH alternate) | Available |

## Notes

- LIS3DH default address is `0x18` (SA0 pin LOW).
- If SA0 is pulled HIGH, address shifts to `0x19`.
- No address conflicts on the shared bus.
- Accelerometer is read via polling only; no interrupt pin is wired.

## Constraints

- No other I2C devices present; no arbitration concerns.
- Bus operates at 3.3V logic level; no level shifting required.
