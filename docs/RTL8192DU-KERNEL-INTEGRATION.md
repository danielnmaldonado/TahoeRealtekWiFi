# DU kernel station integration hooks

`kext/RTW8192DUBackend.inc` and `RTW8192DUBackendMembers.inc` provide the actual
SINGLEMAC_SINGLEPHY and DUALMAC_DUALPHY station adapter. The shared controller/catalog and CMake
remain owned by the integrating task. No personality or family capability is
enabled by the DU module itself.

Add the DU Station, USB and DataPath headers at controller-header file scope,
then the members include in its private section. Include the existing parent
`RTL8192DUFirmware.inc` before the backend implementation in the controller
translation unit. Add `RTL8192DUPHY.c`, `RTL8192DUUSB.c` and
`RTL8192DUStation.c` alongside the already registered Boot/Efuse/DataPath units.
Generated `.inc` files are source dependencies, not separate translation units.

Route DU through the following actual methods:

- Attach/start/stop: `attach8192DU`, `start8192DU`, `stop8192DU`, rather than the
  C/E legacy hardware initializer. `rtwn_start` is reused only as the existing
  host management/mbuf dispatcher. Generic net80211 detach remains applicable.
- State: `state8192DU`. INIT always calls the generic key/port/queue flush even
  with a poisoned USB HAL. RUN requires actual channel/RFK, DU RA programming
  and firmware JOIN; it never opens the host controlled port itself.
- TX: dispatch `sendFrame` immediately to `send8192DUFrame`. It owns/freezes the
  mbuf like the existing path and releases the node only on success. It checks
  calibrated active TX, the selected BSS and controlled port, permits EAPOL
  during RSN, invokes software CCMP, preserves sequence/TID, honors protection
  and ACM, emits the real DU32-byte descriptor and8-byte USB boundary offset.
  It requests no CCX report or TX AMPDU. Do not run a C/E descriptor first.
- RX: dispatch `decodeUSBFrame` to `decode8192DUUSBFrame`. DU consumes exactly
  one USB frame, validates computed FCS and already strips FCS. Do not apply a
  CU aggregate stride or strip another four bytes. Normalize legacy bandwidth;
  unimplemented HT40 is rejected. Include DU in the permitted raw MCS0–15
  receive policy; the generic legacy `rate>=12` rejection must not catch DU.
  BAR eligibility additionally requires authorization and a live HT rate
  context. The adapter installs its own receive-BA callbacks.
- Signal: call `sample8192DURSSI` instead of the CU RSSI parser. It uses the
  source DU OFDM -106 formula, board CCK mode and AP-scoped firmware PWDB.
  Convert its nonzero valid dBm result with the existing node-signal helper.
- Calibration: call `tick8192DU` at2-second intervals, quiesce on errors, and
  reschedule only during a live RUN. Do not call a C/E thermal routine.
- Authorization: require `uses8192DUFirmwareRate()` in addition to the shared
  lifecycle, BSS and `ni_port_valid` checks. The firmware-connected bit is not
  permission for Ethernet traffic before the WPA2 controlled port opens.
- Diagnostics: DU firmware RA replaces host CCX/AMRR; exclude DU from the old
  feedback state. Actual HT/SGI/RAID/mask are in `duMAC_.rates`; HT is20MHz and
  TX AMPDU remains0. It has no implemented firmware rate-telemetry decoder.
- Channel publication: `permits8192DUChannel` exposes actual loaded channel
  support. The registered map contains1–11,36/40/44/48 and149/153/157/161/165.
  The existing parent all-channels-per-group publisher can use that map.
- Ownership/termination: recognize `is8192DUUSBProvider(provider)` for the
  device and sibling interface as well as the primary interface. Device::open
  alone allows child sessions, so the adapter also exclusively opens every
  enumerated interface, validates unique contiguous interface identities and
  refuses a busy sibling. It never seizes another driver's session.
- Release: call `release8192DUStorage` only after transfers/tasks have drained,
  including attach-failure cleanup. It frees PHY storage, extra per-interface
  pipe references and the extra device/sibling sessions. The parent still
  closes its primary provider and releases its normal active-pipe references.
- Request-close/termination: after quiesce/abort, call
  `close8192DUProviders()` outside the command gate alongside primary-provider
  close. It nulls both extra providers and clears ownership before closing
  sibling then device, tolerates synchronous reentry and leaves PHY/pipe
  storage alive for pending completions. Later storage release is idempotent.

DMDP requires two actual vendor interfaces. `prepare8192DUPorts` derives each
endpoint set from the complete configuration and its own PROM identity. It
never borrows MAC0 queue counts for MAC1. Initial scanning selects MAC1/2GHz.
Each cross-band transition pauses/disconnects, synchronously aborts, verifies
all callbacks drained, fully powers off the old MAC, changes both bulk
directions and vendor mapping, and boots/calibrates the newly selected MAC.
An undrained transfer prevents both pipe changes and global power teardown.
Failed shutdown/startup leaves TX stopped. Same-band scans keep their current
MAC. The primary provider/Registry ID and logical station MAC0 address remain
stable; `mac_set_address` programs that logical address only while disconnected
and TX paused, leaving the immutable factory identities unchanged. Both MACs
are never active concurrently. DMDP advertises MCS0–7, SMSP MCS0–15.

All channel records include the actual `IEEE80211_CHAN_HT20` mode flag before
net80211 attachment. Separate native tests execute the imported channel-init,
AUTO mode selection and HT negotiation for both bands and both stream counts.
Removing the channel mode flag reproduces failed HT negotiation even when
HT/SGI capability bits exist. No HT40/VHT/HE mode is advertised.

Firmware38.0 from the parent's immutable blob has signature92d3,32302 bytes and
32270 payload bytes. The real boot code was exercised against that entire
embedded image and reconstructed every payload byte using DU64/8/1 writes.
The source H2C command IDs are JOIN=2,RSSI=5,RA=6, with a two-byte extension at
0x88+2*n, not the newer four-byte mailbox format. This establishes protocol
and host-test compatibility; it does not claim physical firmware validation.
In the exact shipped image, the H2C self-reset handler clears both per-MAC
ready flags before CPUEN. This supports a successful source shutdown followed
by cold boot of the other MAC without invented host flag-clearing writes.
The forced-reset fallback has no equivalent proof of flag clearing; the
existing cold-download stale-ready guard continues to reject that case.

Portable C tests: Boot, ROM, DataPath, PHY, USB, Station and Dual.
Actual kernel methods with the real C HAL and mocked USB/host boundaries:

```
python3 scripts/test-rtl8192du-backend.py --library /path/to/librtwcore.a
python3 scripts/test-rtl8192du-negotiation.py
python3 scripts/check-rtl8192du-backend-kdk.py --out /tmp/du-controller.o
```

The KDK command compiles a temporary controller overlay, with real kernel/USB
headers and a2048-byte stack-frame limit. It does not edit the controller,
install/load a kext, open a USB device or change the running application.

The kernel orchestration model runs actual ownership/descriptor/pipe-switch,
SCAN/AUTH/RUN, TX/RX and close functions. Its boot/firmware boundary is isolated;
after that boundary it executes the real per-MAC C LLT/MAC/PHY/calibration HAL.
Separate boot tests feed the full embedded payload through the real downloader.
These models and native builds do not certify DU RF/WPA2/IP on physical hardware.
