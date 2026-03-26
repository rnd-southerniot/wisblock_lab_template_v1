# Wiring Guide — RAK4631 RS485 Modbus Autodiscovery

## WisBlock Assembly

| Component | Slot | Notes |
|-----------|------|-------|
| RAK4631 WisBlock Core | Core slot | nRF52840 + SX1262 |
| RAK5802 RS485 Module | **Slot A** | Must be Slot A for Serial1 TX/RX |
| RAK19007 Base Board | — | USB-C for power + serial |

> **IMPORTANT:** The RAK5802 MUST be in **Slot A**. This is the only IO slot
> wired to Serial1 (TX=pin 16, RX=pin 15) on the RAK4631.

## RAK5802 Screw Terminal → Modbus Sensor

| RAK5802 Terminal | Wire Color (typical) | Sensor Terminal | Notes |
|-----------------|----------------------|-----------------|-------|
| A+ | Yellow/Orange | A+ | RS485 non-inverting |
| B- | Blue/White | B- | RS485 inverting |
| GND | Black | GND | Common ground reference |

> **IMPORTANT:** The Modbus sensor requires its own external power supply
> (typically 12V or 24V DC). The RAK5802 does NOT provide power to the sensor.

## Power Architecture

```
USB-C ──→ RAK19007 Base ──→ 3.3V main rail
                            │
                     WB_IO2 (pin 34) HIGH
                            │
                       3V3_S rail ──→ RAK5802 (TP8485E powered)
                                      │
                                  Auto DE/RE circuit
                                      │
                                   A+ / B- ──→ External RS485 bus
```

## Signal Path

```
RAK4631 nRF52840                RAK5802 (TP8485E)          Sensor
┌──────────────┐    ┌────────────────────────────┐    ┌──────────┐
│  Serial1 TX  │──→│  DI  →  Auto-DE  →  A+/B-  │──→│  RS485   │
│  (P0.16)     │    │                             │    │  RX      │
│              │    │                             │    │          │
│  Serial1 RX  │←──│  RO  ←  Auto-RE  ←  A+/B-  │←──│  RS485   │
│  (P0.15)     │    │                             │    │  TX      │
│              │    │                             │    │          │
│  WB_IO2=HIGH │──→│  3V3_S power enable          │    │          │
│  (P1.02)     │    └────────────────────────────┘    └──────────┘
└──────────────┘
```

## Key Notes

1. **Auto-direction:** The RAK5802 handles DE/RE switching automatically via
   an on-board circuit tied to the UART TX line. No GPIO toggling needed.
   WB_IO1 (pin 17) is NC (not connected) on the RAK5802.

2. **No `Serial1.end()`:** On nRF52840, calling `Serial1.end()` before
   `Serial1.begin()` will hang. Just call `begin()` directly.

3. **`Serial1.flush()`:** Required after `Serial1.write()` to ensure TX
   completes before the auto-direction circuit switches back to RX mode.

4. **nRF52840 parity:** Only NONE and EVEN supported (no Odd in UARTE hardware).

5. **Sensor power:** External 12V/24V required. Verify sensor power LED is on
   before running the scan.
