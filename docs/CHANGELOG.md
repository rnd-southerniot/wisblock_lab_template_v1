## v3.2-gate2-pass-rak4631 (2026-02-24)

### Gate 2 — RS485 Modbus Autodiscovery on RAK4631: PASSED

- **Discovered** Modbus RTU slave via automated scan: Slave=1, Baud=4800, Parity=NONE
- **Scanned** 4 baud rates × 2 parity modes × 100 slave addresses (800 probes max)
- **Found** device on first probe — TX: `01 03 00 00 00 01 84 0A`, RX: `01 03 02 00 07 F9 86`
- **Fixed** critical DE/RE bug: removed manual GPIO toggling on WB_IO1 (pin 17, NC on RAK5802)
- **Confirmed** RAK5802 hardware auto-direction — TP8485E DE/RE driven from UART TX line
- **Restored** `Serial1.flush()` — works correctly on nRF52840, busy-wait was unnecessary
- **Added** `[env:rak4631_gate2]` PlatformIO environment (nordicnrf52 platform)
- **Added** `[env:rak4631_gate2_raw]` diagnostic environment (single-probe test)
- **Added** `tests/gates/gate_rak4631_rs485_autodiscovery_validation/` — gate test files (5 files)
- **Added** `tests/gates/gate_rak4631_rs485_raw_monitor/` — diagnostic probe gate (2 files)
- **Added** `examples/rak4631/rs485_autodiscovery/` — standalone example (6 files)
- **Added** `docs/test_reports/rak4631_gate2_rs485_autodiscovery_v3.2.md` — full test report

### Critical Finding: RAK5802 Auto-Direction

The RAK5802 RS485 module has hardware auto-direction control. The on-board
TP8485E transceiver's DE/RE is driven automatically from the UART TX line
via an RC/TCON circuit. WB_IO1 (pin 17) is NC (not connected) to DE/RE.
Firmware must NOT toggle pin 17 — only power the module via WB_IO2 (pin 34)
and use `Serial1.write()` + `Serial1.flush()`.

---

## v3.1-gate1-pass-rak4631 (2026-02-24)

### Gate 1 — I2C LIS3DH Validation on RAK4631: PASSED

- **Ported** Gate 1 to RAK4631 WisBlock Core (nRF52840 + SX1262) — 9/9 criteria met
- **Validated** LIS3DH (0x18) WHO_AM_I=0x33, 100/100 accel reads, zero I2C errors
- **Confirmed** I2C pins: SDA=13 (P0.13), SCL=14 (P0.14)
- **Added** `[env:rak4631_gate1]` PlatformIO environment (nordicnrf52 platform)
- **Added** `tests/gates/gate_rak4631_i2c_lis3dh_validation/` — gate test files (3 files)
- **Added** `examples/rak4631/i2c_lis3dh_validation/` — standalone example (5 files)
- **Added** `docs/test_reports/rak4631_gate1_i2c_lis3dh_v3.1.md` — full test report

### RAK4631 Bring-Up Gates (pre-requisites)

- **Gate 0** — Toolchain Validation: compile + serial + LED blink confirmed
- **Gate 0.5** — DevEUI Read: FICR ID0=0xAEED1EB2, ID1=0x88822444, DevEUI=88:82:24:44:AE:ED:1E:B2
- **Gate 1A** — I2C Scan: LIS3DH detected at 0x18
- **Gate 1B** — I2C Identify: WHO_AM_I probes on 0x68/0x69 (ICM-20948 found)

### Platform Notes
- **TinyUSB:** Must use framework built-in v3.6.0 (symlink), not registry v3.7.4 (causes USB CDC failure)
- **USB CDC:** `while (!Serial) { ; }` required — DTR-gated output, no timeout
- **Upload:** Use `--upload-port` explicitly to avoid ststm32 platform auto-detect bug
- **LED:** Green LED (P1.03) not populated on this board; using blue LED (P1.04)

---

## v2.3-gate3-pass-rak3312 (2026-02-24)

### Gate 3 — Modbus Minimal Protocol Validation: PASSED

- **Validated** Modbus RTU Read Holding Register (FC 0x03) round-trip — 7/7 criteria met
- **Implemented** 7-step parser: length → CRC → slave → exception → func → byte_count → value
- **Confirmed** discovered parameters from Gate 2: Slave=1, Baud=4800, 8N1
- **Added** `[env:rak3312_gate3]` PlatformIO environment
- **Added** `tests/gates/gate_modbus_minimal_protocol/` — gate test files (7 files)
- **Added** `examples/rak3312/modbus_minimal_protocol/` — standalone example (6 files)
- **Added** `docs/test_reports/gate_modbus_minimal_protocol_v2.3.md` — full test report

### Validated Protocol
- TX: `01 03 00 00 00 01 84 0A` (Read Holding Register 0x0000, Qty=1)
- RX: `01 03 02 00 06 38 46` (Value=0x0006, CRC valid)
- Parser validated: frame length, CRC-16, slave match, byte_count consistency
- RS-FSJT-N01 wind speed sensor confirmed operational

---

## v2.2-gate2-pass-rak3312 (2026-02-24)

### Gate 2 — RS485 Autodiscovery Validation: PASSED

- **Discovered** Modbus RTU slave via automated scan: Slave=1, Baud=4800, Parity=NONE
- **Scanned** 4 baud rates × 3 parity modes × 100 slave addresses (1200 probes max)
- **Implemented** CRC-16/MODBUS frame validation with exception handling
- **Added** `[env:rak3312_gate2]` PlatformIO environment
- **Added** `tests/gates/gate_rs485_autodiscovery_validation/` — gate test files
- **Confirmed** RS485 bus via RAK5802: EN=GPIO14, DE=GPIO21, UART1 TX=43/RX=44

---

## v2.1-gate1-pass-rak3312 (2026-02-23)

### Gate 1 — I2C LIS3DH Re-Validation under Hardware Profile v2.0: PASSED

- **Re-validated** Gate 1 under Hardware Profile v2.0 (CORE_RAK3312) — 6/6 criteria met
- **Confirmed** I2C pins match v2.0 profile: SDA=GPIO9, SCL=GPIO40
- **Confirmed** No SX1262 reserved pin conflicts (GPIO 9/40 not in reserved set)
- **Renamed** PlatformIO environment from `[env:rak3212]` to `[env:rak3312]`
- **Updated** `src/main.cpp` banner to `RAK3312 (ESP32-S3)`
- **Updated** `GATE1_REPORT.md` with v2.0 re-validation results and notes
- **No changes** to gate logic (`gate_runner.cpp`) or HAL (`hal_i2c.cpp`)

---

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
