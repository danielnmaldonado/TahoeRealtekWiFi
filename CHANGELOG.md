# 0.14.8 / build55 — app 0.3.5 / build47

- DFS HT20 for seven dual-band backends: RTL8192DU, RTL8812AU, RTL8814AU,
  RTL8821AU, RTL8821CU, RTL8822BU, and RTL8822CU. Channels 52–64/100–140;
  channel 144 disabled.
- Dedicated power/calibration indices, corrected AU OTP groups, channel-140
  spur adjustment and the 100–140 thermal group on RTL8814AU; 20 MHz fallback
  for wide-channel APs in the relevant AU backends.
- Passive scanning; authorization bound to a recent beacon from the same
  BSSID/SSID/channel; a local power ceiling constrained by ROM/factory country,
  world baseline, and AP Country/PowerConstraint. Pinned primary wireless-regdb;
  neither beacons nor locale may override the factory country.
- CSA pauses every TX queue, including DU MAC1, drains USB, and restarts
  scanning/WPA2 outside the RX call. Teardown, sleep, and USB failures cancel
  resumption.
- App47 validates and describes advertised DFS groups while retaining ABI v1
  and compatibility with older drivers.
- An exact registered-channel query preserves partial regional ranges, with
  per-adapter/generation maps and fallback only for an older driver returning
  `Unsupported`.
- USB catalog unchanged; no RTL8814BU, WPA3/AX, AP, or local CAC/radar support.
  The new build still requires physical loading and validation.

# 0.14.7 / build54 — app 0.3.4 / build46

- A full TX queue retains ownership and returns stall/retry; completions
  asynchronously wake the output queue when admission can proceed.
- STATUS observes HT/BA across backends separately from rate-control policy.
- Per-controller publication gains odd/even generations and monotonic time;
  the collector distinguishes advanced, unchanged, incomplete, and ongoing
  generations.
- Existing BA/reorder events and the RX-to-ifnet delivery boundary are exposed;
  the seven historical errors are not attributed to any unproven cause.
- Removes fabricated Ethernet 54 Mbps metadata; an incomplete PHY rate remains
  unknown.
- The source contains a complete list of eligible identities and per-family
  restrictions.
- Build53 was observed with fast traffic and no new USB errors/truncations.
  Physical validation of build54 and the cause of the zero BSD RX count remain
  open.

# 0.14.6 / build53 — app 0.3.4 / build46

- RTL8814AU: 32 KiB RX buffers/reads match the 20 KiB DMA threshold;
  allocation, startup, rearming, and recovery use one family-specific policy.
- A real regression reproduces truncation of a valid 20,200-byte aggregate at
  16 KiB; 32 KiB preserves all 13 frames. Other families retain their previous
  buffer capacity.
- Complete AU TX AMPDU: peer limits, per-TID BA, QoS/ACK/fragment handling,
  density, counting in primary units, and checksum/sequence handling;
  I/O failures reject BA.
- AU thermal tracking retains the C2H-accepted rate; TX does not overwrite it
  with the descriptor's initial/fixed rate, including on management, EAPOL,
  and rejected TX frames.
- New scalar RX length metrics; retains the `logs2` collector.
- App46 preserved. Physical validation of BA, over-the-air aggregation, and
  build53 speed remains pending; EFI and loaded driver are unchanged.

## 2026-10-31 — logs2 collector (keeps kext52/app46)

- Clarifies that IORegistry holds the last published values, possibly stale;
  a delta of zero does not prove absence of traffic or new errors.
- Does not force privileged STATUS or alter driver, network, or authentication.

## 2026-10-31 — logs1 collector (keeps kext52/app46)

- `WiFi-Diagnostics.command` saves to `Library/Logs/Tahoe-WiFi-USB`, outside the
  Desktop.
- Captures and compares both controllers by Registry ID, including family,
  VID:PID, interface, and USB/RX/TX counters; distinguishes reset from removal.
- Bounds command durations, preserves partial output/status124, and completes
  ZIP creation.
- Keeps collection read-only and strengthens MAC-address masking for data fields.
- Does not change driver/app or independently resolve USB error `0xe00002ed`.

# 0.14.5 / build52 — app 0.3.4 / build46

- Reviews all 13 operational backends against primary sources and specific tests.
- RTL8192EU: corrects gen2 HT40/SGI descriptors and the 20 MHz subchannel
  inside a 40 MHz channel; preserves RTS/retry fields and gen1 gates.
- RTL8812AU/RTL8821AU: correct PHY positioning after SHIFT; verifies RSSI in
  the codec and firmware/coexistence consumers.
