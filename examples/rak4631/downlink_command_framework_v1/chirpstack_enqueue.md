# ChirpStack Downlink Enqueue Guide

## Prerequisites

- Device must be **joined** (OTAA completed)
- At least **one uplink** sent (opens RX1/RX2 windows)
- ChirpStack v4 web UI access

## How to Enqueue a Downlink

1. Open **ChirpStack** → **Applications** → **Your App** → **Devices**
2. Select the device (DevEUI `88:82:24:44:AE:ED:1E:B2`)
3. Go to the **Queue** tab
4. Click **Enqueue downlink**
5. Set **fport = 10**
6. Enter **hex payload** (no spaces in ChirpStack input)
7. Click **Enqueue**

The downlink will be delivered in the next **RX1 or RX2 window** after the device sends an uplink.

## Test Payloads

### Valid Commands

| Test | fport | Hex (no spaces) | Description |
|------|-------|------------------|-------------|
| Set interval to 10s | 10 | `D101010000271` | 0x2710 = 10000 ms |
| Set interval to 60s | 10 | `D10101 0000EA60` | 0xEA60 = 60000 ms |
| Set slave ID to 5 | 10 | `D1010205` | Slave address 5 |
| Request status | 10 | `D10103` | No payload |
| Reboot (valid key) | 10 | `D10104A5` | Safety key 0xA5 |

### Invalid Commands (for NACK testing)

| Test | fport | Hex (no spaces) | Expected |
|------|-------|------------------|----------|
| Bad magic | 10 | `FF01010000271` | Silently ignored |
| Bad version | 10 | `D10201000027 10` | Silently ignored |
| Interval too low (1 ms) | 10 | `D1010100000001` | NACK (out of range) |
| Interval too high (4M ms) | 10 | `D101013D090000` | NACK (out of range) |
| Slave ID 0 | 10 | `D1010200` | NACK (out of range) |
| Bad reboot key | 10 | `D10104FF` | NACK (invalid key) |
| Unknown CMD 0xFF | 10 | `D101FF` | NACK (unknown) |
| Truncated SET_INTERVAL | 10 | `D10101FFFF` | NACK (payload too short) |

## Expected ACK Uplinks (fport = 11)

After the device processes a command, it sends an ACK on **fport 11**:

| Command | Expected ACK Hex | Meaning |
|---------|------------------|---------|
| SET_INTERVAL 10s | `D10101010000271` | ACK + new interval (10000 ms BE) |
| SET_SLAVE_ID 5 | `D101020105` | ACK + new slave addr (5) |
| REQ_STATUS | `D1010301...` | ACK + 13-byte status payload |
| REBOOT (valid) | `D10104 01A5` | ACK + safety key echo |
| Out-of-range | `D1010100` | NACK (STATUS=0x00) |

## Timing

- **RX1 window**: Opens 1 second after uplink TX complete
- **RX2 window**: Opens 2 seconds after uplink TX complete
- **Delivery**: Downlink is queued; delivered in the *next* RX window
- **Gate 8 flow**: 2 initial uplinks (5s apart) → 60s poll → scheduler ticks every 30s

## Verifying in ChirpStack

After the device processes the downlink:

1. Go to **Devices → Events** tab
2. Look for an **Uplink** event on **fport 11**
3. Decode the hex payload to verify the ACK frame matches expected format
4. For REQ_STATUS: decode the 17-byte payload to see runtime stats
