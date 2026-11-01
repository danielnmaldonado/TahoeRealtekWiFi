# Driver log collector implementation plan

> For agentic workers: execute inline with superpowers:executing-plans.

**Goal:** deliver a standalone double-click log collector for both USB adapters.
**Architecture:** extend WiFi-Diagnostics.command; independent behavioral tests.
**Tech stack:** Bash3.2, macOS CLI tools, system Perl/JSON::PP/POSIX.
**Spec:** ../specs/2026-10-31-driver-log-collector-design.md

- [x] RED: real script with offline tool shims/fake HOME demonstrates missing
  default log directory, RTW snapshots and per-device counter evidence.
- [x] Implement default location, bounded subprocess capture, snapshots/summary
  and redaction while retaining existing explicit flags.
- [x] GREEN: ZIP contents, sibling isolation, timeout/partial failures and
  redaction under meaningful mocked OS boundaries; bash -n and live read-only run.
- [x] Independent review; resolve partial-read/new-ID inference and complete
  aggregate-counter deltas.10 behavioral regressions pass.
- Delivery integrity and the extracted-source collector check are recorded
  externally after freezing the source ZIP; existing build52 archives and the
  installed driver are preserved.

Global constraints: no passwords, real network tests, settings or admin by default.
Review: failed/unplugged adapters, unavailable logs, counter reset, spaces in paths,
subprocess descendants and absent AU calibration telemetry.
