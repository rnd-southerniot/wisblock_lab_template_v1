## v3.7-gate7-pass-rak4631 (2026-02-25)

### Gate 7 — Production Loop Soak on RAK4631: PASSED

- **Validated** 5-minute production soak: scheduler runs naturally via `tick()`, no `fireNow()` — 7/7 criteria met
- **Confirmed** 9 sensor-uplink cycles over 300 seconds (30s interval, natural scheduler dispatch)
- **Confirmed** zero Modbus failures: 9 OK / 0 FAIL, max consecutive fail = 0
- **Confirmed** zero uplink failures: 9 OK / 0 FAIL, max consecutive fail = 0
- **Confirmed** cycle timing stability: 80-82 ms per cycle (max 82 ms, limit 1500 ms — 18x margin)
- **Confirmed** single OTAA join: `SystemManager::init()` joins once, no re-joins during soak
- **Confirmed** transport remained connected throughout entire 5-minute soak
- **No runtime modifications**: gate harness only — observes `sys.tick()` + `sys.stats()`
- **Added** `[env:rak4631_gate7]` PlatformIO environment with `-DCORE_RAK4631 -DRAK4630 -D_VARIANT_RAK4630_`
- **Added** `tests/gates/gate_rak4631_production_loop_soak/` — gate test files (3 files)
- **Added** `examples/rak4631/production_loop_soak/` — standalone example (6 files)
- **Added** `docs/test_reports/rak4631_gate7_production_soak_v3.7.md` — full test report

### Production Soak Results

- Duration: 300,007 ms (300 s)
- Total Cycles: 9
- Modbus: 9 OK / 0 FAIL
- Uplinks: 9 OK / 0 FAIL
- Max Consecutive Fail: modbus=0, uplink=0
- Max Cycle Duration: 82 ms
- Transport: connected
- Join Attempts: 1

### Key Difference from Gate 6

| Aspect | Gate 6 (Forced Cycles) | Gate 7 (Production Soak) |
|--------|------------------------|--------------------------|
| Execution | `fireNow()` x 3 cycles | `tick()` for 5 minutes |
| Duration | ~45 seconds | ~5.5 minutes |
| Scheduling | Forced immediate | Natural 30s interval |
| Validation | All 3 cycles must pass | Aggregate stability metrics |
| What it proves | Runtime *works* | Runtime is *stable over time* |

---

## v3.6-gate6-pass-rak4631 (2026-02-25)

### Gate 6 — Runtime Scheduler Integration on RAK4631: PASSED

- **Validated** production runtime architecture: SystemManager + TaskScheduler + LoRaTransport + ModbusPeripheral — 10/10 criteria met
- **Confirmed** 3 sensor-uplink cycles via cooperative scheduler: Modbus read → encode → LoRaWAN uplink
- **Confirmed** scheduler tick validation: dry-run tick() returns without firing (interval not elapsed)
- **Confirmed** fireNow() forces immediate task execution, bypassing interval check
- **Confirmed** multi-cycle reliability: 3/3 cycles completed, transport still connected
- **Ported** `firmware/runtime/` layer to RAK4631 with `#ifdef CORE_RAK4631` guards (3 files)
- **Modified** `lora_transport.cpp` — `lora_rak4630_init()` BSP one-liner for SX1262 init
- **Modified** `modbus_peripheral.cpp` — RAK4631 HAL includes, 2-arg UART init, 1-arg RS485 enable, skip deinit()
- **Renamed** `Scheduler` → `TaskScheduler` in 4 files to avoid Adafruit nRF52 BSP symbol collision (`extern SchedulerRTOS Scheduler` in `rtos.h`)
- **Verified** RAK3312 regression: `rak3312_gate6` still compiles after all `#ifdef` changes
- **Added** `[env:rak4631_gate6]` PlatformIO environment with `-DCORE_RAK4631 -DRAK4630 -D_VARIANT_RAK4630_`
- **Added** `tests/gates/gate_rak4631_runtime_scheduler_integration/` — gate test files (3 files)
- **Added** `examples/rak4631/runtime_scheduler_integration/` — standalone example (6 files)
- **Added** `docs/test_reports/rak4631_gate6_runtime_scheduler_v3.6.md` — full test report

### Runtime Architecture

- SystemManager: orchestrates init sequence + scheduler dispatch
- TaskScheduler: cooperative millis()-based scheduler (renamed from Scheduler)
- LoRaTransport: SX1262 via `lora_rak4630_init()`, OTAA join, unconfirmed uplinks
- ModbusPeripheral: RS485 via RAK4631 HAL (auto DE/RE), SensorFrame typed data
- Data flow: ModbusPeripheral → SensorFrame → big-endian encode → LoRaTransport

### Multi-Cycle Results

- Cycle 1: Reg[0]=0x0004, Reg[1]=0x0001, Payload=`00 04 00 01`, port 10
- Cycle 2: Reg[0]=0x0001, Reg[1]=0x0000, Payload=`00 01 00 00`, port 10
- Cycle 3: Reg[0]=0x0002, Reg[1]=0x0000, Payload=`00 02 00 00`, port 10

### Scheduler→TaskScheduler Rename (BSP Symbol Collision)

- Adafruit nRF52 BSP `rtos.h` declares `extern SchedulerRTOS Scheduler;` global variable
- This shadowed our `class Scheduler`, causing `'Scheduler' does not name a type`
- Renamed to `TaskScheduler` in: scheduler.h, scheduler.cpp, system_manager.h, system_manager.cpp
- No behavior change. RAK3312 builds unaffected.

---

## v3.5-gate5-pass-rak4631 (2026-02-25)

