# Gate 8 — Downlink Command Framework v1: Pass Criteria

## Gate Identity
- **Gate ID**: 8
- **Name**: `gate_downlink_command_framework_v1`
- **Platforms**: RAK4631 (nRF52840), RAK3312 (ESP32-S3)

## Pass Conditions

### PASS (with downlink)
All of the following must be true:
1. SystemManager init succeeds (LoRa join, Modbus, scheduler)
2. Initial uplinks sent successfully (opens RX windows)
3. Downlink received within 60-second window
4. Command parsed and applied without crash
5. ACK uplink sent on fport 11
6. Serial output shows `PASS — downlink received and processed`
7. Serial output shows `=== GATE COMPLETE ===`

### PASS_WITHOUT_DOWNLINK (no downlink scheduled)
All of the following must be true:
1. SystemManager init succeeds
2. Initial uplinks sent successfully
3. 60-second downlink wait completes with no downlink
4. Serial output shows `PASS_WITHOUT_DOWNLINK — no downlink scheduled`
5. Serial output shows `=== GATE COMPLETE ===`

Both outcomes are valid PASS conditions.

## Fail Codes

| Code | Name | Description |
|------|------|-------------|
| 0 | GATE_PASS | Gate passed |
| 1 | GATE_FAIL_INIT | SystemManager::init() failed |
| 2 | GATE_FAIL_JOIN | OTAA join failed |
| 3 | GATE_FAIL_UPLINK | Initial uplinks failed |
| 4 | GATE_FAIL_PARSE | Downlink parse error |
| 5 | GATE_FAIL_APPLY | Downlink command apply error |

## Expected Serial Output

### Success (with downlink)
```
[GATE8] STEP 1: SystemManager init
[GATE8] STEP 1: PASS
[GATE8] STEP 2: Initial uplinks to open RX windows
[GATE8] Uplink 1/2 sent (cycle 1)
[GATE8] Uplink 2/2 sent (cycle 2)
[GATE8] STEP 2: PASS
[GATE8] STEP 3: Polling for downlink
[GATE8] Downlink received! port=10, len=7, hex=D1 01 01 00 00 27 10
[GATE8] Parse result: ACK SET_INTERVAL: 10000 ms
[GATE8] Sending ACK uplink (fport=11, len=8): D1 01 01 01 00 00 27 10
[GATE8] ACK uplink sent OK
[GATE8] PASS — downlink received and processed
[GATE8] === GATE COMPLETE ===
```

### Success (without downlink)
```
[GATE8] STEP 1: SystemManager init
[GATE8] STEP 1: PASS
[GATE8] STEP 2: Initial uplinks to open RX windows
[GATE8] STEP 2: PASS
[GATE8] STEP 3: Polling for downlink
[GATE8] PASS_WITHOUT_DOWNLINK — no downlink scheduled
[GATE8] === GATE COMPLETE ===
```

## ChirpStack Downlink Test Instructions

### Prerequisites
- Device must be joined and have sent at least one uplink
- Downlinks are queued and delivered in the next RX1/RX2 window

### How to Enqueue a Downlink
1. Open **ChirpStack** > **Applications** > **Your App** > **Devices** > select device
2. Go to **Queue** tab
3. Click **Enqueue downlink**
4. Set fport = 10
5. Enter hex payload (see test payloads below)

### Test Payloads

| Test | fport | Hex Payload | Description |
|------|-------|-------------|-------------|
| Set interval to 10s | 10 | `D101010000271` | 0x2710 = 10000ms |
| Set interval to 60s | 10 | `D10101 0000EA60` | 0xEA60 = 60000ms |
| Set slave ID to 5 | 10 | `D1010205` | Slave address 5 |
| Request status | 10 | `D10103` | No payload |
| Reboot | 10 | `D10104A5` | Safety key 0xA5 |
| Invalid (bad magic) | 10 | `FF01010000271` | Should be silently ignored |
| Invalid (bad range) | 10 | `D1010100000001` | 1ms — below minimum, expect NACK |

### Expected ACK Uplinks (fport = 11)

| Command | Expected ACK Hex |
|---------|-----------------|
| SET_INTERVAL 10s | `D1 01 01 01 00 00 27 10` |
| SET_SLAVE_ID 5 | `D1 01 02 01 05` |
| REQ_STATUS | `D1 01 03 01 ...` (17 bytes with runtime stats) |
| REBOOT (valid) | `D1 01 04 01 A5` (sent before reboot) |
| Out-of-range | `D1 01 01 00` (NACK, STATUS=0x00) |

## Binary Command Schema Reference

### Downlink Frame (fport = 10)
| Offset | Field | Size | Description |
|--------|-------|------|-------------|
| 0 | MAGIC | 1 | `0xD1` |
| 1 | VERSION | 1 | `0x01` |
| 2 | CMD | 1 | Command code |
| 3..N | PAYLOAD | 0-4 | Command-specific |

### ACK Uplink Frame (fport = 11)
| Offset | Field | Size | Description |
|--------|-------|------|-------------|
| 0 | MAGIC | 1 | `0xD1` |
| 1 | VERSION | 1 | `0x01` |
| 2 | CMD | 1 | Echo of command |
| 3 | STATUS | 1 | 0x01=ACK, 0x00=NACK |
| 4..N | VALUE | 0-4 | Current value |
