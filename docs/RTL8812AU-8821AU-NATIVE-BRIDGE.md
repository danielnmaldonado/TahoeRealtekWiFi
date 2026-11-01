# RTL8812AU / RTL8821AU native station bridge

`kext/RTW8812Backend.inc` and `RTW8812BackendMembers.inc` implement the actual
AU controller adapter. The exact USB family selects a typed AU core and its
matching, unmodified Realtek 8051 NIC firmware. There is no 8814 context cast or
legacy rtwn hardware initialization. `rtwn_start` is reused only as the existing
host management/Ethernet mbuf dispatcher. The imported net80211 stack retains
scanning, open-system authentication, association, WPA2 software CCMP,
MIC/replay checks and controlled-port authorization.

The hardware sequence is identify/decode actual OTP, native power flow,
256-entry LLT, matching paged firmware download with all ready flags, MAC/DMA,
actual board PHY/AGC/RF tables, operating-channel power and full per-chip RFK.
Management transmission starts only after successful calibration. RUN additionally
requires a valid negotiated peer rate mask/AID, real RA_INFO 0x40 and media-status H2C,
BSSID/AID/TSF/filter, basic-rate and EDCA programming. A generic RUN transition
never opens an RSN controlled port; unicast Ethernet additionally requires
`ni_port_valid` and a currently valid hardware/firmware state.

Both families use 40-byte TX and 24-byte RX descriptors. Firmware RA is used only
for associated unicast data. Management/EAPOL remain at fixed peer basic rates.
The TX descriptor RAID matches the exact firmware mask, including OFDM-only
2.4GHz peers. The actual QSEL-to-endpoint mapping preserves USB descriptor order
for 2/3/4 OUT pipes; full/high/super packet sizes are validated. Software-encrypted
MPDUs retain their IV/MIC and never request hardware crypto. USB completion is
never reported as an on-air ACK. The MAC RCR deliberately omits APP_FCS, so AU
receive translation does **not** trim another four bytes from the MPDU. HT/VHT traffic remains structurally decodable so later beacons in an aggregate
reach the shared receive policy. Associated high-rate admission checks the actual
operating mode, width and physical NSS, including the VHT20 MCS9 exclusion.
C2H events use the AU 8051 format and are rate telemetry only.

The operating PHY supports legacy20 and HT20/40 in 2.4 GHz, plus HT20/40
and VHT20/40/80 on non-DFS 5 GHz groups 36/40/44/48 and
149/153/157/161 (165 is 20 MHz). Each advertised channel/width is preflighted
against the actual board OTP, PG tables, NSS-specific differences and the
most restrictive imported regional limits. Wider channels program their real
center and primary sideband. VHT20 MCS9 is excluded for both one and two streams.
Missing required ROM values are not replaced by guessed radio power.

Attachment sets the actual channel HT/VHT flags before imported channel-init,
then corrects its inferred AX/HE mode: these chips have no HE PHY. The imported
HT/VHT negotiation retains WPA2/CCMP and peer basic-MCS checks. AUTH uses freshly
calibrated legacy20; RUN retunes and calibrates the final negotiated operating
profile before actual RA_INFO, rates, TSF/filters and EDCA. Firmware masks, RAID,
NSS, SGI, bandwidth and the descriptor agree. Management/EAPOL keep basic rates.
AMPDU requires an authorized WPA2 peer, protected QoS data and an agreed live BA;
its density, peer factor and negotiated BA window bound the descriptor. RX admits
BAR subtype 8 only during HT/VHT RUN for imported net80211 reorder, and INIT
clears both the BAR and data maps.

The periodic tick reads the actual RF thermal meter, uses the imported per-chip,
RFE and band swing tables plus fixed-point thermal EWMA, and adjusts BB swing and
TXAGC. A measured eight-degree change triggers the distinct full chip RFK. Cold
swing calculations use signed arithmetic, avoiding upstream unsigned wrap; each
physical path preserves its own ROM baseline. Invalid thermal samples retry
without fabricating fresh readings. Any actual I/O/RFK failure pauses TX and
invalidates readiness. The firmware heartbeat and associated-peer RSSI remain
separate actual checks. Firmware RA reports are telemetry, not packet ACKs.

