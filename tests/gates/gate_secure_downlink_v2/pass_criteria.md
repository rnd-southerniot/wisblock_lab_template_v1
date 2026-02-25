# Gate 10 — Secure Downlink Protocol v2: Pass Criteria

## Gate Identity
- **Name:** `gate_secure_downlink_v2`
- **ID:** 10
- **Version:** 2.0
- **Hardware:** RAK3312 (ESP32-S3 + SX1262) on RAK19007

## Objective
Validate the Secure Downlink Protocol v2 security module end-to-end:
HMAC-SHA256 authentication, monotonic nonce replay protection, magic/version
enforcement, command dispatch, and live over-the-air downlink processing.

## Protocol Specification

### Frame Format
```
Offset  Size   Field               Encoding
------  ----   -----               --------
0       1      Magic               0xD1 (fixed)
1       1      Protocol Version    0x02 (fixed)
2       1      Command ID          SDL_CMD_* constant
3       1      Flags               0x00 (reserved)
4       4      Nonce               uint32_t big-endian, monotonically increasing
8       0-48   Payload             Command-specific, big-endian
N-4     4      HMAC Truncated      First 4 bytes of HMAC-SHA256
```

- **Minimum frame:** 12 bytes (no payload)
- **Maximum frame:** 60 bytes (48-byte payload)

### HMAC Computation
| Parameter       | Value                                              |
|-----------------|----------------------------------------------------|
| Algorithm       | HMAC-SHA256 (RFC 2104)                             |
| Key length      | 32 bytes                                           |
| Input           | `frame[0 .. len-5]` (all bytes except HMAC field)  |
| Output          | 32-byte SHA-256 HMAC                               |
| Truncation      | First 4 bytes (MSB-first / big-endian truncation)  |

### Nonce Validation
| Rule              | Description                                      |
|-------------------|--------------------------------------------------|
| Monotonic         | `nonce > last_valid_nonce` (strict greater-than)  |
| Persistence       | Written to flash via `storage_hal_write_u32()`   |
| Storage key       | `"sdl_nonce"` (`SDL_NONCE_STORAGE_KEY`)          |
| Default on boot   | `0` (`SDL_NONCE_DEFAULT`)                        |
| Update timing     | Written AFTER successful command application     |

### Validation Order
1. Length check (`SDL_FRAME_MIN_LEN <= len <= SDL_FRAME_MAX_LEN`)
2. Magic check (`buf[0] == 0xD1`)
3. Version check (`buf[1] == 0x02`)
4. Nonce check (replay protection — before HMAC to save CPU)
5. HMAC check (authentication)
6. Command dispatch
7. Nonce persist

### Commands
| ID     | Name                   | Payload                    | Payload Size |
|--------|------------------------|----------------------------|--------------|
| `0x01` | `SDL_CMD_SET_INTERVAL`  | `uint32_t interval_ms` (BE) | 4 bytes     |
| `0x02` | `SDL_CMD_SET_SLAVE_ADDR`| `uint8_t addr`             | 1 byte       |
| `0x03` | `SDL_CMD_REQUEST_STATUS`| (none)                     | 0 bytes      |
| `0x04` | `SDL_CMD_REBOOT`       | (none)                     | 0 bytes      |

## Test Configuration

### Test Key (32 bytes — gate testing only)
```
00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F
```

### Reference Frame: SET_INTERVAL to 10000ms, nonce=1
```
Offset  Hex             Field
------  ---             -----
0       D1              Magic (0xD1)
1       02              Version (0x02)
2       01              CMD_SET_INTERVAL
3       00              Flags (reserved)
4-7     00 00 00 01     Nonce = 1 (big-endian)
8-11    00 00 27 10     Payload = 10000 ms (big-endian)
12-15   A2 7F F4 AB     HMAC-SHA256 truncated (first 4 bytes)
```

**Complete frame (16 bytes):**
```
D1 02 01 00 00 00 00 01 00 00 27 10 A2 7F F4 AB
```

