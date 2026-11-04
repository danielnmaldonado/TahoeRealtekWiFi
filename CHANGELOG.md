# 0.14.20 / build67 — app0.3.14 / build56 — power recovery

- Preserve the configured network across OS interface power-down and restore
  through normal interface enable; retain explicit manual radio-off intent.
- Avoid a spurious firmware-mailbox timeout during RTL8814AU power-off; retain
  checked hardware cleanup and mandatory association command errors.
- Apply mailbox-independent full-stop coexistence cleanup to RTL8821AU,
  RTL8821CU, RTL8822BU and RTL8822CU, retaining antenna handoff and MMIO errors.
- Distinguish screen power notifications from system sleep and refresh current
  status on screen wake. Record bounded app lifecycle metadata.
- Collector7 includes power assertions/history and radio/sleep/manual-off state.
- See `docs/POWER-0.14.20.md` for the reproduced profile-loss defect and the
  remaining physical validation boundary.

# 0.14.19 / build66 — association RSSI feedback

- Restrict firmware RSSI and the shared association average to real PHY samples
  from the current AP in RUN; retain per-frame RSSI for scanning and RX nodes.
- Correct unguarded firmware feedback on RTL8814AU, RTL8192CU, RTL8188EU and
  RTL8192EU. Verify the common path across all 13 enabled station families.
- App0.3.13/build55 is unchanged. See `docs/THROUGHPUT-0.14.19.md` for measured
  build65 results and the outstanding physical throughput target; this update
  is not a claim that 100/30 Mbps has been achieved.

# 0.14.18 / build65 — app0.3.13 / build55

- Preserve a live authentication, association or connected link when a queued
  foreground SCAN reaches the shared control path of any supported family.
- Separate advisory RSSI mailbox readiness from fatal register/USB errors;
  preserve mandatory association and rate-command failure behavior.
- Split RTL8814AU periodic PHY and RSSI failure diagnostics. Collector revision6
  reports AU maintenance, failures and recovery deltas in its summary.
- See `docs/DROPS-0.14.18.md` for the reproduced build64 failure and the remaining
  hardware-validation boundary; stage19 alone does not identify its sub-operation.

# 0.14.17 / build64 — app0.3.12 / build54

- Prevent automatic reconnect and delayed credentials from preempting kernel
  connection recovery; preserve explicit manual connection requests.
- Coalesce network-model work while retaining fresh channel-map authorization;
  reduce equivalent map validation and repeated model checks.
- Provide an asynchronous, copyable RX diagnostic report on demand, with
  coherent selected-driver snapshots and separate filtered-control accounting.
- Regenerate PHY inventory from the compiled catalog and enforce freshness.
  See `docs/CHECKUP-0.14.17.md` for measurements and hardware limits.

# 0.14.16 / build63 — ordinary BlockAck accounting

- Consume strictly validated ordinary BlockAck control frames in the shared
  receive path of all 13 enabled station families. Count them separately from
  RX errors without applying their bitmap or changing TX/BA state.
- Preserve existing admission and rate/CRC/ICV/security checks. Export rejected
  control subtypes and bounded BA format metadata through collector revision 5.
- Retain build62 capacity/mode/alignment corrections. App sources remain build53.
  See `docs/RX-CONTROL-0.14.16.md` for evidence and validation limits.

# 0.14.15 / build62 — RX bounds and rejection diagnostics

- Correct receive capacity, aggregation mode and final alignment handling in
  shared/private RX paths; verify all 13 supported station families against
  their own primary references.
- Preserve admission/error accounting and export scalar rejection reasons and
  context through the driver and collector revision 4. App 53 is unchanged.
- See `docs/RX-ERRORS-0.14.15.md` and its family matrices for capture evidence,
  per-family corrections and hardware validation limits.

# English documentation — collector revision 3

- Translate project documentation, build output and diagnostic messages into
  English while preserving historical findings and hardware-validation limits.
- Use English diagnostic report filenames, STATUS headers and result labels;
  preserve collection, redaction, timeout and counter behavior.
- Keep kext61 and app53 implementation code unchanged.

# 0.14.14 / build61 — app0.3.11 / build53 — macOS13/14/15/26

- App, helper, tests and installer move from a minimum of 26.0 to 13.0; targets
  are Ventura, Sonoma, Sequoia and Tahoe on Intel. Identifiers and preferences
  are preserved.
- The build treats unguarded use of an API newer than the minimum as an error. The
  packager also checks the minimum version of Mach-O executables.
