# RTL8192FU native station backend

`RTL8192FU.c` owns a dedicated opaque FU context. It imports 57 checksum-pinned
Linux rtl8xxxu functions and the actual FU MAC/BB/AGC/radio-A/radio-B tables.
There is no EU/AU/8814 hardware identity or context cast. The separate wire-only
FU specialization retains its BSD-2-Clause codec notice; radio imports select
GPL-2.0-only with the complete `src/RTL8192FU-LICENSE.txt`.

The ROM is the actual FU 512-byte logical layout: OTP ID8129, two42-byte power
blocks at10/3a, channel planB8, crystalB9, thermalBA, RFECa, VID/PID100/102 and
MAC107. Physical OTP is read with a bounded CF69 gate and guaranteed closure;
Kfree consumes captured physical1EA/1EC/1EE bytes after the gate closes. Missing
required ROM values and unsupported board/operating profiles fail preflight.
Initial tested-in-model RFE branches are1/5 and the explicit external-PA
branches7/8/9/12; no unprogrammed RFE value becomes a guessed board.

Boot executes the exact FU power flow, USB queues/DMA, automatic LLT, actual
FU32-byte-header NIC firmware and paged download at **4000**, source feature
report, MAC/BB/AGC/RF and measured two-path LCK/IQK. Firmware7.3 is embedded as
unmodified bytes with SHA256 and the full Realtek redistribution notice. All
checksum/ready flags are verified. MACID/no-link state is set explicitly:
Linux performs that step outside its generic init function. The source RF tables
start on channel7; the adapter explicitly retunes and recalibrates its declared
initial channel1. The first crystal setting is forced even when its valid ROM
value is zero. No synthetic calibrated/ready bit enables transmission.

The imported IQK helpers now require their real report-ready registers. A
zero report followed by source fallback coefficients cannot count as successful
calibration. Measured per-candidate TX/RX masks and LCK timeout determine actual
readiness. Startup, tune and runtime I/O failures keep TX paused. Cold/failed
cleanup retains the original error, clears host readiness and uses the actual
FU shutdown flow. USB disappearance uses a separate host-only forget operation.

Operating profiles are legacy20 and HT20/40, two physical paths, channels1..11
with valid secondary-channel geometry. OTP arithmetic is checked before source
AGC packing, with the actual vendor FU maximum index63. The current power path
is the upstream rtl8xxxu calibrated flat rate-section implementation; it does
not claim imported vendor per-rate PG/regulatory or thermal swing correction.
The periodic runtime verifies live8051 flags and measures the source six-bit
thermal meter. Unknown/invalid samples never become fresh temperature data.
There is no 5GHz/VHT/HE/DFS or BT coexistence scheduler for this2.4GHz chip.

The kernel adapter uses the existing net80211 scan/authentication/association,
WPA2 software CCMP, MIC/replay checks and controlled port. RUN requires final
negotiated HT/width/SGI/rates, source0x40 firmware RA and media H2C, matching RAID
and TX descriptor, BSSID/AID/TSF/response rates and masked EDCA.
The FU firmware ABI leaves reserved RA bits clear and describes its remote AP
with role2 (media21/20). Station-port0 uses MACID0 for both unicast and broadcast;
AU firmware defaults are not substituted for these FU command/descriptor fields. EAPOL uses fixed
basic rates before port authorization. Static/dynamic/reserved peer SMPS
bounds firmware TX to one stream; disabled SMPS permits its real two-stream
MCS. Receive paths retain their independent two-stream capacity. Legacy WMM/QoS works independently of HT.
AMPDU requires an authorized protected QoS peer with a live BA and honors its
factor, density and window, with source hardware RTS/CTS for operational
aggregation. RX admits BAR only for HT RUN. C2H is telemetry,
never an invented on-air packet ACK. RCR omits FCS; the MPDU is not trimmed again.
Jaguar2 PHY pages use the FU's actual old/new CCK AGC and OFDM power formats.

## Parent controller integration

The parent owns shared controller routing, CMake and catalog activation.

1. Include `rtw/RTL8192FU.h` before the controller class, place
   `RTW8192FUBackendMembers.inc` in its private section and include
   `RTW8192FUBackend.inc` once in the controller CPP.
2. Permit exact `RTW_WIFI_RTL8192FU` discovery with1..3 OUT descriptors in
   actual order and full/high packet sizes64/512. Call `attach8192FU()`.
   Do not call legacy `rtwn_attach` or assign an EU chip value.
3. Route enable/start, stop/quiesce and state transitions to `start8192FU`,
   `stop8192FU`, `state8192FU`; preserve shared WPA recovery/bookkeeping.
4. Route TX to `send8192FUFrame`, RX structural decode to
   `decode8192FUUSBFrame`, reportType5 to `handle8192FUC2H`, RSSI to
   `sample8192FURSSI`, timer to `tick8192FU` with the normal bounded rearm.
5. Require `uses8192FUFirmwareRate` in authorization; exclude FU from legacy
   CCX/AMRR and legacy hardware calibration/channel/RSSI operations.
   The controlled port and all shared security checks remain mandatory.
6. Use `accepts8192FUReceiveRate` for negotiated RX admission,
   `uses8192FUHighThroughput` for BAR/RX-BA admission and
   `ampdu8192FUTXStart` for TX-BA. No 92E hardware BA register routine is used.
7. Release `release8192FUStorage` only after transfer/callback drain, including
   partial attach. `supports8192FUBand(true)` is always false; derive2.4GHz
   capability from its actual ROM. Read typed `rtw_r92fu_status_read(fu_,...)`
   if exposing measured RFK/firmware diagnostic properties.
8. Add `src/RTL8192FU.c`, `src/RTL8192FUDataPath.c` and the three test runners
   below to explicit CMake lists; regenerate Xcode after integration.

```
python3 scripts/test-rtl8192fu.py
python3 scripts/test-rtl8192fu-backend.py
python3 scripts/test-rtl8192fu-negotiation.py
python3 scripts/check-rtl8192fu-backend-kdk.py
```

The real portable driver runs over a USB/RF boundary model. Failure sweeps cover
every identify/boot/HT40-RFK transfer both before effects and after a failed
write has already taken effect. Missing firmware checksum/ready, LLT, LCK,
IQK-ready and zero compensation measurements fail closed; actual254-byte
firmware chunks stay within their4KiB page. The wire suite additionally runs
149,933 descriptor/mutation/fuzz checks. The actual backend harness links this real
radio implementation, isolating only OS scheduling, mbufs and host crypto. It
covers1/2/3 OUT pipes, HT20/40, two independent controllers, protected legacy
QoS, EAPOL/controlled-port admission, BA bounds, intact RX and telemetry, lost
firmware readiness and every AUTH/RUN I/O failure. A separate runner executes
verbatim imported channel-init/HT negotiation using actual FU power-derived
flags, then the real operating-profile and RA builders. NODE_HT is never preset
in that negotiation test. Native checks use the real KDK and IOUSB headers.

These source/model/compile results do not establish physical connection,
throughput, unplug/sleep reliability or radio certification. All four catalog
IDs need the corresponding hardware for those claims; an ID table entry alone
is not evidence. No hardware operation, installation or EFI change is part of
these test/generator commands.
