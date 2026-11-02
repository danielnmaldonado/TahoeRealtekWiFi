# Tahoe Wi-Fi USB

Project and macOS adaptation: **MaLd0n**.

Source for the kext and menu-bar application for macOS Ventura 13,
Sonoma 14, Sequoia 15 and Tahoe 26 on Intel.
The 0.14.14/build61 tree and app 0.3.11/build53 are in development. Compilation and software tests do not
replace connection and traffic tests on each adapter and board revision.

The driver exposes an Ethernet interface `enX`, following the model used by
itlwm. The macOS-adapted net80211 controls association and WPA2-Personal/CCMP;
the app selects the USB adapter, scans for networks, connects and displays status. The control
protocol does not depend on private AirPort classes.

## English documentation and diagnostics

Project documentation, build messages and diagnostic reports are in English.
Collector revision 3 writes `README-DIAGNOSTICS.txt` and `RESULT.txt`; its
`STATUS.tsv` columns are `file`, `result`, `exit_status` and `command`, with
`failed` and `expected_state` result labels. Capture behavior, privacy masking,
timeouts and numeric counter semantics are preserved. Historical evidence keeps
its original identifiers and captured log text.

## macOS compatibility

The same app, helper and installer require macOS 13.0 at minimum, including the version
recorded in the executables. The packager rejects an app/helper that requires a
later version. Kext61 retains the existing USB/Ethernet code and dependencies.
See the [compatibility matrix and validation limits](docs/MACOS-COMPATIBILITY.md).
This package allows testing on all four systems; hardware validation of each
remains pending. The Tahoe Wi-Fi USB name and identifiers remain
the same to preserve preferences and passwords already stored in Keychain.

## Re-audit of the 14 logs — collector revision 2

[Per-capture results and outstanding issues](docs/RE-AUDIT-14-20261102.md), with sanitized CSV and
JSON. The capacity fixes from build61 are present in the code;
the two hardware failures, zero native RX counters, the cause of CCMP replays and sustained
speed still have no demonstrated resolution. The collector corrects the
spctl classification and adds recent coverage after a timeout, preserving
the indication of an incomplete historical window. Kext61 and app52 retain their code.

## Performance continuation — build61/app52

RTL8192CU and RTL8188EU now negotiate HT20/MCS, with firmware RA on CU and
AMRR adaptation through CCX reports on 88E. The path includes QoS, descriptors,
reception and BA agreements. HT40 remains disabled for these two families.
RTL8821CU/RTL8822BU/RTL8822CU include HT40 and VHT20/40/80 on non-DFS 5 GHz,
with consistent calibration, rate mask, bandwidth and coexistence. On 2.4 GHz/DFS
they remain at HT20; channel 165 stays at 20 MHz. RTL8812AU/RTL8821AU correct the reading
of 80 MHz power calibration in the Linux OTP layout.

