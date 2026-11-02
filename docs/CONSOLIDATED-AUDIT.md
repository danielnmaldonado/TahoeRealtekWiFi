# Priority round — 24/10/2026 — Kext 0.13.19 build 44 / App 0.2.13 build 39

**Release status: NOT VALIDATED.** Immutable base: build43 ZIP SHA-256 `f0dfe8177e945c60e53a40b6a72a891de15d1e088250d471719bfeee98d694b4`; original manifest intact. There is no `.git` in the package: commits and branches cannot be checked. ZIP integrity and the diff are verifiable; this does not imply completion of the line-by-line audit.

## Two root causes proven and fixed

1. **F-P01 — USB/RX input overwritten by output buffer (`src/FamilyBackend.c`):** `rtw_backend_usb_preflight` and `rtw_backend_rx_preflight` cleared `out` before checking aliasing with `config`/`transfer`. The original same-memory test failed an assertion; with the new `ranges_overlap` routine, inputs are preserved and the call returns an argument error. Exact and partial overlap cases were added for both paths. Demonstrated impact: unintended input modification when the caller supplies overlapping buffers; there is NO evidence of a macOS crash or leak in normal use.
2. **F-P02 — invalidated adapter selection (`app/Protocol.hpp`):** after `invalidate()`, the previous token was rejected, but obtaining the current `generation` could pass `matches()` despite invalidation, while `select(same Registry ID)` did not reactivate the adapter. Added an `invalidated` state with rejection in `matches()` and the ability to restore selection of the same Registry ID. Class tests were reproduced with failure before and success afterward.

Functional changes are limited to these two paths. Build number increments and the Xcode generator are metadata distinguishing this revision. **No firmware, vendor, RF, TX, visual UI, 802.11 networking, signing or security configuration file was changed**. The single patch `docs/diffs/FIXES.patch` is specifically relative to original build43 source (it does not rewrite documented historical patches).

## Actual tests (Linux environment, not macOS)

- CMake Debug ASan/UBSan build: completed.
- CTest 91/91 fast tests, plus USB matrix (270 profiles) 1/1, plus app package 1/1; **93/93 passed** in completed runs. The first attempt at the long matrix test was interrupted by timeout and was not counted as passed.
- Strict Clang C11 on `src/FamilyBackend.c`, bash script syntax, plist parsers and deterministic Xcode generator: passed.
- Single diff applied to a clean build43 copy; all 9 modified files were checked byte for byte. No original file removed.
- Records in `docs/AUDIT-EVIDENCE.txt`. The `docs/AUDIT-COVERAGE.tsv` matrix distinguishes indexes from semantic validation.

## Prioritized outstanding work and insufficient evidence

| Priority | Item | Actual status / next action |
|---|---|---|
| P0 | 5GHz for seven dual-band families | **Pending.** Per-chip RF/PHY backends, firmware, power tables, calibration, HT40/VHT80, TX/RX, WPA2, regulatory limits and hardware evidence are missing. RTL8814AU retains passive RX, with 5GHz TX blocked. Do not enable TX without these requirements. |
| P0 | USB/IOKit hot-unplug | Partial manual reading of `RTWUSBController.cpp`: `message`/`quiesce`/`close` flow, callback drain in `shutdownPlatform`/`deferredCleanup`. Portable tests passed; physical hot-unplug, busy/retain, sleep/wake and callback invalidation **not tested on macOS**. |
| P0 | Native Xcode | **Pending.** Run Release app/kext/helper compilation with Xcode on the Mac and inspect new errors. |
| P1 | Ad-hoc signing | Existing policy preserved and exercised by signing test; actual `codesign -s -` and Mac binary verification **not performed**. |
| P1 | CPU/RAM/leaks/energy | Portable sanitizers passed; Instruments/Leaks/Time Profiler/Energy Log and WiFi benchmark **not performed**. Performance optimization cannot be claimed. |
| P1 | Layout/theme/app | `Main.mm` partially inspected, adapter selection fixed. Dark/Light/HiDPI/VoiceOver in AppKit **not verified**. No theme or layout changed without evidence. |
| P1 | 100% per-function audit | Inventory/lines/manifest updated, focused semantic inspection; thousands of functions were not individually analyzed; **NOT 100%**. |

## Checkpoint for the next round

1. Validate build 44 physically on macOS, hotplug with 2 adapters, RX/TX interruption and IORegistry objects; collect traces of `stop`, `close`, `busy/retain` and callbacks before any new teardown patch.
2. Continue implementing **5GHz RF/PHY per family** on test devices; never advertise operation without proven TX/WPA/regulatory behavior.
3. Run ad-hoc `codesign` and verifier on the Mac and light/dark AppKit tests; measure CPU, RAM and leaks with Instruments; fill the matrix according to lines/functions actually checked.
4. Continue direct inspection of `RTWUSBController.cpp` at `shutdownPlatform`, `deferredCleanup`, `IOCommandGate`, and `app/Main.mm` at async/Keychain callbacks, with associated hardware tests.

**Integrity:** `SOURCE-MANIFEST.sha256` lists source files plus documents (except the manifest itself). The external ZIP and its `.sha256` are checked separately; actual macOS digital signing remains pending.

---

# NATIVE BUILD HOTFIX — 24/10/2026 — kext build 41 / app build 38 (VERSIONS PRESERVED)

**Input:** actual user Xcode 26.6 (17F113), macOS SDK 26.5 log. `xcodebuild` compiled other components and failed in `kext/RTWUSBController.cpp` while loading `include/rtw/DiagnosticPublishPolicy.hpp:3`, with fatal error `fatal error: 'cstdint' file not found`. The kext uses `-nostdinc`/kernel headers; `cstdint` belongs to the C++ standard library and is unavailable in this build. Additional warnings in `vendor/` files are not this fatal cause and were not changed without evidence.

**Minimal fix:** replaced `#include <cstdint>` with `#include <stdint.h>` in `include/rtw/DiagnosticPublishPolicy.hpp`. The `diagnosticCounterNeedsWrite` function and its contract were not changed. `tests/diagnostic_publish_policy_tests.cpp` now uses C headers and is compiled with `-nostdinc++` (only with compilers supporting that option), configured in `CMakeLists.txt`. This prevents regression through an accidental STL dependency in code shared with the kext.

**Reproduction and independent validation:** compiling the policy with `clang++ -std=gnu++14 -nostdinc++ -Iinclude -fsyntax-only` failed on original source because `cstdint` was absent, and passed after the fix. CMake Debug with ASan/UBSan rebuilt; the policy test passed with `-nostdinc++`; the portable suite was split into 90 core tests + 1 USB matrix + 1 app package, all passed (**92/92**). This evidence confirms removal of the specific cause in source, **not** a complete native `xcodebuild`.

**State and scope:** complete source, no new USB models, no changes to radio driver, firmware, layouts, protocol, signing scripts or build numbers. The ad-hoc signing requirement is preserved, but actual `codesign` was not performed on Linux. **macOS Release status: NOT VALIDATED; a new Xcode Release build and actual signing are still pending.** Without `.git` in the supplied ZIP, there is no commit validation; the input package and its hashes are preserved as the base.

**Checkpoint:** next Mac test: rerun `./build.command --kext` with Xcode 26.6/SDK 26.5. If another error appears, use the first `error:` after this fix for a new reproduction; after building, confirm `codesign -dv --verbose=4 Release/TahoeRealtekWiFi.kext` and `codesign --verify --strict` before installation. Do not treat warnings as crashes without evidence.

---

# CONSOLIDATED AUDIT — TahoeRealtekWiFi

## Additional round of 24/10/2026 — direct analysis and proven fixes

**Immutable reference tree:** `TahoeRealtekWiFi-v0.13.19-KEXT41-APP38-AUDITORIA-CONSOLIDADA-ADHOC-FULL-SOURCE.zip`; SHA-256 `9be2b81fc3da5a2b0ce35ffc2c9debd569fedf0257773f76a66b999cdac5136f`; 547 original files. ZIP integrity confirmed, and the source manifest was previously validated. **Resulting source version:** kext 0.13.19 build 41 and app 0.2.13 build 38, with new fixes in shared RX processing sources; build numbers were not changed without native Release compilation. **No `.git` metadata** in the ZIP — commits/branches cannot be guaranteed. Only the package received in this conversation was used as the base.

### Three defects demonstrated with regression failing before the fix

| ID | Actual function | Root cause | Localized fix | Evidence |
|---|---|---|---|---|
| F-02 | `rtw_family_5g_scan_bulk`, `src/Family5GHzScan.c` | The call cleared `stats` before rejecting overlap between output and USB bytes or the identification profile. Input data declared immutable could be modified. | Early rejection of overlapping ranges before `memset`, testing distances between pointers represented as `uintptr_t` without adding past the address limit. | `repro_before.txt`: expected abort, incompatible output; `repro_fixed_family.txt`: test passed; new exact and partial overlap cases. |
| F-03 | `rtw_r12au_passive_on_channel`, `src/RTL8812AUPassive.c` | RX counters could write over the caller-supplied USB aggregate when `stats` overlapped `bulk`. | Prevent counter object aliasing with the RX buffer before any update. | `repro_second_before.txt`: expected abort; `repro_fixed_passive.txt`: regression passed; partial/exact variants. |
| F-04 | `rtw_rx_preflight_transfer`, `src/USBPackets.c` | The preflight report's initial `memset` could erase the transfer to be analyzed if the same storage was passed to both outputs/inputs. | Detect and reject overlap before clearing `out`. | `repro_third_before.log`: expected abort; `repro_third_fixed.log`: regression passed; partial/exact variants. |

**Proven impact:** input is incorrectly modified when the caller contract is violated through overlap; this result was demonstrated in portable tests using the sources themselves. **No exploitation, macOS crash, heap leak, speed improvement or RF failure in normal use was demonstrated**. Those claims are therefore not made. Shared Kext RX code changed in `src/USBPackets.c`; files in `kext/`, app implementation, firmware, USB IDs, interface, theme and signing remained identical.

### Actual scope of this round's semantic inspection

- **Complete direct reading of units and main error paths:** `src/Family5GHzScan.c` (including descriptor format, full aggregate validation and callbacks), `src/RTL8812AUPassive.c` (aggregates, counters and callbacks), `src/USBPackets.c` (RX, preflight, TX descriptor assembly), `src/USBControl.c`, `src/USBRegisterIO.c`, `src/FamilyRadioPolicy.c`, `src/StationPort.c`, `kext/RTWUserClient.cpp`, `app/LoginItem.mm` and the three corrected test units. Additional direct inspections of `app/Transport.cpp`, `app/DriverConnection.mm`, `app/PrivilegedService.mm`, `app/Helper.cpp`, `app/CredentialStore.mm`, `app/Model.hpp`, `app/Protocol.hpp`, `app/signing.sh` and modules cited in the matrix; in larger units only explicit excerpts were inspected.
- **Partial inspection:** `kext/RTWUSBController.cpp` (USB init/teardown/callbacks), `app/Main.mm` (asynchronous/UI flows), `src/EthernetBridge.c`, `src/USBDescriptors.c`, `src/RTL8814AUDataPath.c`, other partially visited modules. This does not equate to analyzing every function/line.
- **Automatic inventory and hash verification:** all present files, but **inventory or passing CTest does not equate to manual inspection**. `docs/AUDIT-COVERAGE.tsv` records distinct statuses for files and heuristic functions. Not all ~2.834 function candidates received semantic evaluation: **full coverage NOT reached**.
- **Causes without a fix in this round:** no other hypothesis obtained unambiguous reproduction and a safe change. In particular, the Clang analyzer warning for `profile->family` in `src/Family5GHzScan.c` depends on assuming the validator would return `identity_verified=true` for `profile=nullptr`; `src/USBWiFiProfiles.c` explicitly demonstrates null/unverified return. The warning remains documented without speculative modification.

### Actual rounds, build, tests and signing

1. **R1 — base and source:** SHA-256 and build 41/app 38 manifest, direct reading of status, USB/RX, IPC, callback and ad-hoc signing paths; risk inventory.
2. **R2 — reproduction:** three tests run *before* each fix, aborting as expected; minimal fix in three shared sources; new tests and partial overlap variants.
3. **R3 — compilation/validation:** portable Debug configuration and build with AddressSanitizer/UBSan; **92/92 CTest groups completed without failures** in six disjoint runs (36 + 1 + 32 + 10 + 1 + 12). Overlap regressions also passed after the final additions. A broad run interrupted by the time limit and an inconclusive packaging test attempt were not counted as successes; its *retry* passed.
4. **R4 — independent analysis:** `clang --analyze` applied to the three changed C modules. Two units without warnings; one conditional cross-unit analysis message was investigated, not treated as proof of a bug. The fix added no dependencies and did not enable 5 GHz transmission.
5. **R5 — final verification and delivery:** the single cumulative patch `docs/diffs/FIXES.patch` includes app build 38 fixes and this round's six new files, applicable to the build 41/app 37 base; final manifest and ZIP verified. Detailed hashes, test output, reproductions and checkpoints are centralized in `docs/AUDIT-EVIDENCE.txt`.

### Integrity, organization and protection of earlier changes

