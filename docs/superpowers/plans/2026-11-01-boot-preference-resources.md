# Boot preference and resource verification implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans. Work directly in the existing source, as requested by the user.

**Goal:** Restore the chosen physical USB/network across process/boot enumeration and deliver measured, verified corresponding source.

**Architecture:** Preserve volatile generation/epoch fencing while resolving persistent identities. Keep authenticated IPC and Keychain separation. Fix measured allocation overhead and teardown retention independently.

**Tech Stack:** Objective-C++/AppKit/Foundation, IOKit C++, Python/CMake.

**Spec:** docs/superpowers/specs/2026-11-01-boot-preference-resources.md

## Global Constraints

- Work in existing source; preserve unrelated changes.
- No password material in preferences/logs/tests/package; isolate test home.
- No forced reboot, EFI change or live kext replacement.
- Preserve prior deliveries and mandatory licenses.

## Review Focus

- New RegistryIDs, delayed hardware, duplicate identities, missing properties.
- Stale callbacks during switch, wake or service reconnection.
- Use-once credentials and autojoin opt-out.
- Teardown with a sibling retaining the shared WorkLoop.
- Wrong/modified SDK and generated files entering GitHub.

### Task 1: Boot selection

- [x] Reproduce lost selection in actual Main with two new/reordered IDs.
- [x] Implement validated physical identity and per-adapter SSID.
- [x] Test delayed/ambiguous devices, overrides, Keychain and callback fences.

### Task 2: Resources

- [x] Capture idle app/helper CPU, footprint and threads.
- [x] Benchmark actual log records; show CPU budget failure then cache formatter.
- [x] Verify concurrent timestamps and nonblocking/private log writer tests.
- [x] Reproduce/fix coex timer retention through production teardown methods.

### Task 3: Publication

- [x] Exclude generated build/out trees; verify git inventory.
- [x] Require exact SDK revision/content with offline fixture tests.
- [ ] Independent review; close findings; update versions/validation.
- [ ] Run full host Release/sanitized/native app suites and native products.
- [ ] Snapshot full source, rebuild ZIP, verify hashes/signatures/payload.

### Additional evidenced resource fix

- [x] Reproduce unbounded browse scanning with empty/incompatible candidates.
- [x] Consume full foreground browse completion without cache purge or JOIN changes.
- [x] Verify actual controller/net80211 closure across13 family admissions.

The last delivery checks are gated by VERIFICATION.json produced after the
source snapshot. That file records the exact native/host results and hashes.