- Generic RX24: uses the correct family-specific PHY layout, preserving legacy
  rtl8xxxu handling.
- RTL8192FU: preserves the software 802.11 sequence on management, EAPOL,
  and non-QoS frames.
- AU/B-C coexistence: retains a requested LNA state while the MP query is busy;
  pending work is bounded and scoped per context.
- Preserves app46, real scans, measured signal, channel-153 adjustment, and
  build51 telemetry. Hardware checks of 5 GHz and other boards/revisions are
  still required.

# 0.14.4 / build51 — app 0.3.4 / build46

- Explicit app scans now request an actual scan even while connected; periodic
  status polling remains read-only.
- Signal: menu, tooltip, and accessibility show measured and filtered dBm plus
  bars; stale/missing readings remain distinct. All three rendering levels were
  checked; −42/−44 dBm correctly corresponds to strong signal.
- RTL8814AU: Linux NBI/CSI adjustment for HT20 channel 153, with register
  checks and transitions to other channels.
- Scalar diagnostics distinguish visited scan channels, PHY observations, and
  per-band cache without exporting network identifiers or credentials.
- Build50 was observed with WPA2 authorized and no USB recovery errors. The
  router's automatic 5 GHz channel remained unknown; physical validation of
  the changes was still required.

# 0.14.3 / build50 — app 0.3.3 / build45

- Fixes premature RX recovery failure when clearing the endpoint transiently
  returns `NotResponding`; retries are spaced and bounded by budget and
  overall deadline.
- Sends eligible synchronous RX submission failures through the recovery path
  while preserving buffer ownership and order.
- Drains partially rearmed reads before restarting a recovery attempt;
  preserves `clearStall(true)` and toggle synchronization.
- Defers scan/calibration/coexistence timers and blocks direct scans while an
  endpoint recovers, preventing concurrent control I/O between attempts.
- Publishes specific recovery stages and results while retaining app45 source
  and compatibility.
- Build49 was confirmed on hardware: one recovery succeeded and another failed.
  Build50 stability still needed physical validation.

# 0.14.2 / build49 — app 0.3.3 / build45

- Completes the passive scan phase before AP selection across all seven
  dual-band backends.
- Recovers zero-byte USB RX transaction errors with endpoint halt clearing,
  draining, and a finite budget; other failures remain fatal.
- Shows all usable, observed bands for a grouped SSID in the app.
- Preserves the catalog, board gates, WPA2, authorized channels, and firmware
  limits.
- Physical validation of the new build remains necessary.

## 2026-10-30 — 0.14.0 development (kext47, app42)

- Integrated 13 distinct operational backends: CU, 88EU, EU, DU, FU,
  8710BU, 8723BU, 8812AU, 8814AU, 8821AU, 8821CU, 8822BU, and 8822CU.
  At this historical version, 230 IDs were verified eligible; 270 entries
  and 39 unconfirmed/conflicting identities were preserved.
- Ported chipset-specific firmware, ROM, MAC/DMA, PHY/RFK, negotiation,
  TX/RX, and lifecycle logic from pinned primary sources.
- AU with HT20/40 and VHT20/40/80; DU, 8814AU, and B/C with HT20; FU and
  8710BU with HT20/40; 8723BU with HT20. Board-specific limits remain explicit.
- RTL8814AU: power and RFK on upper channels 149/153/157/161/165; physical
  reception verified on the attached USB device, not equivalent to WPA2/IP.
- Integrated AU and B/C coexistence with timers/C2H and ACK-confirmed SMPS;
  firmware ownership/readiness checked during states, EDCA, and shutdown,
  with a fast I/O-free admission path on boards without BT.
- The app advertises lower and upper 5 GHz groups separately and describes
  implemented modes based on driver authorization.
- Corrected 8723BU firmware signatures and 8710BU SMIC/UMC pair handling;
  disabled the old generic physical diagnostic path before any I/O.
- Complete source distribution, unchanged firmware, licenses, generators,
  and tests. Physical qualification of association, traffic, and coexistence
  remains pending for families/revisions without validated hardware.

- Confirmed RTL8814B B814 ID in a primary source while keeping its HAL disabled.

- Verified legacy aliases 056e:4010/AU and 20f4:648c/CU against pinned
  primary sources.

- Made imported IQK tables private to each module, fixing a symbol collision
  when linking 8710BU and 8723BU together.

## 2026-10-30 — native validation (kext build45, app build40)


## 0.14.0/build46 + app 0.3.0/build41 — in development

- RTL8814AU RFE0/1: 5 GHz 36/40/44/48 at 20 MHz, ROM/PG power limits,
  RFK, per-band thermal handling, OFDM/SIFS/EDCA, and HT20/MCS0–15 in net80211.
