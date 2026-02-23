# Migration Notes: Hardware Profile v1.0 → v2.0

**Date:** 2026-02-23
**From:** v1.0 (RAK3212 / ESP32)
**To:** v2.0 (RAK3312 / ESP32-S3 + SX1262)

---

## Summary of Change

The core module has been physically replaced on the WisBlock base board.
All v1.0 core-specific assumptions are now **INVALID**.

---

## Invalidated Assumptions

### 1. MCU Architecture

| Assumption | v1.0 | v2.0 | Impact |
|------------|------|------|--------|
| MCU | ESP32 (Xtensa LX6 dual-core) | ESP32-S3 (Xtensa LX7 dual-core) | Different toolchain, different peripheral registers |
| Flash | Not specified | 16 MB | Larger partition table possible |
| PSRAM | Not specified | 8 MB | Available for buffers |
| USB | External USB-UART (CP2102/CH340) | Native USB-Serial/JTAG | Different serial init, CDC flags needed |

### 2. Pin Assignments (CRITICAL)

| Function | v1.0 GPIO | v2.0 GPIO | Breaking? |
|----------|-----------|-----------|-----------|
| I2C SDA | 4 | 9 | **YES** — GPIO 4 is now SX1262 ANT_SW |
| I2C SCL | 5 | 40 | **YES** — GPIO 5 is now SX1262 SCK |
| UART TX | Not specified | 43 | New |
| UART RX | Not specified | 44 | New |
| SPI WB CS | Not specified | 12 | New |
| SPI WB CLK | Not specified | 13 | New |
| SPI WB MOSI | Not specified | 11 | New |
| SPI WB MISO | Not specified | 10 | New |

### 3. SPI Bus — NEW CONSTRAINT

v1.0 had `HW_SPI_DEVICES_ACTIVE = 0` (no SPI devices).
v2.0 has the **SX1262 LoRa transceiver on an internal SPI bus** using GPIO 3,5,6,7.
A separate WisBlock SPI bus exists on GPIO 10,11,12,13.

**Impact:** GPIO 3,4,5,6,7,8,47,48 are now RESERVED and must never be used.

### 4. LoRaWAN Stack

| Assumption | v1.0 | v2.0 |
|------------|------|------|
| LoRa integration | External (not defined) | On-module SX1262 |
| Stack | Not specified | Arduino (Espressif BSP + SX126x library) |
| LoRaWAN spec | Not specified | 1.0.2 |
| MCU radio | None (ESP32 has no radio) | None (SX1262 is external SPI peripheral) |

### 5. Build Configuration

| Parameter | v1.0 | v2.0 |
|-----------|------|------|
| PlatformIO board | `esp32dev` | `esp32-s3-devkitc-1` |
| Toolchain | `toolchain-xtensa-esp32` | `toolchain-xtensa-esp32s3` |
| USB CDC flag | Not needed | `-DARDUINO_USB_CDC_ON_BOOT=1` required |
| USB mode flag | Not needed | `-DARDUINO_USB_MODE=1` required |

### 6. Framework

v1.0 defined `HW_FRAMEWORK_MODE = "HYBRID_ESPIDF_ARDUINO"`.
v2.0 uses `HW_FRAMEWORK_MODE = "ARDUINO_ESP32"` (pure Arduino on Espressif BSP).

---

## What Remains Valid

The following are UNCHANGED between v1.0 and v2.0:

- Base board: RAK19007
- Slot layout: RAK5802 (I/O), RAK1904 (Slot B)
- I2C device address: LIS3DH @ 0x18
- I2C voltage: 3.3V with on-board pull-ups
- RS485 configuration: Modbus RTU, auto-discovery strategy
- Power sources: USB + LiPo
- LoRaWAN region: AS923-1, OTAA, ChirpStack v4

---

## Action Items After Migration

1. ~~Update `core_selector.h`~~ Done
2. ~~Update `hardware_profile.h`~~ Done
3. ~~Update bus maps~~ Done
4. Re-validate Gate 1 with updated pin config (already passed during Gate 1 execution)
5. Any future gates must use v2.0 pin assignments
6. LoRaWAN gate must target SX1262 via SPI, not STM32WL integrated radio