- Kext61 and collector revision 2 preserved. The compatibility matrix distinguishes
  static audit, build and hardware testing; all four systems still require
  validation of loading, WPA2 and traffic for this release.

# 0.14.14 / build61 — app0.3.10 / build52 — collector revision 2

- Reopened the 14 original logs and checked 784 file hashes, plus
  7,368 properties (see report), causes and limits per capture.
- The collector preserves rc1/assessments disabled as an expected state and makes a
  separate recent 2 min attempt only after timeout 124 in the historical query.
- Coverage records received statuses and timestamps; partial history remains
  partial. Crash reports are explicitly identified as historical.
- Preserved 21 earlier tests and added 9, with independent review.
- Kext/app retain build61 code. Zero native RX counters, the CCMP+2 cause, two
  hardware failures and throughput remain open or lack demonstrated resolution.

# Round 61 — changes before this collector revision

- CU/88E: HT20/MCS/QoS/BA/SGI; CU with firmware RA and the peer's SMPS limit;
  88E with host AMRR based on actual CCX reports.
- B/C: HT40/VHT20/40/80 on non-DFS 5 GHz, with bandwidth/PHY/RA/RX/coexistence/BA.
- AU: fix the 80 MHz power layout according to the pinned Linux reference.
- net80211: capabilities/negotiation limited to the destination channel's map.
- Native RX/TX: observation samples before/after delivery, without forcing counts.

# 0.14.13 / build60 — app0.3.9 / build51

- RTL8723BU: four physical OUT endpoints, three DMA queues; bounds/rollback and Bluetooth
  preserved. Exact FF/FF/FF tuple in profile 7392:a611.
- USB discovery: stages/counts remain on the retained original provider,
  including after revocation and failures before the controller.
- AU8812/8821: bounded resident MCU reset according to Linux; timeout/error
  propagation and boot/reset/calibration telemetry without relaxing guards.
- CacheIE: exact copy region, transactional allocation/cleanup and read
  bounds preserving the RSN flow. Local-memory-only tests.
- Collector: USB/AU startup in summary/JSON; individual analysis of the 14 logs and all
  nonzero counters, distinguishing observed rate from throughput.
- App51: updated USB catalog; preference/reconnect retain the previous code.

# 0.14.12 / build59 — app 0.3.8 / build50

- Publishes `IOBuiltin = Yes` as OSBoolean on the network interface of all
  supported adapters, during configureInterface and before registerService.
- Interface presentation flag; does not set IOPrimaryInterface or
  change USB identity/properties, BSD name or service priority.

# 0.14.11 / build58 — app 0.3.8 / build50

- Persists the board by physical identity and resolves RegistryIDs after reboot;
  retains the preference with delayed enumeration and rejects ambiguous identities.
- Confirmed network per board, including one-time use without saving a password; conservative
  migration of a single already-authorized link when upgrading the old app.
- Reconnect epoch cancels queued automatic lookups/requests after
  the option is disabled. Network preference and Keychain password remain distinct.
- A scan to list networks without JOIN ends after a complete sweep,
  preserving cache/reception and JOIN profiles' association retries.
  An optional callback retains the existing net80211 path when unused.
- Removes the B/C coexistence timer before releasing the shared WorkLoop,
  fixing retention when another board keeps the loop active.
- Reuses the timestamp formatter with synchronization. A local benchmark of
  10 thousand records reduced CPU time from about 1.1 s to 20–24 ms.
- Checks SDK revision/contents; excludes generated build-*/out-* directories from Git.

# 0.14.10 / build57 — app 0.3.7 / build49

- Automatic real scan for the selected adapter when idle and the RF cache has no
  fresh completion. Coalescing, generation/epoch, 30 s timeout and bounded retries.
- Connected boards retain cache queries; no new JOIN/password request from refresh.
- Initial RX: four-buffer validation, bounded clear and drain/provenance
  extended to the 13 station backends, preserving each family's capacity.
- Common stop preserves late RX/TX errors and generations before reusing
  buffers; short TX retains Underrun. Callbacks after preflight are checked.
- Verified shutdown and family-specific identity for recovery;
  legacy CU/8188EU/8192EU receive power-off from the pinned Linux sources.
- Diagnostics publish ifnet/Skywalk capabilities to investigate zero native RX;
  no native counter is incremented manually.
- The build56 checkup found both authorized and separate HTTPS working:
  RTL8814AU on 5 GHz/channel 36 and RTL8192EU on 2.4 GHz/channel 6. This does not validate build57.

# 0.14.9 / build56 — app 0.3.6 / build48

