# Downlink Command Schema — Binary Protocol v1

## Downlink Frame Format (fport = 10)

| Offset | Field   | Size  | Value / Description |
|--------|---------|-------|---------------------|
| 0      | MAGIC   | 1     | `0xD1` — identifies a command frame |
| 1      | VERSION | 1     | `0x01` — schema version |
| 2      | CMD     | 1     | Command code (see below) |
| 3..N   | PAYLOAD | 0–4   | Command-specific payload |

## Command Table

| CMD  | Name           | Payload          | Valid Range    | Bytes | Effect |
|------|----------------|------------------|----------------|-------|--------|
| 0x01 | SET_INTERVAL   | uint32_t BE (ms) | 1000..3600000  | 4     | Changes scheduler task 0 interval |
| 0x02 | SET_SLAVE_ID   | uint8_t          | 1..247         | 1     | Changes Modbus slave address |
| 0x03 | REQ_STATUS     | (none)           | —              | 0     | Triggers status uplink |
| 0x04 | REBOOT         | uint8_t safety   | must be `0xA5` | 1     | Software reset (after ACK uplink) |

## ACK Uplink Frame (fport = 11)

Sent after every successfully parsed command:

| Offset | Field   | Size | Description |
|--------|---------|------|-------------|
| 0      | MAGIC   | 1    | `0xD1` |
| 1      | VERSION | 1    | `0x01` |
| 2      | CMD     | 1    | Echo of applied command |
| 3      | STATUS  | 1    | `0x01` = ACK, `0x00` = NACK |
| 4..N   | VALUE   | 0–4  | Current value after change |

## ACK Payload Examples

| Command | ACK Hex | Length | Description |
|---------|---------|--------|-------------|
| SET_INTERVAL 10s | `D1 01 01 01 00 00 27 10` | 8 | 4-byte BE interval |
| SET_SLAVE_ID 5 | `D1 01 02 01 05` | 5 | 1-byte slave addr |
| REQ_STATUS | `D1 01 03 01 SS II II II II UU UU UU UU MM MM MM MM` | 17 | slave_id + interval_ms + uplink_ok + modbus_ok (all BE) |
| REBOOT (valid) | `D1 01 04 01 A5` | 5 | Safety key echoed |
| REBOOT (invalid) | `D1 01 04 00` | 4 | NACK — bad safety key |
| Out-of-range | `D1 01 01 00` | 4 | NACK — STATUS=0x00 |

## Rejection Rules

| Condition | Behavior |
|-----------|----------|
| Frame < 3 bytes | Silently ignored (no ACK) |
| MAGIC ≠ 0xD1 | Silently ignored |
| VERSION ≠ 0x01 | Silently ignored |
| Unknown CMD | NACK uplink (STATUS=0x00) |
| Payload too short for CMD | NACK uplink |
| Value out of valid range | NACK uplink |

## Byte Order

All multi-byte integers are **big-endian** (network byte order).

Example: 10000 ms = `0x00002710` → bytes `00 00 27 10`.

## Port Assignment

| Port | Direction | Purpose |
|------|-----------|---------|
| 10   | Downlink  | Command frames |
| 10   | Uplink    | Sensor data (Modbus register values) |
| 11   | Uplink    | ACK/NACK responses to commands |