- **Code/test files changed relative to the input ZIP:** exactly `src/Family5GHzScan.c`, `src/RTL8812AUPassive.c`, `src/USBPackets.c`, `tests/family_5ghz_scan_tests.cpp`, `tests/rtl8812au_passive_tests.cpp`, `tests/rx_preflight_tests.cpp`. No originally present file was removed; remaining code and firmware stayed byte-for-byte identical.
- **Only one artifact per purpose:** audit report `docs/CONSOLIDATED-AUDIT.md`, evidence `docs/AUDIT-EVIDENCE.txt`, matrix `docs/AUDIT-COVERAGE.tsv`, patch `docs/diffs/FIXES.patch`. Other README, architecture, manuals, firmware/catalog records and tests serve distinct purposes and were preserved.
- **Ad-hoc signing:** `RTW_SIGN_IDENTITY=-` remains mandatory in `app/signing.sh`; policy statically tested. **Actual native signing with `codesign`, Xcode/IOKit, macOS execution and hardware verification remain pending**.

### Release status, risks and resume checkpoint

**Status: NOT VALIDATED for macOS Release.** It was not possible to prove universal absence of bugs, leaks, CPU/RAM/energy problems, functional 5 GHz support for the seven dual-band families, or absence of visual regression in light/dark mode. Catalogued 5 GHz RX remains **passive**, without certified operational RF/TX.

**Exact checkpoint:** start from this round's ZIP, check ZIP SHA-256 and `SOURCE-MANIFEST.sha256`; resume unfinished direct inspection of `kext/RTWUSBController.cpp` per function (retention/abort/callbacks/stop under unplug), then `app/Main.mm` (dispatch, Keychain, timers, menu state) and `app/AdapterDiagnostics.mm` (IORegistry and CF ownership), using the matrix to record lines and functions actually examined. On a Mac, run Debug/Release `xcodebuild`, actual ad-hoc `codesign`, Instruments (leaks/CPU/energy), VoiceOver and regressions for USB disconnect and physical 2.4/5 GHz association. Never count automatically listed code as covered.

---

# CONSOLIDATED AUDIT — TahoeRealtekWiFi

## Audit of 24/10/2026 — current verifiable state

**Audited base:** last ZIP actually delivered in this conversation, TahoeRealtekWiFi v0.13.19 / kext build 41 / app build 37, 5 GHz RX stage 2. Base archive SHA-256: `b6b500fdf82a6c6da7cded57bb1ef76d5d7baa76168404016656790852f5f1ff`. ZIP integrity test and source manifest: passed (555 manifest entries, 556 files in the base tree). **The ZIP contains no `.git`**, so the audit cannot prove commits, branches or a newer remote tree.

**Resulting version:** kext 0.13.19 build 41 (unchanged); app 0.2.13 build 38 (incremented for the transport fix). **Source delivery, not Release binaries.**

### Rounds with evidence — without simulating production validation

1. **Integrity and architecture analysis**: base package hashes checked, 556-file inventory; manual inspection of parts of `app/Main.mm`, `app/DriverConnection.mm`, `app/Transport.cpp`, `app/Protocol.hpp`, `app/Devices.hpp`, `app/PrivilegedService.mm`, `app/Helper.cpp`, `app/CredentialStore.mm`, `app/LoginItem.mm`, `app/CodeIdentity.h`, `kext/RTWUSBController.cpp/.hpp`, `kext/RTWUserClient.cpp`, `kext/RTWNet80211Globals.cpp` and multifamily passive RX. Specific attention to buffers, message validation, lifecycles, queued work, credential retrieval, USB teardown and device control. **Analysis was partial in large units and does not equate to semantic inspection of every line.**
2. **Independent defect reproduction**: the socket test added two locally invalid requests that should erase previous outputs; **before the change, execution aborted in `assertErased`**. Root cause: `rtwui::exchange` returned before cleanup if the request was rejected locally. The equivalent XPC path had the same problem. Minimal fix in `app/Transport.cpp` and `app/DriverConnection.mm` with cleanup *only when* known valid output capacity exists, including rejection of length above `UINT32_MAX`. Portable regression passed **ten scenarios** after the fix. Equivalent native test added in `app/tests/transport-tests.cpp`, but not run without macOS.
3. **Portable build regression**: CMake Debug configuration and compilation with `-fsanitize=address,undefined` passed. **90/90 fast tests** and **2/2 long tests** passed separately after the change. Separate execution avoids extrapolating from a single process timeout. ASan/UBSan instrumentation covers portable components actually compiled, not the macOS kext.
4. **Independent static analysis**: `clang --analyze` run on **50 `src/*.c` and `src/*.cpp` units**, with zero analyzer execution errors and zero files with warnings. Static analysis does not prove absence of concurrency bugs, IOKit leaks or radio behavior.
5. **Signing, lifecycle and documentation review**: `RTW_SIGN_IDENTITY=-` policy preserved, ad-hoc signing rules checked by script reading and policy tests (not native `codesign`). Audit history gathered in this document; historical matrices from earlier releases are replaced by the single active matrix `docs/AUDIT-COVERAGE.tsv`. README/CHANGELOG references updated to this source of truth.

### Reproducible checkpoint and persistent evidence

- **Single record of this round's commands and results:** `docs/AUDIT-EVIDENCE.txt` (reproduction *before* the fix, CTest 90/90, both long CTest 1/1 runs, Clang analysis of 50 units and transport execution 10/10). SHA-256 hashes of original logs are recorded in the file.
- **A single textual diff:** `docs/diffs/FIXES.patch`; checked with `patch -p1 --dry-run` on the base tree. Records the seven modified original files. Consolidated historical documentation, matrix and new records are audit artifacts, not production logic changes.
- **Delivered source integrity** is verifiable through `SOURCE-MANIFEST.sha256` (every saved file, without self-reference), plus the external ZIP SHA-256 checksum; the matrix records per-file hashes and functions only identified heuristically.
- **Resume checkpoint:** kext build 41, app build 38; incomplete manual review recorded in the matrix; precise next inspection: start with `kext/RTWUSBController.cpp` teardown/asynchrony and `app/Main.mm` UI/dispatch lifecycle, reproduce actual cases with macOS instruments before claiming Release or absence of leaks.

### Fixed defect (F-01)

- **Trigger:** call to `exchange` with invalid frame or input, `out` buffer holding an old result and valid size.
- **Expected:** protocol error and zeroed `out`, as for malformed and incomplete responses.
- **Observed before the fix:** protocol error with old buffer preserved (regression failed deterministically).
- **Implementations:** `app/Transport.cpp` (socket) and `app/DriverConnection.mm` (XPC). Output with a null pointer or wrong capacity is still rejected without writing outside declared bounds.
- **Regressions:** `tests/socket_exchange_host_tests.cpp` and `app/tests/transport-tests.cpp`.
- **Change scope:** no change to ABI, firmware, USB profiles, theme, kext or wireless protocols.

### Coverage and per-file/function matrix

The **`docs/AUDIT-COVERAGE.tsv`** inventory distinguishes files whose **integrity** was verified, partially inspected implementation and modules actually exercised by portable tests. Symbol rows discovered through syntactic extraction are candidates only: they do not prove manual inspection. No claim is made of manual auditing of 100% of functions.

### Mandatory ad-hoc signing

Source continues to enforce ad-hoc signing for app/helper/kext through `app/signing.sh`. The portable suite includes policy tests. **Actual `codesign`, `xcodebuild`, binary artifact signing, installation, Instruments, Dark/Light layout tests, USB hardware, 2.4 GHz or 5 GHz testing were not performed in this Linux environment.** Consequently there is no proof of a physical Release or of the seven dual-band families as operational 5 GHz stations.

### Outstanding work, risks and next checkpoint

- **P0 delivery risk**: actual unplug/replug, sleep, hotplug and IOKit client tests under load on macOS Tahoe; check `busy/retain` in IORegistry and asynchronous transfer stability.
- **P0 compatibility risk**: RF/PHY backends, firmware, IQK/DPK, 20/40/80 MHz, WPA2 and 5 GHz TX/RX are still incomplete for seven families; passive beacon readers do **not** constitute functional Wi-Fi in that band.
- **P1 quality risk**: validate AppKit on macOS with VoiceOver, light/dark theme, menu dimensions, high network density and asynchronous state updates; CPU/RAM/energy profiles with Instruments.
- **P1 process risk**: obtain verifiable Git metadata (commit/hash/branch) from the original repository. Packaged source does not include this information.
- **Exact next check**: build the delivered tree with Xcode on macOS, run native `app/tests/transport-tests.cpp` tests, sign ad-hoc and check `codesign -dv --verbose=4`; then perform physical macOS Tahoe tests with unplug/replug and network traffic. Do not mark this stage complete without hardware evidence.

### Organization and history

This is the **only active audit `.md` report**. To avoid losing earlier information, complete predecessor reports are included below with their hashes. `README.md`, `CHANGELOG.md`, `ARCHITECTURE.md` and `SOURCES.md` remain separate because they have their own purposes. Historical `.tsv` matrices are replaced by the current source matrix; their original file hashes are recorded here.

#### Checkpoint: consolidated historical matrices

- `AUDITORIA-INVENTARIO-APP35.tsv`: SHA-256 `ff0d11ad99e1019d612fd4f9b2b631755ad7a88e37c76bbc7f827dfd2c0306b1`; previous release matrix, do not use as the current build inventory.
- `AUDITORIA-INVENTARIO-APP36.tsv`: SHA-256 `d1a7fad386728f86e5e5f6593f72be67bc72976dcdf35cb39b9ab215043299bf`; previous release matrix, do not use as the current build inventory.
- `AUDITORIA-INVENTARIO-BUILD40.tsv`: SHA-256 `80558caf7ef4f1fa00bd9ec599629b1083c63dea9c5c50d0ef4e7477bb2ec3c7`; previous release matrix, do not use as the current build inventory.
- `INVENTARIO-SOURCE-BUILD41.tsv`: SHA-256 `9ea1aaf2bead7a1ade9a925553f9c333b3ab7d82424dee0f22ad604027e713cb`; previous release matrix, do not use as the current build inventory.

---

# APPENDIX — HISTORICAL DOCUMENTATION (does not constitute validation in this round)


## History: `docs/AUDIT.md` (SHA-256 `69688bf02a11c82b5cd9661b786f9424671639bc766520135951c65cc751e84e`)

## 0.13.19 — guarded firmware-stage and optional USB laboratory tool

**Scope:** Added a shared legacy 8051 firmware *transfer and startup stage* for
RTL8192FU/RTL8723BU. RTL8710BU receives a validated firmware layout only,
with boot blocked because the SoC has a distinct power/reset sequence.
All other chipsets fail closed in this module. This is **not** a completed
PHY/RF or macOS station backend.

**Read-only stage:** `rtw_legacy_fw_prepare` requires a catalog-owned,
verified profile and descriptor-validated bulk endpoints. Known firmware
signature/header/size must match family. `rtw_legacy_fw_walk` plans at most
8 × 4096 bytes, using at most 254 bytes per vendor request without crossing
page boundaries; no USB writes and no allocations occur during planning.
Firmware format checking is not a cryptographic provenance guarantee.

**Experimental hardware stage:** `rtw_legacy_fw_boot_experimental` is
available only for 8192FU/8723BU and is deliberately outside kext attach.
It requires explicit caller permission, exclusive interface ownership and
board-specific power initialization to have been completed externally.
Its bounded 8051 sequence rejects stale MCU checksum state, aborts on short
writes or timeout and attempts to disable downloads on failure.
An interrupted transfer may leave the adapter requiring a USB replug.
The optional `rtw-legacy-fwdiag` tool defaults to read-only USB descriptor
inspection and dry-run transfer planning. Writes require two explicit
flags; it neither detaches the kernel driver nor offers Wi-Fi connectivity.
It claims only the descriptor-validated interface, not a hard-coded USB 0.

**Validated:** GCC CMake build, **85/85 CTest groups with UBSan**;
firmware-stage synthetic USB tests for family/signature mismatch, size/page
boundaries, stale checksums, short USB transfers, error cleanup and deadlines.
Clang strict C compilation plus analyzer passed the new translation unit;
Clang C++ syntax checked the optional lab tool against a temporary libusb
API declaration stub. No real libusb compile/link, macOS Xcode,
actual `codesign`, IQK, RF transmission, AP join, WPA2 handshake or real
adapter use was performed.

**Coverage claim:** 14 chipset families are catalogued, but only the four
preexisting station families have eligible kext personalities (122 IDs).
105 confirmed profiles need functional backends and 43 remain unverified.
This release adds **zero** usable/certified station adapters.

## 0.13.18 — cross-family RX / firmware / passive-radio review

**Scope:** RTL8192FU, RTL8723BU, RTL8821CU, RTL8822BU, RTL8822CU receive
decoders; RTL8192DU / RTL8192FU / RTL8723BU / RTL8710BU firmware header
inspection; six unfinished families' passive channel geometry. USB
preflight validates the existing 270 IDs; 122 remain eligible for the
already integrated station code. These are source-level capabilities only.

