# Validation — driver 0.14.6/build53 and app 0.3.4/build46

macOS development and adaptation: DNMTechLabs. Status: development.

## Initial physical evidence — build52

After the user changed the USB port, AU 2357:0106 moved to the direct path @14400000, 480 Mbps, Registry 4294979718/en3, with WPA2 authorized on channel 6 and RSSI near −44 dBm. EU was not connected. The user-rebuilt kext52 remained loaded, UUID 7511954E-85FD-30FC-92E6-28942A938040, with app/helper46.

All five modem pings bound to the en3 IP succeeded without loss: 2.479–5.490 ms. A 4 MiB GET and 1 MiB zero-filled POST against public Cloudflare endpoints, bound to en3/IPv4, completed with HTTP 200 and no proxy. Curl averages were 8.73 Mbps down and 8.93 Mbps up. These were short WAN samples, not maximum throughput. The download added approximately 5.05 MB of RX, 857 aggregates, 53 truncated, and 53 malformed counts; most HT bytes used MCS15. Published values showed zero USB recovery. The immediate post-upload snapshot did not change; absence of traffic was not inferred from this potentially stale snapshot.

## RX capacity consistent with DMA aggregation

The vendor source identifies 0x2005 as a 20 KiB threshold. Linux rtw88 uses 32,768-byte buffers, while the native path had used 16 KiB. `USBRXCapacity.h` now selects 32 KiB for AU, leaving 16 KiB for other backends. Allocation, startup, rearming, and refill use the same capacity and check the size of existing buffers.

A real test uses a valid 20,200-byte, 13-frame aggregate. Cutting it at 16 KiB truncates one MPDU after ten frames and causes truncated followed by malformed in the next read. A 32 KiB read preserves all 13 frames. The mechanism is consistent with the 53 physical occurrences, but build52 did not publish completion lengths; no per-occurrence causal identity is claimed.

New scalar diagnostics are `RTWDiagRXLastReadBytes`, `RTWDiagRXMaxReadBytes`, and `RTWDiagRXAggregateLastErrorReadBytes`. The last refers to a structural parser failure, not the length of a USB error. Packets are not exported. DMA behavior, timing, and recovery budget remain unchanged.

Independent review passed 943 capacity checks, 2,105 recovery checks, and 258 ordering checks; the same test remained RED against build52 sources. The full controller compiled with the native SDK and a 2,048-byte stack-frame limit.

## Thermal-rate authority

`sendFrame` had overwritten the effective C2H rate with the descriptor's initial/fixed rate, including management, EAPOL, and rejected TX. The primary source updates tracking from the C2H rate. Per-packet overwriting was removed while preserving descriptors, PHY baseline, and C2H filters. All 55 Controller→PHY/thermal checks passed; the reproduced thermal delta stayed 3→3 rather than changing 3→4. Association identity of a C2H event still cannot be distinguished using only a local counter; no nonexistent epoch proof was invented in the protocol.

## AU TX AMPDU

Integration programs the peer's byte limit at 0x458 before accepting BA. An I/O failure refuses admission and blocks TX. The descriptor checks the current BA agreement by TID, authorization, HT, QoS/ACK, and fragmentation. `MAX_AGG_NUM` counts MPDU pairs and respects half the BA window; window size one remains nonaggregated. Density, host sequence, and checksum were verified. Management, EAPOL, and group frames are excluded from aggregation. DELBA and reassociation remove eligibility. The absence of AGG_BK matches the primary source; unaggregated bytes are preserved. Descriptor batching per USB transfer remains outside this fix.

Fixtures record five primary hashes and masks. Independent review passed 3,817 TX/BA, 2,025 codec, 2,470 BA/CCMP, and 178 lifecycle checks. Imported negotiation was exercised on both bands; this does not validate actual over-the-air aggregation.

## Integration and delivery

All 153/153 Release and 153/153 host ASan/UBSan tests passed. Independent reviews found no new blockers. Results from the extracted source, native products, signatures, and integrity were recorded in `VERIFICATION.json` and `NATIVE-VERIFICATION.json` in the delivery folder after this source was frozen. App46 and its protocol remained compatible.

Build53 had not yet been loaded on hardware in this report. Speed gains, BA negotiation, and stability needed measurement after the user compiled/replaced the kext. The agent did not install, reboot, modify EFI/router/SIP/AMFI, or use passwords. All files remained under the project folder.

Inventory and gates were preserved: 270 IDs, 263 primary matches, 262 eligible for 13 backends, plus restricted discovery 13b1:0043. RTL8814BU required qualified firmware/profile; DFS, WPA3, and unimplemented modes were not advertised. The real automatic 5 GHz router channel remained unknown; this build52 session was on 2.4 GHz/channel 6. The complete release was not yet qualified.
