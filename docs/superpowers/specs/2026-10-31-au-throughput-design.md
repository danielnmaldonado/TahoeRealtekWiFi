# RTL8814AU throughput correctness

User moved the adapter and authorized continuing work directly in source.
Current AU2357:0106@14400000 is direct USB2/480Mb/s, WPA2 authorized on en3,
channel6, about -44dBm. Only AU is currently attached. Loaded user-built52.
Idle gateway ping5/5,2.479–5.490ms. Bounded HTTP200 transfers bound to en3 measured
8.73Mb/s download4MiB and8.93Mb/s upload1MiB. These are short WAN samples,
not peak capacity. No new published USB recovery errors; download added53
truncated and53 malformed RX aggregates, with RX mostly MCS15.

Correct three primary-proven deficiencies, with actual-code RED→GREEN:
1. AU DMA aggregation threshold is20KiB (vendor0x2005), while native reads16KiB.
   Use32KiB backing buffers and requests for AU only, matching pinned Linux;
   retain16KiB for other families. All startup/refill/recovery reads agree.
   Real codec split fixture20,200bytes/13frames currently loses3frames across
   16KiB boundary;32KiB whole-read preserves all13. Record scalar read lengths
   if needed to qualify the next physical run, never payloads.
2. TX dispatch overwrites PHY thermal-rate state with descriptor initial/fixed
   rates even after accepted firmware C2H. Preserve primary authority: only PHY
   baseline and valid C2H set tracking rate. Descriptor behavior remains intact.
   Real thermal test demonstrates delta change when the state is overwritten.
3. AU TXAMPDU is missing. Port the complete bounded mechanism: peer maxAMPDU
   length cap (vendor notes low speed without it), negotiated per-TID BA,
   AGG_EN/density/max_agg wire fields, QoS/sequence/checksum and exclusions,
   start/stop/reset/epoch cleanup. Enable cap only after these contracts pass.
   USB TX batching is separate, absent in EU too and outside this correction.

User scope already authorizes implementation; no new design approval needed.
Use pinned primary sources and independent wire/behavior checks. Do not alter
firmware blobs, channels, modes, USB timings or recovery budgets. Do not install,
change EFI/security settings, force load, reuse passwords or move hardware.
Deliver complete source for app+kext and verified products after integration.
New code will need physical validation after user compiles/loads it; the USB
root cause on previous hub path remains unproven. RTL8814BU/DFS gates persist.

Alternatives considered: shrinking DMA threshold would diverge from the primary
MAC profile; global32KiB would change unrelated backends; reassembling arbitrary
USB tails adds state without evidence it is needed when correctly sized reads
are used. Per-family primary sizing is the minimal chosen implementation.