**Safety:** Both 24-byte RX wire variants validate bounded USB aggregates
before publishing any result, including CRC/ICV, rates, sizes, frame
limits and descriptor alignment. Family RX helpers never access physical
USB, never allocate memory and do not authorize TX. Firmware checks are
signature/size checks only, not cryptographic authentication. The
channel plan is passive-only and does not assert regional permissions.

**Testing:** CMake portable build; 84/84 CTest cases with UBSan and the
system Python; 60,000 RX plus 35,000 firmware/channel randomized inputs;
four modified C units compile strictly and pass Clang static analysis.
`plutil -lint` validates the Xcode project; the kext plist retains
122 personalities. Native Xcode kext/app compilation, real codesign,
IQK, USB hardware operation, AP association and physical RSSI validation
were not performed on this Linux test host.

**Remaining blockers:** RTL8192DU RX/TX requires a checked silicon-specific
wire format; RTL8710BU/RTL8814BU still lack verified station hardware
implementations. All new RX-only families still need firmware upload,
PHY/RF calibration, TX, WPA2, macOS kernel interface integration and
physical regression. Do not interpret a passed decoder test as a
functioning radio.

## 0.13.17 — Cross-family backend boundary (portable stage)

The **14-family** registry and USB preflight are implemented for every
catalogued chipset. The source checks actual class/subclass/protocol,
configuration value, interface number, optional bcdDevice and a safe paired
bulk endpoint mapping. Unverified ID matches are rejected. No adapter is
claimed or modified by this portable code.

The available RX wire-format codecs are **RTL8192CU, RTL8188EU, RTL8192EU,
RTL8814AU, RTL8812AU, RTL8821AU**, with all-frame atomic offline decoding.
Eight other families still have **no chip-specific RX/TX codec**. Shared
preflight is **not** a substitute for a station backend: no hardware
transmission is authorized by `FamilyBackend.c`.

The kext's runtime discovery retains its four existing station families
and now cross-checks the backend capability bit before IOEthernet attach.
This check was compiled only as portable C and contract-tested; no native
IOKit ABI, RF calibration, association, WPA2 exchange, kext loading,
codesign, or hardware interoperability was verified here.

USB catalog retained unchanged: 122 eligible personalities in the four
existing families, 105 verified IDs pending full backends and 43 unverified
or conflicting IDs. The requested full 14-family hardware support remains
**unfinished**. The next engineering work is porting firmware boot / PHY
tables, calibrations, RX/TX and station attachment for each missing silicon
generation, then verifying those functions on the actual USB devices.

# Compatibility and protocol review — driver 0.13.16 / app 0.2.12

Review date: 2026-10-23. Input: source release 0.13.15 / app 0.2.12.
This is the single current audit. `validation.json` contains executed evidence;
`source-inventory.csv` inventories the shipped source/configuration files.

## Changes audited in 0.13.16

Input: driver 0.13.15 / app 0.2.12. Corrected `FirmwareBoot.c` to clear and
verify the write-one-to-clear MCUFWDL checksum status before each firmware
download. The USB mock models W1C rather than treating MCUFWDL as normal RAM.
Regression cases cover a stale status during a failed new boot and an
unclearable status that must abort before uploading firmware data.

`Efuse.c` and `RFCalibration.c` now reject saturated six-bit thermal code
0x3F as an eFuse baseline. This only avoids a false RF preflight result;
no actual IQK, LCK or power tracking is run by the RTL8812AU preflight.

`RTL8812AUStation.c` parses all advertised RSN pairwise/AKM suites and
accepts the WPA2-PSK/CCMP subset of transition networks with optional PMF.
It rejects SAE-only, TKIP-only, mandatory PMF and truncated/malformed
suite lists. This parser operates offline and cannot grant carrier.

Portable UBSan build: **80/80 CTest groups passed**, 0 failed, using the
system Python. Four changed C modules passed Clang -Werror strict compilation
and Clang static analysis without diagnostics. No native macOS Xcode,
actual codesign, USB hardware boot, memory-leak instrumentation or physical
802.11 association tests were completed. Enabled USB identities remain 122,
and 148 other catalog identities remain disabled.

## Changes audited in 0.13.15

Source input: 0.13.14 / app 0.2.12. The station kext's scan previously
treated the available pairwise CCMP indication as enough to advertise a
WPA2-PSK/CCMP connection. The new `rtw_network_security_classify` also
requires RSN, PSK, **group CCMP** and absence of mandatory PMF; the
group field comes from net80211 `ni_rsngroupcipher`, not the negotiated
pairwise `ni_rsncipher`. The model remains conservative: a displayed AP
is only *protocol-eligible*, not proven able to associate over the radio.

`src/RTL8812AUStation.c` now validates association-response ESS/privacy
capabilities and advertised rates/IE structure before reporting even
offline association. These helpers are not wired into a full real station
backend for RTL8812AU/RTL8821AU, cannot transmit or negotiate WPA2 keys,
and do not increase the kext's enabled ID set.

Portable Linux Debug/UBSan CTest: **80 of 80** pass with `/usr/bin/python3`
and four-worker scheduling. Includes 64 security combinations, malformed
association-response cases, and a source contract checking the native-only
macOS scan's actual group-cipher field. Clang strict C/C++ syntax
compilation and Clang static analysis of the modified portable station
module passed. USB matching generation and catalog checks passed.
**Native Xcode/kext compile, real codesign, hardware RF/link tests,
ASan/LSan and macOS Instruments were not performed.**
The baseline 122 mapped/enabled USB profiles are not hardware-certified,
and the remaining 148 catalog profiles stay disabled.

## Changes audited in 0.13.14

`src/RFCalibration.c` / `include/rtw/RFCalibration.h` extend the RTL8812AU
B-cut/TSMC/USB/RFE3 channel-6 passive *laboratory diagnostic*, not the
operational kext station attach path. Preflight requires explicit opt-in,
positive prior path-A RF channel readback, prepared USB chip identity and
six matching MAC/BB monitor registers. It reads two path-A thermal values
from RF[0x42] via Jaguar HSSI (PI or SI mode), comparing their 6-bit sensor
codes and an optional factory eFuse code at logical offset 0xBA. Missing,
unstable or untrusted values do not silently become a valid calibration.
A delta of 8 or more code units is only an advisory for a future IQK
implementation. Code units are not degrees Celsius.

After a complete register snapshot, the preflight temporarily adjusts the
CCA guard and RF serial selector. It attempts selector/CCA restoration on
every path after a write and verifies both saved values; restore failures
override sampling failures. It does not program analog RF, start a temperature
conversion, run IQK/LCK/DPK, change TX power, transmit 802.11 frames,
authenticate or advertise a station link. A cached RF register value may
look consistent without being fresh, so successful observations cannot prove
functional calibration. `rtw-hwdiag` invokes the preflight only in the
already authorized lab mode and attempts MAC RX quiescence on malformed
USB transfer lengths as well as normal/other failure exits.

Portable Linux Debug build configured with UBSan (`-fsanitize=undefined`):
**78/78 CTest groups passed** using `/usr/bin/python3` with `-j4`;
the new synthetic USB calibration test passed **96 checks**. Separate RF
readback tests passed 119 checks. HAL assertions passed 504 checks, with
12,000 adversarial eFuse/RX test inputs. Strict Clang warnings-as-errors
syntax of the new C module passed. No Apple `codesign`, macOS/Xcode,
real libusb link, device RF/thermal readout, power calibration or association
was performed. Previously eligible 122 profiles and pending 148 profiles
are unchanged. The UI is still English and mandatory ad-hoc signing policy
remains in place but was not verified by native codesign in this host.

## Changes audited in 0.13.13

This increment adds `RFReadback.h` / `RFReadback.c` to diagnose the
hardware RF channel of the previously opt-in RTL8812AU B-cut/TSMC/USB/RFE3
channel-6 monitor. `rtw_rf_readback_channel6_8812au` first re-verifies the
six MAC/BB registers and the monitor authorization flag; if this read-only
guard fails, no additional register writes occur.

After saving registers 0x838 (CCA) and 0x8B0 (HSSI), the RF probe temporarily
sets the secondary-CCA guard and selects RF register 0x18 in HSSI. It reads
path A and B via the appropriate PI or SI readback register, twice each.
Path A must encode channel 6 with 2.4-GHz band bits. Path B is diagnostic:
zero/invalid/stale B readback is explicitly inconclusive, never evidence
of working dual-chain RF. Selector and CCA original values are restored
on all post-write exits and verified by reading them again. A failed restore
takes precedence over the original I/O error and blocks capture.

`rtw_monitor_quiesce_rx_channel6` reads CR (0x100), clears only MACRXEN,
RXDMA_EN and HCI_RXDMA_EN, and verifies their cleared state. The libusb
diagnostic calls it on normal scan exit, after RF/MAC verification failure
and on partial RX configuration failures. It is **not** an RF power-down
or full hardware reset. It does not alter the station kext's attach path.

Host validation: Linux debug build with `-fsanitize=undefined` throughout,
**77/77 portable CTest groups passed** using `/usr/bin/python3` in 34.91 s.
The new mock RF test covers 2T2R PI/SI selection, path-B zero values,
two path-A samples, invalid channel/band, report authorization, USB
read/write error injection, RF settling delay failures, BB restore readback
failures and MAC RX quiescence. Both changed C modules pass Clang strict
`-Wall -Wextra -Wconversion -Wshadow -Wpedantic -Werror` syntax checks
and `clang --analyze` with no emitted diagnostics.

`tools/rtw-hwdiag.cpp` passes Clang C++ syntax checking against temporary
test declarations for the `libusb` API. This is **not** equivalent to
compiling and linking against the real `libusb` SDK. No physical adapter
or macOS/Xcode build was available; no real RF register readings, IQK/LO
calibration, TX power verification, association, WPA2 or codesign was tested.
The 122 previously enabled profiles are unchanged; the RTL8812AU/RTL8821AU
station IDs are still disabled.

## Changes audited in 0.13.12

This increment connects the **opt-in libusb passive laboratory RX path**
to `rtw_r12au_rx_next` (specific 8812AU aggregate layout) and
`rtw_r12au_scan_parse` (beacon/probe/RSN with FCS). The two-pass aggregate
preflight prevents a truncated tail from publishing an earlier AP from the
same USB transfer. The receiver uses caller-owned buffers, no heap allocation,
a 65,536-byte transfer bound and up to 64 frames; callbacks do not store
pointers. Unsupported security can appear in diagnostics with an explicit
unsupported indication, never as a usable station backend.

`rtw_monitor_verify_channel6` makes exactly six read-only vendor-register
requests on success, comparing the 0xF0 SYS_CFG, 0x100 MAC RX enable,
0x608 RX filter and selected BB gate bits at 0x800, 0x808 and 0x834.
A mismatch stops the tool before bulk capture. It does **not** read RF
path 3-wire channel register 0x18, prove IQ calibration, guarantee reception
or authorize station TX. The earlier opt-in profile/table write sequence
is unchanged; its risks and supported board restriction remain.

Portable evidence: full non-instrumented CMake/CTest suite 76/76 passing in
31.58 s (`-j4`, `/usr/bin/python3`). Four affected tests were separately
built with `RTW_ENABLE_SANITIZERS=ON` and
`RTW_HOST_SANITIZERS=undefined`; all 4/4 passed. New RX tests ran
20,000 deterministic hostile-transfer iterations. Clang strict
`-Wall -Wextra -Wconversion -Wshadow -Werror` and `clang --analyze`
completed for the two modified C translation units without diagnostics.
Xcode project plist syntax passed `plutil -lint`. The optional libusb tool
compiled **only in syntax mode against temporary test declarations**, not
against the real libusb library. No macOS Xcode/kext build, real codesign,
ASan/LSan, Apple Instruments or physical hardware capture was performed.
The 270-profile matrix, 122 enabled / 148 disabled split, app UI strings,
firmware blobs and signing policy were not modified.

## Changes audited in 0.13.11

Implemented `RTL8812AURadio.{h,c}` (pure channel/frequency geometry) and
`RTL8812AUStation.{h,c}` (beacon/probe-response and WPA2-RSN inspection,
fixed-size selected AP state, open-system authentication and association
frame serialization and reply state transitions). Both modules are portable,
make no USB calls, allocate no heap memory, and grant no TX authorization.

The channel planner handles 2.4 GHz channels 1-11 at 20 MHz and selected
5 GHz standards-based groupings at 20/40/80 MHz. It rejects inconsistent
primary/center pairings, unsupported widths, channel 14 and 2.4 GHz 40 MHz,
and marks every plan passive-only. DFS is a flag requiring full regulatory
handling and hardware CAC; no passive-only plan may be used to transmit.
Real RFE and per-board transmit-power calibration remains unimplemented
for the new families.

The station parser requires the PHY receiver's externally known tuned
channel and rejects DS/HT primary mismatches, duplicate SSID/RSN/rate IEs,
truncated fields, conflicting or invalid RSN suites, hidden SSIDs, bad MAC
addresses, ICV/CRC descriptor errors and invalid FCS when available.
It permits only open networks or WPA2-PSK with CCMP group/pairwise suites;
WEP, WPA1, SAE, 802.1X, TKIP, PMF-required and mixed-mode RSN are not
negotiated. CRC32 is not authentication and cannot prevent forged beacons.