- RTL8814AU: manual SCAN/JOIN retry after an eligible error, only after
  draining, verified shutdown and reidentification of the same device.
  Bounded budget; original error separate from cleanup result.
- Fixes stop/start before callbacks finish and completes shutdown after
  the last completion, without starting the radio or bypassing USB/profile/identity errors.
- Initial AU RX: NotResponding receives bounded cleanup and EAGAIN, without false
  success. Provenance survives EBUSY and distinct late errors remain fatal.
- IOKit: power registration in registerWithPolicyMaker, before attach,
  with an initial state 1 request and PMstop before releasing resources on failures.
- Diagnostics export core errno, actual states and AU initialization phases.
- App48: exact-ID authenticated CLI for LIST/STATUS/MAP/SCAN and private JSONL,
  without credentials. Logs cannot block on a FIFO or another process's lock.
- Diagnostic fixtures use an isolated home and do not pollute the installed app's log.
- Hardware test on build55 reproduced SCAN rejection in 0.765 ms, USB error 0; the first
  failure's code was not published. Hardware validation of build56 remains necessary.

# 0.14.8 / build55 — app 0.3.5 / build47

- DFS HT20 on the seven dual-band backends: RTL8192DU, RTL8812AU, RTL8814AU,
  RTL8821AU, RTL8821CU, RTL8822BU and RTL8822CU. Channels 52–64/100–140; 144 closed.
- Dedicated power/calibration indices, corrected AU OTP groups, spur 140
  and thermal group 100–140 on 8814AU; fallback to 20 MHz on wide APs on the corresponding AU backends.
- Passive scanning, authorization tied to a recent beacon from the BSSID/SSID/channel and
  local power ceiling bounded by ROM/country, world, and the AP's Country/PowerConstraint.
  Pinned primary wireless-regdb; no country substitution from beacon/locale.
- CSA pauses all TX queues, including MAC1 DU, drains USB and repeats scanning/WPA2
  outside the RX call. Teardown/sleep/USB failure cancel resumption.
- App47 validates and describes advertised DFS groups, retaining ABIv1/older driver support.
- Exact query of registered channels preserves partial regional ranges;
  map per adapter/generation and fallback only for an old driver returning Unsupported.
- USB catalog unchanged; does not enable 8814BU, WPA3/AX, AP or local CAC/radar.
  Loading and hardware validation of the new build remain necessary.

# 0.14.7 / build54 — app 0.3.4 / build46

- A full TX queue preserves ownership and returns stall/retry; completions resume the
  output queue asynchronously when admission can proceed.
- STATUS observes HT/BA on all backends, separately from rate-control policy.
- Per-controller publication gains an odd/even sequence and monotonic time;
  the collector distinguishes advanced, identical, incomplete or in-progress generations.
- Exposes existing BA/reorder events and the RX/ifnet delivery boundary;
  does not attribute the seven historical errors to a cause without evidence.
- Removes invented Ethernet 54 Mb/s metadata; an incomplete PHY rate remains unknown.
- Source includes the complete list of eligible identities and per-family limits.
- Build53 was observed with fast traffic and no new USB errors/truncations.
  Hardware validation of build54 and the cause of BSD's RX0 accounting remain open.

# 0.14.6 / build53 — app 0.3.4 / build46

- RTL8814AU: 32 KiB RX buffers/reads consistent with the 20 KiB DMA threshold;
  allocation, startup, rearm and recovery use the same per-family policy.
- An actual regression test reproduces 16 KiB truncation of a valid 20,200-byte aggregate; 32 KiB
  preserves all 13 frames. Other families retain their previous capacity.
- Complete AU TX AMPDU: peer limit, BA per TID, QoS/ACK/fragments, density,
  count in primary units and checksum/seq; I/O failure rejects BA.
- AU thermal tracking preserves the C2H-accepted rate; TX does not overwrite it with the
  descriptor's initial/fixed rate, including management/EAPOL/rejected TX.
- New scalar RX length metrics; retains collector logs2.
- App46 preserved. Hardware validation of BA, on-air aggregation and build53
  speed remains pending; no change to EFI/loaded driver.

## 2026-10-31 — collector logs2 (retains kext52/app46)

- Makes explicit that the IORegistry contains the latest published values, possibly
  stale; delta0 does not prove an absence of traffic or new errors.
- Does not force privileged STATUS or change the driver, network or authentication.

## 2026-10-31 — collector logs1 (retains kext52/app46)