- App: per-adapter 5 GHz capability, recovered selection, and JOIN revalidation.
- Independent AU/B/C/DU ports: ROM/boot/firmware/MAC/PHY tables/USB;
  remaining integration and calibration work is under development.
- B/C: port 0/filters/software CCMP, EDCA, H2C with shared cursor, and HT20
  rate adaptation. This module does not publish a network interface or
  authorize the WPA2 port.
- Corrected CCNC 0bda:2102 identity to RTL8822BU with ff/ff/ff interface.
- Radio-test assertions remain active in Release builds.
- No certification of WPA2/IP/traffic for the complete package.

- Use public KPI `thread_call_allocate_with_options` for deferred cleanup.
- Recover the selected adapter token when the same Registry ID returns.
- Native RTL8192EU IOKit diagnostics and acceptance of multiple compatible
  interfaces.
- Native regressions and physical tests documented in `docs/NATIVE-20261030.md`.

## 2026-10-24 — USB preflight and selection recovery (kext build44, app build39)

- `src/FamilyBackend.c`: prevent USB descriptors and RX aggregates from being
  erased when output and input share memory. Reject before writing.
- `app/Protocol.hpp`: after disconnection invalidation, reject old tokens
  and allow reselecting the same Registry ID.
- Regressions reproduced on the previous version and passed after correction.
- Ad-hoc signatures, firmware, RF/TX, appearance, and protocols remain unchanged.
- Operational 5 GHz and native macOS Release were still unverified.

## 2026-10-24 — memory/aliasing audit (kext build43, app build38)

- Reject memory overlap between output and input only for the functions
  `rtw_family_5g_join_prepare`, `rtw_family_radio_prepare_profile`, and
  `rtw_r12au_scan_parse`, before clearing caller buffers.
- Add regressions that fail on build42 sources and pass on build43, without
  changing protocols, RF/PHY, firmware, hardware TX, or the AppKit interface.
- Host ASan/UBSan: 93 portable test groups passed in completed runs.
  Native macOS compilation, ad-hoc signing, GUI, and active 5 GHz unverified.
- Continue to document partial semantic review; do not declare a release.

## 2026-10-24 — RTL8814AU 5 GHz passive RX (kext build42)

- Implemented actual RF channel switching for passive 20 MHz reception on
  36/40/44/48, with RFE 0/1, band selection, AGC, and all four RF paths.
- Integrated into net80211's passive scan channels. Returning to 2.4 GHz
  requires recalibration before normal transmission is authorized.
- 5 GHz remains **without TX authorization, authentication, or association**
  for this backend; it did not establish operational 5 GHz or full HT/VHT.
- Mandatory ad-hoc signing preserved; the app remained build38.

## 2026-10-24 — consolidated audit: app build38, kext still build41

- Fix the application's socket and XPC transports to clear the previous
  response when an invalid local request is rejected (including lengths
  greater than `UINT32_MAX` when the buffer is valid).
- Add real socket-function regressions and equivalent native-test cases
  reproducing the pre-fix failure.
- Consolidate old audit reports into `docs/AUDITORIA-CONSOLIDADA.md`, update
  the verifiable inventory `docs/AUDITORIA-COBERTURA.tsv`, and preserve a diff
  in `docs/diffs/CORRECOES.patch`.
- Preserve the kext, radio TX/RX, catalog, firmware, theme, and mandatory
  ad-hoc-signing policy. This package contained **source only**; native macOS
  tests, `codesign`, benchmarks, and active 5 GHz remained pending.

## 2026-10-23 — radio capability matrix: driver build 41, app build 37

- Centralize silicon-family RF/band/PHY ceilings separately from modes actually
  integrated into the macOS station driver. Mark unknown RTL8814BU hardware
  unknown instead of inferring a similar chipset. All 270 immutable USB IDs
  are audited via per-profile API, CLI JSON and generated CSV/JSON inventory.
- Extend **passive-only**, fail-closed channel geometry to 13 known chipset
  families. This does not enable real 5-GHz tuning, DFS, TX, firmware, HT/VHT
  or additional USB personalities in the macOS kernel extension.
- Correct the app RTL8192EU text to HT20/HT40 and expose the reference-vs-driver
  limitations in the read-only USB diagnostics and catalog.
- Tests for each family and every USB profile; signing policy, USB data paths,
  network associations, firmware, encryption and the existing UI styling stay
  unchanged. **Full-speed/multi-band operation is not implemented by this patch.**

## 2026-10-23 — app build 36 (0.2.13), driver remains build 40 (0.13.19)