The join state machine checks BSSID/source/destination and response status,
tracks timeout/disconnect/reject states, emits a restricted RSN IE for
WPA2 and refuses invalid management transitions. Successful management
responses do not create a trusted station link: OPEN remains UNVERIFIED;
WPA2 remains KEYS_REQUIRED, with no four-way handshake or CCMP TX.
No code connects these new functions to the active kext station path and no
USB IDs were changed.

Validated in a Linux portable CMake build with UBSan and all 75 CTest
groups passing (31.1 s final run, -j5), 100,495 targeted RF/join checks including
50,000 deterministic parser fuzz inputs, 11 CLI integration scenarios, and
Clang --analyze without warnings for the two new implementation files.
Existing C/RTL8192EU warnings and previously reported instrumentation
limits remain; there was no native macOS build, codesign invocation,
Apple Instruments leak measurement, radio transmission or physical testing.

## Changes audited in 0.13.10

This increment implements a portable RTL8812AU/RTL8821AU wire-format layer
without enabling any new USB device, kext attach path, firmware radio sequence
or operating station. The unchanged 270-profile matrix contains 122
backend-eligible entries, 105 waiting for a station backend and 43 with
conflicting or inadequate identity evidence. Of the 105 waiting, 34 are verified
RTL8812AU IDs and 22 verified RTL8821AU IDs. No new adapter is claimed working.

Descriptor memory is caller-owned: no allocations, threads or retained pointers
are created. Bulk input length is bounded at 65,536 bytes, each RX frame at
4,096 bytes, and a transfer at 64 frames. Truncated, oversized and malformed
descriptors return error statuses without advancing the successful-frame cursor.
PHY metadata has the correct position before a separate alignment shift.
The 40-byte TX serializer validates candidate chain count, link-mode options,
rates, EAPOL, multicast and QoS, then computes and checks the 16-bit XOR
checksum over the descriptor prefix. Non-QoS frames request hardware sequence
assignment, while QoS frames carry a caller-assigned sequence.

Tests confirmed the module builds in the Linux CMake project and is listed in
the native Xcode kext target (the latter was not compiled). The new unit test
exercised 149,935 assertions, including 50,000 deterministic fuzz iterations.
The same isolated unit test passed under Clang UndefinedBehaviorSanitizer.
The read-only inspector decoded a synthetic RX transfer for both families.
The complete 73-group portable CTest suite then passed in 33.31 seconds,
with the system Python interpreter and `RTW_HOST_SANITIZERS=undefined`
configured in CMake. The matrix test also passed four of four unittest
cases when run separately. Earlier attempts with the original environment
timed out during interpreter startup or failed to allocate ASan shadow memory;
those attempts are not counted as passes. No macOS / on-device testing occurred.

USB adapter identity, English application strings, signing policy and firmware
from 0.13.9 were preserved. Ad-hoc signing remains mandatory in the build
scripts, but no actual macOS signature was produced. The actual 8812/8821
PHY calibration, chipset/RFE variants, WPA2 and station operations remain to
be implemented and validated before their 56 verified IDs may be enabled.

## Baseline compatibility review from 0.13.9

The following 20-alias compatibility review was performed in the previous
release and is preserved as historical evidence, not a claim that the complete
suite ran again in 0.13.10.

## Result and scope

The implementation enables **20 additional USB identities** through existing
station backends: 19 C/CU and one RTL8814AU. The complete catalog has 270 entries,
with 122 enabled: C/CU 86, 8188EU 13, 8192EU 9, 8814AU 14.

This is not an implementation of all 250 legacy profiles or all possible Realtek
families. The original 148 disabled entries remain disabled: 105 have confirmed
numeric family evidence but no integrated station backend, and 43 still have
unverified/conflicting identity evidence. No new radio HAL or firmware was added.
The RTL8188FU/FTV path was investigated but remains incomplete and is not enabled.

The review focused on catalog evidence, shared matching, kernel discovery,
generators and app diagnostics. It is not a fresh line-by-line audit of every
vendor source. Previous signal, lifecycle and resource fixes are preserved;
their historical changes are recorded in CHANGELOG.md.

## Implementation changes

All 20 extension entries are represented in the source catalog, generated C
table, generated IOKit personalities, kernel full-descriptor lookup, app reports
and the single CSV matrix. They route to real existing implementations, not
dummy success paths. Existing endpoint, silicon/ROM, firmware, board/RFE and
connection-state checks are retained.

The 250 original matching dictionaries are preserved byte-value-for-byte-value
and protected by a canonical SHA-256 identity pin. New records have their own
origin and matching policy, not invented original plist provenance. The schema
requires exact numeric source evidence, an explicit revision/release, supported
backend membership, unique identities, bounded descriptor values and nonempty
NUL-free display names.

The kernel now passes class/subclass/protocol fields from the real USB interface
descriptor. Shared C matching checks explicit mask bits. The older five-argument
API fails closed for profiles that need the missing class descriptors. The app
reads the same descriptor fields and distinguishes missing data from mismatch.
A Bluetooth interface cannot qualify as a vendor-specific Wi-Fi interface just
because its containing USB device has a known VID/PID.

New C/CU aliases use configuration 1/interface 0/class 0xff as conservative
project policy. Verified AU profiles also require subclass/protocol 0xff/0xff
from the reviewed primary driver. A different genuine hardware layout is still
excluded; this is not a claim of unrestricted revision/interface coverage.
Legacy revision/configuration/interface constraints were not weakened.

C lookup now uses bounded binary search over the generated sorted table.
No radio polling, allocation rate, USB transfer size or calibration constant was
increased. CPU or throughput gains were not measured.

## New identities

C/CU:
04bb:0950, 050d:11f2, 0846:9042, 0846:9043, 0bda:17c0, 0bda:8170,
0bda:817c, 0bda:817d, 0bda:817e, 0bda:8186, 0bda:8191, 0bda:819a,
0bda:8754, 0eb0:9071, 1058:0631, 13d3:3358, 13d3:3359, 2357:0100,
9846:9041.

RTL8814AU: 20f4:809b.

The correct new identities 2357:0100 and 0eb0:9071 do not silently relabel the
different legacy entries 038b:0100 and 0e0b:9071. Those remain disabled.
All source locations are consolidated in SOURCES.md and per-profile metadata.

## Validation

The portable build and full 72-group CTest suite passed with explicitly selected
UndefinedBehaviorSanitizer. Assertions remain enabled. Tests include all 270
catalog entries, dispatch accounting, deterministic generation, preserved legacy
constraints, extension provenance mutation, unknown/wrong USB identities,
class/subclass/protocol byte domains, absent/overflowing registry descriptors,
composite-interface isolation, original radio/stack harnesses and mandatory
ad-hoc signing/package policy fixtures.

Signing tests use a controlled replacement for codesign; they do not produce
or verify a real Apple signature. Native app/kext builds and radio hardware
were not available. AddressSanitizer, LeakSanitizer and Instruments were not
executed for this release; UBSan is not a substitute for leak detection.

Static analysis results, exact commands and captured test output are recorded
in validation.json. File hashes prove archive consistency, not runtime safety.
No claim is made that all bugs/leaks are eliminated, that all enabled boards
work physically, or that performance improved by a measured percentage.

## Native acceptance still required

Build the app/helper/kext with Xcode on the target Intel Mac; verify mandatory
ad-hoc signatures and publication failure on broken signatures. The app installer
does not install the kext. Confirm actual USB descriptors and board revision
for every newly claimed adapter before physical certification.

Exercise scan, association, WPA2/CCMP, DHCP, Ethernet traffic, reconnect,
unplug/replug, sleep/wake, multiple adapters and sustained load. Check preserved
RSSI behavior, packet/error counters and runtime initialization diagnostics.
Profile CPU and memory with Instruments on the Mac. Capture light/dark/high
contrast and VoiceOver behavior for the updated diagnostic reports.

## Source delivery and documents

The source archive contains the complete app, helper, kext, portable core,
vendor stack, firmware, generators and tests, except the separately fetched
MacKernelSDK dependency already required by the baseline. It contains no
newly compiled macOS executable, installer or kext.

Documentation remains one file per purpose. The old fixed-size CSV filename
was replaced by USB-WIFI-ADAPTERS.csv, not duplicated. The existing README,
changelog, architecture, audit, provenance guide, validation record, source
inventory and integrity manifest were updated in place. Distinct third-party
license texts and component hash records are preserved unchanged.


---

## History: `docs/AUDITORIA-APP35.md` (SHA-256 `d9ef19c4841328011e328f7e9d6d13f421781b102bf80be68745b1a6b56e2dcf`)

# Follow-up audit — TahoeRealtekWiFi / App build 35

**Date:** 23/10/2026  
**Exact source:** `TahoeRealtekWiFi-v0.13.19-build40-APP-build34-ADHOC-AUDITED-FULL-SOURCE.zip`  
**Deliverable:** kext `0.13.19` **build 40** (untouched); app `0.2.13` **build 35**. Protocol/ABI was not modified.

## Source control, coverage and limits

- The base ZIP passed `unzip -t`, package SHA-256 checking and verification of all **536 entries** in `SOURCE-MANIFEST.sha256`; **537 files** in the original source. No older version was used.
- `docs/AUDITORIA-INVENTARIO-APP35.tsv` lists every regular file in the updated package, except itself and the self-referential manifest, with size, lines, SHA-256 and state compared with build 40. The updated manifest protects every file except the manifest itself.
- Checking all paths/bytes and automated checks do not equate to human semantic inspection of every line of approximately 140 thousand lines. Targeted reviews covered AppKit sources, reconnection, Keychain, XPC, packaging, scripts, signing policy and USB driver. Third-party code and firmware were preserved.
- Available platform: Linux with Clang compiler, CMake and portable tests. It was **not** possible to compile or execute AppKit/IOKit code on macOS Tahoe, observe actual UI, produce ad-hoc signed code or measure actual CPU/RAM use.

## Actual fixes — restricted scope

1. **Late automatic connection after disabling:** `app/Main.mm` checked interface identity and a JOIN generation after asynchronous Keychain reading, but did not check whether the user had disabled/paused auto-reconnect during that interval. Added a check on return to the main thread **before** `startJoin`; the delayed result is discarded and the temporary password erased. Manual connection remains allowed. The pure decision is in `app/ReconnectPolicy.hpp` with a combinatorial test in `app/tests/reconnect-policy-tests.cpp`.
2. **Malformed XPC response:** `app/DriverConnection.mm` accepted a validly framed response containing a semantically invalid payload; after `memcpy`, the function returned `protocolError`, leaving untrusted content in the caller's buffer without invalidating the connection. The error path now zeroes the buffer and closes the XPC connection. `tests/app_source_contract_tests.py` adds a static contract test; it does not replace a native XPC test.
3. **Traceable identification:** isolated app `CFBundleVersion` increment (34 → 35), with `CHANGELOG.md`, `README.md` and this report. `CFBundleShortVersionString` remains `0.2.13`, and the kext remains build 40.

No file was removed; no `kext/`, `src/` or `vendor/` source, USB profile, firmware, icon, theme, privilege, user setting or signing script was changed. The preexisting build 39 hot-unplug fix remains incorporated in build 40.

## Review matrix and evidence

| Subsystem | Check | Result / caveat |
|---|---|---|
| Architecture and tree | Inventory and per-file SHA, comparison with verified source | 100% file coverage; not proof of line-by-line semantic review |
| App and asynchronous states | `Main.mm`, `ReconnectPolicy.hpp`, scenario and contract tests | Late reconnection fixed; actual Keychain callback not exercised on macOS |
| IPC / helper | Framing, response content, error handling in `DriverConnection.mm` | Buffer and connection sanitized on invalid-payload path; actual XPC pending |
| Kext / USB lifecycle | Stop/close paths and preexisting portable tests | Kext untouched; physical disposal and actual retentions pending |
| CPU, memory, leaks, threads | ASan/UBSan build, CTest and static analysis of 49 `src` files | No failures in exercised scenarios; not proof of global absence of leaks/races |
| Wi-Fi, WPA, RF, firmware and USB | Matrix, WPA/net80211 and backend suite | 270 catalog profiles tested; firmware and protocol untouched |
| Interface/theme/fluidity | Reading menu states and preserving AppKit/SF Symbols | No visual change; requires light/dark mode inspection on macOS |
| Ad-hoc signing/installation | `app/signing.sh` and policy tests, .pkg routines | Requirement intact; no actual `codesign` in this environment |
| Third-party dependencies | Files hash-compared with build 40 | Unchanged; does not guarantee exhaustive upstream code audit |

## Tests and analysis performed

- CMake Debug build with `RTW_ENABLE_SANITIZERS=ON`, `RTW_HOST_SANITIZERS=address,undefined` — completed without error.
- CTest: **87/87 portable groups passed**, with **84** groups in one run and **3** longer tests run individually (`rtw-app-pkg-tests`, `rtw-net80211-pae-tests`, `rtw-usb-wifi-matrix-tests`). Sanitizers were enabled in the portable build; some Python tests have their own subprocesses.
- Targeted regressions: `rtw-menu-reconnect-policy-tests`, `rtw-menu-source-contract-tests` and `rtw-signing-policy-tests`, passed.
- Clang Static Analyzer (`core`, `unix`, `deadcode`, `security`) on 49 portable `src/` units: no error diagnostics; 96 generic `memset`/`memcpy`/`memmove` warnings from the rule for buffer functions considered unsafe. These warnings were not treated as confirmed leaks or fixed indiscriminately.
- Bash/Python syntax, plist parsing, manifest and ZIP hashes checked during packaging. Some Mac-dependent tests can only run on Darwin.