Current board bounds remain 8812 RFE0..3 with two physical RF paths, or 8821 RFE0
with one; the unported 8812 one-chain restriction fails preflight. All 5 GHz scan
channels are passive. There is no DFS,160/80+80,HE or
external radio-certification claim.

Normal stop, failed-stage rollback and USB loss clear driver readiness and
always call generic INIT to invalidate host keys and controlled-port state.
Even a failed firmware disconnect mailbox invalidates calibrated/TX metadata.
Shutdown power and PHY cleanup are bounded/best effort, retaining original
errors. PHY storage is released only after the parent's drained teardown.

## Controller integration hooks

The parent owns the shared controller and family profile activation. These are
the precise hooks; adding USB IDs without this routing does not operate a radio.

1. Include `rtw/RTL8812AURuntime.h` (transitively AU bootstrap/MAC/PHY/data path)
   before the controller class, then `RTW8812BackendMembers.inc` inside its
   private section. Include `RTW8812Backend.inc` once in the controller CPP;
   it includes both distinct AU firmware arrays itself.
2. Permit the exact 8812AU/8821AU families during discovery/platform setup.
   Preserve actual OUT descriptor order, allow 2..4 OUT pipes and actual
   packet sizes 64/512/1024. Do not assign a 92C/8814 chip identity or call
   `rtwn_attach` for AU; dispatch to `attach8812()`.
3. Dispatch station enable/start to `start8812()`, stop/quiesce to `stop8812()`,
   and stateTransition to `state8812(ic,state,argument)`. Keep existing shared
   WPA recovery and profile bookkeeping around that call. AU watchdog/ioctl
   and task callbacks are set by attach.
4. Dispatch `sendFrame` early to `send8812Frame`. It consumes the mbuf on every
   path, releases the node on successful submit only, and uses the shared
   ledger/submit/completion implementation.
5. Dispatch `decodeUSBFrame` early to `decode8812USBFrame`. Route reportType 5
   to `handle8812C2H`, and AU RSSI to `sample8812RSSI`. Convert valid dBm to
   the existing node/signal-cache units; never call legacy rtwn RSSI parsing
   for AU. Retain shared CRC/ICV/software-decrypt/BSSID/net80211 checks and
   use `accepts8812ReceiveRate` for the negotiated high-rate admission.
6. Require `uses8812FirmwareRate()` in the AU branch of `authorized()`, in
   addition to all existing common predicates and `ni_port_valid`. Exclude
   AU from CCX/AMRR activation, sampling and `ni_txrate` mutation. Use AU
   firmware telemetry for observations only.
7. Route the calibration timer to `tick8812()` and rearm it at 2 seconds only
   on success. The AU tick verifies real 8051 ready flags and optionally sends
   actual thermal tracking/RFK and associated-AP RSSI; do not call legacy
   `rtwn_calib`. On failure latch core error and use shared quiesce.
8. Call `release8812Storage()` from the final platform-release path after
   callbacks/transfers have drained, including partial-attach failure.
   Derive IPC 5 GHz support from `supports8812Band(true)`; report the actual
   PHY TX/RX IQK masks if exposing an IQ-calibration property. Do not use
   legacy core RF masks as evidence for this independent hardware backend.
9. Route high-throughput BAR/RX-BA admission to `uses8812HighThroughput()`,
   receive rates to `accepts8812ReceiveRate(packet)` and TX-BA start to
   `ampdu8812TXStart`. These retain all shared BSSID/RSN/controlled-port checks.
10. Add `src/RTL8812AUBootstrap.c`, `RTL8812AUMAC.c` and
   `RTL8812AURuntime.c` to explicit CMake source lists and the new host test
   scripts to CTest. The Xcode generator already enumerates `src/*.c` and
   the new INC files; regenerate its project after integration.

## Primary evidence and verification