- WiFi-Diagnostics.command saves in Library/Logs/Tahoe-WiFi-USB, outside the Desktop.
- Captures and compares both controllers by Registry ID, including family,
  VID:PID, interface and USB/RX/TX counters; distinguishes reset from removal.
- Bounds command time, preserves partial output/status124 and continues ZIP creation.
- Retains read-only collection and strengthens masking of MAC addresses in data format.
- Does not change the driver/app or itself resolve USB error 0xe00002ed.

# 0.14.5 / build52 — app 0.3.4 / build46

- Review of the 13 operating backends against primary sources and specific tests.
- RTL8192EU: fixes gen2 HT40/SGI descriptor and 20-in-40 MHz subchannel; keeps RTS/retry fields intact and the gen1 gates.
- RTL8812AU/RTL8821AU: fixes PHY after SHIFT; validates RSSI in the codec and firmware/coexistence consumers.
- Generic RX24: uses the correct PHY layout per family; preserves legacy rtl8xxxu.
- RTL8192FU: preserves the software 80211 sequence in management/EAPOL/non-QoS.
- AU/B-C coexistence: does not lose the desired LNA request when the MP query is busy; pending work remains bounded and per-context.
- Preserves app46, real scanning, measured signal, adjustment 153 and build51 telemetry. Hardware validation of 5 GHz and other boards/revisions remains necessary.

# 0.14.4 / build51 — app 0.3.4 / build46

- Explicit app scanning now requests a real scan even when connected, keeping periodic queries read-only.
- Signal: menu, tooltip and accessibility show measured and filtered dBm and bars; stale/missing states remain distinct. Rendering of all three levels was verified; −42/−44 dBm correctly corresponds to a strong signal.
- RTL8814AU: Linux NBI/CSI adjustment for channel 153 HT20, with register verification and transitions to other channels.
- Scalar diagnostics separate scan visits, PHY observations and cache by band, without exporting network identifiers or credentials.
- Build50 observed with WPA2 authorized and no USB recovery errors. The router's automatically selected 5 GHz channel remains unknown; hardware validation of the changes remains necessary.

# 0.14.3 / build50 — app 0.3.3 / build45

- Fixes premature RX recovery failure when endpoint cleanup returns transient NotResponding; retries are spaced and bounded by the budget and total deadline.
- Routes eligible synchronous RX rejection to recovery, preserving buffer ownership and order.
- Also drains partially rearmed reads before restarting an attempt; preserves clearStall(true) and toggle synchronization.
- Defers scan/calibration/coexistence timers and rejects direct scanning while the endpoint is in recovery, preventing concurrent control I/O between attempts.
- Publishes specific recovery stage and results; retains app45 compatibility and its complete source.
- Build49 confirmed on hardware: one recovery completed and another failed. Build50 stability still requires hardware validation.

# 0.14.2 / build49 — app 0.3.3 / build45

- Completes the passive phase before AP selection on the seven dual-band backends.
- Recovers a USB RX transaction error with no bytes through halt clear, draining and a finite budget; retains fatal handling for other failures.
- Shows all usable bands observed in a grouped SSID in the app.
- Preserves catalog, board gates, WPA2, authorized channels and firmware limits.
- Hardware validation of the new build remains necessary.

## 2026-10-30 — 0.14.0 development (kext47, app42)

- Integrated 13 separate operating backends: CU, 88EU, EU, DU, FU,
  8710BU, 8723BU, 8812AU, 8814AU, 8821AU, 8821CU, 8822BU and 8822CU.
  There are 230 verified eligible IDs; 270 records and 39
  unconfirmed or conflicting identities preserved.
- Ported firmware, ROM, MAC/DMA, PHY/RFK, negotiation, TX/RX and lifecycle
  specific to the new chipsets, from the pinned primary sources.
- AU with HT20/40 and VHT20/40/80; DU, 8814AU and B/C with HT20; FU and 8710BU
  with HT20/40; 8723BU with HT20. Per-board limits remain explicit.
- 8814AU: power and RFK on upper channels 149/153/157/161/165;
  hardware reception verified on the connected USB adapter, without equating it to WPA2/IP.
- AU and B/C coexistence integrated with timers/C2H and ACK-confirmed SMPS;
  firmware ownership and readiness checked in states, EDCA and
  shutdown, with fast authorization without I/O also on boards without BT.
- The app advertises lower/upper 5 GHz groups separately and
  describes implemented modes, according to driver authorization.
- Corrected 8723BU firmware signatures and 8710BU SMIC/UMC pairs;
  old generic hardware diagnostics disabled before any I/O.