## Mandatory ad-hoc signing

`app/signing.sh` rejects `RTW_SIGN_IDENTITY` other than `-`, rejects notarization profiles in this path, signs with `codesign --sign -` and checks signatures on helper, binary, app bundle and kext. This audit **does not relax** policy or **disable SIP, AMFI or any macOS mechanism**. Distributed material is **source code**, without previously ad-hoc signed macOS binaries. Signing will be produced and checked through the existing flow on a Mac; by itself it does not bypass macOS security requirements for loading kexts.

## macOS validation still required

1. On macOS Tahoe Intel with Xcode and MacKernelSDK: `./build.command --all` and `./build.command --tests`; check logs before installation.
2. Confirm identities with `codesign -dv --verbose=4` and verify each bundle/binary with `codesign --verify --strict`.
3. Test Keychain with a deliberately delayed read while disabling reconnection; observe that no new automatic JOIN appears.
4. Test truncated/malformed XPC responses natively and confirm zeroing, connection invalidation and recovery.
5. Instrument CPU/heap with Instruments, compare before/after; check energy/idle, light and dark menu/theme, HiDPI resolution, sleep/wake, two Realtek adapters, USB disconnection and absence of residual IORegistry services.

**Result:** two reliability defects fixed in the app layer. Declaring absence of all bugs, leaks, layout regressions or hardware stability without the native tests above is not technically justified.


---

## History: `docs/AUDITORIA-APP36.md` (SHA-256 `f74913c0136afe93240e20b836dc095b40799bcc4bcdb6ebe351526ddeabb888`)

# General technical audit — TahoeRealtekWiFi, app build 36

**Date:** 23/10/2026. **Verified base:**
`TahoeRealtekWiFi-v0.13.19-KEXT-build40-APP-build35-ADHOC-AUDITED-FULL-SOURCE.zip`.
**Delivery:** kext 0.13.19 **build 40 (unchanged)** and app 0.2.13
**build 36**. Complete source; no macOS binaries or applied signature.

## Audit method and limits

- The input ZIP passed artifact SHA-256 checking and its integrity
  test. The complete tree was inventoried and every file hash-compared
  against that version, including sources, firmware, resources and
  third-party code. No file was removed from the original version.
- Portable code built in Debug with AddressSanitizer and
  UndefinedBehaviorSanitizer. The **88 groups** registered in CTest passed
  in batches **35/35 + 30/30 + 23/23**, including USB lifecycle, WPA policies,
  adapter matrices, packaging, app contracts and ad-hoc
  policy. Tests can vary in coverage and do not inspect every
  possible runtime path.
- Targeted review of app contracts, IPC data flow, cancellation
  path, socket operations and read failures, USB lifecycle,
  build/packaging scripts and signing. Complete tree and hash checking
  **does not equate** to human line-by-line semantic auditing of all
  third-party and driver code, or guarantee absence of future defects.
- Execution environment: Linux/Clang. No native AppKit/IOKit build,
  Instruments measurement, Realtek hardware testing or graphical validation
  of light/dark modes in this delivery. Therefore, 100% absence of
  bugs, leaks, freezes, excess CPU use or visual defects is not claimed.

## Confirmed defect and implemented fix

**File:** `app/Transport.cpp` — `rtwui::exchange`, legacy socket transport.

**Before (build 35):** a valid response header followed by EOF after
part of the payload made `readExact` return false and `exchange` return
`transportError`, but left both previously existing bytes
and partially read, untrusted data in the output buffer. Responses with
invalid version or invalid device lists also failed to sanitize
their buffer before returning `protocolError`.

**Now (app build 36):** after sending the request, output is cleared before
any response read; it is cleared again on errors after partial reading
or payload validation. Send failure also clears output. Ordering
preserves possible sharing between input/output buffers
until sending finishes. Framing, ABI, protocol, selectors and status are unchanged.
The active XPC component was not changed: this fix specifically protects
the legacy socket exchange path.

**Causal reproduction:** the test with implementation extracted **from build 35**
aborts in `assertErased()` after a truncated response (buffer containing
nonzero bytes). With the current implementation **extracted unmodified** from source,
seven actual `socketpair` scenarios passed: valid payload, truncated
response, invalid version, helper-reported error, wrong registry ID,
incomplete header and malformed device list. The regression
executable is compiled with sanitizers. Equivalent cases were added
in `app/tests/transport-tests.cpp` for later native execution and a
contract in `tests/app_source_contract_tests.py`.

## Coverage matrix

| Subsystem | Evidence from this review | Condition/outstanding work |
|---|---|---|
| Package / provenance | Inventory, hashes, comparison with base ZIP; no original removed | Verifiable integrity, but not proof of global correctness |
| App transport / IPC | Bug reproduced in legacy path; seven socket scenarios and source contract | Main XPC unchanged; native XPC execution pending |
| Reconnection / Keychain | Build 35 regressions and logic retained | Actual Keychain latency test pending |
| IOKit driver / hot-unplug | Code identical to build 40; portable lifecycle tests | Actual USB disconnection, IORegistry retentions pending |
| WPA / RF / firmware | Portable tests included and artifacts preserved | Radio and hardware association not checked |
| CPU, memory, threads | Debug + ASan/UBSan, proven buffer fix | No actual CPU/heap measurement; global absence of leaks not guaranteed |
| AppKit / layout / theme | Assets and visual code preserved, portable contracts checked | macOS visual QA, animations and HiDPI pending |
| Ad-hoc / helper / kext | `app/signing.sh` policy retained; policy test passed | `codesign`/installation not performed on Linux |
| Final package | SHA-256 manifest and ZIP checked byte for byte | Provenance/changes traceable in inventory |

## Ad-hoc signing (mandatory)

In `app/signing.sh`, `rtw_require_adhoc_identity` rejects identities other than
`-` and notarization profiles; `rtw_sign_app` and `rtw_sign_kext` use
`codesign --sign -` and check **`Signature=adhoc`** for
applicable components. Scripts and policy remained **byte-for-byte
unchanged**. The distributed file is source, so it contains no already-signed app,
helper or kext. The signature must be issued and verified
on a Mac; it does not replace kext loading permissions required by macOS.

## Tracked changes

- `app/Transport.cpp`: safe buffer cleanup on receive,
  send and invalid-payload failures.
- `app/tests/transport-tests.cpp`: three new macOS regressions (not run).
- `tests/app_source_contract_tests.py`: new sanitization contract.
- `tests/socket_exchange_host_tests.cpp` + `scripts/test-app-socket-exchange.py`:
  host test based on literally extracted production functions, 7 scenarios.
- `CMakeLists.txt`: registers the new test group.
- `app/Info.plist`: only build 35 → 36 (retains `0.2.13`).
- `README.md`, `CHANGELOG.md`, `docs/AUDITORIA-APP36.md`,
  `docs/AUDITORIA-INVENTARIO-APP36.tsv`, `SOURCE-MANIFEST.sha256`: documentation,
  hash trail and history. No `kext/`, `src/`, `vendor/`,
  `include/` file, firmware, visual asset, protocol or signing script
  was changed relative to the verified package.

## macOS validation still needed

1. Build on macOS Tahoe Intel with required SDKs using the project
   flow (`./build.command --all` and `./build.command --tests`).
2. Check `codesign -dv --verbose=4` with `Signature=adhoc` on each
   component, plus `codesign --verify --strict` after packaging.
3. Exercise `app/tests/transport-tests.cpp` natively; force
   interrupted/invalid responses and observe recovery without partial data.
4. Measure CPU, heap, wakeups and battery with Instruments; check icons,
   layout, fluidity, login item and light/dark mode in actual AppKit.
5. Test USB plug/unplug, sleep/wake, multiple adapters and IORegistry
   retentions, comparing `RTWUSBController` and USBHost before and after.

**Delivery result:** one actual failure reproduced and fixed, with
regressions introduced and all other subsystems fully preserved.
**There is no proof that all bugs/leaks have been eliminated.**


---

## History: `docs/AUDITORIA-GERAL-BUILD40.md` (SHA-256 `ffd3e38ba1a0f58808f4e56fd08d1fca041d76c53fce00b8b06694a0631121e0`)

# General technical audit — TahoeRealtekWiFi build 40

**Date:** 2026-10-23  
**Audited base:** complete ZIP `TahoeRealtekWiFi-v0.13.19-build39-USB-HOTUNPLUG-FULL-SOURCE.zip` (533 files), without reusing an earlier version.  
**Delivery:** kext functional version 0.13.19, **build 40**; app functional version 0.2.13, **build 34**. ABI and USB profiles unchanged.

## Scope precision and limits

All files present in the base ZIP were inventoried, compared byte for byte with the delivery and covered by automated content/syntax checks appropriate to their format. The list of files with size, lines, hash and classification is in `docs/AUDITORIA-INVENTARIO-BUILD40.tsv`. This **does not equate to manual semantic review of each of more than 100 thousand lines of code**. Human review focused on the driver, lifecycle, diagnostic publication, AppKit interface, packaging, signing and tests; third-party source and firmware were preserved byte for byte. Eradication of all bugs is not claimed.

Execution took place on **Linux**, not macOS Tahoe. Without Xcode, IOKit, AppKit, native `codesign` or actual USB adapters, performance, actual consumption, graphical fluidity, absence of kernel leaks, kext loading compatibility or an actually produced signature cannot be claimed. No SIP, AMFI, Gatekeeper, user settings or services were adjusted.

## Coverage matrix

| Component | Analysis and evidence | Status / limit |
|---|---|---|
| Tree and dependency inventory | All source files compared against build 39; no original excluded | Full inventory coverage, not semantics of every line |
| Kext / USB lifecycle | Checked `start`, `stop`, provider close, retentions and build 39 tests | Earlier fix preserved; physical unplug pending |
| USB ID, families and firmware | Profile and catalog suite, resources and upstream preserved | Profiles unchanged; hardware validation pending |
| Diagnostics / CPU and heap | Eliminated `OSNumber` writes for unchanged values during status and calibration | Host tests passed; production savings unmeasured |
| WPA2, packets, RF, net80211 | Protocol/isolation tests and host memory sanitizers | Portable paths tested; actual kernel not instrumented |
| Menu/AppKit | Auto-reconnect and startup options with immediate visual state without rebuilding menu during tracking; native resources retained | AppKit tests only analyzed in source; actual appearance not photographed |
| Threads/asynchronous state | Existing lifecycle, state machine, timeout and epoch tests | Host tests passed; macOS race detection pending |
| App/helper, IPC and credentials | Existing code and contracts preserved; host regression test verification | Actual XPC/Keychain flow pending |
| Installation/packaging | `.pkg` policy tests, shell syntax and fail-closed paths | Creating `.pkg` and installing on macOS pending |
| Mandatory ad-hoc signing | Inspection of `app/signing.sh`, `app/package.command`, `app/build.command`, `build.command` and policy tests | Requirement preserved; actual `codesign` check pending |
| Build and infrastructure | Python discovered before first two tests; deterministic Xcode project generator | CMake tests executed on Linux host |

## Confirmed problems and fixes

1. **Tests silently not run**: `find_package(Python3 ...)` was called after two CTest entries, leaving them without a command. Moved to the beginning of `BUILD_TESTING`.
2. **Repeated diagnostic writes in IORegistry**: status publishers and `chooseRateControl` created new `OSNumber` objects for identical 64-bit values. Added `publishDiagnosticCounter` and the testable pure decision `diagnosticCounterNeedsWrite`; all published values, types and names remain unchanged. A counter is written only when absent, the wrong width or a different value.
3. **Menu visually stale during click**: auto-reconnect and login startup toggles now update `NSMenuItem.state` immediately. Login reflects **macOS-confirmed state**, including pending approval, not just the request.
4. **Test simulators out of sync with the driver**: three host environments extracting the actual method lacked the new publication API. They were updated without reducing existing checks.
5. **Stale version log**: updated textual message from 0.13.14 to 0.13.19 without modifying the protocol.

## Tests executed

- CMake Debug with `-DRTW_ENABLE_SANITIZERS=ON`, `RTW_HOST_SANITIZERS=address,undefined` (AddressSanitizer / UBSan).
- Suite **87/87 CTest groups passed**, including ad-hoc signing (tested with a simulated utility), WPA, USB, firmware, menus/portable state, teardown and ID matrix. This does not replace Mac process and kernel instrumentation.
- Publisher regression: 100.000 counter steps and an integrated test confirming 0 new writes during stable recalibration and exactly one write when a metric changes.
- Python AST, `bash -n` and Property List parsing of every respective package file, without errors; deterministic Xcode project generator.
- All new SHA-256 manifest records must match; the manifest itself is excluded from its list because self-hashing is impossible.

## Ad-hoc signing (mandatory)