The collector includes numeric samples before/after native delivery, separate
from the counters read afterward. Zero native RX counters remain under investigation: the
counters were not forced. See [this round's changes and limits](docs/PERFORMANCE-0.14.14.md)
and [validation](docs/VALIDATION-0.14.14.md). New modes need hardware regression
testing; the 14 historical logs remain evidence for build55.

## Audit of 14 adapters — build60/app51

This release fixes USB8723BU topology, resident MCU reset on 8812/8821 and
memory bounds in the receive cache. The collector shows failures before
controller publication and AU boot/power stages. Built-In and earlier fixes
remain. See [the report for each board, errors and speed modes](docs/ADAPTER-TESTS-20261101.md)
and [validation limits](docs/VALIDATION-0.14.13.md). The package's 12 connections
were recorded on build55; this release requires new hardware regression testing.

## Built-In — build59

The Ethernet interface of each adapter published by the kext receives the
boolean property `IOBuiltin = Yes` before registration in the IORegistry. The flag is on the
network interface; the USB device, BSD name and service order retain
their own states. This path is shared by the supported station backends.
The app remains at version 0.3.8/build50. The flag appears after the new kext is
loaded; compiling does not change the instance already loaded.

## Preference after reboot — build58/app50

The selected board is remembered by VID/PID and validated physical identity
(serial/MAC; USB port when no durable identifier exists). The RegistryID is
resolved again on every boot. The preference waits for a board that is slow to
appear; two indistinguishable devices require manual selection. Identity
published later strengthens the preference within the same session.

Each board saves its last confirmed network. Choosing a network for one-time use
also makes it the preferred network, but reconnecting requires a password already saved in Keychain and
automatic reconnect enabled. Disabling the option invalidates queued password reads and
automatic JOINs. When upgrading from versions without a board
preference, a single already-authorized link can be adopted without a new association; with
two links or an ambiguous identity, selection remains manual.

A scan to list networks without a JOIN profile ends after one complete sweep
of the authorized active/passive channels, preserving the cache. It
does not leave an idle board switching channels indefinitely. A scan to
connect a profile retains the original association attempts.

The source includes a fix for a coexistence timer retained during B/C shutdown,
synchronized reuse of the log formatter and SDK content verification.
An idle measurement of app49 found 0–0.1% CPU and about 39 MB in `top`;
the helper used 0% and 3.2 MB. This does not attribute `kernel_task` memory/CPU to the kext.
See [validation and its limits](docs/VALIDATION-0.14.11.md).

## Two adapters and automatic refresh — build57/app49

Each adapter maintains its own scan state. When an idle USB adapter is selected,
the app starts a real SCAN if it has no recent completion. Validity lasts
30 seconds from the end of the scan, distinguishing cache reads from radio activity.
Wake/hotplug invalidate the origin; stale callbacks do not change another board.
An already-connected board continues with cache queries, preserving the link;
Refresh networks remains available for a manual scan.

Simultaneous scans are coalesced. A failure is retried after 4 and 8 seconds;
the third failure starts a 300-second pause. A scan without completion has a
30-second limit. This policy generates no new password request or JOIN.

Power registration, runtime RX recovery and initial RX validation/draining
are shared by the 13 station backends. Bootstrap uses shutdown and
identity routines specific to each silicon family, retaining original errors and retry limits.
AU power routines do not replace those of another family. Shutdown for
legacy CU/8188EU/8192EU uses the pinned Linux sequences, with verified results.

In the build56 hardware checkup, T9UH/RTL8814AU connected on channel 36 (5 GHz) and
RTL8192EU on channel 6 (2.4 GHz). HTTPS directed to each interface returned HTTP 200.
This is not a hardware test of the new build57 or validation of the other boards.
Tahoe's native RX counters remained at zero despite delivered traffic;
diagnostics show native capabilities for investigation without inflating counters.
See [validation of this release](docs/VALIDATION-0.14.10.md).

## T9UH recovery — build56/app48

Build56 fixes the blockage after a transient RTL8814AU initialization failure,
with complete stop/drain, verified shutdown, reidentification and a retry
bounded per SCAN/JOIN. Original errors and actual radio phases are preserved.
Failed initial RX is not advertised as ready; late errors from the previous epoch
are checked. Power is registered in the IOKit family's policy-maker hook.

App48 includes local diagnostics by exact ID, using the existing authentication:

```sh
"/Applications/Tahoe Wi-Fi USB.app/Contents/MacOS/TahoeUSBWiFi" --diagnose-adapter ID --scan
```

Numeric records are stored in `~/Library/Logs/Tahoe-WiFi-USB/control.jsonl`.
The collector includes this file. These records contain no SSID, MAC, password or PMK.
The actual test on build55 returned `0xe00002d8` before the scan; the first
internal cause was not exported in that version. Hardware recovery from the failure requires its own test; the later checkup already
found build56 loaded and both adapters carrying functional traffic. RTL8814AU remains at HT20, with up to two streams, without VHT/AC.

## Implemented modes

The limits describe station code. USB identity, ROM, silicon
revision, RF paths, factory power and endpoints are checked during
initialization. The 13 operating backends are integrated into the controller.

| Family | Bands | Station mode |
|---|---|---|
| RTL8192CU | 2.4 GHz | HT20, up to 2 streams |
| RTL8188EU | 2.4 GHz | HT20, 1 stream; I-cut unsupported |
| RTL8192EU | 2.4 GHz | HT20/40, up to 2 streams |
| RTL8192DU | 2.4 and 5 GHz | HT20; SMSP up to 2 streams, DMDP 1 stream per MAC |
| RTL8192FU | 2.4 GHz | HT20/40, up to 2 streams |
| RTL8710BU | 2.4 GHz | HT20/40, 1 stream; SMIC/UMC supported |
| RTL8723BU | 2.4 GHz | HT20, 1 stream; Bluetooth coexistence controls |
| RTL8812AU | 2.4 and 5 GHz | HT20/40 and VHT20/40/80, up to 2 streams |
| RTL8814AU | 2.4 and 5 GHz | HT20, up to 2 active streams; RFE0/1 |
| RTL8821AU | 2.4 and 5 GHz | HT20/40 and VHT20/40/80, 1 stream |
| RTL8821CU | 2.4 and 5 GHz | HT20 on 2.4 GHz/DFS; HT40/VHT20/40/80 on non-DFS 5 GHz; 1 stream |
| RTL8822BU/CU | 2.4 and 5 GHz | HT20 on 2.4 GHz/DFS; HT40/VHT20/40/80 on non-DFS 5 GHz; up to 2 streams |
| RTL8814BU | Station disabled | Dedicated HAL/USB and PHY; NIC firmware/protocol pending |

In the current AU implementation, RTL8812AU accepts RFE 0–3 and RTL8821AU accepts RFE 0,
with the documented ROM defaults. RTL8812AU boards with active Bluetooth
remain blocked: the selected primary descriptor lacks the required
coexistence callbacks. These board limits apply even when the USB
ID is in the catalog.

Models 04bb:0959, 04bb:095a and 0bda:2003 follow the OEM's 5 GHz-only
installation: they neither advertise nor accept 2.4 GHz transmission.

Profiles use channels 1–11 on 2.4 GHz and non-DFS channels 36/40/44/48 and
149/153/157/161/165 on 5 GHz, depending on the backend and ROM. Build55 adds
DFS HT20 on 52/56/60/64 and 100/104/108/112/116/120/124/128/132/136/140 to the seven
dual-band backends. ROM/RFE, factory country and power limits may reduce
this set. The loaded driver advertises each group separately; the catalog
alone does not enable transmission. Channel 144 remains closed.

DFS scanning is passive. A recent beacon from the selected BSSID/SSID/channel permits
directed authentication; the driver respects the local power ceiling and the one advertised
by the AP. CSA interrupts TX, closes the link and starts a new scan/WPA2 association.
APs with a wider channel use the implemented DFS20 path. Existing VHT40/80
on non-DFS channels is available on the corresponding AU/B/C backends. There is no AP mode,
local CAC/radar, WPA3 or Wi-Fi6/AX. See [the DFS implementation](docs/DFS-STATION.md).

PHY ceilings are not download/upload results. Speed depends on the AP,
signal, USB and implemented functions. CU/8188EU use HT20; B/C
allow wider modes on non-DFS 5 GHz. RTL8814AU remains at HT20.

## Catalog and release status

`data/legacy-usb-adapters.json` preserves the old package's 250 original records
and 20 extensions with provenance evidence. There are 270 IDs in the inventory and 263
with numeric matches in a primary table. The remaining seven have
unconfirmed or conflicting identities, with no static enablement. ID
13b1:0043 has a separate, restricted discovery path: the chip is read before
selecting B/C, with another check before power and firmware. The other
cases are not enabled based on name similarity.
RTL8814B now has its own ported HALMAC/USB, OTP and codecs. The station
path remains blocked until licensed NIC firmware and its
protocol, factory/regulatory profile and remaining tracking functions are qualified,
in addition to hardware tests. See [the detailed status](docs/RTL8814BU-STATUS.md).
A similar name does not permit using another chipset's HAL.

Of these, 262 IDs are eligible for the 13 integrated backends. See the
[complete matrix](docs/USB-WIFI-ADAPTERS.csv) and
[validation report](docs/VALIDATION-0.14.9.md). Eligibility of
an ID does not prove all commercial revisions that use it. This source
is not release certification for the 270 records.

## Scan and receive fixes in this build

The initial scan on dual-band backends completes the enabled
passive channels before choosing the AP. The app shows all usable bands
observed when the same SSID appears on 2.4 and 5 GHz; AP selection is
automatic in the driver.

USB reception handles a transaction error with no bytes through bounded
endpoint recovery. Build50 fixes premature termination when endpoint
cleanup itself fails transiently: it repeats recovery with an interval
and finite budget, drains old or partially rearmed reads and only then
rearms the four buffers. Synchronous RX rejections enter this same path.
Diagnostics distinguish cleanup, draining and rearming. This path is shared
by the 13 station families; persistent failures and other errors retain a
safe stop. Build49 recovered one hardware occurrence, but failed on the next.
Build50 was found stable on RTL8814AU, with WPA2 authorized and no
USB recovery errors at the time of capture. This does not validate other boards
or all channels. Build53 was loaded and examined in the 31/10 checkup; build54 needs to be loaded again to validate its fixes.

## Explicit scan and measured signal

App48 requests a real scan when Refresh networks is clicked, including when
already connected. This scan may briefly reassociate; background status
queries remain read-only. The cache does not represent a
new radio sweep.

The menu, tooltip and accessibility show measured RSSI in dBm and the filtered value.
Bars retain their thresholds: the signal observed between −44 and −42 dBm corresponds
to three bars. Native images were checked at all three levels; no
icon stuck at maximum was found. Stale or missing readings are indicated.

The RTL8814AU HT20 path includes the channel-153 NBI/CSI adjustment from the
pinned Linux source. Scan counters indicate visited channels and PHY
observations, separate from the network cache. The router's automatically selected 5 GHz channel still
has not been identified; these fixes do not prove a hardware 5 GHz connection.

## Review of the other families

Build52 corrects RTL8192EU HT40/SGI/subchannel transmit fields according to
the primary Realtek setters, keeping RTS and retries separate. Frames at
20 MHz on an HT40 radio receive the correct subchannel. CU/8188EU retain their
implemented modes and reject HT options that have not been ported.

RTL8812AU/RTL8821AU correct the PHY position when the descriptor contains SHIFT,
including signal measurements and firmware/coexistence consumers. The generic
RX24 codec distinguishes this rtw88 layout from the legacy rtl8xxxu layout.
RTL8192FU preserves the host's 80211 sequence in management, EAPOL and data.
AU/B-C coexistence preserves LNA requests when another MP query is
pending, with a bound, stale-session discard and per-device state.

The review covered the 13 operating backends. The absence of another
confirmed defect in a code review does not replace tests on physical boards.

## Extracting logs

Double-click `WiFi-Diagnostics.command`. It uses only macOS
tools, without needing to compile the app or kext. The folder and ZIP are saved in
`~/Library/Logs/Tahoe-WiFi-USB/Diagnostics/`; Terminal shows the final path.

`RTW-SUMMARY.txt` identifies each controller by Registry ID, family,
VID:PID and interface, and compares two readings of USB/RX/TX counters.
The original properties and numeric JSON are stored in `drivers/RTW-before/after`.
These are values published in the IORegistry, which may be stale when the app
does not query that adapter. A zero delta indicates identical published values;
it does not confirm an absence of new errors or traffic during collection. Build54 provides a publication generation and monotonic time per controller. An odd generation indicates publication in progress; reading the IORegistry is not atomic. Older versions do not provide these markers.
Read failures and timeouts are recorded in `STATUS.tsv`, without preventing ZIP creation.
On AU, the old `RTWDiagCalib*` counters do not measure calibration time.

Default collection does not request administrator access, run a speed test or
change the network/driver. It does not read passwords or Keychain. It applies the existing basic
masking, including to MAC addresses in data format; review logs before sharing.
`--output-dir`, `--minutes`, `--command-timeout` and `--no-pause` are covered in the help.

## RTL8814AU: performance on build53

AU reception now uses 32 KiB buffers and reads, consistent with the primary
20 KiB DMA threshold. The old 16 KiB path could truncate valid aggregates;
the other backends retain 16 KiB. New scalar values show read lengths
and the length of the failed read, without exporting packets.

TX AMPDU includes the peer's byte limit, BA per TID and descriptor fields
validated against the pinned sources. Management, EAPOL, group and fragmented frames remain
outside aggregation. The effective rate reported by firmware controls thermal
tracking, without being overwritten by the descriptor's initial rate on each send.

Build52, on the new direct port, completed short transfers at about
8.7 Mb/s download and 8.9 Mb/s upload with zero published recovery errors,
but added RX truncations/malformed frames. These are build52 results;
in the later checkup, build53 completed short transfers between 48.3–73.9 Mb/s download and 42.0–55.2 Mb/s upload, with no new published USB errors or truncations. These are short WAN samples, not PHY ceilings.

## Checkup fixes — build54

A full TX queue now preserves the packet and requests stall/retry from the IOKit
output queue. Asynchronous resumption occurs when there is space and authorization; an absent queue
or closed connection retains safe discard. This prevents admission losses under
queue pressure, without increasing its limits.

STATUS publishes HT/BA for backends that returned before legacy calibration,
without executing AMRR or radio I/O. It includes existing reorder/BA causes and
per-controller publication markers. The fixed 54 Mb/s Ethernet speed
was removed: without a complete, recent effective PHY rate, the value is unknown.

Internal RX accounting advanced with delivered packets, while BSD
reported zero in checkup53. Build54 checks the delivery/flush return and
exposes the input boundary and native ifnet reading, without duplicating counters.
This still does not prove that BSD's zero was fixed, or retrospectively explain the seven
observed input errors. Hardware validation of these points requires loading build54.

The [complete list by chipset and VID:PID](docs/USB-WIFI-SUPPORT.txt) explains the
262 eligible identities, restrictions and revisions; it is not hardware validation of
all boards. The catalog remains the same as build53.

## Building

Usage requirements: Intel Mac with Ventura 13, Sonoma 14, Sequoia 15 or Tahoe 26.
To build: full Xcode with macOS SDK 13 or later, CMake,
Python 3.10+ and C/C++ tools. Node is used for an independent
cryptographic check. The script fetches MacKernelSDK at a pinned revision;
the SDK is not included in the ZIP. `RTW_OFFLINE=1` prevents downloads and requires a local SDK.
The complete header/library contents are checked against the SHA256 manifest;
an SDK extracted without Git is also accepted if it matches the pinned contents.
A different revision or altered contents stops the build and preserves the local SDK.
The build environment verified for this release is Xcode 26.6 with macOS SDK 26.5;
the app/helper deployment target is 13.0. Builds with older Xcode versions
still need their own validation.

At the root of the extracted source:

```sh
bash ./build.command --all
```

`--all` builds the kext, app and tests. `--kext`, `--app` and `--tests` select
the stages. Products are saved in `Release/`. Source archives and release packs are saved in
`releases/`, in the project's main folder; they are not written to the Desktop. The scripts verify
ad-hoc signatures; they do not install or load the kext.

For native app, AppKit, Keychain and transport tests:

```sh
bash ./app/test.command
```

To generate the installer after building the app:

```sh
bash ./app/package.command --no-build
```

The installer places `Tahoe Wi-Fi USB.app` in `/Applications` and installs the
helper. The kext goes separately into the EFI. The app allows saving the password
in macOS Keychain. No personal password is needed to build.

## Loading through Clover

Identify the EFI that actually boots the computer. Back up the previous kext
outside the injection folder and copy the entire bundle:

```text
Release/TahoeRealtekWiFi.kext
  → EFI/CLOVER/kexts/Other/TahoeRealtekWiFi.kext
```

The copy will be used on the next boot through the correct EFI. The driver already loaded
remains in memory until then. The scripts do not restart the computer or
change SIP, AMFI or Clover configuration. See [Clover](docs/CLOVER.md).

## Checking the result

Check version/build, selected adapter, WPA2 authorization, IP address and
traffic through the corresponding USB interface. The presence of `enX` or the kext
in the IORegistry does not prove a connection. With wired Ethernet connected, confirm
which interface the speed test uses.

`rtw-backends --json` and `--adapters-json` show source capabilities;
they do not access hardware. Firmware fields in legacy diagnostics describe
only `rtw-legacy-fwdiag`. The kext's actual loader uses its own backend.
The old generic hardware boot path is disabled.

## Origins and licenses

The source includes app, helper, kext, net80211, cryptography, redistributable
firmware, tests, generators and license notices. See
[origins](docs/SOURCES.md), the `*-Upstream.json` files, the
`firmware/*-provenance.json` records and the licenses beside each component.
Firmware images are unmodified binaries embedded in the source; they are not
Realtek firmware source code.

Generators validate the pinned primary inputs before writing files.
The release includes `SOURCE-MANIFEST.sha256` to check source integrity.
