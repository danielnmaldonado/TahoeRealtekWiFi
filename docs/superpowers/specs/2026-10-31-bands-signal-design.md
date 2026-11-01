# Band discovery and measured signal

The user reports stable RTL8814AU operation with build50, but only2.4GHz and a full menu-bar icon. The router is Wi-Fi6 with automatic channel/band operation; the actual5GHz channel is unknown. Continue directly in source; do not alter router, installed software, EFI, credentials or USB port.

Live build50 reports an authorized link and zero USB recovery errors. PHY RSSI varies between-44 and-42dBm. Native AppKit tests show distinct1/2/3-bar images and correct cache transitions; full bars are correct at these measurements. Keep thresholds, filtering and icon-only status-bar width. Show precise raw/filtered dBm and stale/unknown distinctions in the connected row, tooltip and accessibility label.

Explicit Refresh networks currently reads only cache when connected. It should request one real foreground scan, with duplicate/inflight/adapter-generation guards and profile reuse. Background polls remain read-only; do not create periodic disruptive scans. The existing foreground scan can briefly reassociate.

The current scan traverses all nine implemented non-DFS5GHz channels. The proven RF omission is the pinned-Linux channel153 HT20 NBI/CSI spur workaround; port it with independent final-register comparisons, including transitions away. Do not claim it explains an unknown automatic channel. Do not change speculative scan gain, channel permissions or unsupported DFS behavior.

Add scalar, credential-free scan telemetry to distinguish attempted/visited channels, successful scan-to-selection transitions, failed transitions, software-channel beacons and physical-channel observations/mismatches. Include cache band counts and current channel in IORegistry. Never infer a physical channel when the frame has no valid PHY channel. Preserve packet delivery behavior and existing USB recovery.

Deliver complete app46 and kext51 source with fresh native and host verification. Physical5GHz operation remains an evidence-dependent result, not an assertion from host tests.