`rtw_require_adhoc_identity` rejects identities other than `-` and notarization profiles. `rtw_sign_app` signs the helper first and then the bundle, checking helper, executable and app; `rtw_sign_kext` signs and verifies the extension. Build and packaging scripts abort on failures. Policy tests check order, `--sign -`, signing failures and rejection of non-ad-hoc signatures.

**Security notice:** ad-hoc signing differs from Developer ID and notarization. It **does not guarantee** kext loading under default macOS security settings. The supplied ZIP is **source**, not signed macOS binaries. No security setting was changed to bypass system restrictions.

## Changes and preservation

No changes to firmware, binary data, `vendor/`, USB catalogs, credentials, networking/transmission, IPC architecture, graphical resources or user preferences. The only build numbers changed are kext 39 → 40 and app 33 → 34; semantic versions were preserved.

## Remaining mandatory validation on macOS Tahoe

1. Build kext/app in Xcode and check ad-hoc signing with `codesign -dv --verbose=4` and `codesign --verify --strict`.
2. Measure CPU, heap and physical memory with Instruments (Time Profiler, Allocations/Leaks) during idle, scan, association and sustained traffic.
3. Validate light/dark theme, open menu, HiDPI monitor and login approval states using actual AppKit.
4. Test removal/connection of both adapters, `ioreg` state, pending USB transfers, sleep/wake and USB port changes; test repetition to investigate retentions.
5. Validate installed `.pkg` on a test machine, installer/service and compatibility with owner-defined security policies.

**Global absence of bugs, leaks or unexercised regressions has not been proven.** Corrected source, tests and inventory make the change scope traceable.


---

## History: `docs/AUDITORIA-RADIO-BUILD41.md` (SHA-256 `5946fd9a6df3d53b0a35f1a164b2e6ac109cd389dac51fc914f1f5ef3dafbe8a`)

# Technical report — bands, speeds and USB matrix (build 41 / app 37)

**Date:** 23/10/2026. **Base:** complete TahoeRealtekWiFi v0.13.19 source (kext build 40) / app v0.2.13 (build 36). **Result of this cycle:** kext build 41, app build 37; no new operational Wi-Fi station enabled.

## Technical result, without extrapolation

This package **does NOT deliver functional 5 GHz for all adapters or maximum speed across all families**. Doing so requires implementing and validating USB initialization, firmware, RF/PHY radio, eFuse, power calibration, regulatory rules, channels, DFS, MIMO/NSS, HT/VHT, association, security and RX/TX in each backend. None of these sequences can be inferred from a USB ID or rate table. **Only nominal family capabilities, passive geometry inspection and app metadata** were safely extended in this revision.

It is also impossible to enable 5 GHz on 2.4 GHz-only hardware such as RTL8192EU and RTL8188EU. The rate indicated as *PHY ceiling* is not actual TCP/UDP throughput or proof that an adapter from a particular brand has all of the family's RF chains.

- 270 USB VID:PID listed, without new IDs or exclusions.
- 227 profiles with USB identity catalogued as verified; **43 unverified** remain ineligible for new binding.
- 122 profiles associated with the four existing station backends (RTL8192CU/8188EU/8192EU/8814AU), subject to additional runtime checking.
- **0** profiles with operational 5 GHz proven by the macOS backend. No HT/VHT mode was arbitrarily unlocked.
- 13 families have sufficient nominal descriptions for **passive** channel preflight. RTL8814BU remains unknown/blocked.

## Audited matrix by family

`VID:PID` means a catalog pair, **not** an individually tested adapter. The family's PHY limit assumes a reference design and ideal conditions; `0` in the driver column means the station backend does not yet exist.

| Family | IDs / verified | IDs with station | Nominal family bands | Station on macOS | Family / driver PHY ceiling |
|---|---:|---:|---|---|---|
| RTL8192CU | 92 / 86 | 86 | 2.4 GHz | 2.4 GHz | 300 / 54 Mbps |
| RTL8188EU | 18 / 13 | 13 | 2.4 GHz | 2.4 GHz | 150 / 54 Mbps |
| RTL8192EU | 10 / 9 | 9 | 2.4 GHz | 2.4 GHz | 300 / 300 Mbps |
| RTL8192DU | 5 / 5 | 0 | 2.4 and 5 GHz | not implemented | 300 / 0 Mbps |
| RTL8192FU | 4 / 4 | 0 | 2.4 GHz | not implemented | 300 / 0 Mbps |
| RTL8710BU | 2 / 2 | 0 | 2.4 GHz | not implemented | 150 / 0 Mbps |
| RTL8723BU | 3 / 2 | 0 | 2.4 GHz | not implemented | 150 / 0 Mbps |
| RTL8812AU | 37 / 34 | 0 | 2.4 and 5 GHz | not implemented | 867 / 0 Mbps |
| RTL8814AU | 17 / 14 | 14 | 2.4 and 5 GHz | 2.4 GHz | 1300 / 54 Mbps |
| RTL8814BU | 1 / 0 | 0 | undetermined | not implemented | undetermined / 0 |
| RTL8821AU | 36 / 22 | 0 | 2.4 and 5 GHz | not implemented | 433 / 0 Mbps |
| RTL8821CU | 13 / 5 | 0 | 2.4 and 5 GHz | not implemented | 433 / 0 Mbps |
| RTL8822BU | 30 / 29 | 0 | 2.4 and 5 GHz | not implemented | 867 / 0 Mbps |
| RTL8822CU | 2 / 2 | 0 | 2.4 and 5 GHz | not implemented | 867 / 0 Mbps |

Note: RTL8192EU has an HT20/HT40 path with up to two spatial streams **when negotiated** (300 Mbps PHY is a theoretical ceiling); RTL8192CU/8188EU and RTL8814AU still have a legacy station limit of 54 Mbps PHY in this source. RTL8814AU is nominally dual-band 802.11ac, but its station path is still legacy 2.4 GHz only. Nothing in this revision changes these limits.

## Files and specific fixes

1. `include/rtw/USBWiFiProfiles.h` and `src/USBWiFiProfiles.c`: shared type and functions for family capability versus implemented capability; identity checked **by immutable catalog pointer** before reporting station capability. Explicit C/C++ compatible declaration to avoid breaking compilers with `-Werror`.
2. `src/FamilyRadioPolicy.c` and `include/rtw/FamilyRadioPolicy.h`: validation **without hardware writes**, rejecting unspecified families, bands, widths and channels; no connection or transmission state enabled. Retains fail-closed policy.
3. `src/FamilyBackend.c` and `tests/family_backend_tests.cpp`: align passive preflight indicators for the 13 families with the same radio catalog. These flags do not activate backends or USB adapters.
4. `tools/rtw-backends.cpp`: new `--adapters-json` lists all 270 IDs, verified identity, band/width/NSS and nominal and station PHY ceilings. `--json` reflects the same distinction per family.
5. `app/AdapterDiagnostics.mm` and `app/AdapterSupport.hpp`: clearer catalog/diagnostic reports distinguishing nominal from implemented; RTL8192EU correctly described as HT20/HT40. No change to icons, appearance, colors or connection behavior.
6. `tests/radio_capabilities_tests.cpp` and `tests/radio_cli_contract_tests.py`: validation of all 14 families, all 270 IDs and assurance that no nominal 5 GHz capability is incorrectly exposed as operational on macOS.
7. `docs/USB-PHY-CAPABILITIES.csv` and `.json`: complete per-VID:PID matrix. `README.md` and `CHANGELOG.md` updated. `CMakeLists.txt` updated to register both new tests.
8. `kext/Info.plist` and `app/Info.plist`: **only** build numbers updated, retaining functional versions and bundle IDs. Xcode project regenerated from the existing script.

## What remains to fully meet the requirement

**2.4 GHz-only 802.11n families:** RTL8192CU, RTL8188EU, RTL8192EU, RTL8192FU, RTL8710BU and RTL8723BU. They must operate only in this band; reaching their ceiling requires completing/validating HT20/40, rate control, aggregation and eFuse calibration, respecting the number of RF chains in each adapter.

**Dual-band 802.11n family:** RTL8192DU. Requires a macOS station backend, firmware, 5 GHz/DFS validation and actual HT20/40.

**Dual-band 802.11ac families:** RTL8812AU, RTL8814AU, RTL8821AU, RTL8821CU, RTL8822BU and RTL8822CU. Require backends with 5 GHz RF initialization, per-board parameters, eFuse, calibration, 80 MHz/VHT, NSS/SGI negotiation, WPA and robust USB transport. RTL8814AU already has a **partial** station backend; the others do not yet.

**RTL8814BU:** catalog with unverified identity; do not enable or advertise band/PHY until the specific silicon and actual endpoints are confirmed.

Next, for each family, implement in isolated, testable stages: (a) identification + firmware + controlled boot; (b) RF/eFuse calibration and permitted channels; (c) scan and WPA association; (d) TX/RX + HT/VHT and regulatory calibration; (e) actual 2.4/5 GHz measurements of width, MCS, NSS, throughput and hot-unplug under macOS Tahoe. This requires logs and representative physical adapters; it must not be declared ready from offline tests.

## Tests and signing policy

- Portable tests with AddressSanitizer/UndefinedBehaviorSanitizer: 90 CTest groups run in three blocks. A C/C++ compile error (`-Wmissing-field-initializers`, third block) was identified and fixed before a complete repeat. **90/90 tests passed on the final tree, in three blocks of 30**, with sanitizers enabled.
- Matrices of all USB identities and passive channel scenarios: passed local tests. No new 5 GHz backend enabled.
- **Mandatory, fail-closed ad-hoc** signing: original scripts retained. Actual signing and AppKit/IOKit compilation require macOS/Xcode; they cannot be verified on this Linux host.
- Tests with actual dongles, 5 GHz routers, speed results, DFS, RF or power certification **were not performed**. The package contains no signed macOS binaries, only source code.

## Local verification by the macOS developer

```
./build.command --kext
kmutil showloaded | grep -i TahoeRealtekWiFi
```

Loaded build `41` confirms only version, not radio support. Before considering a new chipset operational, physical results are required for scan/association, HT/VHT width, MCS/NSS, measured throughput and clean USB detachment. `codesign` output must be checked on every bundle compiled through the project procedure.

## Public technical references

- Realtek, RTL8192EU — official specification, 802.11b/g/n 2.4 GHz: https://www.realtek.com/Product/Index?cate_id=194&id=601&menu_id=291
- Realtek, RTL8814AU — official specification, 802.11ac/abgn, three spatial streams and USB 3.0: https://www.realtek.com/Product/Index?cate_id=194&id=574
- Linux Wireless, USB/rtw88/rtl8xxxu driver matrix: https://wireless.docs.kernel.org/en/latest/en/users/drivers/rtl819x.html
- Linux kernel Kconfig for rtl8xxxu, with documented restrictions: https://kernel.googlesource.com/pub/scm/linux/kernel/git/rt/linux-rt-devel/+/master/drivers/net/wireless/realtek/rtl8xxxu/Kconfig


---

## History: `docs/5GHZ-RX-IMPLEMENTATION-STATUS.md` (SHA-256 `16d480662cd43947983ca715e4118398520698f3fc140cafbcf8040aecb7ea5c`)

# 5 GHz — incremental implementation and actual coverage

**Date:** 2026-10-24  
**Verified base:** TahoeRealtekWiFi v0.13.19, kext build 41 / app v0.2.13 build 37.  
**Versions unchanged:** this revision modifies only portable offline RX/scan and passive validation components; it includes no new binary, firmware or kext transmission integration.

## Operational conclusion

**There is no new operational 5 GHz in this delivery.** This is actual progress in 5 GHz data/scan validation, NOT a complete implementation of RF/PHY, association and TX/RX for every family. No adapter should be advertised as functional dual-band merely because it is in the catalog. All channel checks remain `passive_only` and do not constitute regulatory or transmission authorization.

## Dual-band hardware families present (7)

| Family | 5 GHz hardware (reference) | Kext: 5 GHz association/TX | RX/scan developed here |
|---|---|---|---|
| RTL8192DU | Yes | **No** | Passive identity and geometry preflight |
| RTL8812AU | Yes | **No** | Offline Jaguar decoder, HT/VHT beacons, FCS |
| RTL8814AU | Yes | **No** | Passive preflight; current station backend remains 2.4 GHz |
| RTL8821AU | Yes | **No** | Offline Jaguar decoder, HT/VHT beacons, FCS |
| RTL8821CU | Yes | **No** | Passive identity and geometry preflight |
| RTL8822BU | Yes | **No** | Passive identity and geometry preflight |
| RTL8822CU | Yes | **No** | Passive identity and geometry preflight |

RTL8814BU identification is pending and no capability is inferred. 2.4 GHz-only families must not and do not gain 5 GHz transmission.

## New and changed code

