# Validation — driver 0.14.7/build54 and app 0.3.4/build46

macOS development and adaptation: DNMTechLabs. Status: development.

## Physical checkup of build53

On October 31, 2026, the driver found loaded was build53 rebuilt by the user, UUID F700B442-AB96-3173-A65B-595D3F1B44CE. The machine ran Tahoe 26.7.2/25G332, with RTL8814AU 2357:0106 on direct USB port 14400000 at 480 Mbps, Registry 4294968504/en3. WPA2/CCMP was authorized; IPv4/IPv6 and the default route used USB. The link was on channel 6/2.4 GHz, RSSI −46 to −42 dBm. App 0.3.4/build46 and its active helper passed ad-hoc-signature integrity checks; the helper had one execution and no recorded termination. No current app/helper crash or panic was found in the DiagnosticReports directories examined.

Five valid 4 MiB downloads and 1 MiB/4 MiB uploads used IPv4 curl bound to en3, verified TLS, no proxy, and HTTP 200. Short samples measured 48.3–73.9 Mbps down and 42.0–55.2 Mbps up. A separate 16 MiB request received HTTP 403 and was excluded. These are not speed ceilings, nor do they use the same server/methodology as the user's 84.9/32.7 Mbps screenshot.

All 90/90 gateway pings succeeded without loss. During transfers, mean latency was 7.838 ms and the maximum 190.015 ms; afterward, without agent-generated traffic, the mean was 4.526 ms and the maximum 12.267 ms. The peak under load remains an observation and was not attributed to the radio or modem without more evidence.

General snapshots contained nine distinct RX values over 50 seconds and remained authorized on the same controller. During the full interval, RX published +38,773,035 bytes, +6,365 aggregates, and +30,912 queued Ethernet packets. USB/recovery errors, malformed/truncated aggregates, and CCMP MIC/replay remained at zero. The largest actual read was 21,498 bytes, above the former 16 KiB limit, with no aggregate truncation. These were the latest published values, not automatically live counters or an atomic capture.

TXQueueDrops (12,132) and TXLinkDrops (5) did not increase. InputErrors rose from 5 to 12 without increases in the causes exported by build53. Internal inputPackets advanced by 30,912, while netstat and getifaddrs reported BSD RX of zero. There is no proof that the seven errors were caused by USB, CCMP, or any specific BA condition.

Boot logs contained an initial abort of four RX requests and early Apple framework errors; no corresponding recurrence was found in the inspected interval. A logd disk-writing diagnostic was not attributed to the driver. The report does not claim that every log was error-free. Scan/cache records included 5 GHz, but physical association stayed at 2.4 GHz. The agent did not force scans, switch APs, change the router, reinstall, reboot, or modify the EFI.

## Build54 fixes

A full internal TX queue now returns a stall to IOGatedOutputQueue and retains the mbuf for retry. The wrapper neither releases the packet nor increments an error counter under temporary queue pressure. After completion/draining, the upper queue is asynchronously serviced only when authorized and capacity is available. A missing queue remains a real failure. Teardown, reset, concurrency, and per-adapter isolation preserve ownership. The upper queue is finite; sustained overload can still exhaust it.

The return values of inputPacket/flushInputQueue are no longer ignored. A missing native interface or mismatch in the number of submissions returns an error without touching transferred mbufs again. Eight per-interface counters observe this boundary; ifnet_stat KPI and native index allow comparison with the BSD stage. This corrects false reporting of successful delivery but does not establish the cause or repair of BSD RX zero observed in build53. No native counters were manually incremented or reset to conceal a failure.

STATUS publishes HT/BA across 13 backends separately from AMRR policy. An odd generation indicates publication in progress; the next even generation completes the general snapshot. Monotonic time belongs to that controller. Timers may update subsets between polls, and an IORegistry read is not atomic. The collector keeps that qualification, suppresses comparisons for ongoing/regressed publication, and respects the validity/index of native statistics. Older versions remain readable with unknown freshness.

Existing BA/reorder events are published without inventing fragmentation counts or a one-to-one decomposition of errors. `no_buf` includes both a missing buffer and a duplicate slot. Fixed Ethernet 54 Mbps metadata was replaced by unknown (`0`): there is no complete, recent snapshot of TX rate, width, GI, and association across all backends. RX MCS, negotiated capability, and C2H thermal rate remain distinct. RF, cryptography, firmware, ABI, AU 32 KiB RX capacity, and build53 TX AMPDU were not changed.

## Verification and delivery

New tests reproduced defects in build53 sources before implementation. Tests of actual methods covered 77 TX/wake cases, 24 completion cases, and 36 RX-delivery cases. Controller and `_mbuf` compiled against the native SDK with a 2,048-byte stack-frame limit. Real publication covers all 13 families, association states, invalid KPI, and reading without RF policy changes. The collector has 15 offline behavior tests using Bash/Perl/ditto.

All 155/155 Release and 155/155 ASan/UBSan host tests passed in the recorded validation. Independent review, extracted-ZIP build, products, signatures, and hashes were recorded in `VERIFICATION.json`/`NATIVE-VERIFICATION.json` in the delivery folder. The source contains complete app46, helper, kext, tests, and scripts. Build54 still needed to be loaded for physical measurements.

## Support

The inventory contains 270 identities: 263 confirmed in primary tables, 262 eligible for 13 backends, and restricted discovery for 13b1:0043. Complete lists are in `USB-WIFI-SUPORTE.txt` and `USB-WIFI-ADAPTERS.csv`. RTL8814BU station remains disabled pending firmware, protocol/profile, and tracking. DFS/WPA3/AX were not enabled in this historical build. All families and board revisions still require physical validation.
