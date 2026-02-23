## v2.0-hardware-rak3312 (2026-02-23)

### Hardware Profile v2.0 — Core Module Change: RAK3212 → RAK3312

- **Changed** Core module from RAK3212 (ESP32) to RAK3312 (ESP32-S3 + SX1262)
- **Updated** `core_selector.h`: `CORE_RAK3212` → `CORE_RAK3312`
- **Updated** `hardware_profile.h` v2.0: full RAK3312 pin map, SX1262 internal connections, WisBlock IO assignments, power specs
- **Added** `docs/hardware_profiles/v2.0_RAK3312.md` — complete hardware profile documentation
- **Added** `docs/hardware_profiles/MIGRATION_v1_to_v2.md` — migration notes and invalidated assumptions
- **Added** `docs/architecture/i2c_bus_map_v2.md` — I2C1 SDA=GPIO9, SCL=GPIO40
- **Added** `docs/architecture/uart_bus_map_v2.md` — UART1 TX=GPIO43, RX=GPIO44
- **Documented** 8 reserved GPIOs (SX1262 internal SPI: GPIO 3,4,5,6,7,8,47,48)
- **Documented** SPI WisBlock external bus: CS=12, CLK=13, MOSI=11, MISO=10

### Invalidated v1.0 Assumptions
- GPIO 4/5 are NOT I2C — they are SX1262 ANT_SW/SCK
- MCU is ESP32-S3, not ESP32 — different toolchain required
- LoRaWAN uses SX1262 via SPI, NOT STM32WL integrated radio
- USB is native USB-Serial/JTAG, not external USB-UART chip
- Framework is Arduino (Espressif BSP), not Hybrid ESP-IDF/Arduino

---

## v1.1-gate1-pass (2026-02-23)

### Gate 1 — I2C LIS3DH Validation: PASSED

- **Added** `[env:rak3212]` PlatformIO environment targeting ESP32-S3
- **Added** `src/main.cpp` — minimal gate test entry point (no LoRaWAN)
- **Added** `examples/gate1_i2c_lis3dh/` — standalone LIS3DH I2C example
- **Added** `GATE1_REPORT.md` — full test report with serial log
- **Fixed** I2C pin mapping for RAK3312: SDA=GPIO9, SCL=GPIO40
- **Fixed** HAL I2C read for ESP32-S3 Wire library compatibility
- **Fixed** USB CDC serial output with `ARDUINO_USB_CDC_ON_BOOT=1`

### Validated Hardware
- RAK3312 (ESP32-S3) + RAK19007 Base + RAK1904 (LIS3DH) Slot B
- I2C bus: 0x18, 100 kHz, 3.3V, on-board pull-ups
- 100/100 consecutive reads, zero timeouts, WHO_AM_I=0x33

---

## v1.0 (2026-02-23)

- Initial template
