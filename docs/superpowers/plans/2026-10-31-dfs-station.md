# DFS in all dual-band families — Implementation Plan

> **For agentic workers:** Use superpowers:subagent-driven-development. Checkboxes record progress; root integrates shared files and delivery.

**Goal:** Implement DFS HT20 station mode in the seven dual-band backends, with complete app and source.
**Architecture:** Family-specific PHY, common discovery/CSA state and explicit v1 capabilities per group.
**Tech Stack:** C/C++ IOKit, net80211, pinned Linux/vendor sources, CMake/Python, AppKit.
**Spec:** docs/superpowers/specs/2026-10-31-dfs-station-design.md

## Global Constraints

- Work in the authorized source; no changes to the running system/EFI/network.
- Preserve factory ROM/RFE/power, WPA2/CCMP and fixes53/54; do not guess144.
- Do not require AP CAC/radar for station mode; do not remove passive-scan restrictions.
- Root: ABI/app/registration8814/CMake/versions/docs/delivery. Agents: PHY8814;
  other PHYs/backends; common state/CSA in Controller. Separate shared blocks.

## Review Focus

- Probe/management/data must not transmit before DFS station permission.
- A foreign or invalid beacon/action must neither grant TX nor program RF.
- CSA must silence and recover without deauth on the old channel or RX reentry.
- Power/calibration must belong to the correct revision/RFE/channel, including exit from140.
- The app must not authorize DFS in an old driver or hide width/channel limits.

### Task1: RTL8814AU PHY

- [x] RED54 on new channels and spur140/thermal100–140.
- [x] Port primary indices/limits and functions with backup/restore and existing gates.
- [x] GREEN on 24 centers, both RFE variants and transitions; compare primary sources and compile against SDK.

### Task2: Other six dual-band backends

- [x] RED54 for DFS20 on DU/AU/B-C and keep invalid/ROM/RFE cases closed.
- [x] Fix OTP groups/geometry/registration and fallback20 for wide APs.
- [x] GREEN on every implemented channel/group/revision and SDK compilation.

### Task3: Common DFS station and CSA

- [x] RED for passive-scan, beacon-freshness/identity and CSA.
- [x] Implement bounded parser/cache, BSS authorization and deferred stop/rescan/rejoin.
- [x] GREEN for invalidity, blocked TX, sleep/teardown/multiple USB adapters and SDK.

### Task4: App, capabilities and delivery

- [x] RED54 on flags52–64/100–140 and band grouping in the app.
- [x] Implement channel helper, v1 flags, description/validation in app47 and registration8814.
- [x] Integrate tests, review primary sources independently and run complete suites.
- [x] Prepare build55/app47 and delivery scripts; final ZIP, signature and hash evidence recorded in VERIFICATION.json outside the source.
