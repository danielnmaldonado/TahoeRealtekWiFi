# B/C kernel integration hooks

Owned implementation: `kext/RTW88Backend.inc` and
`kext/RTW88BackendMembers.inc`; codecs: `src/RTW88RX.c` with its generated
RSSI include. This is actual attach/start/MLME/TX/RX/stop code, not a profile
flag. Physical B/C WPA2/IP/throughput remains untested.

Bluetooth boards now use the typed `RTW88Coex` runtime and controller timer/
state/CCX/SMPS integration described in `docs/RTW88-COEX.md`. Firmware H2C
retains its single shared mailbox cursor. No Bluetooth board is promoted
through a missing or failed policy context.

Controller header: include `RTW88PHYOperating.h`, `RTW88Station.h`,
`RTW88RX.h`, `RTW88USBDataPath.h`; include the members file in the private
section. Controller implementation: include `RTW88Backend.inc` once.

Controller routing, before any legacy rtwn hardware call:

- Platform attach: `is88Family()` → `attach88()`.
- Enable/init: `start88()`; stop/quiesce/suspend/unplug: `stop88()`.
- Final platform release, after gated work/USB completions drain:
  `release88Storage()`.
- `stateTransition`: `is88Family()` → `state88(ic,state,arg)`.
- `sendFrame`: `is88Family()` → `send88Frame(packet,node)`.
- The carrier/authorization predicate must require `uses88FirmwareRate()`
  for B/C as well as the existing net80211 controlled-port decision. RUN
  alone is never authorization.
- Calibration timer: `tick88()`; on error latch `core_.core_error` and
  quiesce; otherwise reschedule in2 seconds. Preserve work-loop serialization.
- `decodeUSBFrame`: route B/C to `decode88USBFrame`. C2H report type6 routes
  to `handle88C2H`. Ordinary decoded MPDUs have exactly one FCS removed;
  firmware event lengths are unchanged.
- Admit B/C legacy/HT rates only:0..11,12..(11+8*actual TX/RX stream cap).
  Structurally decode unsupported foreign rates, then drop that frame so a
  later beacon in the same aggregate survives.
- PHY RSSI: route B/C to `sample88RSSI(p)` and update `rxi_rssi`/average
  using the existing signal-validity helpers. Do not use legacy Jaguar PHY
  interpretation for B/C page0/page1 reports.
- Retain CRC/ICV,MAC/BSSID frame admission,hardware-decrypt rejection,
  net80211 software CCMP/replay and controlled-port delivery checks.
- `ampduRXStart`: accept B/C only when `uses88FirmwareRate()`,
  `r88Rates_.rates.ht`, actual controlled-port authorization and the normal
  peer/TID/window checks pass. RX reorder remains in net80211.
  Keep TX AMPDU disabled for B/C.
- AMRR/legacy TX-report selection must exclude B/C,whose C2H RA messages
  contain rate telemetry rather than packet ACK/retry outcomes.
- Status/performance reporting must use actual firmware feature NSS and
  negotiated HT20 rates,with max72/144Mbps SGI PHY ceilings. Do not expose
  VHT/40/80 merely because the lower PHY can tune those widths.

The backend owns typed B/C contexts and64KiB operating scratch plus bounded
FW/OTP scratch. OTP is read with running firmware before interface attach,
then the chip powers down. Start reboots, re-reads the MAC and actual C2H19
hardware features, and requires consistency with the attached interface.
Physical RF count remains distinct from feature-limited active antennas/NSS.
TRX/power/RA use the actual caps,including1-antenna8822C with physical2-path
DAC calibration. No chip-family maximum is substituted for a feature report.

Firmware bootstrap uses the exact verified profile and family. Reserved
firmware packets use queue16; IQK/startup packets use queue19. Both synchronous
bulk paths follow the MAC queue mapping and apply explicit zero padding when
aligned to MPS. The host interface must be open and endpoints verified before
attach. No legacy firmware loader,reset or EEPROM programming is used.

Start order: typed boot/FW/OTP/features → MAC → PHY tables → operating PHY
parameters → software-security station initialization → RX-only initial tune
→ start USB RX → general/PHYDM H2C startup info → net80211 SCAN.
Active scans/AUTH require actual IQK/calibrated channel state. Passive5GHz
SCAN stays RX-only/TX-paused. Radio callbacks check exact channel/center/
width/index, permitted channel set, MAC/PHY/FW readiness and calibration.

Mailbox commands share `firmware.h2c_box`. Startup and IQK share the separate
`r88PacketSequence_` through `radio_set_h2c_sequence`; that pointer remains
valid until radio invalidation. A firmware restart resets it. Stop always
flushes host keys/port state,including missing USB or sleep,then invalidates
radio and cancels work/transfers; it never issues hardware requests after
USB absence/sleep has been observed.

Root build registration: add `src/RTW88RX.c` to core/Xcode, the
`rtw88_rx_tests.cpp` executable, `scripts/test-rtw88-backend.py`, and Darwin
`scripts/check-rtw88-backend-native.py`. Existing operating/boot tests cover
shared packet sequencing, hardware feature caps and single-active-path RFK.
The isolated backend harness executes production controller methods with
mocked host/USB/RF boundaries, including 24 AUTH and 37 RUN I/O failures,
HT20 masks, protected TX/port rejection, TX48 padding/node ownership,
FCS/SHIFT/PHY/C2H decoding, startup order, stale tuples and stop/unplug/sleep.
It is not an AP/USB hardware emulator or physical throughput validation.

The watchdog uses the 3081 firmware readiness format from pinned Linux
`rtw88/reg.h`: `(MCUFW_CTRL & 0xcfff) == 0xc078`, including both checksum/
download/initialization authorities and excluding the two CPU clock bits.
The legacy 8051 `0xc6` format is rejected. RUN applies peer SMPS to the TX
MCS mask and stream count; static, dynamic and reserved SMPS use one stream
until a per-packet multi-stream wake/protection path exists. Board RX paths
and the peer's original RX MCS remain intact. EDCA consumes AP parameters
only when both global QoS and peer `NODE_QOS` are present; non-WMM peers use
the imported stack's band/rate defaults, even after a previous WMM AP.

The harness executes the actual watchdog, RUN, EDCA and TX methods, with no
EDCA stub. It imports the default EDCA table directly from net80211 and
checks 99 firmware/clock/stale-state/I/O cases, 288 SMPS/SGI/stream/RA/protected
TX/C2H/RX cases, and 192 EDCA/default/stale-AP/slot cases across 8821CU, 8822BU
and 8822CU on channels 6, 36 and 165. `--case firmware|smps|edca` and
`--backend-source PATH` support independent failure reproduction against
an earlier backend without replacing the working source.


Regression for actual stack negotiation: `scripts/test-realtek-imported-ht.py`
executes the full imported `ieee80211_channel_init`, `ieee80211_ht_negotiate`
and width-negotiation functions using production channel/capability expressions.
The B/C channels now include `IEEE80211_CHAN_HT20` before interface attachment.
No fixture pre-sets `NODE_HT`. Both bands negotiate HT20/SGI with CCMP; TKIP
and unsupported mandatory MCS remain rejected. The test also covers8814 once
its parent-owned channel flag correction is applied.