1. `src/RTL8812AUStation.c` and `include/rtw/RTL8812AUStation.h`: offline HT Operation and VHT Operation parsing (20/40/80 MHz), primary/center channel, geometry mismatches, invalid states and FCS, without enabling TX. New `operating_width_mhz` and `center_channel` results reflect **AP advertisement**, not speed negotiation.
2. `src/RTL8812AUPassive.c` and `include/rtw/RTL8812AUPassive.h`: `rtw_r12au_passive_on_channel` accepts a physically known channel and RTL8812AU/RTL8821AU formats, validates input and two passes over USB aggregates; channel 6 wrapper retained for compatibility.
3. `src/FamilyRadioPolicy.c` and `include/rtw/FamilyRadioPolicy.h`: `rtw_family_radio_prepare_profile` requires an actual pointer and verified catalog identity before returning **passive-only** geometry, covering all 270 IDs without authorizing any nonexistent radio.
4. `tools/rtw-acusb-decode.cpp`: adds `advertised_width`, `center`, `ht`, `vht` **after** all old fields without breaking the output contract.
5. `tests/radio_5ghz_rx_tests.cpp` (new) and `CMakeLists.txt`: regressions per family and USB ID, HT40/VHT80, channel mismatch, passive DFS inspection, FCS, malformed USB buffers, single/dual-stream chips.
6. `SOURCE-MANIFEST.sha256`: manifest updated for the final tree (excluding the manifest file itself).

**No** change to firmware, `kext/`, `vendor/`, `app/`, signing, hardware power/tuning or installation. The USB fix and mandatory ad-hoc signing remain preserved.

## Validation performed

- Input build 41 ZIP: distributor SHA-256 validated, integrity test without error.
- Linux, `-fsanitize=undefined`: **89/89** fast regression tests + **1/1** 270-profile matrix + **1/1** packaging; 91/91 groups passed in three runs.
- Linux, `-fsanitize=address,undefined`: **3/3** receive/scan groups (including the new test) passed.
- Previous CLI and channel 6 parser: compatibility tests passed.
- Not performed: native Xcode build, actual binary signing, actual 5 GHz with adapters/router, TX power, DFS/CAC, firmware, WPA2 association, MCS/VHT on hardware, throughput or 5 GHz hot-unplug.

**Meeting the original requirement** still requires implementing and validating (per family) USB driver + firmware, board/eFuse, 5 GHz RF and power/RFE calibration, region-permitted channels, DFS where applicable, actual RX/TX, HT/VHT/MCS/NSS negotiation and security. In particular, RTL8814AU still limits PHY to channels 1–11/20 MHz and RTL8812AU/RTL8821AU have no kext-integrated station backend. This cannot be replaced by channel advertisement or hardware-free testing.

## Verification on macOS

Use the project flow to build natively and verify ad-hoc signatures without installing unaudited third-party binaries. The decoder works only on a capture previously obtained with authorization and passively:

```sh
rtw-acusb-decode rtl8812au /caminho/captura-usb.bin --channel 36 --fcs
```

`--channel 36` requires evidence of actual physical tuning; it must never be inferred from beacon content. This command **does not enable** the adapter in 5 GHz.


---

## History: `docs/5GHZ-MULTIFAMILY-RX-STAGE2.md` (SHA-256 `0d5890d5f1967875f409f4aebd9d95154641c184d3b60a29462968ac4e7dfc2d`)

# 5 GHz — cross-family receive stage 2 (NOT a functional Wi-Fi station)

**Date:** 2026-10-24  
**Checked base:** TahoeRealtekWiFi v0.13.19, kext build 41, app v0.2.13 build 37, package `5GHZ-RX-STAGE`.  
**Policy retained:** mandatory, fail-closed ad-hoc signing for app, helper and kext; no new TX/region/DFS enablement.

## Operational warning

**NONE of the seven nominally dual-band families received active 5 GHz station support in this revision.** Recognizing beacons or decoding USB transfers in software does not equate to turning on the radio, selecting a channel, authenticating, associating, transmitting or achieving 802.11ac throughput. No macOS Xcode build or hardware test was performed; there is no basis for promising functional 5 GHz on any of the 270 USB IDs.

This stage adds an ACTUAL but **offline/passive** USB RX aggregate pipeline for six families whose descriptor formats were already in source. The seventh format (RTL8192DU) does not exist in verified form and remains unavailable for safety.

## Source changes

- `src/Family5GHzScan.c`, `include/rtw/Family5GHzScan.h`: `rtw_family_5g_scan_bulk()` identifies immutable/verified catalog profiles, requires an externally tuned channel and explicit FCS metadata, maps RTL8812AU/8821AU (Jaguar), RTL8814AU (8814A) and RTL8821CU/8822BU/8822CU (rtw88) descriptors, and validates the entire aggregate before the first callback. Distinguishes C2H, CRC, ICV, incompatible beacons and wrong channel. Contains no USB commands, `IOMalloc`, firmware changes or TX operations.
- `tests/family_5ghz_scan_tests.cpp`: reception regressions for six families, HT/VHT80 beacon, wrong channel, altered FCS, CRC and atomic discard of a corrupt aggregate. Checks all 270 profiles, including unverified identities and a fabricated profile. **934 assertions passed** in a local run.
- `docs/5GHZ-USB-RX-COVERAGE.csv`: per-VID:PID status distinguishing nominal 5 GHz support from **offline** descriptor reading and operational kext support.
- `CMakeLists.txt`, `TahoeRealtekWiFi.xcodeproj/project.pbxproj`: new source added to all relevant build configurations; Xcode regenerated through the original deterministic `scripts/generate-kext-project.py` to prevent silent omission.
- `README.md`, `SOURCE-MANIFEST.sha256` and this report updated.

## Coverage of the 270 USB IDs

| Nominally dual-band family | Catalogued IDs | IDs with verified identity + offline 5 GHz parser | 5 GHz station in kext |
|---|---:|---:|---:|
| RTL8192DU | 5 | 0 | NO |
| RTL8812AU | 37 | 34 | NO |
| RTL8814AU | 17 | 14 | NO |
| RTL8821AU | 36 | 22 | NO |
| RTL8821CU | 13 | 5 | NO |
| RTL8822BU | 30 | 29 | NO |
| RTL8822CU | 2 | 2 | NO |
| **Nominal dual-band total** | **140** | **106** | **0** |

The other 130 catalog IDs have no nominal 5 GHz capability proven in this matrix. `RTL8814BU` remains without verified identification, without silicon extrapolation. The 106 profiles do not represent 106 hardware-tested adapters; only decoding availability **without radio access**.

## Completed checks

- Original ZIP and its SHA-256 checked before the change.
- 90 fast tests + app package test + USB matrix: **92/92 portable test groups passed** (Linux, `-fsanitize=undefined`, fast set and two long tests run separately).
- New and preexisting reception tests with `-fsanitize=address,undefined`: **3/3 passed**.
- New C syntax with Clang `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`: passed.
- Xcode coverage and kext linkage test: passed after adding the new C file to the project.
- Original `kext/`, `vendor/`, `app/`, firmware and signing scripts remain byte-for-byte identical to the base (except the Xcode project compilation list).

## Mandatory outstanding work for FUNCTIONAL 5 GHz delivery

1. **RTL8192DU** — correct RX decoder, power-on/eFuse, firmware, 5 GHz RF, station, USB TX/RX and 802.11n validation.
2. **RTL8812AU and RTL8821AU** — complete boot/RF with certified board profiles, macOS association, WPA2, USB TX, 20/40/80 MHz with VHT and signal validation.
3. **RTL8814AU** — implement 5 GHz RF, region/board-specific power tables, 5 GHz calibration and HT/VHT/80 MHz rate path; in this source `set8814Channel` still rejects channels above 11 and active PHY remains 2.4 GHz / 20 MHz.
4. **RTL8821CU, RTL8822BU, RTL8822CU** — port/integrate their RF/PHY/firmware and station drivers specifically for each silicon, without treating identical descriptors as proof of equivalent calibration.
5. **All** — local regulatory rules, DFS/CAC where applicable, WPA, stability, USB disconnection, actual Mac tests for each chipset and actual width/MCS/NSS/throughput measurement.

**Editing only a channel table, capability string or PHY limit to pretend this implementation exists is unsafe.** Consequently no family or ID was reclassified as functional 5 GHz.


---

## History: `docs/USB-TEARDOWN-FIX.md` (SHA-256 `68c8f7af46b3c47cbd9235b753252492912d635d937ac586127e3f30d1afa231`)

# USB hot-unplug lifecycle correction — driver 0.13.19 build 39

## Reproduced evidence (user-reported; build 38)

- `kmutil showloaded` reports `org.research.TahoeRealtekWiFi (38)`.
- With both USB adapters unplugged, three `RTWUSBController` objects remain
  `!registered`, `!matched`, `inactive`, `busy 1`, with retain counts 9/6/9.
- Two `802.11n NIC` USB nodes remain `inactive`, `busy 2`.
- The preceding `dmesg` query had no matching driver lines, and the unified
  log showed temporary kext history lookup errors but no disconnect traceback.
- Apple IOKit `IOService::terminate` semantics defer calling a client's
  `stop()` until that client closes an opened provider. The original code
  opened `usb_` in `start()`, but only closed it in `releasePlatform()` after
  `stop()`. This is a real circular lifecycle dependency. Source:
  https://github.com/apple/darwin-xnu/blob/main/iokit/IOKit/IOService.h

The exact contribution of any outstanding asynchronous USB callback to the
observed retain counts **cannot be determined** from these logs alone.

## Narrow source change

- `kext/RTWUSBController.cpp`: the `Terminated` gate operation now sets
  `closing_` and `present_` appropriately **before** quiescing and aborting
  transfer submissions. Both termination and request-to-close events now
  safely close the opened provider **after the gate is released**.
- `closeUSBProvider()` atomically claims `usbOpened_` under `lifecycleLock_`
  and retains the provider across `close(this)`. In `message()`, an explicit
  controller retain protects against immediate `stop()` / teardown.
- `releasePlatform()` continues to own the fallback close for early start
  failures; `usbOpened_` is consumed only once. Pending DMA buffers are still
  protected by the existing `retainedForPending_` deferred cleanup.
- Source build is identifiable as **39**, not 38. The app remains **0.2.13
  build 33**. No USB catalog, driver personalities, firmware, security,
  networking policies, or app behavior were changed.

## Executed portable validation (Linux)

- Original 528-file source manifest: **528/528 valid before edits**.
- CMake Debug portable build with UBSan: **built successfully**.
- **86/86** portable CTest groups passed across complementary runs; three
  longer tests (`rtw-app-pkg-tests`, `rtw-usb-wifi-catalog-tests` and
  `rtw-usb-wifi-matrix-tests`) were re-run and passed individually after
  concurrent test runs exceeded the environment's execution time limit.
- `rtw-usb-termination-tests`: compiles the actual `message()` and
  `closeUSBProvider()` methods using host IOKit doubles; validates
  synchronous `stop()` during `close()`, once-only provider close, no
  close under a held command gate, failure handling, concurrent close
  attempts, and 10,000 iterations.
- `rtw-deferred-cleanup-tests`: 10,000 iterations of safe deferred teardown
  with pending USB callbacks.

**Not executed:** native Xcode/kernel SDK build, actual kext signature,
installation, live USB hot-unplug/replug, RF or network hardware tests.
The release contains the **full source**, not a prebuilt kext.

## Mac verification (requires installing a separately built/signed build 39)

1. Build natively on the intended Mac with `./build.command --kext`.
   Follow the existing signing/installation policy; no SIP/AMFI changes are
   performed by the source bundle. Confirm build and signing results.
2. Reboot to clear prior build 38 kernel objects before beginning a clean
   hot-unplug test; do not mistake old retained objects for build 39 failures.
3. Check the loaded version with
   `kmutil showloaded | grep -i 'org.research.TahoeRealtekWiFi'`;
   the result must show `(39)`.
4. With each dongle attached, record `ioreg -p IOService -r -c
   RTWUSBController -w0 -l`; physically unplug each device (no software
   eject required), then run the same command after the USB stack settles.
   No `RTWUSBController` instances for removed adapters should remain.
5. If an inactive/busy object persists, preserve the full `ioreg` output,
   an exact unplug timestamp, and kernel logs. Do not force-release an
   outstanding DMA transfer without proving its USB callback is complete.

The physical outcome is deliberately **unverified** until performed on
macOS with this source compiled, signed and loaded.


---

Legacy inventory file `docs/source-inventory.csv`: SHA-256 `d778f88e41fd94d9801102d8f99f55ade5125ac906705b17dc6425d153fbb8c3`; replaced by the current matrix.

---

## 2026-10-24 — Implementation checkpoint: 5 GHz management across seven families

**Input:** `TahoeRealtekWiFi-Source-Fixed.zip`, SHA-256
`d901747ce73a9f3e466b9d3a52975b5dda7185130fe7f0069df05aeccc8b13a5`.
This is the latest complete source package supplied in the conversation; no
`.git` history is included, so commits/branches cannot be verified here.

**New, implementation-level work:**

- `include/rtw/Family5GHzStation.h` and `src/Family5GHzStation.c` introduce a
  shared IEEE 802.11 management join session, using the existing
  `rtw_r12au_join_*` authentication and association builders. The new API
  performs a binding between immutable, verified USB profiles and observed
  5-GHz APs. It accepts all seven hardware dual-band families individually.
- The validator rejects copied/fabricated/unverified USB identities; rejects
  2.4-GHz-only chipsets, mismatch between tuned channel and AP, bad 20/40/80
  MHz primary/center geometry, unsupported 80-MHz requests on RTL8192DU,
  contradictory HT/VHT announcements, invalid security and PMF-required APs.