- Complete source distribution, unmodified firmware, licenses,
  generators and tests. Hardware validation of association, traffic and
  coexistence remains pending for families/revisions without validated hardware.

- Confirmed RTL8814B B814 ID in a primary source, keeping the HAL disabled.

- Verified legacy aliases 056e:4010/AU and 20f4:648c/CU against pinned primary sources.

- Made imported IQK tables private per module, fixing the symbol
  collision when linking 8710BU and 8723BU together.

## 2026-10-30 — native validation (kext build45, app build40)


## 0.14.0/build46 + app 0.3.0/build41 — in development

- RTL8814AU RFE0/1: 5 GHz 36/40/44/48 at 20 MHz, ROM/PG/power limits,
  RFK, per-band thermal handling, OFDM/SIFS/EDCA and HT20/MCS0–15 in net80211.
- App: per-adapter 5 GHz capability, restored selection and revalidation on JOIN.
- Independent AU/B/C/DU ports: ROM/boot/firmware/MAC/PHY tables/USB;
  remaining integration and calibrations are still in development.
- B/C:port0/filters/software CCMP, EDCA, H2C with a shared cursor and HT20 RA;
  this module does not publish a network interface or authorize the WPA2 port.
- Corrected CCNC 0bda:2102 identity to RTL8822BU, with interface ff/ff/ff.
- Radio test assertions remain active in Release too.
- No WPA2/IP/traffic certification for the complete package.

- Use the public thread_call_allocate_with_options KPI for deferred cleanup.
- Recover the selected adapter's token after the same Registry ID returns.
- Native IOKit RTL8192EU diagnostics and acceptance of multiple compatible interfaces.
- Native regressions and hardware tests documented in docs/NATIVE-20261030.md.

## 2026-10-24 — USB preflight and selection recovery (kext build 44, app build 39)

- `src/FamilyBackend.c`: avoid erasing USB descriptors and RX aggregates when
  output and input share memory. Reject before writing.
- `app/Protocol.hpp`: after disconnection invalidation, reject
  stale tokens and allow reselection of the same Registry ID.
- Regressions reproduced on the previous version and passed after the fix.
- Ad-hoc signing, firmware, RF/TX, appearance and protocols preserved.
- Operating 5 GHz and macOS Release still unproven.

## 2026-10-24 — memory/aliasing audit (kext build 43, app build 38)

- Reject memory overlap between output and inputs only in the
  `rtw_family_5g_join_prepare`, `rtw_family_radio_prepare_profile` and
  `rtw_r12au_scan_parse` flows, before clearing caller buffers.
- Add regressions that fail on source build 42 and pass on build 43;
  do not change the protocol, RF/PHY, firmware, hardware TX or AppKit interface.
- ASan/UBSan host tests: 93 portable groups passed in completed runs.
  macOS compilation, native ad-hoc signing, GUI and active 5 GHz not verified.
- Continue documenting partial semantic review; do not declare Release.

## 2026-10-24 — RTL8814AU passive 5 GHz RX (kext build 42)

- Implemented real RF switching in 20 MHz passive receive mode,
  channels 36/40/44/48, with RFE 0/1, band selection, AGC and all four RF paths.
- Integrated into the net80211 scanner's passive channels; the return path to
  2.4 GHz requires recalibration before allowing normal transmission.
- 5 GHz remains **without TX, authentication or association authorization** in this
  backend; this does not represent operating 5 GHz or full HT/VHT support.
- Mandatory ad-hoc signing retained; application remains build 38.

## 2026-10-24 — consolidated audit: app build 38; kext remains build 41

- Fix the application's socket and XPC transports to clear the previous
  response when an invalid local request is rejected (including
  lengths greater than `UINT32_MAX` when the buffer is valid).
- Add regressions of the actual socket functions and corresponding cases
  to the native test, reproducing the failure before the fix.
- Consolidate old reports into the single document
  `docs/CONSOLIDATED-AUDIT.md`, update the verifiable inventory
  `docs/AUDIT-COVERAGE.tsv` and save a diff in
  `docs/diffs/FIXES.patch`.
- Preserve kext, radio TX/RX, catalog, firmware, theme and the mandatory
  ad-hoc signing policy. This package contains **source only**;
  actual macOS tests, `codesign`, benchmarks and active 5 GHz remain pending.

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
- Release source details: `docs/CONSOLIDATED-AUDIT.md`.

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
  performed. Audit details: `docs/CONSOLIDATED-AUDIT.md`.

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

See `docs/CONSOLIDATED-AUDIT.md` for scope, evidence and validation steps.

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