**HMAC verification:**
```
HMAC-SHA256(key, D1 02 01 00 00 00 00 01 00 00 27 10) =
  a27ff4ab 957d28a1 1418b1a8 28efa56e c9ab2d66 844016d8 ad769d19 c41fec5f
Truncated: A2 7F F4 AB
```

## Pass Criteria (all must be met)

### Phase A — Local Synthetic Validation

#### STEP 1: SystemManager Init
SystemManager::init() succeeds. Required for command dispatch target.

#### STEP 2: Nonce Storage Reset
Persisted nonce cleared to `GATE_SDL_INITIAL_NONCE` (0). Readback confirms.

#### STEP 3: Valid Frame Accepted
Submit reference frame (SET_INTERVAL, nonce=1, 10000ms).
- `result.valid == true`
- `result.cmd_id == 0x01`
- All error flags false

#### STEP 4: Replay Rejected
Submit identical frame again (same nonce=1).
- `result.valid == false`
- `result.replay_fail == true`

#### STEP 5: Bad Magic Rejected
Submit frame with magic byte `0xAA` instead of `0xD1`.
- `result.valid == false`
- `result.version_fail == true`
- HMAC check must NOT execute (fail-fast)

#### STEP 6: Tampered HMAC Rejected
Submit frame with valid structure but zeroed HMAC.
- `result.valid == false`
- `result.auth_fail == true`

#### STEP 7: Nonce Persistence Verified
Read back persisted nonce.
- Stored value == 1 (from STEP 3, unmodified by rejected frames)

### Phase B — Live Over-the-Air Validation

#### STEP 8: Live Downlink from ChirpStack
Wait up to 120 seconds for a downlink from the network server.
Operator queues a valid secure downlink via ChirpStack.
- Downlink received within timeout
- Frame passes full validation pipeline (magic, version, nonce, HMAC)
- Command applied to SystemManager

#### STEP 9: Live Replay Rejection
Re-submit the same live downlink frame.
- `result.valid == false`
- `result.replay_fail == true`

## Fail Codes

| Code | Constant                        | Meaning                                    |
|------|---------------------------------|--------------------------------------------|
| 0    | `GATE_PASS`                     | All security checks passed                 |
| 1    | `GATE_FAIL_INIT`                | SystemManager::init() failed               |
| 2    | `GATE_FAIL_VALID_REJECTED`      | Valid frame was incorrectly rejected        |
| 3    | `GATE_FAIL_WRONG_CMD`           | Parsed cmd_id does not match expected       |
| 4    | `GATE_FAIL_REPLAY_ACCEPTED`     | Replay frame was incorrectly accepted       |
| 5    | `GATE_FAIL_REPLAY_FLAG`         | Replay rejected but flag not set            |
| 6    | `GATE_FAIL_BAD_VERSION_ACCEPTED`| Bad magic/version frame was accepted        |
| 7    | `GATE_FAIL_VERSION_FLAG`        | Rejected but version_fail not set           |
| 8    | `GATE_FAIL_BAD_HMAC_ACCEPTED`   | Tampered HMAC frame was accepted            |
| 9    | `GATE_FAIL_AUTH_FLAG`           | HMAC rejected but auth_fail not set         |
| 10   | `GATE_FAIL_NONCE_PERSIST`       | Nonce not persisted after valid command      |
| 11   | `GATE_FAIL_DL_TIMEOUT`          | No downlink received within 120s timeout    |
| 12   | `GATE_FAIL_DL_INVALID`          | Live downlink failed validation             |
| 13   | `GATE_FAIL_DL_REPLAY_ACCEPTED`  | Live downlink replay was accepted           |
| 14   | `GATE_FAIL_SYSTEM_CRASH`        | Runner did not reach GATE COMPLETE          |

## Verification
1. Serial output shows `[GATE10] PASS` and `[GATE10] === GATE COMPLETE ===`
2. All 9 steps report individual PASS
3. No Guru Meditation errors or watchdog resets
4. Nonce storage survives read-back in STEP 7
5. ChirpStack shows downlink was delivered in STEP 8
