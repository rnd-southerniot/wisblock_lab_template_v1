# WisBlock Multi-Core Lab — Firmware Development Workflow

**Branch:** `hal-consolidation-v1`  
**Version:** 4.1  
**Date:** 2026-02-28  
**Architecture:** RAK4631 (nRF52840) primary · RAK3312 (ESP32-S3) secondary  
**Network:** LoRaWAN AS923-1 · ChirpStack v4 · OTAA  

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Gate Progression Map](#2-gate-progression-map)
3. [Firmware Architecture](#3-firmware-architecture)
4. [Claude Team Workflow](#4-claude-team-workflow)
5. [PlatformIO Conventions](#5-platformio-conventions)
6. [Gate Development Procedure](#6-gate-development-procedure)
7. [Gate 10 — Recovery Status](#7-gate-10--recovery-status)
8. [Project Recovery Procedure](#8-project-recovery-procedure)
9. [Changelog Summary](#9-changelog-summary)
10. [Quick Reference](#10-quick-reference)

---

## 1. Project Overview

A production-grade IoT firmware stack for WisBlock hardware, developed incrementally through numbered validation gates. Each gate proves one subsystem before the next is built. The firmware reads industrial sensors via Modbus RTU, transmits over LoRaWAN, and accepts authenticated remote commands via Secure Downlink Protocol v2.

### Hardware Stack

| Component | Part |
|-----------|------|
| Base Board | RAK19007 WisBlock Base |
| Core A (primary) | RAK4631 — nRF52840 + SX1262 |
| Core B (secondary) | RAK3312 — ESP32-S3 + SX1262 |
| RS485 Module | RAK5802 (IO Slot) — Modbus RTU |
| IMU | RAK1904 LIS3DH (Slot B) — I2C @ 0x18 |
| Sensor (RS485) | RS-FSJT-N01 wind speed — Slave=1, 4800 8N1 |
| LoRaWAN Region | AS923-1 |
| Network Server | ChirpStack v4 (OTAA) |
| Security Protocol | SDL v2 — HMAC-SHA256 + monotonic nonce |

### Repository State

| Item | Value |
|------|-------|
| Branch | `hal-consolidation-v1` |
| Latest tag | `v4.0-persistence-layer` |
| Latest commit | `6175e5b` — storage + gate9 files committed |
| Recovered Gate 10 package | `~/Developer/projects/prod/zip-files-non-git/new_files/` |
| In-repo Gate 10 proof | No local tag, commit, or test report found |
| Next repo action | Run Gate 10 on hardware, capture serial + ChirpStack evidence, then tag/report PASS |

---

## 2. Gate Progression Map

Each gate is an isolated PlatformIO build environment and test runner. Gates must pass in order — later gates depend on hardware assumptions confirmed by earlier ones.

| Gate | Name | Tag | Status |
|------|------|-----|--------|
| 1 | I2C LIS3DH Validation | `v3.1-gate1-pass-rak4631` | ✅ PASSED |
| 2 | RS485 Modbus Autodiscovery | `v3.2-gate2-pass-rak4631` | ✅ PASSED |
| 3 | Modbus Minimal Protocol | `v3.3-gate3-pass-rak4631` | ✅ PASSED |
| 4 | Modbus Robustness Layer | `v3.4-gate4-pass-rak4631` | ✅ PASSED |
| 5 | LoRaWAN OTAA Join + Uplink | `v3.5-gate5-pass-rak4631` | ✅ PASSED |
| 6 | Runtime Scheduler Integration | `v3.6-gate6-pass-rak4631` | ✅ PASSED |
| 7 | Production Loop Soak (5 min) | `v3.7-gate7-pass-rak4631` | ✅ PASSED |
| 8 | Downlink Command Framework v1 | `v3.9-gate8-pass-rak4631` | ✅ PASSED |
| 9 | Persistence Layer (ESP32 + nRF52) | `v4.0-persistence-layer` | ✅ PASSED |
| **10** | **Secure Downlink Protocol v2** | `no local tag/report` | ⚠ SOURCE RESTORED, BUILD VERIFIED |

Recovered on 2026-03-12 from `~/Developer/projects/prod/zip-files-non-git/new_files/`,
restored into this repository, and compile-verified with:
`pio run -e rak4631_gate10` and `pio run -e rak3312_gate10`.

Recovered source package contents:
`gate_config.h`, `gate_runner.cpp`, `pass_criteria.md`, `main.cpp`, and
`platformio_gate10_addition.ini`.

No local git tag (`v4.1-gate10-pass-rak4631`), Gate 10 commit, or
`docs/test_reports/` entry currently proves a completed hardware PASS in this checkout.

---

## 3. Firmware Architecture

### 3.1 Directory Structure

```
wisblock_lab_template_v1-2/
├── src/main.cpp                    # Multi-platform gate dispatcher (v4.1)
├── platformio.ini                  # All PlatformIO environments
├── firmware/
│   ├── config/
│   │   └── hardware_profile.h      # Pin maps, HW constants
│   ├── hal/
│   │   └── hal_common.h            # RAK4631 HAL declarations
│   ├── runtime/
│   │   ├── system_manager.*        # Top-level orchestrator
│   │   ├── scheduler.*             # Cooperative task scheduler
│   │   ├── lora_transport.*        # LoRaWAN via SX126x-Arduino
│   │   ├── modbus_peripheral.*     # Modbus RTU over RS485
│   │   ├── downlink_security.*     # SDL v2 validator
│   │   └── crypto_sha256.*         # SHA-256 (no mbedTLS dependency)
│   └── storage/
│       ├── storage_hal.h           # Platform-agnostic KV store API
│       ├── storage_hal_esp32.cpp   # NVS Preferences (ESP32-S3)
│       └── storage_hal_nrf52.cpp   # LittleFS Preferences (nRF52840)
├── tests/gates/
│   ├── gate_template/              # Scaffold for new gates
│   ├── gate_*/  (gates 1–9)        # Completed gates
│   └── gate_secure_downlink_v2/    # Gate 10 harness
├── .claude-team/
│   ├── agents/                     # AI agent system prompts
│   └── scripts/start-ai-team.sh   # tmux launcher
└── docs/
    ├── CHANGELOG.md
    ├── hardware_profiles/
    └── test_reports/
```

### 3.2 Gate Dispatch — `src/main.cpp` (v4.1)

Gate selection is controlled by `-DGATE_N` build flags. The outermost `#ifdef` wins, so any gate can be active regardless of core target:

```cpp
#ifdef GATE_10      →  gate_secure_downlink_v2_run()
#elif GATE_9        →  gate_persistence_reboot_v1_run()
#elif GATE_8        →  gate_downlink_command_framework_v1_run()
#elif CORE_RAK4631  →  gate_rak4631_production_loop_soak_run()  [default]
#else               →  gate_production_loop_soak_run()           [RAK3312]
```

### 3.3 Runtime Layer — SystemManager Call Hierarchy

```
SystemManager::init()
  └── LoRaTransport::init()       # SX1262 hw_config + lmh_init
  └── LoRaTransport::connect()    # Blocking OTAA join with timeout
  └── ModbusPeripheral::init()    # RS485 EN/DE + UART init
  └── Scheduler::registerTask()   # Register sensorUplinkTask at 30s

SystemManager::tick()             # Non-blocking — call from loop()
  └── Scheduler::tick()           # Fires tasks when interval elapsed
      └── sensorUplinkTask()      # read() → encode → transport.send()
```

### 3.4 Storage HAL API

```cpp
// firmware/storage/storage_hal.h
bool     storage_hal_write_u32(const char* key, uint32_t value);
uint32_t storage_hal_read_u32 (const char* key, uint32_t default_val);
```

Backed by ESP32 NVS (`Preferences`) and nRF52 LittleFS (`Preferences`). Namespace: `"sdl"`. Used by SDL v2 for nonce persistence via key `"sdl_nonce"`.

### 3.5 Secure Downlink Protocol v2 — Frame Format

```
Byte 0    Byte 1    Byte 2   Byte 3   Bytes 4–7    Bytes 8–N    Last 4 bytes
MAGIC     VERSION   CMD_ID   FLAGS    Nonce (BE)   Payload      HMAC-trunc
0xD1      0x02      SDL_CMD  0x00     uint32_t BE  0–48 bytes   First 4 of SHA-256
```

HMAC input: `frame[0 .. len-5]`  
Key length: 32 bytes  
Truncation: first 4 bytes (MSB-first)  
Nonce: monotonically increasing, persisted to flash, defaults to 0 on first boot

| CMD | ID | Payload |
|-----|----|---------|
| `SET_INTERVAL` | `0x01` | uint32_t interval_ms (4 bytes BE) |
| `SET_SLAVE_ADDR` | `0x02` | uint8_t addr (1 byte) |
| `REQUEST_STATUS` | `0x03` | none |
| `REBOOT` | `0x04` | none |

---

## 4. Claude Team Workflow

### 4.1 What Is the Claude Team?

A set of Claude CLI instances running in a tmux session, each loaded with a specialist system prompt from `.claude-team/agents/`. The orchestrator agent coordinates the other agents to develop and validate each gate in sequence.

**Important:** The Claude Team is the development engine. Do not manually edit gate files while the team session is active. Interact through the orchestrator agent only.

### 4.2 Agent Roles

| Agent | Role |
|-------|------|
| `system-orchestrator` | Coordinates gates, reviews pass criteria, commits on PASS |
| `firmware-engineer` | Writes gate runners, HAL code, runtime modules |
| `test-architect` | Defines pass criteria, writes test vectors, reviews FAIL codes |
| `hal-specialist` | Platform-specific HAL: nRF52 / ESP32-S3 abstractions |

### 4.3 Starting the Team

```bash
cd ~/Developer/projects/prod/wisblock_lab_template_v1-2
bash .claude-team/scripts/start-ai-team.sh
```

The script:
- Checks if tmux session `wisblock_lab_template_v1-2-claude` already exists
- If yes: re-attaches without creating a new session (safe to re-run)
- If no: creates new session, spawns one pane per `agents/*.md`, tiles layout

tmux navigation:
- `Ctrl+B` + arrow keys — move between panes
- `Ctrl+B`, `d` — detach session
- `tmux attach -t wisblock_lab_template_v1-2-claude` — re-attach manually

### 4.4 Resuming After a Break

When returning to the project after time away:

1. Check repository state:
   ```bash
   cd ~/Developer/projects/prod/wisblock_lab_template_v1-2
   git log --oneline -5
   git status --short
   git tag --sort=-creatordate | head -5
   ```
2. Identify the current HEAD tag — this is the last completed gate.
3. Check for untracked files that may be uncommitted gate or firmware files.
4. Stage and commit any untracked `firmware/` or `tests/gates/` files before starting the team.
5. Start the Claude Team: `bash .claude-team/scripts/start-ai-team.sh`
6. Tell the orchestrator: `"We are at [tag]. Next gate is Gate N. Resume."`

---

## 5. PlatformIO Conventions

### 5.1 Environment Naming

| Target | Environment Name |
|--------|-----------------|
| RAK4631 default | `rak4631` |
| RAK4631 gate N | `rak4631_gateN` |
| RAK3312 default | `rak3312` |
| RAK3312 gate N | `rak3312_gateN` |

### 5.2 Gate Environment Template (RAK4631)

```ini
[env:rak4631_gateN]
platform = nordicnrf52
board = wiscore_rak4631
framework = arduino
monitor_speed = 115200
board_build.variants_dir = variants
lib_deps =
    beegee-tokyo/SX126x-Arduino@^2.0.32
build_src_filter =
    +<main.cpp>
    +<../tests/gates/gate_name/>
    +<../tests/gates/gate_rak4631_modbus_minimal_protocol/>
    +<../firmware/runtime/>
    +<../firmware/storage/>
build_flags =
    -DCORE_RAK4631
    -DRAK4630
    -D_VARIANT_RAK4630_
    -DGATE_N
    -I tests/gates/gate_name
    -I tests/gates/gate_rak4631_modbus_minimal_protocol
    -I firmware/runtime
    -I firmware/hal
    -I firmware/storage
```

### 5.3 Key Build Commands

| Action | Command |
|--------|---------|
| Compile only | `pio run -e rak4631_gateN` |
| Flash | `pio run -e rak4631_gateN -t upload` |
| Serial monitor | `pio device monitor -e rak4631_gateN` |
| Flash + monitor | `pio run -e rak4631_gateN -t upload && pio device monitor -e rak4631_gateN` |
| Clean build | `pio run -e rak4631_gateN -t clean && pio run -e rak4631_gateN` |

### 5.4 Include Path Rules

Gates at `tests/gates/gate_name/` use relative includes:

```cpp
// From tests/gates/gate_name/gate_runner.cpp:
#include "../../firmware/runtime/system_manager.h"
#include "../../firmware/runtime/downlink_security.h"
#include "../../firmware/storage/storage_hal.h"
```

RAK4631-specific gates include `hal_common.h`; RAK3312 gates include `hardware_profile.h`:

```cpp
#ifdef CORE_RAK4631
#include "../../firmware/hal/hal_common.h"
#else
#include "../../firmware/config/hardware_profile.h"
#endif
```

---

## 6. Gate Development Procedure

Follow this procedure for every new gate. Steps marked `[TEAM]` are done by the Claude Team; steps marked `[HUMAN]` require operator action.

### Step 1 — Define the Gate `[TEAM]`

1. Create `tests/gates/gate_name/` from `gate_template/`.
2. Write `gate_config.h`:
   - Gate identity (`GATE_NAME`, `GATE_VERSION`, `GATE_ID`)
   - Serial log tags (`[GATE_N]`, `[PASS]`, `[FAIL]`, `[STEP]`)
   - Timing constants (join timeout, test timeout, poll interval)
   - Test vectors (frames, keys, expected values)
   - Pass/fail codes (all failures enumerated as `#define GATE_FAIL_*`)
3. Write `pass_criteria.md`: numbered criteria, fail codes, expected serial output.

### Step 2 — Add PlatformIO Environment `[TEAM]`

1. Append `[env:rak4631_gateN]` to `platformio.ini` following the template in Section 5.2.
2. Add `-DGATE_N` build flag.
3. Add `firmware/storage/` to `build_src_filter` if the gate uses the storage HAL.
4. Add RAK3312 environment if this is a dual-platform gate.

### Step 3 — Update `src/main.cpp` `[TEAM]`

1. Add `#ifdef GATE_N` at the top of the dispatch chain (above all lower gates).
2. Add `extern void gate_name_run(void);` declaration.
3. Add banner line: `Serial.println("[SYSTEM] Gate: N - Name");`.
4. Add `gate_name_run();` call in `setup()`.
5. Increment `@version` in file header.

### Step 4 — Write the Gate Runner `[TEAM]`

Follow the established pattern from Gates 8–9:

- `gate_result = GATE_FAIL_SYSTEM_CRASH` as default (indicates runner didn't complete)
- All parameters from `gate_config.h` — zero hardcoded values in runner
- Each step: log `[STEP]`, execute criterion, log `[PASS]` or `[FAIL code=N]`
- Use `goto done` for early exit on failure
- `done:` label logs final `PASS` or `FAIL (code=N)`, then `=== GATE COMPLETE ===` unconditionally

### Step 5 — Compile `[TEAM + HUMAN]`

```bash
pio run -e rak4631_gateN
# Must produce: SUCCESS with 0 errors, 0 warnings
```

If there are compile errors, fix before proceeding. Do not flash a build with errors.

### Step 6 — Flash and Validate `[HUMAN]`

```bash
pio run -e rak4631_gateN -t upload
pio device monitor -e rak4631_gateN
```

Observe serial output. Verify all steps print `[PASS]`. For LoRaWAN gates, also verify uplinks in ChirpStack device event log.

### Step 7 — Commit on PASS `[TEAM]`

```bash
git add tests/gates/gate_name/
git add src/main.cpp
git add platformio.ini
git add firmware/<any new modules>/
git add docs/CHANGELOG.md
git commit -m "Gate N PASSED: <one-line description>"
git tag vX.Y-gateN-pass-rak4631
```

**Never commit a gate that has not passed on hardware.**

---

## 7. Gate 10 — Integration Status

Gate 10 implementation artifacts were recovered on 2026-03-12 from:

```bash
~/Developer/projects/prod/zip-files-non-git/new_files/
```

Recovered files:

- `gate_config.h`
- `gate_runner.cpp`
- `pass_criteria.md`
- `main.cpp`
- `platformio_gate10_addition.ini`

The recovered sources have now been restored into this repository:

- `tests/gates/gate_secure_downlink_v2/`
- `firmware/runtime/downlink_security.*`
- `firmware/runtime/crypto_sha256.*`
- `src/main.cpp` dispatch for `GATE_10`
- `platformio.ini` environments `rak4631_gate10` and `rak3312_gate10`

Compile verification completed on 2026-03-12:

- `pio run -e rak4631_gate10` → SUCCESS
- `pio run -e rak3312_gate10` → SUCCESS

This restores the Gate 10 code path, but it does not prove a historical PASS.
No local `v4.1-gate10-pass-rak4631` tag, no Gate 10 test report under
`docs/test_reports/`, and no Gate 10 commit message were found in the current
repository.

### 7.1 What Gate 10 Validates

Gate 10 validates the Secure Downlink Protocol v2 (SDL v2) end-to-end:

**Phase A — Synthetic (no network required):**
- STEP 1: `SystemManager::init()` — LoRa radio + OTAA join
- STEP 2: Reset nonce storage to 0 — confirm storage HAL functional
- STEP 3: Valid frame (SET_INTERVAL, nonce=1, 10000ms) — must accept
- STEP 4: Replay same frame (nonce=1 again) — must reject, `replay_fail=true`
- STEP 5: Bad magic frame (0xFF) — must reject, `version_fail=true`
- STEP 6: Tampered HMAC (zeroed) — must reject, `auth_fail=true`
- STEP 7: Nonce persistence — stored nonce must equal 1

**Phase B — Live OTA (ChirpStack required):**
- STEP 8: Wait for live downlink from ChirpStack (120s timeout)
- STEP 9: Replay live frame — must reject, `replay_fail=true`

### 7.2 Protocol Constants

| Field | Value |
|-------|-------|
| Magic byte | `0xD1` |
| Version | `0x02` |
| Min frame | 12 bytes |
| Max frame | 60 bytes |
| HMAC key length | 32 bytes |
| HMAC truncation | 4 bytes (MSB-first) |
| Nonce storage key | `"sdl_nonce"` |
| Test HMAC key | `0x00 0x01 ... 0x1F` (sequential, test only) |

### 7.3 Restored Files

Recovered files originated from `~/Developer/projects/prod/zip-files-non-git/new_files/`.
The repo now contains:

| File | Purpose |
|------|---------|
| `tests/gates/gate_secure_downlink_v2/gate_config.h` | Gate identity, test vectors, HMAC key, fail codes |
| `tests/gates/gate_secure_downlink_v2/gate_runner.cpp` | 9-step runner: Phase A + Phase B |
| `tests/gates/gate_secure_downlink_v2/pass_criteria.md` | All 9 pass criteria + ChirpStack payload instructions |
| `firmware/runtime/downlink_security.*` | SDL v2 validation and command application |
| `firmware/runtime/crypto_sha256.*` | Local SHA-256 implementation used by SDL v2 |
| `src/main.cpp` | Dispatcher with `GATE_10` at top of chain (v4.1) |
| `platformio.ini` | `rak4631_gate10` + `rak3312_gate10` environments |

### 7.4 Verification Commands

```bash
cd ~/Developer/projects/prod/wisblock_lab_template_v1-2
# Gate 10 build verification
pio run -e rak4631_gate10
pio run -e rak3312_gate10

# Flash and monitor on target hardware
pio run -e rak4631_gate10 -t upload
pio device monitor -e rak4631_gate10
```

### 7.5 ChirpStack Live Downlink — Payload Construction

For STEP 8, queue this payload in ChirpStack device queue (any port, e.g. port 2):

```python
# Compute HMAC for nonce=2, CMD_SET_INTERVAL=30000ms
import hmac, hashlib

key = bytes(range(32))                             # 0x00..0x1F test key
msg = bytes([
    0xD1, 0x02, 0x01, 0x00,                        # MAGIC, VER, CMD_SET_INTERVAL, FLAGS
    0x00, 0x00, 0x00, 0x02,                        # Nonce = 2 (big-endian)
    0x00, 0x00, 0x75, 0x30,                        # Payload = 30000ms (big-endian)
])
trunc = hmac.new(key, msg, hashlib.sha256).digest()[:4]
frame = msg + trunc
print(frame.hex())  # Queue this hex string in ChirpStack
```

The frame must use nonce > 1 (greater than the stored nonce left by Phase A).

### 7.6 Expected Serial Output on PASS

```
[GATE10] === GATE START: gate_secure_downlink_v2 v2.0 ===
[GATE10] Platform        : RAK4631 (nRF52840)
[GATE10] STEP 1: SystemManager init
[GATE10]   PASS: SystemManager init OK
[GATE10] STEP 2: Reset nonce storage to 0
[GATE10]   PASS: Nonce storage reset OK
[GATE10] STEP 3: Submit valid frame (CMD_SET_INTERVAL, nonce=1, 10000ms)
[GATE10]   PASS: Valid frame accepted, cmd=0x01, interval=10000 ms
[GATE10] STEP 4: Replay same frame (nonce=1, expect reject)
[GATE10]   PASS: Replay correctly rejected (replay_fail=true)
[GATE10] STEP 5: Bad magic frame (0xFF, expect version_fail)
[GATE10]   PASS: Bad magic correctly rejected (version_fail=true)
[GATE10] STEP 6: Tampered HMAC frame (zeroed, expect auth_fail)
[GATE10]   PASS: Bad HMAC correctly rejected (auth_fail=true)
[GATE10] STEP 7: Verify nonce persistence
[GATE10]   PASS: Nonce persisted correctly (=1)
[GATE10] STEP 8: Waiting for live downlink (timeout=120000 ms)
[GATE10]   --> Queue a valid SDL v2 frame in ChirpStack now <--
[GATE10]   Downlink received: port=2 len=16
[GATE10]   PASS: Live downlink validated and applied
[GATE10] STEP 9: Replay live downlink (same nonce, expect reject)
[GATE10]   PASS: Live replay correctly rejected
[GATE10] PASS
[GATE10] === GATE COMPLETE ===
```

### 7.7 Fail Code Reference

| Code | Constant | Meaning |
|------|----------|---------|
| 0 | `GATE_PASS` | All 9 steps passed |
| 1 | `GATE_FAIL_INIT` | `SystemManager::init()` failed |
| 2 | `GATE_FAIL_VALID_REJECTED` | Valid frame incorrectly rejected |
| 3 | `GATE_FAIL_WRONG_CMD` | `cmd_id` does not match expected |
| 4 | `GATE_FAIL_REPLAY_ACCEPTED` | Replay frame incorrectly accepted |
| 5 | `GATE_FAIL_REPLAY_FLAG` | Replay rejected but `replay_fail` not set |
| 6 | `GATE_FAIL_BAD_VERSION_ACCEPTED` | Bad magic/version frame accepted |
| 7 | `GATE_FAIL_VERSION_FLAG` | Rejected but `version_fail` not set |
| 8 | `GATE_FAIL_BAD_HMAC_ACCEPTED` | Tampered HMAC accepted |
| 9 | `GATE_FAIL_AUTH_FLAG` | HMAC rejected but `auth_fail` not set |
| 10 | `GATE_FAIL_NONCE_PERSIST` | Nonce not persisted correctly |
| 11 | `GATE_FAIL_DL_TIMEOUT` | No live downlink within 120s |
| 12 | `GATE_FAIL_DL_INVALID` | Live downlink failed validation |
| 13 | `GATE_FAIL_DL_REPLAY_ACCEPTED` | Live downlink replay accepted |
| 14 | `GATE_FAIL_SYSTEM_CRASH` | Runner did not reach `GATE COMPLETE` |

### 7.8 On PASS — Commit And Tag

```bash
git add tests/gates/gate_secure_downlink_v2/
git add src/main.cpp
git add platformio.ini
git add docs/CHANGELOG.md
git commit -m "Gate 10 PASSED: Secure Downlink Protocol v2 on RAK4631"
git tag v4.1-gate10-pass-rak4631
```

Do not apply this tag or commit message unless the hardware PASS evidence is
reproduced and a Gate 10 test report is restored or regenerated.

### 7.9 Current Status Summary

- Gate 10 source is restored in-repo.
- Both Gate 10 PlatformIO environments compile successfully.
- Hardware PASS evidence still needs to be re-run and documented.

---

## 8. Project Recovery Procedure

If development is interrupted or the working copy is lost, an archive snapshot exists at:

```
~/Developer/projects/git-clean/artifacts/force_delete_snapshots/
wisblock_lab_template_v1-20260228-023548/
├── repo_backup.tgz       # Full git repo tarball (all history + tags)
├── diff.patch            # Uncommitted changes at snapshot time (empty = clean)
├── status.txt            # git status at snapshot time
└── wisblock_lab_template_v1/  # Working tree copy
```

### 8.1 Verify Archive Matches Repo

```bash
SNAP=~/Developer/projects/git-clean/artifacts/force_delete_snapshots/wisblock_lab_template_v1-20260228-023548

# The diff.patch should be empty if the snapshot was taken from a clean working tree
wc -c $SNAP/diff.patch     # 0 bytes = nothing was uncommitted

# Check what commit the archive captures
cat $SNAP/status.txt
```

### 8.2 Restore from Archive

```bash
SNAP=~/Developer/projects/git-clean/artifacts/force_delete_snapshots/wisblock_lab_template_v1-20260228-023548

# Restore full git history
mkdir -p /tmp/restored
tar -xzf $SNAP/repo_backup.tgz -C /tmp/restored
git -C /tmp/restored/wisblock_lab_template_v1 log --oneline -10

# Verify tags
git -C /tmp/restored/wisblock_lab_template_v1 tag --sort=-creatordate | head -5

# Copy to projects directory
cp -r /tmp/restored/wisblock_lab_template_v1 \
      ~/Developer/projects/prod/wisblock_lab_template_v1-2

# Apply any uncommitted changes (only if diff.patch is non-empty)
cd ~/Developer/projects/prod/wisblock_lab_template_v1-2
git apply $SNAP/diff.patch
```

### 8.3 Verify Recovery

```bash
git log --oneline -10
git tag --sort=-creatordate | head -5
git status --short

# Confirm compile still works
pio run -e rak4631_gate10   # or the last known passing gate
```

---

## 9. Changelog Summary

Full detail in `docs/CHANGELOG.md`. Key milestones:

| Tag | Gate | Key Changes |
|-----|------|-------------|
| `source restored, no pass proof` | Gate 10 ⚠ | Gate 10 source restored in repo and compile-verified; no local tag/report/test report |
| `v4.0-persistence-layer` | Gate 9 ✅ | `storage_hal` (ESP32 NVS + nRF52 LittleFS), persistence reboot test |
| `v3.9-gate8-pass-rak4631` | Gate 8 ✅ | Downlink command framework, bidirectional LoRaWAN validated |
| `v3.8-hal-v2-consolidation` | — | HAL v2 refactor: encapsulated LoRaMAC, removed runtime lib dependency |
| `v3.7-gate7-pass-rak4631` | Gate 7 ✅ | 5-min soak, 9/9 cycles, 82ms max cycle, zero failures |
| `v3.6-gate6-pass-rak4631` | Gate 6 ✅ | Cooperative scheduler + SystemManager integration |
| `v3.5-gate5-pass-rak4631` | Gate 5 ✅ | LoRaWAN OTAA join + Modbus uplink end-to-end |
| `v3.4-gate4-pass-rak4631` | Gate 4 ✅ | Multi-register Modbus read with retry and robustness |
| `v3.3-gate3-pass-rak4631` | Gate 3 ✅ | Modbus RTU minimal protocol, 7-step CRC parser |
| `v3.2-gate2-pass-rak4631` | Gate 2 ✅ | RS485 autodiscovery: Slave=1, Baud=4800, 8N1 confirmed |
| `v3.1-gate1-pass-rak4631` | Gate 1 ✅ | I2C LIS3DH WHO_AM_I=0x33, 100 consecutive reads |

---

## 10. Quick Reference

### Git

```bash
git log --oneline -10
git tag --sort=-creatordate | head -5
git status --short
git add <files> && git commit -m "Gate N PASSED: ..."
git tag vX.Y-gateN-pass-rak4631
```

### PlatformIO

```bash
pio run -e rak4631_gateN                # compile only
pio run -e rak4631_gateN -t upload      # flash
pio device monitor -e rak4631_gateN     # serial monitor
pio run -e rak4631_gateN -t clean       # clean build artifacts
```

### Claude Team

```bash
cd ~/Developer/projects/prod/wisblock_lab_template_v1-2
bash .claude-team/scripts/start-ai-team.sh        # start or re-attach session
tmux attach -t wisblock_lab_template_v1-2-claude  # re-attach manually
```

### HMAC Test Vector Verification

```python
import hmac, hashlib

# Gate 10 test frame: CMD_SET_INTERVAL, nonce=1, interval=10000ms
key = bytes(range(32))                             # test key 0x00..0x1F
msg = bytes([0xD1, 0x02, 0x01, 0x00,
             0x00, 0x00, 0x00, 0x01,
             0x00, 0x00, 0x27, 0x10])
h = hmac.new(key, msg, hashlib.sha256).digest()[:4]
print(h.hex())  # expected: a27ff4ab
```

### SDL v2 Validation Order

Gate 10 enforces fail-fast validation. The check order determines which flag is set:

```
1. Length check          → too short/long
2. Magic check  (0xD1)   → version_fail=true   [no HMAC computed]
3. Version check (0x02)  → version_fail=true   [no HMAC computed]
4. Nonce check           → replay_fail=true    [no HMAC computed]
5. HMAC check            → auth_fail=true
6. Command dispatch      → result.valid=true, cmd_id set
```

---

*End of document. Revision history tracked in git.*