- Fix `app/Transport.cpp` legacy local socket exchange: clear stale/partially read
  output on receive EOF, truncated replies, semantic payload validation errors,
  remote error frames and write failures. Preserve shared in/out aliasing until
  the request has been transmitted; the wire format remains unchanged.
- Reproduce the original bug on build 35, then exercise the production socket
  functions extracted verbatim against real Unix socketpairs (seven scenarios,
  host ASan/UBSan), plus native source regression scenarios and a Python
  source-contract test. Add one CTest group (88 total).
- Keep all kext sources, chipset families, firmware, helper XPC behavior,
  existing UI themes and visual assets, privileges, and fail-closed mandatory
  ad-hoc signing exactly as in build 35.
- Offline build/test: 88/88 portable CTest groups passed in three batches
  (35+30+23). macOS AppKit/IOKit compilation, actual ad-hoc signature,
  physical USB hot-unplug, Instruments and live UI tests remain pending.
- Release source details: `docs/AUDITORIA-CONSOLIDADA.md`.

## 2026-10-23 — app build 35 (0.2.13), driver stays at build 40 (0.13.19)

- App: ignore delayed automatic Keychain password lookups if automatic reconnect
  has since been disabled or paused; preserve explicit manual connections.
- App: reject malformed successful XPC payloads by clearing the output buffer
  and invalidating the untrusted helper connection. Correct framing alone is
  not proof that the payload contents are valid.
- Extend reconnect-policy combinations and AppKit/XPC source-contract tests.
- Preserve the build-40 kext, USB unplug fix, firmware, network protocols,
  app layout, themes, default settings, and fail-closed mandatory ad-hoc
  signing policy byte-for-byte except for the app sources listed above.
- Offline CI: 87/87 portable test groups passed (84 together; 3 long groups
  individually); no native AppKit/IOKit compilation, signing or UI tests
  performed. Audit details: `docs/AUDITORIA-CONSOLIDADA.md`.

## 2026-10-23 — build 40 (driver 0.13.19), app build 34 (0.2.13)

- Audited all source-package file paths and contents with a reproducible
  per-file inventory, Python/Bash/plist checks, regenerated deterministic
  Xcode metadata and sanitized host regression tests.
- Driver: skip publishing unchanged `OSNumber` diagnostics in status and
  calibration paths; keep identical keys, values and 64-bit types.
- App: refresh the live menu-item states immediately for auto-reconnect and
  macOS-managed login startup; no new UI assets or theme overrides.
- CMake: correct Python interpreter discovery order; all registered test
  groups now execute. Host-only extraction shims updated, with new diagnostic
  de-duplication regressions.
- Build metadata only: kext build 39 -> 40, app build 33 -> 34; functionality
  versions and ABI remain 0.13.19, 0.2.13 and unchanged respectively.
- Keep fail-closed mandatory ad-hoc signing, all USB profiles, firmware,
  passwords, installation choices and existing build39 unplug fix untouched.
- macOS native app/kext build, actual codesign and physical hot-unplug tests
  still need to be validated on a macOS Tahoe machine.

## Driver 0.13.19 build 39 — USB hot-unplug termination (2026-10-23)

- On `kIOMessageServiceIsTerminated` / `kIOMessageServiceIsRequestingClose`,
  quiesce the transfer engine, block new commands, then close the opened
  `IOUSBHostInterface` outside the `IOCommandGate`. IOKit can now enter
  `stop()` instead of waiting on an open provider indefinitely.
- Claim the provider-open state under `lifecycleLock_` before closing it.
  A synchronous/reentrant `stop()` cannot cause a duplicate `close()`.
  Hold the controller and provider references across the call.
- Retain the original deferred DMA/completion cleanup mechanism; do not
  release memory belonging to transfers whose callbacks are outstanding.
- Add a host regression test exercising the actual `message()` and
  `closeUSBProvider()` functions, synchronous stop/reentrancy, concurrent
  close attempts, and 10,000 lifecycle iterations.
- Driver kext build changed from 38 to 39; **app stays at 0.2.13 build 33**.
  USB matching profiles, firmware, network stack, security and app are unchanged.
- Portable host test groups: 86/86 passed in split runs with UBSan for
  the CMake build. Native Xcode build, code signing, and physical unplug
  on macOS Tahoe have not been performed on this Linux host.

See `docs/AUDITORIA-CONSOLIDADA.md` for scope, evidence and validation steps.

## Driver 0.13.19 — guarded legacy firmware upload stage (2026-10-23)

- Added `LegacyFirmwareStage` with validated firmware format, bounded
  4096-byte pages/254-byte vendor transfers and 8-page capacity. Planning
  supports RTL8192FU, RTL8723BU and RTL8710BU; other families fail closed.
