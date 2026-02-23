# Wiring — Modbus RTU via RAK5802 (RS485)

## Hardware Required

| Component | Model | Role |
|-----------|-------|------|
| Core Module | RAK3312 (ESP32-S3) | MCU |
| Base Board | RAK19007 | WisBlock carrier |
| RS485 Module | RAK5802 | IO Slot — RS485 transceiver |
| RS485 Device | Any Modbus RTU slave | e.g. RS-FSJT-N01 wind sensor |
| Power | 12V DC (for sensor) | Sensor power supply |
| Cable | 2-wire twisted pair + GND | RS485 bus |

## RAK5802 Screw Terminal

The RAK5802 has a 4-pin screw terminal:

```
+---------+
| A  B  G |  (left to right, facing screw heads)
+---------+
```

| Terminal | Signal | Connect To |
|----------|--------|------------|
| A | RS485 A (+) | Slave device A |
| B | RS485 B (-) | Slave device B |
| G | GND (common) | Slave device GND |

## Wiring Diagram

```
 RAK3312 (ESP32-S3)          RAK5802 (IO Slot)          RS485 Slave
+-------------------+       +------------------+       +-----------+
|                   |       |                  |       |           |
| GPIO 43 (TX) ----+------>| UART1 TX         |       |           |
| GPIO 44 (RX) <---+-------| UART1 RX         |       |           |
|                   |       |                  |       |           |
| GPIO 14 (EN) ----+------>| Module Enable    |       |           |
| GPIO 21 (DE) ----+------>| Driver Enable    |       |           |
|                   |       |                  |       |           |
|                   |       | A ---- twisted --+------>| A (+)     |
|                   |       | B ---- pair -----+------>| B (-)     |
|                   |       | GND -------------+------>| GND       |
+-------------------+       +------------------+       +-----------+
                                                        |
                                                        | 12V DC
                                                        | (sensor power)
```

## RS485 Bus Notes

- **Termination:** Not required for short runs (< 10m). For longer runs, add 120-ohm resistor between A and B at each end.
- **GND:** Always connect GND between RAK5802 and slave. Floating GND causes communication failures.
- **A/B polarity:** A is positive (+), B is negative (-) at idle. If no response, try swapping A/B.
- **Half-duplex:** The RAK5802 DE pin controls direction. HIGH = transmit, LOW = receive.

## Validated Configuration

| Parameter | Value |
|-----------|-------|
| Slave Address | 1 |
| Baud Rate | 4800 |
| Frame Format | 8N1 (8 data, no parity, 1 stop) |
| Protocol | Modbus RTU |
| Test Device | RS-FSJT-N01 wind speed sensor |
| Bus Voltage | A=3.1V, B=1.8V (idle) |
