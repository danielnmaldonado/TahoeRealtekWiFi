# RTL8723BU native station port

The new typed RTL8723BU core and kernel adapter implement the Linux rtl8xxxu
station path for verified USB IDs **0bda:b720** and **7392:a611**. The
unverified **20f4:108a** is rejected before I/O. The implemented operating
scope is 2.4 GHz channels 1..11, HT20, one RF path/stream, software WPA2 CCMP
and the imported net80211 controlled port. HT40, VHT and TX AMPDU are capped
until their complete MLME/RA paths are implemented.

This is an implemented source path with register/USB/OS boundary tests and
real MacKernelSDK compilation. Connection, traffic and concurrent Bluetooth
validation still require an RTL8723BU adapter; these tests do not substitute
for physical hardware validation.

## Primary source and firmware

`src/RTL8723BU-Upstream.json` pins SHA256 and primary URLs for Linux rtl8xxxu
`8723b.c`, `8723a.c`, `core.c`, `rtl8xxxu.h` and `regs.h` at commit
`0e135910deec43ffe97eca0b6af1f8f847285b59`. The generator verifies those hashes
before importing 53 actual power/PHY/RFK/firmware/coexistence functions and
the chip's MAC/BB/AGC/RF tables. GPL-2.0-only attribution and the repository
license are retained. No AU or B/C device context is cast or reused.

The unchanged NIC firmware is Linux-firmware **35.0**, signature **0x5301**,
32,108 bytes, from official tag `20250211`; its SHA256, URL and redistribution
notice are in `firmware/rtl8723bu-firmware-provenance.json`. Raw bytes and a
reproducible kernel include are supplied, so normal compilation needs no
network access.

## Hardware behavior and gates

* Identify checks real silicon/test-chip and WiFi/BT multi-function registers.
  OTP access selects the WiFi bank, disables programming voltage, bounds each
  header/address and restores the bank/access grant. The USB MAC comes from
  the actual 0x107 factory offset. Grouped TX power deliberately uses the
  chip's **B** factory indexes, matching `rtl8723b_set_tx_power`, with signed
  deltas and safe 0..63 clamping.
* Cold power, queue/SIE/RQPN, page boundary, 8051 reset, paged firmware,
  hardware checksum/ready, MAC/DMA, PHY/AGC/RF tables and crystal trim execute
  before the adapter can request association. Timeout and transport failures
  keep calibration invalid; firmware-download cleanup is bounded and never
  retries a failed payload transfer.
* Channel changes program the real HT20 RF/BB registers and per-group TXAGC.
  Actual LCK, TX/RX IQK coefficients and channel/width readback are required.
  RFK keeps TX paused: the station MAC writes RRSR before enabling management
  traffic; active scans restore the primary scan response bitmap after leaving an
  OFDM-only basic-rate AP. Receive-only passive scans clear media/BSSID filtering and remain
  paused without claiming calibration.
* Firmware station RA, peer AP role and TX descriptors consistently use
  **MACID2**. RX24/TX40 descriptors, USB padding, explicit FCS removal,
  software sequence/CCMP, real rate reports and chip-specific CCK/OFDM RSSI
  are implemented. HT RUN admits BAR to the existing software RX reorder;
  parent routing must apply its established peer/address/session filter.
* Real single-antenna WLAN/BT ownership, PTA, S0/S1 selection, H2C grant,
  TDMA and coexistence tables are imported. Runtime BT inquiry/profile events
  drive those actual tables. A primary A2DP-only flag typo is corrected so
  HID+A2DP keeps both profiles and reaches the intended coexistence table.
  A three-endpoint VO mapping typo is also corrected to follow programmed
  VOQ rather than VIQ. Both corrections have behavior regressions.
* Core thermal sensing reads the actual running meter. No invented host
  temperature/power slope or ROM-derived calibration readiness is added.

## Parent integration

The shared Controller/CMake/catalog files intentionally remain parent-owned.

1. Add `src/RTL8723BU.c` and `src/RTL8723BUDataPath.c` to host/kernel builds;
   their private `.inc`/header files and firmware include are supplied.
2. Include `rtw/RTL8723BUDataPath.h`, class members
   `RTW8723BUBackendMembers.inc`, and `RTW8723BUBackend.inc` in the controller.
3. Dispatch family `RTW_WIFI_RTL8723BU` to `attach8723`, `start8723`,
   `stop8723`, `release8723Storage`, `state8723`, `send8723Frame` and
   `tick8723`. Route band/channel capability to `supports8723Band` and
   `permits8723Channel`. Actual RF readiness is `r23RadioState(...)=2`.
4. Route RX through `decode8723USBFrame`, RSSI through `sample8723RSSI`,
   and report type **7** through `handle8723C2H`. Include this family in the
   existing HT BAR/session/address filtering and software RX reorder gates.
5. Use `uses8723FirmwareRate` in controlled-port/telemetry policy. Actual
   rate/SGI are `r23Core_.tx_rate/tx_sgi`; RF count is one. Do not infer ACKs
   from USB completion or rate reports. Allow this family's 1..3 bulk OUT
   endpoints and 64/512-byte packets without relaxing other families.
6. Register the five checks below and update catalog/backend capability only
   when the new adapter is actually linked. The common app/control ABI and
   software WPA2 key handling do not change.

```
python3 scripts/test-rtl8723bu-core.py --out /tmp/rtl8723bu-core
python3 scripts/test-rtl8723bu-datapath.py --out /tmp/rtl8723bu-datapath
python3 scripts/test-rtl8723bu-backend.py --out /tmp/rtl8723bu-backend
python3 scripts/test-rtl8723bu-imported-ht.py
python3 scripts/check-rtl8723bu-backend-native.py
```

The controller harness executes the actual bootstrap and actual core, rather
than substituting successful firmware/PHY stubs. Coverage includes every cold
boot/OTP/AUTH/RUN I/O failure position, timeout/cleanup, ROM rejection without
I/O, signed power bounds, MACID2 wire format, CCMP/EAPOL/controlled-port gates,
RRSR order, passive scanning, suspend/unplug/stop and firmware rate/RSSI.
Imported HT tests run real `channel_init`, AUTO selection and HT negotiation
from production channel/capability expressions; NODE_HT is never preset in
that regression. TKIP and unsupported mandatory MCS reject HT as expected.

Final native-host verification passed 19,721 core checks, 152,235 actual
controller/core checks, 21 wire-boundary checks, real imported HT negotiation,
and native MacKernelSDK compilation of the complete adapter plus both C
modules with a 2,048-byte stack-frame limit. Every 819 cold-boot, 347 OTP,
522 AUTH and 28 RUN I/O failure position was exercised. Source-table and
firmware regeneration reproduced the supplied files byte for byte.

Optional regeneration:

```
python3 scripts/generate-rtl8723bu.py --source /path/to/pinned/linux/rtl8xxxu
python3 scripts/generate-rtl8723bu-firmware.py
```

Subsequent station review corrections and the per-chip EDCA width audit are
recorded in [RTL8723BU-REVIEW-FIXES.md](RTL8723BU-REVIEW-FIXES.md).
