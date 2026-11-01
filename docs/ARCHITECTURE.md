# Architecture and invariants

## Application and privileged boundary

`app/Main.mm` is the AppKit menu-bar controller. AppKit state is owned by the main
thread. A serial utility queue performs synchronous driver exchanges; a separate
serial queue performs Keychain work. Both queues drain autoreleased objects per
work item. Polling never performs blocking XPC or interface enumeration directly
on the UI thread.

`DriverConnection.mm` and `PrivilegedService.mm` provide the approved XPC path.
`CodeIdentity.h` retains the existing code-identity checks. `Transport.cpp` also
contains the legacy local-socket transport used by its tests/CLI path; repeated
session creation must release the preceding endpoint before replacing it.

The service is not implicitly installed by launching an arbitrary source binary.
Deployment uses the system Installer and service approval. Password data is not
included in diagnostic logs. Keychain remains the credential store; no application
file stores the administrator password.

## Ownership and cancellation

An adapter selection has a registry ID and generation. `WorkEpoch.hpp` adds an
atomic cancellation generation for application work. Main-thread lifecycle events
invalidate the epoch. A worker checks the epoch before a control operation and its
completion checks again before mutating UI state.

Cancellation does not pretend to interrupt an already running kernel/XPC call.
A radio operation already submitted may complete, but stale completion data must
not update a different adapter or inactive session. Kernel/IPC deadlines remain
responsible for bounding outstanding calls.

Sleep and inactive-session flags are independent. Resuming one must not clear
the other. Timers stop while either suspension flag is set. Termination invalidates
work and schedules connection cleanup without waiting on the UI thread. It does
not send an unintended radio disconnect.

The poll-busy flag, poll token and command-pending flag bound duplicate work.
Credential refreshes coalesce but retain one requested follow-up. The instance
lock descriptor starts at -1 so pre-launch cleanup cannot close standard input.

## Status and signal

`PollPolicy.hpp` determines finite polling intervals and tolerances.
`Model.hpp` handles scan records, compatible AP selection and signal display.
`SignalHistory.hpp` maintains a bounded 128-entry ring for read-only diagnostics.
There is no unbounded per-poll array or continuously growing signal log.

A missing or expired signal is unavailable, not a fake zero-dBm sample. The
previous release's AP-matched valid PHY sample retention and 10-second expiry
remain in the driver. UI signal filtering uses a three-sample median and hysteresis.
Link/disconnection state bypasses the signal filter.

The adapter list and per-interface address lookup have independent cache windows.
A network list read is not an instruction to initiate another radio scan. Scan
commands remain separate and repeated pending commands are coalesced.

Duplicate SSID/security entries prefer a compatible 2.4 GHz AP before comparing
strength; an unsupported 5 GHz AP must not hide a usable weaker one.
The original SSID bytes, not a truncated menu title, are used for joining.

## Driver

`kext/RTWUSBController.cpp` owns USB/controller lifecycle, serialized control,
receive/transmit completion and Ethernet exposure. `RTWUserClient.cpp` validates
the user-client ABI. `RTW8192EUHAL.cpp` bridges the imported rtwn core.

Portable components in `src/` validate USB identities, descriptors, firmware,
OTP/eFuse ranges, receive framing, transmission layout and backend decisions.
`vendor/rtwn` provides radio/core logic. `vendor/itl80211` provides the imported
station/crypto stack and the macOS compatibility layer; its production sources
are selected by `vendor/itl80211/sources.json`.

The work loop and command gate serialize radio/stack state. Deferred cleanup
must retain objects while USB callbacks still own them; freeing outstanding
objects to make a memory counter smaller would create use-after-free risk.
Existing shared-runtime lifetime code must not tear down state still referenced
by another controller. Host tests exercise these ownership decisions, not real
USB callbacks or macOS kernel ABI behavior.

## USB compatibility registry

`data/legacy-usb-adapters.json` is the single catalog. Schema 2 distinguishes
original legacy identities from upstream extension records. The 250 original
`original_matching` dictionaries are digest-pinned; extensions cannot silently
replace or relax them. Each extension needs exact numeric VID:PID evidence
from a named release/commit and one of the four assembled station backends.

`matching_constraints()` is shared by the C-table, Info.plist and CSV generators.
Interface constraints are explicit bit masks, not wildcard sentinel bytes:
0xff is a real descriptor value. C lookup uses binary search over a sorted,
unique immutable table. The full API checks configuration, interface number,
optional revision and each constrained interface descriptor. The older API
cannot confirm class-restricted profiles because it does not receive those
descriptors, so it returns no match. Fabricated/copied profiles cannot activate
a backend.

Kernel discovery passes actual USB descriptors to the full API before opening
pipes. Existing endpoint counts, packet sizes, silicon/eFuse, firmware and
board/RFE validation remain in force. App diagnostics read only allowlisted
USB registry fields and use the same API; missing constrained descriptors
remain unavailable. App observations are not an authorization mechanism.

The extension deployment policy is deliberately conservative: configuration 1,
interface 0 and vendor-specific class. For RTL8814AU, class/subclass/protocol
all equal 0xff, matching the reviewed primary source. A genuine adapter on a
different configuration/interface is not automatically enabled by this policy.

