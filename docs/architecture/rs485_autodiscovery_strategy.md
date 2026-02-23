# RS485 Auto-Discovery Strategy v1

**Hardware Profile:** v1.0
**Date:** 2026-02-23
**Status:** FROZEN

---

## Objective

Automatically detect a Modbus RTU slave device on the RS485 bus by scanning through all valid combinations of baud rate and parity until a valid response is received.

## Scan Matrix

| # | Baud Rate | Parity | Frame |
|---|-----------|--------|-------|
| 1 | 4800 | None | 8N1 |
| 2 | 4800 | Odd | 8O1 |
| 3 | 4800 | Even | 8E1 |
| 4 | 9600 | None | 8N1 |
| 5 | 9600 | Odd | 8O1 |
| 6 | 9600 | Even | 8E1 |
| 7 | 19200 | None | 8N1 |
| 8 | 19200 | Odd | 8O1 |
| 9 | 19200 | Even | 8E1 |
| 10 | 115200 | None | 8N1 |
| 11 | 115200 | Odd | 8O1 |
| 12 | 115200 | Even | 8E1 |

**Total combinations:** 12 (4 baud rates x 3 parity modes)

## Scan Parameters

| Parameter | Value |
|-----------|-------|
| Data Bits | 8 (fixed) |
| Stop Bits | 1 (fixed) |
| Slave Address Range | 1 - 247 |
| Stop Condition | **First valid response** |
| Modbus Function | Read Holding Registers (0x03) or Device ID (0x11) |

## Algorithm

```
FOR each combination IN scan_matrix (ordered 1-12):
    1. Configure UART: baud_rate, parity, 8 data bits, 1 stop bit
    2. Wait for UART stabilization (50ms)
    3. FOR slave_addr = 1 TO 247:
        a. Send Modbus RTU request (function 0x03, register 0x0000, count 1)
        b. Wait for response (timeout = response_timeout_ms)
        c. IF valid Modbus response received:
            - Record: slave_addr, baud_rate, parity
            - RETURN SUCCESS (stop all scanning)
        d. IF Modbus exception response received:
            - Device found (address valid, register may differ)
            - Record: slave_addr, baud_rate, parity
            - RETURN SUCCESS (stop all scanning)
        e. IF timeout or CRC error:
            - Continue to next address
    4. All addresses exhausted for this combination
    5. Continue to next combination

IF all 12 combinations exhausted with no response:
    RETURN FAILURE (no device found)
```

## Timing Calculations

### Response Timeout per Address

| Baud Rate | 3.5 char time | Recommended Timeout |
|-----------|--------------|---------------------|
| 4800 | ~7.3 ms | 100 ms |
| 9600 | ~3.6 ms | 100 ms |
| 19200 | ~1.8 ms | 50 ms |
| 115200 | ~0.3 ms | 50 ms |

### Worst-Case Scan Duration

| Baud Rate | Timeout | Addresses | Time per Combo |
|-----------|---------|-----------|----------------|
| 4800 | 100 ms | 247 | ~24.7 s |
| 9600 | 100 ms | 247 | ~24.7 s |
| 19200 | 50 ms | 247 | ~12.4 s |
| 115200 | 50 ms | 247 | ~12.4 s |

**Worst-case total (all 12 combos, no device):** ~222 seconds (~3.7 minutes)

**Best-case (device at address 1, first combo):** < 200 ms

## Success Output

On successful discovery, store the following in a runtime configuration struct:

```
discovered_slave_addr   : uint8_t
discovered_baud_rate    : uint32_t
discovered_parity       : uint8_t (0=None, 1=Odd, 2=Even)
discovered_data_bits    : uint8_t (always 8)
discovered_stop_bits    : uint8_t (always 1)
```

## Failure Handling

- If no device found after full scan: log error, retry after configurable delay.
- Retry count should be configurable (default: 3 retries before giving up).
- On persistent failure: report status via LoRaWAN uplink.

## Notes

- The scan uses Modbus function 0x03 (Read Holding Registers) as the probe because it is universally supported by Modbus RTU devices.
- A Modbus exception response (e.g., illegal function, illegal address) still confirms a device is present at that address with the current baud/parity.
- CRC errors or timeouts indicate wrong baud rate, wrong parity, or no device at that address.
- Once discovered, the baud rate and parity should be cached and reused for all subsequent communication.
