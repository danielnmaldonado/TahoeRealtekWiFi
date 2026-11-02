# Multi-adapter Refresh and Recovery Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans; current user explicitly requests direct execution.

**Goal:** Refresh networks for the selected USB adapter and extend verified fixes without mixing families or controllers.
**Architecture:** Refresh state per ID/epoch in the app, common RX transport and bootstrap recovery with each family's own operations.
**Tech Stack:** Objective-C++/AppKit/XPC, C/C++/IOKit/net80211, CMake, clang, ASan/UBSan tests.
**Spec:** docs/superpowers/specs/2026-11-01-multi-adapter-design.md

## Global Constraints

- Source in the existing Documents folder; previous releases immutable; authorized logs on Desktop.
- No credentials, new association, reboot or EFI operations by the agent in this round.
- Public protocol and app/helper authentication preserved; no chipset enabled without HAL.
- Diagnostic tests with isolated home; no reset/clean of the dirty repository.

## Review Focus

- A→B→A switching during SCAN: an old callback must not alter B or give the cache fresh provenance.
- Connected adapter: automatic refresh must not break association/traffic.
- USB rejects initial RX: no false success announcement or infinite retry.
- Late callback/mixed error: no release/reinitialization before draining or loss of the first error.
- Changed identity/ROM, failed stop or fatal USB: recovery blocked, without clearing the error.

## Task 1: Read-only physical verification

- [x] Discover current IDs and capture LIST/STATUS/MAP for both using the installed app.
- [x] Check loaded build, USB, WPA2/IP/route and deltas without mixing adapters.
- [x] Save a local report with limits; do not turn metadata into radio/traffic evidence.

## Task 2: App refresh

Files: app/Main.mm, small state policies if necessary, app/tests/*.
- [x] Prove that switching to an idle USB adapter currently only reads cache.
- [x] Test automatic SCAN by need and completion, without a new automatic association.
- [x] Implement per-ID state, completion time, invalidation and bounded retry/timeout.
- [x] Test rapid switching, hotplug, wake, empty cache, ongoing scan and active link.

## Task 3: Common initial RX

Files: kext/RTWUSBController.cpp/.hpp, tests/rtwn_rx_capacity_tests.cpp.
- [x] Run RED for rejection in every station family.
- [x] Extend existing guards to the station-backend registry.
- [x] Test family-specific capacity, four rejections, deferred abort/fatal error, budget and cancellation.
- [x] Run capacity, RX runtime and ordering with sanitizers.

## Task 4: Family bootstrap/stop

Files: RTWFamilyRecovery.inc/Members, typed backends, stop_checked APIs and real tests.
- [x] Reproduce latch and stop before drain; retain RED evidence.
- [x] Implement stop_checked preserving the original error and checking cleanup separately.
- [x] Guard identity/ROM/provider and bounded explicit retry in SCAN/JOIN.
- [x] Finish teardown only after drain; prevent bypass through watchdog/initTask.
- [x] In the legacy path, require primary poweroff before persistent retry; test separately.

## Task 5: Integration and delivery

- [x] Independent review of deltas; fix findings and rerun affected checks.
- [x] Bump driver57/app49, documentation and factual coverage table.
- [ ] CTest Release/sanitizers, AppKit tests, SDK and app/kext compilation from the final ZIP.
- [ ] Check signatures, installer payload, source manifest and pack hashes.
- [ ] Install the authorized app; leave kext/EFI compilation/loading to the user.

This plan snapshot precedes ZIP generation. Final build/test/installer results
are recorded in the delivery's VERIFICATION.json.
