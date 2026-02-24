# Wiring Guide — RAK4631 Production Loop Soak

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

## LoRaWAN Gateway

| Component | Requirement |
|-----------|-------------|
| ChirpStack v4 gateway | Within LoRa range of RAK4631 |
| Frequency plan | AS923-1 |
| Device profile | OTAA, Class A, LoRaWAN 1.0.2 |
| DevEUI | `88:82:24:44:AE:ED:1E:B2` (or read from FICR) |
| AppEUI | `00:00:00:00:00:00:00:00` |
| AppKey | `91:8C:49:08:F0:89:50:6B:30:18:0B:62:65:9A:4A:D5` |

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

RAK4631 SX1262 (built-in)       LoRa Gateway            ChirpStack
┌──────────────┐    ┌────────────────────────┐    ┌──────────────┐
│  SX1262 RF   │~~~│  LoRa AS923-1          │───│  Network     │
│  (on-board)  │    │  (via antenna)          │    │  Server v4   │
└──────────────┘    └────────────────────────┘    └──────────────┘
```

## Production Soak Data Flow

```
Sensor (RS-FSJT-N01)
   │  ← Modbus RTU read (FC 0x03, qty=2)
   ▼
TaskScheduler (30s interval, millis()-based)
   │  → sensor_uplink_task() — fires naturally via tick()
   │  → NO fireNow() — real production timing
   ▼
RAK4631 (nRF52840)
   │  → Extract Reg[0] (wind speed) + Reg[1] (wind direction)
   │  → Encode as 4-byte payload (big-endian)
   │  → Track soak metrics: modbus/uplink OK/FAIL, cycle timing
   ▼
SX1262 (LoRa radio)
   │  → Unconfirmed uplink (port 10)
   ▼
ChirpStack v4
   │  → Decode payload in device events
   │  → ~10 uplinks observed per 5-minute soak
```

## Key Differences from Gate 6

| Aspect | Gate 6 (Forced Cycles) | Gate 7 (Production Soak) |
|--------|------------------------|--------------------------|
| Pattern | fireNow() × 3 cycles | tick() for 5 minutes |
| Duration | ~45 seconds total | ~5.5 minutes (join + soak) |
| Scheduling | Forced immediate fire | Natural 30s interval |
| Metrics | Per-cycle pass/fail | Soak aggregate: OK/FAIL counts |
| Validation | All 3 cycles must pass | Min uplinks, max consec fail, cycle timing |

## Key Notes

1. **Auto-direction:** RAK5802 handles DE/RE automatically. No GPIO toggling.
2. **`Serial1.flush()`:** Required after write to ensure TX completes.
3. **No `Serial1.end()`:** Hangs on nRF52840 — do not call.
4. **nRF52840 parity:** Only NONE and EVEN (no Odd in UARTE hardware).
5. **Sensor power:** External 12V/24V required.
6. **LoRa antenna:** Must be connected to SX1262 antenna port on RAK4631.
7. **Build flags:** Must use `-DRAK4630 -D_VARIANT_RAK4630_` for SPI_LORA instantiation.
8. **No fireNow():** Scheduler runs naturally. Cycles fire every 30 seconds via tick().
9. **Soak duration:** 5 minutes. Allow ~6 minutes total (join + soak + summary).