## Presentation

Use native template symbols for status icons and semantic AppKit text/background
colors. Do not force a global light/dark theme. Menus use a minimum width and
bounded single-line labels; original data remains available to the operation.
Report text uses a selectable scroll view with screen-aware dimensions.
Accessibility labels distinguish network/password fields and the status button.

## Build and signing

`app/signing.sh` is the only implementation of signature policy. The app builder,
kext wrapper, generated Xcode phase, package builder and installer verification
share it. Code is signed inside-out and verified strictly. `--deep` is used for
verification, never as a substitute for explicit nested-code signing.

The app builder stages a complete bundle and preserves the previous bundle when
pre-publication checks fail. Shell pipeline handling checks both producer and
log writer. Packaging verifies source and copied payload. Installer hooks refuse
a missing/invalid/non-ad-hoc signature before attempting launch.

`INSTALLER_SIGN_IDENTITY` affects only an optional signed package container.
Ad-hoc code remains development/local code, not Developer ID notarized software.

## RTL8812AU / RTL8821AU offline radio and station staging

`RTL8812AUDataPath.c` decodes/synthesizes RX/TX descriptors only.
`RTL8812AURadio.c` performs allocation-free channel geometry without any
register I/O and returns a passive-only plan; it does not grant regional
regulatory permission or complete DFS channel availability checks.
`RTL8812AUStation.c` consumes 802.11 MPDUs and creates offline management
frames using only source-owned buffers, enforcing strict IE bounds, the
PHY-tuned channel and open/WPA2-PSK/CCMP-only RSN policy.

The station state machine is NOT connected to the kext, physical MAC,
firmware RA, scan offload or key management. Its ASSOCIATED_UNVERIFIED
and KEYS_REQUIRED states must never be interpreted as link, carrier or
controlled-port authorization. No RTL8812AU/RTL8821AU profile is newly
enabled. Native RF power/channel tables and regulatory controls remain
separate, explicitly authorized diagnostic operations for only the
documented RTL8812AU B-cut/RFE3 profile.

`RFReadback.c` checks the RF[0x18] channel using shared Jaguar HSSI and
per-path PI/SI readback, only after `rtw_monitor_verify_channel6` and
explicit lab consent. It saves, restores and verifies HSSI and secondary CCA
state, and fails closed when path A disagrees or restoration fails.
Path B remains informational because reference boards can return zero
despite an active RF path. After each capture or readback failure, the
hardware diagnostic attempts to disable MAC RX/RX DMA in CR via
`rtw_monitor_quiesce_rx_channel6`; this does not fully power down the RF.
Neither component enables kext station attachment or TX.

## Verification boundaries

Portable host shims are confined to tests; they are never production SDK headers.
The protocol harnesses execute selected actual source functions with synthetic
frames and substitute clocks/mbufs/work loops. They do not transmit traffic.
Native tests exercise AppKit and macOS APIs only when run on macOS.
See the audit for remaining visual, profiling, signing and hardware checks.


## Multi-family receive inspection layer (0.13.18)

`src/FamilyRX24.c` uses two documented 24-byte little-endian USB RX descriptor
ABIs: Linux `rtl8xxxu` for 8192FU/8723BU, and Linux `rtw88` for
8821CU/8822BU/8822CU. The shared bounded iterator does not synthesize RX
data, convert it into live network frames or enable a kext station backend.
`src/FamilyBackend.c` dispatches by catalog silicon family and refuses
unknown descriptor ABIs. `src/USBWiFiFirmware.c` checks known legacy
32-byte header signatures, while `src/FamilyRadioPolicy.c` offers an
offline passive-only geometry model, never an RF programming sequence.

`rtw-backends --json` reports all four capabilities independently:
`rx_wire_codec`, `firmware_header_inspector`, `passive_radio_geometry`
and `kext_station_integrated`. Only `kext_station_integrated` corresponds
to existing hardware-station code; no RX-only family was enabled.


## Legacy 8051 firmware stage (0.13.19)

`LegacyFirmwareStage.h/.c` builds on `FamilyBackend` USB descriptor preflight
and `USBWiFiFirmware` family-specific firmware header verification. Its
immutable geometry permits 8192FU/8723BU boot attempts, and 8710BU
transfer planning only. The staging API distinguishes identity,
image, capacity, USB I/O, checksum and startup failures, with strict page
boundaries and no hidden heap allocation. It never calls the macOS kext or
registers new USB personalities.

Only the optional `rtw-legacy-fwdiag` utility (built when real libusb is
available) can issue experimental firmware control writes. Read-only
inspection and dry-run planning are the defaults. The write path requires
explicit opt-in, a claim of the catalog-validated USB interface, no active
kernel driver detach and prior board-specific power-on. Firmware RAM boot
is **not** PHY/RF calibration, TX, WPA2 or a net80211 station implementation.

`rtw-backends` advertises firmware planning and experimental boot as separate
JSON booleans. `kext_station_integrated` remains the only bit describing a
compiled macOS station path; even that bit is not hardware certification.
