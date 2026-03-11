# Gate 10 — Recovered Evidence Note (v4.1)

Date recorded: 2026-03-12

## Status

Gate 10 source has been restored into the repository and compile-verified for both targets:

- `pio run -e rak4631_gate10` → SUCCESS
- `pio run -e rak3312_gate10` → SUCCESS

This file is not a formal PASS report. It records the current evidence state for Gate 10.

## In-Repo Evidence Present

- Gate 10 harness exists in `tests/gates/gate_secure_downlink_v2/`
- SDL v2 runtime exists in `firmware/runtime/downlink_security.*`
- Local SHA-256 implementation exists in `firmware/runtime/crypto_sha256.*`
- Gate 10 dispatcher exists in `src/main.cpp`
- Gate 10 PlatformIO envs exist in `platformio.ini`

## In-Repo Evidence Missing

The current repository does not contain:

- a Gate 10 git tag
- a Gate 10 pass commit
- a formal Gate 10 hardware test report generated at time of validation

## External Conversational Evidence

A recovered conversation log claims Gate 10 was validated on RAK3312 hardware with:

- Phase A synthetic checks passed
- live ChirpStack downlink accepted
- replay rejection confirmed
- nonce persistence confirmed

That conversation is useful historical evidence, but it is not fully consistent with the current repository state. Some claims in the transcript do not match the final restored codebase, including file lists and intermediate implementation details.

## Technical Facts Confirmed From Current Source

- Frame magic: `0xD1`
- Protocol version: `0x02`
- Nonce storage key: `"sdl_nonce"`
- HMAC truncation: first 4 bytes of HMAC-SHA256
- Known-good test frame in current Gate 10 config:
  - `D1 02 01 00 00 00 00 01 00 00 27 10 A2 7F F4 AB`

## Current Conclusion

The repository now proves:

- Gate 10 source recovery
- Gate 10 integration into the project
- successful compilation on both RAK4631 and RAK3312

The repository does not yet prove:

- an official Gate 10 PASS milestone
- a reproducible hardware validation record stored in-repo

## To Promote Gate 10 To PASS

Required artifacts:

- hardware serial log capture for full Gate 10 run
- ChirpStack downlink evidence
- formal test report under `docs/test_reports/`
- changelog entry tied to a commit
- git tag for the validated milestone