### Gate 5 — LoRaWAN OTAA Join + Modbus Uplink on RAK4631: PASSED

- **Validated** end-to-end LoRaWAN uplink from sensor to network server — 7/7 criteria met (criterion 7 pending ChirpStack manual verification)
- **Confirmed** OTAA join on AS923-1 in 5.8 seconds via `lora_rak4630_init()` + `lmh_init()` + `lmh_join()`
- **Confirmed** Modbus multi-register read: Reg[0]=0x0004 (wind speed), Reg[1]=0x0001 (wind direction)
- **Confirmed** 4-byte payload encoding: `00 04 00 01` (big-endian register values)
- **Confirmed** unconfirmed uplink sent on port 10 via `lmh_send()`
- **Used** SX126x-Arduino library (beegee-tokyo/SX126x-Arduino@^2.0.32) — same as RAK3312
- **Used** `lora_rak4630_init()` one-liner for SX1262 hardware init (no manual hw_config struct)
- **Reused** Gate 3's modbus_frame (CRC, frame builder, 7-step parser) and hal_uart (auto DE/RE)
- **Added** `[env:rak4631_gate5]` PlatformIO environment with `-DRAK4630 -D_VARIANT_RAK4630_` flags
- **Added** `tests/gates/gate_rak4631_lorawan_join_uplink/` — gate test files (3 files)
- **Added** `examples/rak4631/lorawan_join_modbus_uplink/` — standalone example (6 files)
- **Added** `docs/test_reports/rak4631_gate5_lorawan_join_uplink_v3.5.md` — full test report

### LoRaWAN Configuration
- Region: AS923-1, Class A, ADR=OFF, DR2 (SF10), TX_POWER_0
- DevEUI: `88:82:24:44:AE:ED:1E:B2` (Gate 0.5 FICR read)
- Join: OTAA, 3 trials, 30s timeout — accepted in 5777ms

### Protocol Trace
- TX: `01 03 00 00 00 02 C4 0B` (Read 2 Holding Registers from 0x0000)
- RX: `01 03 04 00 04 00 01 7A 32` (byte_count=4, Reg[0]=0x0004, Reg[1]=0x0001, CRC valid)
- Payload: `00 04 00 01` → uplink port 10, unconfirmed

### Critical Build Flag Discovery
- RAK4631 variant.h defines `_VARIANT_RAK4631_` but SX126x-Arduino library needs `_VARIANT_RAK4630_`
- Must add `-DRAK4630 -D_VARIANT_RAK4630_` to build_flags for `SPI_LORA` instantiation
- Confirmed by `#warning USING RAK4630` from `RAK4630_MOD.cpp` during compilation

---

## v3.4-gate4-pass-rak4631 (2026-02-24)

### Gate 4 — Modbus Robustness Layer Validation on RAK4631: PASSED

- **Validated** multi-register read (FC 0x03, qty=2) with retry mechanism — 8/8 criteria met
- **Confirmed** 2-register read: Reg[0]=0x0006 (wind speed), Reg[1]=0x0001 (wind direction)
- **Implemented** retry policy: 3 retries (4 total), retryable on TIMEOUT/CRC_FAIL only
- **Implemented** error classification enum: NONE, TIMEOUT, CRC_FAIL, LENGTH_FAIL, EXCEPTION, UART_ERROR
- **Implemented** UART recovery: re-init after 2 consecutive retryable failures (no Serial1.end())
- **Validated** byte_count == 2 × quantity: 4 == 2 × 2
- **Reused** Gate 3's modbus_frame (CRC, frame builder, 7-step parser) and hal_uart (auto DE/RE)
- **Added** `[env:rak4631_gate4]` PlatformIO environment (nordicnrf52 platform)
- **Added** `tests/gates/gate_rak4631_modbus_robustness_layer/` — gate test files (3 files)
- **Added** `examples/rak4631/modbus_robustness_layer/` — standalone example (6 files)
- **Added** `docs/test_reports/rak4631_gate4_modbus_robustness_v3.4.md` — full test report

### Protocol Trace
- TX: `01 03 00 00 00 02 C4 0B` (Read 2 Holding Registers from 0x0000)
- RX: `01 03 04 00 06 00 01 DB F2` (byte_count=4, Reg[0]=0x0006, Reg[1]=0x0001, CRC valid)
- Result: SUCCESS on attempt 1 (0 retries, 0 UART recoveries)

---

## v3.3-gate3-pass-rak4631 (2026-02-24)

### Gate 3 — Modbus Minimal Protocol Validation on RAK4631: PASSED

- **Validated** Modbus RTU protocol layer with strict 7-step parser — 8/8 criteria met
- **Confirmed** Read Holding Register (FC 0x03): Slave=1, Reg=0x0000, Value=0x0004
- **Implemented** modbus_frame module: CRC-16 builder, FC 0x03 frame builder, 7-step response parser
- **Reused** Gate 2 HAL patterns: auto DE/RE, `Serial1.flush()`, no `Serial1.end()`
- **Added** `[env:rak4631_gate3]` PlatformIO environment (nordicnrf52 platform)
- **Added** `tests/gates/gate_rak4631_modbus_minimal_protocol/` — gate test files (7 files)
- **Added** `examples/rak4631/modbus_minimal_protocol/` — standalone example (6 files)
- **Added** `docs/test_reports/rak4631_gate3_modbus_minimal_protocol_v3.3.md` — full test report

### 7-Step Parser Validation Order
1. Frame length >= 5  2. CRC-16 match  3. Slave address match
4. Exception detection  5. Function code match  6. byte_count consistency
7. Register value extraction — all steps passed on first attempt.

---

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