- Implemented opt-in **experimental, userspace-only** 8051 firmware download
  for RTL8192FU/RTL8723BU: power-state preconditions, bounded polling,
  stale checksum detection, USB failure propagation and best-effort cleanup.
  RTL8710BU stays planning-only because SoC reset/power control differs.
- Added optional `rtw-legacy-fwdiag` (libusb) with read-only default,
  no driver detachment and explicit hardware-write confirmation; corrected
  USB interface claim to use the validated profile's interface number.
- Extended `rtw-backends` JSON with firmware planning and experimental boot
  flags, independent of `kext_station_integrated`.
- Added portable firmware staging tests and regression cases; **85/85**
  host tests passed with UBSan. New C module passed Clang static analysis;
  optional USB tool only passed syntax validation against test declarations.
- Preserved all 270 catalog profiles, 122 existing kext station-eligible
  profiles, English app 0.2.13, ad-hoc signing policy and retained firmware
  and upstream license notices. No new families were attached to macOS.

**Not completed:** native Xcode compilation, actual signing, live USB
firmware startup, RF calibration, association, WPA2 and physical validation.

## Driver 0.13.18 — all-family backend iteration (2026-10-23)

- Added allocation-free 24-byte RX descriptor/aggregate validation for RTL8192FU,
  RTL8723BU, RTL8821CU, RTL8822BU and RTL8822CU (offline; no station TX).
- Added family-specific legacy 8051 firmware header preflight for RTL8192DU,
  RTL8192FU, RTL8723BU and RTL8710BU. Prevents accepting cross-family
  signatures and truncation; does not authenticate or load firmware.
- Added passive-only channel geometry preflight for six unfinished families,
  with conservative band, width and DFS checks.
- Added explicit separate capabilities in `rtw-backends --json`, maintained
  all 270 catalog entries and the existing 122 kext station personalities.
- Added firmware/radio negative tests, bounded RX fuzzing and CLI capability
  contracts. No unsupported USB aliases were newly enabled.
- App remains version 0.2.13 with English UI and mandatory ad-hoc policy.
  Native macOS/Xcode, real codesign and hardware testing are pending.

## 0.13.17 / app 0.2.13 — multi-family transport and RX boundary

- Added a common, allocation-free family backend registry for all 14 catalogued
  chipsets. A USB endpoint preflight now verifies real configuration/interface
  descriptors and independently reports incomplete station implementations.
- Added shared offline USB receive-aggregate preflight for the six chipsets
  already having verified wire-format decoders (RTL8192CU, RTL8188EU,
  RTL8192EU, RTL8814AU, RTL8812AU and RTL8821AU). Malformed/CRC/ICV frames
  fail without authorizing any USB transmit.
- Kept all 122 previously eligible station IDs unchanged and left the other 148
  disabled (105 verified identities awaiting station backends; 43 ambiguous IDs).
  The eight other chipset families have descriptor preflight only; this release
  does NOT complete their firmware, PHY/RF, RX/TX or WPA2 station code.
- Added the `rtw-backends` diagnostic tool, 270-profile compatibility tests and
  20,000 randomized parser-input checks. The kernel attach path now checks
  explicit backend capabilities instead of inferring station readiness from a
  recognized vendor/product ID.
- Expanded English capability explanations for each pending chipset family.
  Preserved ad-hoc signing requirements and existing documentation structure.
- Portable Linux UBSan build and 81/81 test groups passed. No native macOS
  Xcode/kext build, physical card verification, or real codesign was performed.

# Changelog

## Driver 0.13.16 / app 0.2.12 — 2026-10-23

Driver build 35; app remains 0.2.12 (build 32), with English UI and
mandatory ad-hoc signing scripts. Complete source only.

Prevent stale RTL8812AU firmware checksum reports from authorizing a new
download: reset the MCUFWDL W1C checksum bit and confirm it is clear before
writing pages. Add simulated stale-status and stuck-clear USB cases.

Reject saturated six-bit eFuse temperature code 0x3F as a factory reference
in both identity extraction and thermal preflight. This is diagnostic only;
it does not execute IQK or grant TX authorization.

Accept WPA2-compatible suite selections in transition-mode RSN beacons:
group CCMP, an offered pairwise CCMP suite and PSK AKM, without mandatory
PMF. The offline association request still selects only PSK/CCMP. WPA3-SAE
key exchange and physical RTL8812AU/RTL8821AU station operation are not
implemented; no additional USB identities are enabled.

