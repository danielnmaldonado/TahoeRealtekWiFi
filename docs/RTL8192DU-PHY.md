# RTL8192DU radio implementation

The DU port uses the pinned mainline rtlwifi RTL8192D/U algorithms and actual
MAC/BB/AGC/RF tables. `src/RTL8192DUPHY-Upstream.json` records14 SHA256 inputs,
81 imported functions and their immutable primary URLs. Regenerate with:

```
python3 scripts/import-rtl8192du-phy.py --reference /path/to/pinned/rtlwifi
```

The importer verifies every input hash before writing generated files. The
adapter provides synchronous USB reads/writes/delays, typed ROM and private
state storage; no Linux runtime or fake successful RF callback is used.

Implemented operation is20MHz, legacy plus HT MCS0–15 with two RF paths, on
channels1–11,36/40/44/48 and149/153/157/161/165. The source algorithms program
actual ROM power groups, per-band RF/IMR/AGC,20MHz bandwidth, LC curves, IQK
matrices, PA bias and periodic thermal compensation. C/D/E cuts, USB Full/High
Speed and D/E internal versus external5GHz PA tables are distinguished.
Base-power values must be programmed; erased/out-of-range base groups are
rejected instead of authorizing the Linux base-power fallback. Source-defined
differential-power fallbacks remain. The test gives the three upper-band ROM
power groups different values and checks actual TXAGC writes for149/157/165.

The source LC deadline, IQK deadline and zero-matrix paths are hardened to
return failure. Fresh LC samples are required independently on each physical
path; one working path cannot mask a dead second path. IQK requires nonzero TX
and RX results on every used path. Imported thermal indices remain signed
until clipped, and RX-gain table indices are bounded. Disabled debug expressions
do not perform extra RF reads. After an I/O, short-transfer, wait or RFK fault,
ordinary I/O is fenced and only one bounded TX-pause cleanup is attempted per
HAL context. TX cannot resume on a failed or stale calibration.

Scan retunes reuse already measured LC curves.2GHz can use the validated IQK
cache for probes;5GHz scans stay paused and receive-only. AUTH performs the
operating-channel calibration before releasing TX. `phy_radio_ready` describes
calibrated operation, `phy_receive_ready` permits passive reception, and
`phy_tx_active` additionally requires an explicit successful TX activation.

DUALMAC_DUALPHY radio primitives preserve the source's fixed startup assignment
(MAC0=5GHz,MAC1=2.4GHz), one RF path and cross-PHY accesses. Cross-band moves on
one such context are rejected. `RTL8192DUUSB.c` supplies the actual vendor MAC1
aperture remapping, including MAC1→PHY0 pseudo-addresses. The kernel Ethernet
adapter now switches between both real data interfaces after abort/drain and
full shutdown, rebuilding ROM/boot/MAC/PHY state for the selected MAC. One
logical station address is programmed into the disconnected, paused MAC.
SMSP advertises two streams; DMDP advertises one stream. No DUALMAC_SINGLEPHY
override or simultaneous independent station is guessed.

The kernel adapter uses software sequence numbers, net80211 WPA2/CCMP/replay
and its controlled port. It advertises HT20 with receive reordering and sends
individual MPDUs;40MHz, VHT, USB RX aggregation and TX AMPDU are not advertised.
`RTL8192DUStation.c` implements real LLT, queues, MAC filtering, EDCA, DU H2C
rate/association/RSSI commands and bounded MAC/MCU/analog power-off. Software
crypto explicitly clears hardware security/CAM, and both MAC_ON masks are
checked before global shutdown.

These are host model and native compiler results. No RTL8192DU was attached
or programmed by these tests, and physical association/IP/throughput on DU
has not been certified. The existing attached adapters are other chipsets.
