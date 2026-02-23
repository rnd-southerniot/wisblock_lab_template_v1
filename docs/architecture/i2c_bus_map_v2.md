# I2C Bus Map v2

**Hardware Profile:** v2.0 (RAK3312)
**Date:** 2026-02-23
**Status:** FROZEN
**Replaces:** i2c_bus_map_v1 (RAK3212)

---

## Bus Topology

```
RAK3312 (ESP32-S3)
  |
  +--- I2C1 (SDA=GPIO9, SCL=GPIO40) --- WisBlock Connector
  |       |
  |       +--- [Slot B] RAK1904 (LIS3DH) @ 0x18
  |
  +--- I2C2 (SDA=GPIO17, SCL=GPIO18) --- Secondary / Expansion
```

## I2C Bus Assignments

| Bus | SDA | SCL | Pull-ups | Usage |
|-----|-----|-----|----------|-------|
| I2C1 | GPIO 9 | GPIO 40 | On-board (RAK19007) | WisBlock Slots A-D |
| I2C2 | GPIO 17 | GPIO 18 | External required | Expansion |

## Device Table

| Slot | Module | IC | I2C Address | Voltage | Bus | Mode | Interrupt |
|------|--------|----|-------------|---------|-----|------|-----------|
| B | RAK1904 | LIS3DH | `0x18` | 3.3V | I2C1 | Polled | No |

## Address Map (I2C1)

| Address | Device | Status |
|---------|--------|--------|
| `0x18` | RAK1904 (LIS3DH) | Occupied |
| `0x19` | RAK1904 (LIS3DH alternate) | Available |

## Bus Parameters (I2C1)

| Parameter | Value |
|-----------|-------|
| Bus Type | Shared WisBlock I2C |
| Pull-ups | On-board (RAK19007) |
| Clock Speed | 100 kHz (default) |
| Max Devices | 1 (current) |
| Logic Level | 3.3V |

## Pin Change from v1.0

| Function | v1.0 (RAK3212) | v2.0 (RAK3312) | Reason |
|----------|-----------------|-----------------|--------|
| I2C SDA | GPIO 4 | **GPIO 9** | GPIO 4 reserved for SX1262 ANT_SW |
| I2C SCL | GPIO 5 | **GPIO 40** | GPIO 5 reserved for SX1262 SCK |

## Notes

- LIS3DH default address is `0x18` (SA0 pin LOW).
- If SA0 is pulled HIGH, address shifts to `0x19`.
- No address conflicts on the shared bus.
- Accelerometer is read via polling only; no interrupt pin is wired.
- I2C1 is validated by Gate 1 (PASSED 2026-02-23).

## Constraints

- No other I2C devices present on I2C1; no arbitration concerns.
- Bus operates at 3.3V logic level; no level shifting required.
- GPIO 4 and 5 are NOT available for I2C — they are SX1262 internal pins.