Full portable build with UBSan and **80/80 CTest groups passed**;
strict Clang build and static analysis of four C modules passed.
Hardware RF/USB operation, native macOS build and actual codesign were
not executed.

## Driver 0.13.15 / app 0.2.12 — 2026-10-23

Driver build 34; the English app remains at 0.2.12 (build 32).
**Complete source only; existing 122 mapped USB identities preserved.
No additional chipsets activated or physical functionality certified.**

Fix station-scan WPA2 classification in `kext/RTWUSBController.cpp`:
require RSN, PSK, pairwise CCMP, **group CCMP** (`ni_rsngroupcipher`),
and no mandatory PMF. An AP with TKIP group encryption, WEP or required
PMF is no longer presented as connectable through the app. Add portable
`NetworkSecurity.h`, exhaustive six-boolean combination regression
and a native-only kernel source contract.

Harden the *offline, non-transmitting* RTL8812AU/RTL8821AU association
response parser: reject non-ESS/IBSS, security capability changes, malformed
or duplicate Supported/Extended Rates IEs and rates not advertised by
the selected AP before changing association state. This does not provide
WPA2 key exchange, on-air association or carrier.

Linux host Debug build with UBSan: **80/80 CTest groups passed**;
strict Clang compile and analyzer passed for the modified C code; USB
profile/personality/matrix generator checks passed. The macOS Xcode/kext
build, actual ad-hoc codesign, libusb hardware diagnostic link and USB/RF
tests on each adapter have NOT been executed. All 148 previously disabled
catalog identities remain disabled pending the appropriate backend or
USB identity evidence.


## Driver 0.13.14 / app 0.2.12 — 2026-10-22

Driver build 33; app unchanged at 0.2.12 (build 32).
**Source only; no additional USB ID enabled or new station backend activated.**

Add `RFCalibration.h`/`RFCalibration.c`, a strictly gated RTL8812AU B-cut,
USB/RFE3, channel-6 passive **thermal preflight**. The laboratory helper checks
the current read-only MAC/BB receive state and prior RF channel verification,
then snapshots CCA and HSSI selections, observes two path-A RF[0x42] samples
through PI/SI serial readback, restores register state and verifies restoration.
It rejects missing/unstable codes, parses the factory eFuse reference at `0xBA`,
and reports an eight-code-unit drift advisory for future IQK investigation.
A readout may be stale; these are not Celsius measurements. **No IQK/LCK/DPK,
RF write, calibration correction, transmission or association is performed.**

Add USB mock fault-injection tests for authorization, selector/CCA preservation,
sampling/settling errors, sensor codes, factory eFuse, missing or unstable
readings and restore failures. Fix the `rtw-hwdiag` invalid-length return path
to attempt MAC RX quiescence. Integrate source in CMake and Xcode.
The full Linux portable Debug/UBSan CTest suite passed **78/78** tests;
new thermal mock test passed **96 assertions**. Strict Clang compilation passed.
Actual `libusb` headers, Mac Xcode/codesign and physical device were unavailable;
the helper's syntax was checked only against temporary mock API declarations.
The 122 existing eligible profiles remain unchanged.

## Driver 0.13.13 / app 0.2.12 — 2026-10-22

Driver build 32; the app is unchanged at 0.2.12 (build 32).
**Source only. No additional USB adapter ID is enabled in the kext.**

Add guarded RF[0x18] channel readback for the sole supported RTL8812AU
B-cut/TSMC/USB/RFE3 channel-6 passive laboratory profile. Query both
Jaguar LSSI/HSSI readback paths, supporting per-path PI and SI modes,
with bounded 10-microsecond settling waits. Read the A and B paths twice,
validate channel 6 on path A before passive capture, and mark path B as
inconclusive when its known unreliable readback cannot be confirmed.

Save and restore the exact 0x838 CCA and 0x8B0 HSSI values, including on
partial USB errors. Verify the restoration by readback and refuse capture
if any required state cannot be restored. Add `rtw_monitor_quiesce_rx_channel6`
to clear MACRXEN/RXDMA/HCI_RXDMA without altering other MAC CR bits,
attempted on successful capture exit and capture/verification failures.
The radio is not fully powered down by this operation.

Extend the optional `rtw-hwdiag` with English RF/cleanup diagnostics.
Add a model USB regression suite including PI/SI selection, transient RF
values, absent chain B, permission gating, forced USB/readback/delay errors,
restoration failures and RX quiescence. Linux host build and 77/77 CTest
groups pass with UBSan. Strict Clang syntax and static analyzer run
without diagnostics for the two changed C modules; `rtw-hwdiag` passes
syntax checking only against temporary API declarations, not linked
against actual libusb. No macOS/Xcode build, codesign or RF hardware
validation was performed. RF calibration, association and WPA2 remain
absent for RTL8812AU/RTL8821AU.

