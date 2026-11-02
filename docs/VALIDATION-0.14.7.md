# Validation — 0.14.7/build54 and app 0.3.4/build46

Project and macOS adaptation: MaLd0n. Status: development.

## Hardware check of build53

On 31/10/2026, build53 rebuilt by the user was found loaded,
UUID F700B442-AB96-3173-A65B-595D3F1B44CE. Tahoe 26.7.2/25G332, RTL8814AU
2357:0106 on direct port 14400000 at 480 Mb/s, Registry 4294968504/en3. WPA2/CCMP
authorized, IPv4/IPv6 and default route on USB. Channel 6 / 2.4 GHz, signal between −46 and −42 dBm.
App 0.3.4/build46 and active helper, intact by ad hoc signature; helper with one
execution and no recorded termination. No current app/helper crash/panic
was found in the queried DiagnosticReports directories.

Five valid 4 MiB downloads and 1 MiB / 4 MiB uploads used curl IPv4 bound
to en3, verified TLS, no proxy and HTTP 200. Short samples: 48,3–73,9 Mb/s
download and 42,0–55,2 Mb/s upload. A separate 16 MiB request received HTTP 403
from the server and was excluded from measurement. These are not speed ceilings, nor the same
server/methodology as the user's capture, 84,9/32,7 Mb/s.

90/90 gateway pings responded without loss. During transfers, the average was
7,838 ms and maximum 190,015 ms; afterward, without agent-generated transfer,
the average was 4,526 ms and maximum 12,267 ms. The peak under load remains an observation; it was not
attributed to the radio or modem without additional evidence.

General snapshots had nine distinct RX values over 50 s and remained
authorized on the same controller. Over the total interval, RX published +38.773.035 bytes,
+6.365 aggregates and +30.912 queued Ethernet packets. USB/recovery,
aggregate/truncated/malformed errors and CCMP MIC/replay remained 0. The largest actual read
was 21498 bytes: above the old 16 KiB, without cutting the aggregate. These values are
last publications, not automatically live counters or an atomic capture.

TXQueueDrops 12132 and TXLinkDrops 5 did not increase. InputErrors 5→12, without increase
in the causes exported by build53. Internal inputPackets accounting advanced
by the same 30912 packets, while netstat and getifaddrs returned BSD RX 0.
There is no proof that the seven errors are USB failure, CCMP or a specific BA cause.

Boot logs contained an initial abort of four RX requests and initial Apple
framework errors; there was no corresponding repetition in the queried excerpt.
The logd disk-write diagnostic was not attributed to the driver. No claim is made
that all logs are error-free. Scan/cache records include 5 GHz;
the current physical association remained 2.4 GHz. The agent did not force a scan, change AP,
change the router, reinstall, restart or change EFI.

## Build54 fixes

A full internal queue now returns stall to IOGatedOutputQueue and preserves the mbuf
for retry. The wrapper does not free the packet or add an error for this temporary pressure.
After completion/drain, the upper queue receives asynchronous service only
when authorization and capacity allow. A missing queue remains a genuine failure.
Teardown, reset, concurrency and adapter isolation preserve ownership.
The upper queue is finite; sustained overload can still exhaust it.

The inputPacket/flushInputQueue return is no longer ignored. Missing native
interface or a different submission count returns an error without touching
transferred mbufs again. Eight per-interface counters observe the boundary; KPI
ifnet_stat and native index allow comparison with the BSD stage. This fixes false
delivery success but does not prove the cause or repair of RX 0 observed in build53.
No native counter was manually incremented or reset to conceal failure.

STATUS publishes HT/BA for all 13 backends, separately from AMRR policy. An odd generation
indicates publication in progress and the next even one completes the general snapshot; monotonic
time belongs to that controller. Timers may update subsets between
queries and an IORegistry read is not atomic. The collector retains this caveat,
suppresses comparisons of publication in progress/regressed publication and respects validity/index
of native statistics. Older versions remain readable with unknown freshness.

Existing BA/reorder events are published without inventing a fragmentation counter
or one-to-one decomposition of errors. no_buf includes a missing buffer and duplicate slot.
Fixed Ethernet metadata of 54 Mb/s was replaced with unknown 0: a complete recent
snapshot of TX rate, width, GI and association does not exist across all backends. RX MCS,
negotiated capacity and C2H thermal rate remain separate. RF,
cryptography, firmware, ABI, AU RX 32 KiB capacity and build53 TXAMPDU were not changed.

## Verification and delivery

New tests reproduced the defects in source53 before implementation. Tests of
actual methods check 77 TX/wake, 24 completion and 36 RX delivery cases; Controller
and _mbuf compiled against the native SDK with a 2048-byte stack limit. Actual publication
covers all 13 families, association states, invalid KPI and reading without RF
policy change. The collector has 15 offline behavioral tests with Bash/Perl/ditto.

155/155 Release tests and 155/155 tests with ASan/UBSan on the host passed.
Independent review, build of the extracted ZIP, products,
signatures and hashes are recorded in VERIFICATION.json/NATIVE-VERIFICATION.json
in the delivery folder. Source contains the complete app46, helper, kext, tests and scripts.
Build54 still needs loading to measure its physical behavior.

## Support

270 inventoried identities, 263 confirmed in a primary table, 262 eligible for 13
backends and restricted discovery 13b1:0043. Complete list in USB-WIFI-SUPPORT.txt and
USB-WIFI-ADAPTERS.csv. RTL8814BU still has no enabled station because firmware,
protocol/profile and tracking are pending. DFS/WPA3/AX were not enabled.
Hardware validation of every family/revision remains necessary.
