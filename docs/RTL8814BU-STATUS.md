# RTL8814BU — source status

macOS development and adaptation: MaLd0n. The station backend remains disabled.

## Available components

The HALMAC/USB implementation is specific to the B14 generation: positive `SYS_CFG2=0x11` identification, 1,024-byte OTP, TX descriptors with 16-byte bodies plus IEs, 24-byte RX descriptors, USB/reserved-page/DDMA/MCU firmware flow, and context-ownership checks. It does not reuse RTL8814AU HAL, descriptors, or RF tables.

The PHY/RFK imports 159 functions from the revision pinned in `src/RTW8814BPHY-Upstream.json`: BB/AGC/RFK-init tables, four RF paths and two synthesizers, DACK, RTC/LCK, TXGAPK, IQK, and DPK. External PA bypasses follow the primary source. Calibration contexts are owned by individual objects. A private HAL lease binds PHY operations to the current firmware session; power transitions, failures, and shutdown invalidate that relationship without USB I/O.

Sources, headers, generators, and notices are included. Manifests document the primary commit and local adaptations, and the licenses are preserved.

## Remaining station-mode blockers

- **Licensed USB NIC firmware and qualified station protocol.** Accepting an ABI11 header in the parser does not establish operational compatibility. Router ABI15 and AU firmware images differ; no proprietary firmware extracted from a Windows driver is distributed in this package.
- **Qualified factory/regulatory USB profile.** This requires RFE, package, front-end, and crystal metadata derived from OTP. The PHY validates the representation of its power plan, but that plan does not yet derive from a qualified factory/regulatory USB profile.
- **Incomplete TSSI and tracking support.** Factory TSSI/thermal data can be read. TSSI DE/DCK programming, generic runtime TSSI/thermal swing, and CFO tracking have not been ported. `track` currently performs only primary-source DPK tracking.
- **Station integration and physical validation.** Association/WPA2, DHCP, traffic, reconnection, sleep, USB removal, and different board revisions require testing.

Component channel geometry is limited to non-DFS HT20: channels 1–11, 36/40/44/48, and 149/153/157/161/165. This implementation does not enable a B14 station personality or modify the catalog to advertise connectivity.

## Verification

Host tests exercise imported algorithms against modeled DSP/registers, I/O failures, and state restoration under ASan/UBSan. The real HAL has separate firmware, OTP, codec, and lease-lifecycle tests. Those results are not RF measurements or hardware throughput validation. Native compilation uses actual kernel headers and checks stack frames.
