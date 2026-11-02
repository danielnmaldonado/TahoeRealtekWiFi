# Validation — 0.14.5/build52 and app 0.3.4/build46

Project and macOS adaptation: MaLd0n. Status: development.

## Review by family

All 13 station backends were reviewed, divided into legacy 2.4 GHz,
DU/AU and B/C. Review compared implementation, descriptors, RF/PHY, bands,
association, RX/TX, coexistence and lifecycle against pinned primary sources.
Previous tests could pass because they repeated incorrect field positions;
the fixes below have new independent references and RED→GREEN evidence.

RTL8192EU used gen1 masks in DW4 for HT40, but its gen2 descriptor uses
DW5. The result could be BW20 with changed RTS/retry. The new format
separates DATA_BW, DATA_SC and DATA_SHORT/SGI, including 20 MHz management/EAPOL on an
HT40 radio. Textual setters from the pinned Realtek header serve as reference; its
hash/provenance accompanies the tests. CU/8188EU reject the new HT fields.

RTL8812AU/RTL8821AU placed PHY before SHIFT, differing from rtw88.
Cases 0–3 were fixed in both families, with RSSI and firmware/coexistence
commands, aggregate boundaries and absent PHY. The operational
RTL8814AU, DU and B/C codecs already had the correct position.

The auxiliary FamilyRX24 codec now applies SHIFT to PHY only for rtw88
families. For FU/8723BU, rtl8xxxu source places PHY immediately after the
descriptor and padding before the MPDU; this layout was preserved. A rule
from another generation was not applied to all chipsets.

RTL8192FU replaced host management/non-QoS sequence with hardware
sequencing. The port uses software 80211 sequence; management, EAPOL,
QoS and other data now preserve the value and fragments of the received header.
8710BU and 8723BU already preserved these values and were not changed for this.

AU/B-C coexistence could remember a desired LNA level and lose its
request when another MP query was pending. It now retains only the latest
desired level per context and attempts admission after ACK or timeout
of the previous query. The cache changes after successful H2C/scoreboard admission,
according to primary semantics; this does not confirm physical application by the radio.
Errors, stop and loss of firmware/ownership/session discard pending work.
There are coalescing, cancellation, isolation and callback failure tests,
including an actual public BT-info/HID sequence across the three B/C families. This does not equate
to RF qualification with Bluetooth on hardware.

## Content preserved from build51

App46 requests an actual scan only for the explicit Refresh networks action;
periodic polls remain read-only. Measured/filtered dBm signal,
stale/absent readings and accessibility are distinguished. The icon renders
all three levels correctly; −42/−44 dBm measured on build50 is strong signal.
The NBI/CSI 153 adjustment and scan/PHY/cache counters by band remain.
See VALIDATION-0.14.4.md for details of these changes.

## Verification and limits

The integrated set passed in Release (150/150) and with ASan/UBSan on the host
(150/150). An independent review found no new blockers in the
fixes. App46 preserves the 15 native suites verified in build51; the
integrated set also rebuilds and verifies its native tests. Native build
of the ZIP, metadata, signatures and integrity are in VERIFICATION.json
and NATIVE-VERIFICATION.json alongside the distribution files.
The previous inventory, identities and gates are preserved: 270 IDs,
263 primary identities, 262 eligible for the 13 operational backends, plus
restricted discovery 13b1:0043. Code tests do not certify every revision.

RTL8814BU has its own HAL/USB and PHY components, but station operation remains
disabled without qualified NIC firmware/protocol and factory profile.
Still-unproven identities are not enabled by name similarity.
Unimplemented modes and channels, including DFS, are not advertised. The actual
5 GHz channel of the automatic router remains unknown.

During original build52 preparation, build50 was loaded on RTL8814AU,
with authorized WPA2, zero RX recovery errors and a last sample of −37 dBm. At that time, build52/app46 still
needed loading to validate 5 GHz scan, traffic and the other adapters.
The agent did not install, restart, change EFI/router or use credentials
in this review. The source remains in the project folder, with no outputs
on the Desktop.

## logs1 update — 2026-10-31

The user rebuilt/loaded kext52 (UUID
7511954E-85FD-30FC-92E6-28942A938040). The 10:59 capture recorded
RTL8814AU 2357:0106/en3 without an authorized WPA2 port and with USB error 0xe00002ed:
25 errors, 24 completed recoveries and one exhaustion. RTL8192EU
2357:0108/en1 was authorized, with a default route and zero USB recovery
errors. The capture does not measure throughput: the displayed test failed with HTTP500 and
the collector did not start any speed test.

WiFi-Diagnostics.command now generates the ZIP in Biblioteca/Logs/Tahoe-WiFi-USB,
with two text/JSON readings, a summary of deltas by Registry ID and status of
every command. Read failure is distinguished from removal; deltas from an
incomplete capture are unavailable. Each command has an execution limit, including
partial output, and only that command's children are terminated if it exceeds the limit.
Ten offline regressions passed; an actual collection produced an intact ZIP.

logs1 changes the collector, its documentation and test integration. Native
kext/app source, firmware, versions52/46 and built products remain
the same as the original delivery52. The new source ZIP is separate and does not
overwrite the earlier delivery. AU USB error and slowness remain under
investigation; creating a collector does not fix USB communication.

## Analysis of the 11:17 report and logs2

The user's ZIP contains logs from 10:47:34 to 11:17:11. Since AU errors already
existed at 10:43, the first failure fell outside this window. A read-only local
query since boot recovered 25 AU@14130000 RX0x81 errors between
10:41:59.912 and 10:42:06.261, followed by final abort of three reads.
There were also 11 EP0 errors, consistent with the 11 counted clear/retry failures;
assigning each to CLEAR_FEATURE remains an inference without the setup packet.

The same XHCI controller records keyboard errors on another hub, including
before the current AU enumeration. This does not prove a common cause or physical defect.
The data locate the failure in RX transport, before WPA2/DHCP; they do not determine
its origin. Recovery budget, buffer and USB timeout were not changed.

General RTWDiag publication occurs in status(). The app queries the selected
adapter; on the other, properties may be stale. Scan and some
HT/RA fields have separate publication. The logs2 collector documents this
limit: delta 0 means only equal published values. Kext52 does not
provide a general publication timestamp/generation, and the collector does not bypass access
control to force STATUS. Native sources remain identical to delivery52.
