## v1.1-gate1-pass (2026-02-23)

### Gate 1 — I2C LIS3DH Validation: PASSED ✅

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
