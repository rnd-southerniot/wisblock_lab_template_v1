# Wiring Guide — RAK4631 Modbus Robustness Layer

## WisBlock Assembly

| Component | Slot | Notes |
|-----------|------|-------|
| RAK4631 WisBlock Core | Core slot | nRF52840 + SX1262 |
| RAK5802 RS485 Module | **Slot A** | Must be Slot A for Serial1 TX/RX |
| RAK19007 Base Board | — | USB-C for power + serial |

> **IMPORTANT:** The RAK5802 MUST be in **Slot A**. This is the only IO slot
> wired to Serial1 (TX=pin 16, RX=pin 15) on the RAK4631.

## RAK5802 Screw Terminal → Modbus Sensor

| RAK5802 Terminal | Sensor Terminal | Notes |
|-----------------|-----------------|-------|
| A+ | A+ | RS485 non-inverting |
| B- | B- | RS485 inverting |
| GND | GND | Common ground reference |

> The Modbus sensor requires its own external power supply (12V/24V).

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

## Modbus RTU Protocol (Multi-Register)

This gate validates a multi-register read (qty=2) with retry support:

```
RAK4631                          Modbus Slave
   │                                  │
   │── TX: 01 03 00 00 00 02 C4 0B ─→│  Read 2 Holding Regs from 0x0000
   │                                  │
   │←─ RX: 01 03 04 XX XX YY YY CC ──│  byte_count=4, Reg[0], Reg[1]
   │                                  │
   │  (if timeout or CRC fail, retry) │
   │                                  │
```

## Key Notes

1. **Auto-direction:** RAK5802 handles DE/RE automatically. No GPIO toggling.
2. **`Serial1.flush()`:** Required after write to ensure TX completes.
3. **No `Serial1.end()`:** Hangs on nRF52840 — UART recovery re-calls `begin()`.
4. **nRF52840 parity:** Only NONE and EVEN (no Odd in UARTE hardware).
5. **Sensor power:** External 12V/24V required.
6. **Retry policy:** 3 retries (4 total), only on TIMEOUT and CRC_FAIL.