The PHY and bootstrap manifests pin Linuxrtw88 at
[f47d223627f80cb7a1eb14c585fe6cb35de5e984](https://github.com/torvalds/linux/tree/f47d223627f80cb7a1eb14c585fe6cb35de5e984/drivers/net/wireless/realtek/rtw88).
Runtime H2C/rates/RSSI come from `fw.c/fw.h`, `rtw88xxa.c` and the separate
`rtw8812a.c`/`rtw8821a.c` gain rules. CCK high-power reporting reads the Jaguar
register 0x804 bit 16, rather than the older 92C 0xA04 layout. The firmware manifest
pins actual bytes by SHA256 and preserves Realtek's redistribution notice;
no moving-branch binary is identified by an invented commit.

The cold OTP grant also matches FreeBSD's shared
[`rtwn_efuse_switch_power`](https://github.com/freebsd/freebsd-src/blob/685dc743dc3b5645e34836464128e1c0558b404b/sys/dev/rtwn/if_rtwn_efuse.c#L53):
CF69 access plus ELDR/LOADER/ANA8M clocks, then guaranteed gate closure.
A full firmware/MAC power-on is not assumed as a prerequisite for that read.
The earlier ROM-layout cross-check retains its BSD-2-Clause notice separately.

Run:

```
python3 scripts/test-rtl8812au-bootstrap.py
python3 scripts/test-rtl8812au-phy.py
python3 scripts/test-rtl8812au-runtime.py
python3 scripts/test-rtl8812au-modes.py
python3 scripts/test-rtl8812au-negotiation.py
python3 scripts/test-rtl8812au-backend.py
python3 scripts/test-rtl8821au-coex.py
python3 scripts/check-rtl8812au-backend-kdk.py
```

The actual backend method harness covers both exact families and both bands,
HT/VHT20/40/80, BA-window/density bounds, BAR map lifetime, encrypted-QoS
boundaries, negotiated RX admission,
power/LLT/MAC/FW/PHY/RFK/host ordering, failed startup stages, 19 AUTH and 18 RUN
register failure positions, failed disconnect and generic-state rollback,
controlled-port/EAPOL gating, TX endpoint/USB padding, mbuf/node ownership,
intact RX payload/FCS policy, foreign HT followed by a beacon, C2H, RSSI monitor
and lost firmware readiness. The separate negotiation test executes the verbatim imported channel-init, HT
and VHT functions with real AU power-derived channel flags and final-profile/RA
building: NODE_HT/VHT is never preset. It includes CCMP, rejected TKIP and user
NOVHT. The PHY suites sweep all legacy and wider-tune/RFK/thermal I/O positions,
including failed measured RFK and cold signed swing. EDCA preserves reserved bits
and rejects oversized TXOP before any bus access.
The RF/USB scheduling boundaries are isolated;
real boot/MAC/runtime/PHY model suites exercise their actual register logic.
The native check compiles the complete adapter against the real KDK/IOUSB and
imported net80211 declarations, using a temporary copy of the complete controller. Optional declaration
overlays support early integration; the delivered shared controller already
contains the AU hooks. It makes no hardware calls.

These model and compile results are **not physical connection, throughput or
radio certification results**. AU hardware must still be exercised for cold
load, scan, WPA2 key exchange/traffic on both bands, unplug/replug and two-device
independence after controller integration. No profile should be labelled
hardware-validated based solely on these tests.

Peer SMPS is enforced independently of raw advertised MCS: static, dynamic and
reserved values bound firmware TX to one stream until explicit multi-stream
wake/protection exists. This bounds both HT MCS and VHT NSS, preserving the
physical receive capacity. Disabled SMPS retains real board/peer NSS limits.


Observed-BluetoothRTL8821AU now uses the real typed coex policy and a live
PHY TX lease; see `RTL8821AU-COEX.md`. Scan/authentication/media/band events,
BT-info/MP C2H, the pre-RUN watchdog and stop/removal are connected. Lease
identity changes flush host keys/port state and forget without register or
mailbox I/O. BT-active8812AU is still refused because the pinned8812 descriptor
has no coex backend. The1T1R8812 OTP path uses one stream for advertised
capabilities, negotiated firmware rates, RX admission, power and RFK.

Every AU board, including no-Bluetooth hardware, has a retained positive-identify
binding independent of the coex context. State/INIT/stop and the actual EDCA
callback require ownership and live MCU health before MAC/mailbox operations.
TX/rate/BA/RX consumers use the pure ownership predicate; they add no USB health
read to the per-frame path. Typed bus, waiter, semantic board/ROM, family/USB
layout and actual provider/pipe mutations invalidate the binding. Tests cover
988 ownership vectors, 252 health vectors and normal stop/restart.
