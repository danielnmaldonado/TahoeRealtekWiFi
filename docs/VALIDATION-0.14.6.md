# Validation — 0.14.6/build53 and app 0.3.4/build46

Project and macOS adaptation: MaLd0n. Status: development.

## Initial hardware evidence — build52

After the user changed ports, AU 2357:0106 moved to direct path
@14400000, 480 Mb/s, Registry 4294979718/en3, WPA2 authorized on channel 6,
RSSI near −44 dBm. EU was not connected. Kext52 rebuilt by the user
remained loaded, UUID 7511954E-85FD-30FC-92E6-28942A938040; app/helper46.

Five pings to the modem, bound to the en3 IP, responded without loss:
2,479–5,490 ms. A GET of 4 MiB and POST of 1 MiB of zeros to public
Cloudflare endpoints, bound to en3/IPv4, completed with HTTP200 and no proxy. Curl
averages: 8,73 Mb/s and 8,93 Mb/s. These are short WAN samples, not maximum capacity.
The download added about 5,05 MB of RX, 857 aggregates, 53 truncated and
53 malformed; most HT bytes were at MCS15. Published values
showed zero USB recovery. During upload, the immediate snapshot did not change;
absence of traffic was not inferred from this possibly stale snapshot.

## RX compatible with DMA aggregation

Vendor code identifies 0x2005 as a 20 KiB threshold. Linux rtw88 uses buffers
of 32768 bytes; native code used 16 KiB. USBRXCapacity.h now selects 32 KiB
for AU and retains 16 KiB for other backends. Allocation, start, rearm and refill
use the same capacity and check the existing buffer's size.

The actual test uses a valid aggregate of 20200 bytes/13 frames. Splitting it at
16 KiB cuts an MPDU after ten frames, produces truncated and then malformed in
the next read. A 32 KiB read preserves all 13 frames. The mechanism is
consistent with the 53 physical cases, but build52 did not publish the lengths
of those completions; case-by-case causal identity is not asserted.

New scalars: RTWDiagRXLastReadBytes, RTWDiagRXMaxReadBytes and
RTWDiagRXAggregateLastErrorReadBytes. The last refers to structural failure
in the parser, not the length of a USB error. Packets are not exported.
DMA, timing and recovery budget remain unchanged.

Independent review: 943 capacity, 2105 recovery and 258 ordering checks
passed; the same test remains RED against source52. The complete Controller
compiled with the native SDK and a 2048-byte stack frame limit.

## Thermal rate authority

sendFrame overwrote the effective C2H rate with the initial/fixed descriptor rate,
including for management, EAPOL and rejected TX. Primary code updates tracking
using the C2H rate. The per-packet write was removed, preserving descriptors,
PHY baseline and C2H filters. The 55 Controller→PHY/thermal checks passed;
the reproduced thermal delta remains 3→3, whereas it previously changed 3→4.
C2H without association identity still cannot be distinguished by a local
counter alone; no proof of an epoch absent from the protocol was invented.

## AU TX AMPDU

Integration programs the peer byte limit in 0x458 before accepting BA.
I/O failure refuses admission and blocks TX. The descriptor consults the current
BA agreement per TID, authorization, HT, QoS/ACK and fragments. MAX_AGG_NUM uses
MPDU pairs and respects half the BA window; window 1 remains individual.
Density, host sequence and checksum were checked. Management,
EAPOL and group frames do not aggregate. DELBA and reassociation remove eligibility.
The absence of AGG_BK matches primary code; nonaggregated bytes are preserved.
Descriptor batching per USB transfer remains outside this fix.

Fixtures record five primary hashes and masks. In independent review,
3817 TX/BA, 2025 codec, 2470 BA/CCMP and 178 lifecycle checks passed. Imported
negotiation was exercised in both bands. This does not validate physical over-the-air aggregation.

## Integration and delivery

153/153 Release tests and 153/153 with ASan/UBSan on the host passed. Independent
reviews found no new blockers. Results from extracted source,
native products, signatures and integrity are in VERIFICATION.json and
NATIVE-VERIFICATION.json alongside the delivery, after freezing this source.
App46 and its protocol remain compatible.

Build53 has not yet been loaded on hardware. Speed gains, BA negotiation
and stability require measurement after the user builds/replaces the kext.
The agent did not install, restart, change EFI, router, SIP/AMFI or use
passwords. All files remain in the project folder.

Inventory and gates preserved: 270 IDs, 263 primary, 262 eligible for 13 backends,
plus restricted discovery 13b1:0043. RTL8814BU depends on qualified firmware/profile;
DFS, WPA3 and unimplemented modes are not advertised. The automatic router's actual 5 GHz
channel remains unknown; this session52 stayed in 2.4 GHz/channel6.
The full release is not yet certified.
