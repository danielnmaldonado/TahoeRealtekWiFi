# Throughput investigation — driver 66, app 55

## Measured before this correction

On November 3, 2026, the loaded build 65 was tested on the connected RTL8814AU
TP-Link 2357:0106. It used USB high-speed, WPA2, 5 GHz/channel 36, HT20, 2 streams
and negotiated SGI20. Other supported USB adapters were not present for these
tests. Interface-bound tests ran sequentially, without altering routes or
router configuration. Private captures and addresses remain outside this source.

| Test on build 65 | Download Mbps | Upload Mbps |
| --- | ---: | ---: |
| Apple networkQuality, native Wi-Fi control | 575.1 | 245.9 |
| Apple networkQuality, USB control | 26.3 | 27.9 |
| Apple networkQuality, USB with CPU observation | 28.8 | 23.6 |
| Direct CacheFly HTTPS, complete 100 MiB body | 69.2 | — |

NetworkQuality values are the tool's throughput estimates. CacheFly is the
average of the complete 104857600-byte response over 12.12 seconds, HTTP 200. The
services use different endpoints and test algorithms; their results are not
interchangeable. The user's 100 Mbps down/30 Mbps up target was not established.

Fresh-session upload was 28.7 Mbps with no RSSI mailbox deferrals and 41.9 Mbps
after deferrals began. Therefore, mailbox contention alone has not been shown
to cause the slowdown. An earlier fresh-session test started before DHCP and
returned error −1009; it is invalid and is not a 0 Mbps performance measurement.

System-wide CPU idle stayed above 94% during the sampled tests. Kernel process
usage stayed below 23% of one CPU during download. This does not isolate the
kext's exact CPU cost, but does not demonstrate processor saturation. Aggregate
RX, USB-transfer and decrypt failures were not observed during these tests.

The live firmware mailbox eventually remained occupied and C2H rate feedback
was absent. Effective TX rate is unknown; RX MCS15, an accepted USB transfer,
or an AMPDU request does not prove actual TX rate, aggregation or delivery.
A bounded MAC clock probe did not change this state and restored its original
value. It is not included in the driver or source release.

## Corrected defect

The receive path updated the RTL8814AU firmware RSSI and the shared average
from any decoded PHY sample. Neighboring AP beacons could consequently become
input for the connected AP's rate control. The same shared average is consumed
by firmware RSSI reporting on RTL8192CU, RTL8188EU and RTL8192EU. This defect
also exists in older source; its presence does not establish that it caused
the specific build 65 regression or that it is the only throughput limitation.

The common update now requires a valid PHY measurement from the current AP in
RUN. It reuses the existing bounded frame-direction/transmitter/BSSID check.
It allows RUN before WPA authorization. Per-frame RSSI, scanning, node signal
and the already guarded private feedback caches of the other nine families
are preserved. There are no extra timers, buffers, USB requests or allocations.

The actual production receive signal block and update method are exercised
across all 13 enabled station families. Tests cover 338 cases: AP data/beacons,
foreign transmitter/BSSID, uplink/WDS, scanning/association, absent BSS/PHY,
invalid power and short frames, with empty and established averages. The old
production block fails the foreign-AP average assertion; the corrected block
passes. Synthetic values keep raw PHY percentage distinct from the net80211
node scale. PHY decoders and private-cache implementations are outside this
specific host harness; the legacy average call is observed at its boundary.

## Remaining physical validation

Build 66 must be compiled/loaded and measured before it can be called a
throughput fix. No candidate kext was installed, injected into EFI or loaded
by this follow-up. No GitHub or security-policy changes were made.

Software coverage across 13 families is not physical validation across 13
adapters. Each board/revision needs its own radio, WPA2 and sustained traffic
test. The current RTL8814AU station path remains HT20; this correction does not
add HT40/VHT. A 1 stream HT20 link has a maximum PHY rate below 100 Mbps, so a
universal 100 Mbps payload guarantee cannot be made for every supported mode.
