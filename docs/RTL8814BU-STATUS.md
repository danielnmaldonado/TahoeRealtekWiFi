# RTL8814BU — source status

Design and macOS adaptation: MaLd0n. The station backend remains disabled.

## Components present

HALMAC/USB is specific to the B14 generation: positive SYS_CFG2=0x11 identification,
1024-byte OTP, 16-byte TX descriptor bodies with IEs and 24-byte RX descriptors,
a USB/reserved-page/DDMA/MCU firmware flow and context ownership checks. It does
not use RTL8814AU HAL, descriptors or RF tables.

PHY/RFK imports 159 functions from the revision pinned in
`src/RTW8814BPHY-Upstream.json`: BB/AGC/RFK-init tables, four RF paths and
two synthesizers, DACK, RTC/LCK, TXGAPK, IQK and DPK. External-PA bypasses
follow the primary source. Calibration contexts belong to each object.
A private HAL lease limits the PHY to the current firmware session; power
transitions, failures and closure invalidate this relationship without USB I/O.

Sources, headers, generators and notices are included in the source tree.
Manifests record the primary commit and local adaptations; licenses are preserved.

## What still prevents enabling station mode

- Qualified, licensed USB NIC firmware and station protocol. The parser accepting
  a header/ABI11 does not establish operational compatibility. ABI15 router
  images and AU images are distinct; no proprietary firmware extracted from a
  Windows driver is distributed in this package.
- A real factory/regulatory USB profile, including generation of RFE, package,
  front-end and crystal metadata from OTP. The power plan received by the PHY
  is validated for representation; it is not derived from an already qualified
  factory/regulatory USB profile.
- Factory TSSI/thermal data reading exists. TSSI DE/DCK programming, generic
  TSSI/thermal-swing runtime and CFO tracking have not been ported.
  `track` currently performs only DPK tracking from the primary source.
- Station integration and physical validation: association/WPA2, DHCP, traffic,
  reconnection, sleep, USB removal and different board revisions.

The component's geometry is limited to non-DFS HT20: channels 1–11, 36/40/44/48
and 149/153/157/161/165. Implementing this component does not enable a B14 station
personality or change the catalog to advertise connection capability.

## Verification

Host tests exercise the imported algorithms with modeled DSP/registers, I/O
failures and state restoration, using ASan/UBSan. The real HAL has separate
firmware, OTP, codec and lease-lifecycle tests.
These results are neither RF measurements nor hardware throughput validation.
The native build uses real kernel headers and checks stack frames.
