# rak-wisblock-lab-template

WisBlock lab template v1. Starter project for RAK WisBlock modules — pre-configured with OTAA join, sensor read loop, and Cayenne LPP payload encoding.

## Overview

| Attribute | Value |
|-----------|-------|
| Platform  | RAK4630 (WisBlock Core) |
| Framework | Arduino / PlatformIO |
| Protocol  | LoRaWAN (OTAA) |
| Group     | `hw-rak4630` |

## Features

- OTAA join with retry logic
- Sensor read loop (configurable interval)
- Cayenne LPP payload builder
- ADC, I2C, and UART sensor examples
- Low-power sleep between transmissions
- Serial debug output

## Getting Started

```bash
git clone https://github.com/rnd-southerniot/rak-wisblock-lab-template
cd rak-wisblock-lab-template

# PlatformIO
pio run -t upload

# Or open in Arduino IDE with RAK BSP installed
```

## Configuration (`config.h`)

```cpp
#define OTAA_DEVEUI  {0xAC, 0x1F, 0x09, ...}
#define OTAA_APPEUI  {0x00, 0x00, ...}
#define OTAA_APPKEY  {0x01, 0x02, ...}
#define TX_INTERVAL  60   // seconds
#define DR           3    // Data rate
```

## Payload Structure (Cayenne LPP)

| Channel | Type | Description |
|---------|------|-------------|
| 1 | Temperature | Sensor temp (°C) |
| 2 | Humidity | Relative humidity |
| 3 | Analog | ADC raw value |

## Related

- [`rak4630-eink-display`](https://github.com/rnd-southerniot/rak4630-eink-display)
- [`rak3172-sensor-node`](https://github.com/rnd-southerniot/rak3172-sensor-node)

## License

MIT