- `src/FamilyBackend.c` now advertises **offline management support**, not
  operational station support, for the five additional dual-band families.
  `RTW_BACKEND_CAP_KEXT_STATION`, station 5-GHz bands and TX authorization are
  **unchanged**. RTL8192DU has no trusted USB RX descriptor; only offline
  IEEE 802.11 management preparation is available for that chip.
- `tests/family_5ghz_station_tests.cpp` executes all seven family management
  transitions, produces actual 802.11 auth and association request bytes and
  validates synthetic AP responses through to `WPA2_KEYS_REQUIRED`. A second
  test starts from the existing USB bulk-decoder callback for six families.
  It also tests all 270 catalogued USB identities; **1,100 assertions**.
- Added the portable C source to CMake and to the x86_64 Xcode kext source
  phase using `scripts/generate-kext-project.py`, without editing signing
  scripts or the existing interface/UI.

**Real validation:** CMake portable build succeeded; 91/91 non-long CTest
cases and 2/2 long cases passed; the four focused tests ran with
ASan+UBSan; final 5-GHz management integration test passed with
ASan+UBSan. New C implementation passed Clang with
`-std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`.
Actual macOS Xcode compilation, ad-hoc `codesign`, physical radio tests,
iperf and TX-power calibration **were not performed**.

**Explicit non-claims:** No RTL8192DU/RTL8812AU/RTL8814AU/RTL8821AU/
RTL8821CU/RTL8822BU/RTL8822CU adapter is demonstrated to transmit, associate
or connect on 5 GHz in this build. Negotiation state is not proof of keys
installed or an authorized RF configuration. DFS channels remain tagged
passive-only; no DFS-CAC or regulatory transmit permission is implemented.

**Checkpoint / exact next implementation point:**

1. `kext/RTW8814Backend.inc`: map verified 5-GHz `ieee80211_channel`
   selection to RF/PHY. Current `set8814Channel()` still uses 20-MHz/2.4-GHz;
   do not remove this gate before RFE board, power tables and IQK support.
2. `src/RTL8814AUPHY.c` and `src/RTL8814AUAlgorithms.inc`: port and trace
   verified per-board 5-GHz RF/PHY configuration, power regulatory tables,
   correct HT/VHT TX rates and calibration, then test with physical hardware.
3. `src/RTL8812AUStation.c` / `src/RTL8812AURadio.c`: integrate management
   frames with runtime USB TX/RX and WPA2 key installation for 8812AU/8821AU.
4. RTL8821CU/8822BU/8822CU: implement individual runtime firmware/PHY and
   TX/RX, never assuming common RX descriptors imply identical RF hardware.
5. RTL8192DU: establish the USB RX codec and the unique PHY/firmware path.
6. All: use macOS regulatory/channel control (including DFS-CAC where needed),
   verify ad-hoc signing, test link/throughput/disconnect on each chip in 5 GHz.

**Release status:** source-only development checkpoint. Not a 5-GHz-ready
or Release-certified driver. No additional unrelated functionality was changed.


---

## 2026-10-24 — Stage 4: physical 5 GHz RX-only tuning on RTL8814AU (kext build 42)

**Confirmed base:** Stage 3, SHA-256
`2d1c3fb10274e4328cb5cb005108b2aae0451f27c69f88f77975f46a60b6097f`,
550 files; no `.git` directory to confirm commits. This delivery's tree
starts from the immutable base without replacing firmware or third-party code.

### Inspection and cause

`src/RTL8814AUPHY.c::rtw_8814au_phy_set_channel` accepts only 1–11/20 MHz and
applies **2.4 GHz-only** power tables. The scanner in backend
`kext/RTW8814Backend.inc` did not include 5 GHz channels. These paths,
RF helper functions in `RTL8814AUAlgorithms.inc`,
`RTL8814AURegisters.inc` registers, net80211 transitions, the calibration
interface and RFE 0/1 scheme were examined. Band/channel switching was
compared against BSD-3-Clause `rtw8814a.c` in the Linux `rtw88` driver, commit
documented in the project `0e135910deec43ffe97eca0b6af1f8f847285b59`.

### Implementation performed — do not confuse with functional association

- `rtw_8814au_phy_set_passive_5g_channel` programs all four RF paths,
  RFE0/RFE1-specific pinmux, CCK/OFDM band, CLKTRK, AGC and channel for
  **36/40/44/48 at 20 MHz**, with actual register transfers
  performed through existing PHY I/O callbacks.
- `REG_TXPAUSE=0x3f` is applied before reprogramming RF and remains applied;
  `phy_initialized=0` and `channel_set=false` block station calibration and
  5 GHz TX. There are no writes to the 2G TX power tables.
- The controller registers only `IEEE80211_CHAN_PASSIVE` 5 GHz channels;
  active scan skips these channels. `state8814` calls the new path
  exclusively in SCAN state and returns `ENOTSUP` for AUTH/ASSOC/RUN.
- Returning to a 2.4 GHz channel requires `set8814Channel` and actual calibration
  before resuming normal scan/station operation.
- Kext `CFBundleVersion`/Xcode project advanced from 41 to **42**, app
  remains **0.2.13 build 38**. No ad-hoc policy change.

### Regressions executed

- PHY source + fake USB register bus under ASan/UBSan: RFE 0/1, four paths,
  4 permitted channels, invariant TXPAUSE, absence of 2G TX indices,
  rejection of DFS channels and 40/80 MHz widths, injected I/O failure,
  recovery and 2.4 GHz recalibration: **passed**.
- Actual state machine extracted from `RTW8814Backend.inc`, with isolated
  boundaries, under ASan/UBSan: 5 GHz SCAN entry, AUTH refused and return to
  2.4 GHz with calibration: **passed**.
- Portable CMake build: **passed**.
- CTest: **91/91** fast tests completed and **2/2** long tests completed
  separately (USB matrix and app package), total **93/93 groups passed**.
  A combined run exceeded available time during the long test;
  it was repeated and completed successfully, not counted from the interrupted run.
- Original script regenerated the Xcode project with **82 units** and the only
  generated `.pbxproj` difference was `MODULE_VERSION=41 -> 42`.
- **Not performed:** native macOS Tahoe build, actual codesign of app/helper/kext,
  hardware 5 GHz scan, radio calibration/DFS/Tx power/iperf.

### Release status and next checkpoint

**Not ready for Release as a 5 GHz driver.** This is a **5 GHz RX-only**
implementation, not yet physically validated. Beacon reception may
fail because of insufficient RX/RF calibration on actual adapters. TX/association,
HT40/VHT80 width or 5 GHz on the other six families were not enabled.
The common Stage3 API for seven families remains preserved but does not
constitute an operational connection.

Exact next point: `src/RTL8814AUPHY.c::power_tables` still explicitly rejects
all `r->band != 0` rows and calculates only 2 GHz limits;
`RTW8814Backend.inc::set8814Channel` continues blocking 5 GHz TX. Per-RFE
implementation and validation are needed for 5 GHz limits, TX tables,
IQK/LOK, 40/80 MHz primary/center selection, H2C/TX/RX, WPA2 and regulation,
using a physical adapter. The other six families need their own backends
(RTL8192DU, RTL8812AU, RTL8821AU, RTL8821CU, RTL8822BU, RTL8822CU).

This stage's analysis was **direct but partial**, focused on the functions
above; the matrix does not claim complete inspection of every file/line.


---

## 24/10/2026 — Additional audit: 5 GHz aliasing, kext build 43 / app build 38

**Verified reference:** input ZIP `TahoeRealtekWiFi-Source-Full-Build42.zip`, SHA-256 `62f8037364eb66163db9f0fabdd8cd6b447d2544cf561cb7b6347c6e239635b8`; 550 files in the input package, original manifest verified without mismatches. There is no `.git` in the package, so commits/branches are unknown. The original package remained immutable. A new macOS build is unavailable in this environment.

### R1 — Direct semantic inspection and diagnosis

The concrete logic in `src/Family5GHzStation.c`, `src/FamilyRadioPolicy.c` and `src/RTL8812AUStation.c` was examined: argument validation, USB identity, AP state, channel geometry, writes to output objects and borrowed inputs. Partial examination of `kext/RTWUSBController.cpp` (teardown/refs/callbacks), `kext/RTW8814Backend.inc` (passive scan, SCAN/AUTH/ASSOC/RUN transitions), `src/RTL8814AUPHY.c` (TXPAUSE, return to 2.4 GHz), `app/DriverConnection.mm` and `app/Transport.cpp` (asynchronous/XPC/IPC responses), `build.command`, `app/signing.sh`, CMake rules and Xcode generation scripts. Reading does not equate to validating every branch. No additional reproduced defects were found in these teardown/GUI excerpts during this round.

### R2 — Three problems reproduced before fixing

| Defect | Code and root cause | Proven effect with overlapping input | Minimal fix |
|---|---|---|---|
| S5-01 | `rtw_family_5g_join_prepare`, `src/Family5GHzStation.c`: `rtw_family_5g_join_reset(out)` on error before checking overlap with `profile`, `ap`, `station_mac`. | Incorrect destruction of bytes of input declared read-only when the caller reuses the same region. | Reject overlap before any write; preserve previous behavior in every valid and invalid nonaliasing case. |
| S5-02 | `rtw_family_radio_prepare_profile`, `src/FamilyRadioPolicy.c`: `memset(out)` before validating alias with USB identity. | Overlapping copy of the forged profile was erased upon returning `UNSUPPORTED`. | Reject profile/output alias before plan reset; USB identity still requires a catalog pointer. |
| S5-03 | `rtw_r12au_scan_parse`, `src/RTL8812AUStation.c`: output zeroed before checking overlap with RX descriptor or payload. | Beacon or descriptor bytes could be modified by the parser when output and input overlapped. | Explicit overlap rejection before `memset`; other 802.11, RSN and channel rules preserved. |

Reproduction: uncorrected source 42 tree with the three new regression tests; `ctest` returned code 8, **3/3 expected failures**. Corrected tree: all three tests passed with ASan/UBSan. Overlap checks convert pointers to `uintptr_t` and compare distances without adding lengths to addresses, avoiding wraparound in that calculation. No public API or structure layout change.

### R3 — Actual build and regression tests

- Portable CMake Debug built with `RTW_HOST_SANITIZERS=address,undefined` and `RTW_ENABLE_LIBUSB=OFF`: **build completed**.
- **93/93 CTest groups passed in three completed runs**: 91 fast, 1 adapter matrix (270 IDs) and 1 app packaging. Logs are summarized in the single evidence file.
- Three changed sources passed Clang C11 with `-Wall -Wextra -Wconversion -Wshadow -Wpedantic -Werror` (no compilation diagnostics from these three files). `clang --analyze` flagged a preexisting conditional possibility in `FamilyRadioPolicy.c: profile->family`; implementation checks `caps.identity_verified` before access, returning `UNSUPPORTED` for `profile=nullptr`. This warning **was not classified as a proven crash** or fixed speculatively.
- After build update, five targeted tests (3 code, signing and kext personality) passed again.
- **Single patch** `docs/diffs/FIXES.patch` applies specifically to the build 42 ZIP identified above; application was tested on an original copy and all 10 corrected/updated files reproduced the resulting tree byte for byte. The delivered ZIP manifest is regenerated separately and includes the patch.

### Integrity and changes

- Changed 3 implementation files (all related to 5 GHz input/output), 3 test files, `kext/Info.plist`, generated Xcode project, `README.md`, `CHANGELOG.md`, and the four audit/manifest artifacts retained in their unique paths. No original file was removed, and `vendor/`, firmware, `app/` and TX/RF logic remain identical to build 42.
- Kext advanced from build **42 to 43** to distinguish potential compilation of changed sources; app remains build **38**. The `.pbxproj` update matches the plist version, with **82 compilation units**.
- Mandatory ad-hoc signing code (`app/signing.sh`, build/installer scripts) preserved; policy test passed, but actual `codesign` was not performed.
- The `docs/AUDIT-COVERAGE.tsv` matrix retains per-file/function rows and expressly distinguishes reading, inspection and validation; modified file hashes are updated, and self-references do not receive fictitious hashes.

### What was NOT proven — Release status and next checkpoint

**Full semantic coverage not reached; macOS Release not validated.** Native Xcode/macOS Tahoe build, actual ad-hoc signing, light/dark AppKit visual tests, physical USB/hotplug tests, Instruments for leaks/memory/CPU, actual benchmark, active 5 GHz RF, calibration and DFS were not performed. Portable tests do not replace these checks. None of the seven dual-band families has a proven functional 5 GHz station; RTL8814AU retains only experimental passive 5 GHz RX tuning (36/40/44/48, 20 MHz, TX blocked).

**Exact checkpoint for the next audit:** `kext/RTWUSBController.cpp` — provider close after termination, `shutdownPlatform()`, `deferredCleanup()`, races with `IOCommandGate`, callback-held references and actual adapter tests; then `app/Main.mm` — request generation, Keychain cancellation, UI/theme and macOS measurements. For active 5 GHz, implement RF/PHY, power tables, calibration, HT/VHT, firmware, WPA2/TX/RX and regional policy per hardware/board before enabling transmission. Do not declare completion based on simulation or beacon reading.