## Driver 0.13.12 / app 0.2.12 — 2026-10-22

Driver build 31; the app is unchanged at 0.2.12 (build 32).
Source-only RTL8812AU B-cut/RFE3 laboratory receive integration. **No new
adapter is bound to the kext and no station/radio readiness is claimed.**

Introduce `RTL8812AUPassive.{h,c}`: a bounded, allocation-free physical USB
bulk-RX bridge connecting the 24-byte RTL8812AU descriptors to the strict
802.11 beacon/probe and RSN parser. Validate each complete USB aggregate
before delivering any AP callbacks; reject malformed/truncated tails,
discard C2H, CRC and ICV errors, verify on-air FCS, and compare advertised
channels to the independently known channel-6 lab profile. Count reception
failures by category; include unsupported security APs in diagnostics without
claiming association.

Update the explicit opt-in `rtw-hwdiag --passive-scan-6` libusb path to use
the RTL8812AU-specific decoder instead of the earlier generic aggregate parser.
Add a six-register **read-only** post-init monitor check for device identity,
MAC RX enable/filter and BB receive gates before entering the receive loop.
That check cannot prove RF-chain calibration, transmitted power, real reception,
WPA2, security or any connection to an AP.

Add host regression tests for USB aggregates, malformed buffers, callback
abort, channel mismatch, FCS/CRC/ICV errors and 20,000 adversarial inputs.
Run full portable suite (76/76 passing using system Python) and four relevant
tests rebuilt with UBSan. Clang strict compilation and static analysis of
the changed C files reported no diagnostics. The libusb tool passed syntax
checking against test-only API declarations, but the actual libusb headers
were unavailable here. Native Xcode build, Apple codesign, and physical
USB/radio reception remain unverified. The 122 enabled/148 pending profile
counts, English UI and mandatory ad-hoc signing are unchanged.

## Driver 0.13.11 / app 0.2.12 — 2026-10-22

Driver build 30, app build 32. Source-only development increment for
RTL8812AU and RTL8821AU. **No additional adapter is enabled.**

Add independently tested `RTL8812AURadio.c` channel geometry for supported
2.4/5-GHz standard 20/40/80-MHz channel groups, recognizing DFS but never
granting regulatory or TX permission. Add `RTL8812AUStation.c` for bounded,
allocation-free parsing of 802.11 beacon/probe-response IEs and RSN suites,
optional FCS validation, strictly matched primary channels, and offline
open-system authentication/association frame construction. Only open or
WPA2-PSK/CCMP networks without required PMF can be candidates. Parsing
an association response cannot authorize an Ethernet link; WPA2 remains
blocked until an unimplemented secure key exchange/CCMP datapath exists.

Extend the capture inspector with explicit `--channel` / `--fcs` options
and hexadecimal SSIDs, avoiding terminal-control injection. Integrate
new sources in CMake and Xcode, add adversarial RX/IE/state-machine tests,
50,000 randomized malformed management frames and 11 end-to-end capture
CLI regressions. Linux UBSan CMake build and all 75 CTest groups passed.
Clang static analysis passed for both new C files. macOS/Xcode compilation,
native ad-hoc `codesign`, USB RF calibration, management transmission and
physical link tests remain pending.

Preserve English UI, ad-hoc signing requirements, third-party licenses,
the 270-profile adapter catalog, 122 existing eligible profiles and 148
disabled entries. Audit, changelog, inventory and validation records remain
consolidated to one file per function.


## Driver 0.13.10 / app 0.2.11 — 2026-10-22

Driver build 29, app build 31. Source-only, fail-closed RTL8812AU /
RTL8821AU development increment. **No additional USB adapter was enabled.**

Introduce an allocation-free C USB RX descriptor decoder and restricted 40-byte
TX descriptor serializer for 1- and 2-chain 802.11ac chipsets. Parse C2H
notifications separately from data frames, reject truncated and malformed
transfers, bound MPDU and bulk lengths, and validate rates against chain count.
TX requires consistent QoS, multicast, EAPOL, PHY bandwidth, retry limits and
protection options, and emits the hardware USB XOR checksum. Non-QoS TX uses
hardware sequence assignment; QoS TX uses the supplied 12-bit sequence.

Add a read-only CLI for inspecting saved bulk-RX captures, portable descriptor
and adversarial parser tests, and source integration into the CMake library and
native Xcode project. Firmware, PHY/RF calibration, AP scans, 802.11 station
association and WPA2/CCMP for these new families are **not** implemented.
The decoder/serializer is not called by the active kernel station path.
RTL8812AU (34 verified candidate IDs) and RTL8821AU (22 verified IDs) remain
disabled; 122 existing profiles and 148 disabled profiles are unchanged.

