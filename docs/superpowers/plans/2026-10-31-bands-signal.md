# Band discovery and signal implementation plan

**Goal:** Make explicit refresh perform a scan, expose actual measured signal, correct the proven channel153 RF omission, and make band discovery observable.

**Architecture:** Preserve existing AppKit/IOKit control ABI, net80211 all-band traversal and USB recovery. Use bounded scalar telemetry and independent primary-source RF register comparisons.

**Spec:** ../specs/2026-10-31-bands-signal-design.md

- [x] Verify actual native status-button bar rendering and add raw/filtered/held/unknown tooltip and accessibility tests before UI changes.
- [x] Add failing connected-refresh driver mock tests, then request one explicit SCAN and read fresh results without changing background polling or user preferences.
- [x] Compare pinned Linux complete20MHz/spur registers, add failing153/transition tests, then port the NBI/CSI correction.
- [x] Add failing portable telemetry tests, integrate successful/error state-transition and PHY-beacon hooks, and publish scalar scan/cache/channel values.
- [x] Bump driver0.14.4/build51 and app0.3.4/build46; regenerate the native project and document evidence/limits.
- [ ] Obtain independent review, run Release/sanitized/native checks, build from extracted corresponding source, verify manifests/signatures and deliver archives in project releases.

No installation, router changes, reboot, credential operations or unsupported channel unlocking is part of this source change.

## Verification record before packaging

AppKit and RF independent review found no actionable issues. Core telemetry
review passed; two additional root accuracy checks were added: capture the
requested channel before backend cleanup and do not record successful tuning
when an I/O error is latched despite a zero backend return. These changes affect
diagnostics only. The actual controller suite passed 1,186 checks and the
portable telemetry suite 34 checks. Full Release/sanitized and delivered-source
verification are recorded separately in release VERIFICATION.json after completion.
