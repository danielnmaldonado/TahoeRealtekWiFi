# Audit of the 14 adapters — 01/11/2026

Historical record of build55, written during round60. For the state of source61 and current outstanding work, see [the 02/11 re-audit](RE-AUDIT-14-20261102.md).

The user supplied the 14 groups: 12 work and two fail. The captures use macOS12.7.6/21H1320, kext55 and app0.3.5/build48; they are not hardware validation of build60. The current version remains targeted at Tahoe/Intel.

The 12 connections show authorized WPA2, an active interface with IPv4/route, and increasing Ethernet delivery counters. This demonstrates local connectivity and reception during the window, without demonstrating internet testing, sustained speed or the absence of all drops.

| Capture | USB VID:PID | Family | Observed mode | Errors/drops during the window |
|---|---|---|---|---|
| 18:49:15 | 0BDA:B812 | RTL8822BU | HT20 / 2 stream(s); channel 157 | no increase in RX/TX/USB errors in the available counters |
| 18:54:14 | 2357:010C | RTL8188EU | b/g, no HT; channel 6 | CCMP replay +2; decrypt +2; net80211 RX +2; duplicates +7 |
| 18:58:06 | 2357:0107 | RTL8192EU | HT40 / 2 stream(s); channel 6 | duplicates +2; BA below window +3 |
| 19:00:28 | 2357:0109 | RTL8192EU | HT40 / 2 stream(s); channel 6 | BA below window +11 |
| 19:02:05 | 0BDA:0179 | RTL8188EU | b/g, no HT; channel 6 | duplicates +7 |
| 19:03:36 | 0BDA:C811 | RTL8821CU | HT20 / 1 stream(s); channel 157 | no increase in RX/TX/USB errors in the available counters |
| 19:04:55 | 0BDA:C811 | RTL8821CU | HT20 / 1 stream(s); channel 157 | BA below window +2 |
| 19:06:32 | 0BDA:8179 | RTL8188EU | b/g, no HT; channel 6 | no increase in RX/TX/USB errors in the available counters |
| 19:08:00 | 20F4:808A | RTL8822BU | HT20 / 2 stream(s); channel 157 | BA below window +2 |
| 19:09:42 | 0BDA:8178 | RTL8192CU | b/g, no HT; channel 6 | duplicates +1 |
| 19:11:07 | 2357:0115 | RTL8822BU | HT20 / 2 stream(s); channel 157 | no increase in RX/TX/USB errors in the available counters |
| 19:13:20 | 7392:A611 | RTL8723BU (USB profile) | no controller | USB start rejected, 0xe00002c7 |
| 19:15:01 | 2357:0108 | RTL8192EU | HT40 / 2 stream(s); channel 6 | no increase in RX/TX/USB errors in the available counters |
| 19:20:26 | 2357:0120 | RTL8821AU | no channel/association | channel/SCAN/RX/WPA do not advance |

## Speed

No capture contains a download/upload test. IOLinkSpeed=0 and LinkSpeedKnown=0 mean unknown rate. RXLastRate and histograms include management frames and cumulative samples; they are not connection speed or maximum capacity. The attached JSON retains rates/MCS/SGI/histograms for each group.

- RTL8192EU: the three samples negotiated HT40 with two streams.
- RTL8821CU/RTL8822BU: five samples operated at HT20 in 5GHz; none negotiated VHT. The captured build55 B/C implementation was HT20. Source61 adds the modes listed in the re-audit.
- RTL8188EU/RTL8192CU: four samples use the b/g path without HT, as defined in the captured source55. This implementation limit reduces the possible speed of these N adapters.
- RTL8821AU2357:0120: no negotiated mode, insufficient data to assess speed.

## All counters and their interpretation

The CSV ADAPTER-ERRORS-20261101.csv lists 58 nonzero observations of errors/filters: before/after/delta values, category and meaning. These are not 58 distinct failures. Several counters reflect the same drop, and many are historical with no increase during the capture. SignalForeignFrames/SignalMissingPHY filter signal measurement and do not count total data loss.

The 2357:010C had CCMP replay+2, decrypt+2 and inputErrors+2, plus duplicates+7. The connection remained authorized. The cause of these retransmissions/drops was not recorded; there is no basis for disabling CCMP protection or claiming USB failure or an incorrect password. The other groups contain duplicates/BA and some historical errors with no increase, as shown in the CSV.

Across the 12 connected adapters: USB/TX errors, RX recovery and RX aggregation errors have zero values/deltas. Native RX totals stay at zero even while Ethernet delivery increases; this counting discrepancy remains open, without artificial increments.

## Fixes in build60

- Edimax 7392:a611: the source rejected four physical OUT endpoints despite the Linux mapping of up to three logical queues. Queue storage, discovery, initialization and routing were fixed, preserving bounds and Bluetooth guards. The log records six endpoints and discovery failure; the raw descriptor is still needed to confirm that four OUT endpoints are the exact physical cause.
- AU8812/8821: MCU firmware resident before boot now undergoes the bounded reset used in Linux. Previously it returned EBUSY without recovery. It cannot be asserted that the 2357:0120 was in this state. The new telemetry shows stage/original error/reset and power results on reference channels without bypassing factory calibration.
- Common receive path: IE cache copy bounds and memory ownership on allocation failure were fixed. The two historical panics included in the ZIP are consistent with this path; the exact UUID/binary was not located, so hardware causality is unproven.
- Collector: shows USB provider failure even without a controller, preserves AU stages and errors, and distinguishes published results from new operations.

## Capture and validation

The same panic files were repeated in the 14 ZIPs; they represent two earlier events, not 14 new crashes. The header and body conflict over the macOS version. Five log queries timed out and the Gatekeeper command returned 1 in every group; the timeouts are collection failures; exit 1 with assessments disabled describes Gatekeeper state. Neither demonstrates radio failure.

The package includes software tests, comparison against pinned Linux and a native build from source. Edimax and 2357:0120 need another hardware test on build60. The 12 previous results also need connection/traffic regression testing when changing the driver. No benchmark, RF test or universal certification is implied.