Preserve English app localization and the mandatory ad-hoc signing policy.
The actual macOS build, codesign execution and physical USB/Wi-Fi tests remain
pending. Existing license notices and one-file-per-document-purpose structure
are retained.

## Driver 0.13.9 / app 0.2.10 — 2026-10-22

Driver build 28, app build 30. Source-only compatibility expansion.

Add 20 independently evidenced USB aliases: 19 use the existing C/CU backend
and one uses the existing RTL8814AU backend. Enabled profiles increase from
102 to 122; catalog size increases from 250 to 270. Preserve the original
250 identity dictionaries and pin their canonical digest. Preserve all 148
disabled entries; this release does not implement a new radio family.

Introduce full interface-class/subclass/protocol matching in the shared C API,
kernel discovery, generated personalities and app diagnostics. The older
descriptor-incomplete API fails closed for class-constrained profiles.
New aliases require configuration 1/interface 0/vendor-specific class. All
verified RTL8814AU entries also enforce the upstream 0xff/0xff/0xff interface.
Missing registry descriptors are reported as unavailable, not fabricated.

Use binary search over the immutable sorted profile table. Reject malformed,
duplicate, unpinned, mismatched or unsupported extension metadata. Add
provenance/matching regressions and exhaustive descriptor-byte tests. Rename
the existing support matrix to USB-WIFI-ADAPTERS.csv rather than keeping a
second matrix. Keep English UI, mandatory ad-hoc signing, firmware, PHY tables,
signal fixes and existing runtime resource policies.

## Driver 0.13.8 / app 0.2.9 — 2026-10-22

This is a source maintenance release. Driver build metadata advances to 27 and app
metadata to 29. The adapter catalog, firmware and radio calibration are unchanged.

### Runtime and resource ownership

Cancel queued or stale UI work using an atomic work epoch across adapter changes,
suspension, session changes, disconnect and termination. Keep sleep and session
activity independent. Stop polling while suspended. Do not synchronously wait for
an XPC queue on AppKit's main thread during termination.

Use per-work-item autorelease queues for driver and credential work. Close the
previous listener and retire its temporary endpoint before reusing a transport
session. Preserve a credential-list refresh requested while another is in flight.
Deduplicate saved SSIDs with a set. Coalesce repeated pending control commands.

### Performance and presentation

Adapt scheduled polling to activity: 2 seconds with the menu open or a join in
progress, 4 seconds while connected with the menu closed, 8 seconds without an
adapter and 15 seconds without the service. Permit timer coalescing with tolerance.
Cache interface enumeration longer while the menu is closed. These are scheduling
changes, not measured CPU or battery savings.

Prefer a supported access point when duplicate SSIDs include a stronger
unsupported-band access point. Preserve complete data separately from bounded,
single-line menu labels. Add accessible names and native semantic colors.
Constrain diagnostic windows to available screen space. Show invalid RSSI as
unavailable instead of printing a misleading numeric reading.

### Build, signing and distribution

Centralize mandatory ad-hoc signing and strict verification. Sign nested code
inside-out, verify app/helper separately, reject certificate/notarization modes,
verify the staged installer payload and verify again before postinstall launch.
Add the same policy to the generated Xcode kext project. A check-only app build
now compiles, signs and verifies a disposable full bundle.

Fail on logging/build/signature errors and reject unexpected destination links.
Use the installer as the only application installation path instead of maintaining
a second privileged copy implementation. Bound portable build parallelism.

### Verification and maintenance

Add concurrent cancellation, poll policy, duplicate-network, bounded-history and
signing-policy regressions. Extend actual production-function harnesses to Linux
with test-only compatibility headers. Add native lifecycle, menu and repeated
socket-session tests, which still require macOS execution.
Consolidate historical instructions, audits and run logs into one file per purpose.
Preserve upstream legal notices, firmware and machine-readable source pins.

## Driver 0.13.7 / app 0.2.8 — input release

The supplied previous source release introduced English application strings,
signal/adapter diagnostic reports, median/hysteresis display filtering and
RSSI-validity handling. Its CCK calculation restricted the special weak-signal
cutoff to the applicable LNA case; missing PHY samples no longer replaced a
valid AP sample with zero. That source reported 102 enabled profiles.

Earlier experimental backend development remains represented by source,
per-file legal notices and upstream metadata. Historical test claims are not
carried forward as evidence that this release passed native or hardware tests.
