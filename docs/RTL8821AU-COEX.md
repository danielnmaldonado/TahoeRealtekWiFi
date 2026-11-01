# RTL8821AU station coexistence

`RTL8821AUCoex.c` ports the actual Linux rtw88 station policy, 8821 antenna/PTA
callbacks and chip tables. Its source is pinned to
[f47d223627f80cb7a1eb14c585fe6cb35de5e984](https://github.com/torvalds/linux/tree/f47d223627f80cb7a1eb14c585fe6cb35de5e984/drivers/net/wireless/realtek/rtw88).
`RTL8821AUCoex-Upstream.json` records SHA-256 inputs for `coex.c`, `coex.h`,
`fw.c`, `fw.h`, `main.h`, `reg.h` and `rtw8821a.c`. All generated policy/types,
callbacks and tables are supplied; compiling needs no network or generator.
The selected dual-license option is BSD-3-Clause, with the retained Realtek
notice in `RTL8812AUPHY-LICENSE.txt`.

The module accepts only a positively identified 8821AU/RFE0/one-path board
whose observed Bluetooth-function flag requires coexistence. Shared/separate
antenna comes from the actual ROM bit, including the primary's valid 0xff packed
selector. It does not cast an 8812, 8814 or newer-chip hardware context. The
pinned 8812 descriptor has no coex callbacks/tables, so BT-active 8812 is still
rejected by its PHY preflight.

The kernel bridge starts real coex power/init after MAC/firmware/PHY setup and
before the first scan transmission. It sends scan, association, media and band
notifications, dispatches BT-info/MP/debug C2H, and runs the watchdog before RUN
as well as during an association. The watchdog uses actual packet/byte counts,
the primary bytes/2s throughput EWMA, observed rates/RSSI, measured CCK CRC
counters and the real scan-cache count. The native controller's 2s timer drives
retained work deadlines on its serialized gate; no Linux workqueue is linked.

All firmware commands share the bootstrap's canonical `h2c_box`. The port
retains the exact 0x60/61/62/63/66/67/69/73 packing from the pinned firmware API.
MP requests complete asynchronously because waiting for C2H on the same
controller gate would block its completion. A valid SCAN_TYPE response applies
its verified payload directly, independent of the periodic modulo-three query
gate, without counting an old BT report again. Responses arriving at/after the
primary 10 ms deadline are rejected. The inactive-BT threshold counter saturates
at 10; the primary u8 wrap would otherwise falsely re-enable inactive BT after 256
checks. The upstream SCAN_FINISH placeholder count 30 is replaced with the real
host scan-cache count. These adaptations are explicit in the generator/source.

Identity is retained at initialization: family, semantic board/calibration,
USB layout, bus callbacks/context and waiter callbacks/context. Every public
I/O path, primitive wrapper and live PHY lease checks that immutable binding.
Changing ownership forgets queued work and returns an error without touching
either adapter. ABI padding is not board identity. PHY TX cannot resume with an
unstarted, stopped, foreign or unhealthy coex context. A matching lease
samples the exact live firmware health again before each PHY TX resume.

All AU controllers also retain their own identity after positive identify,
independently of optional Bluetooth coexistence. This covers semantic board
calibration, logical ROM, typed bus and waiter callbacks/context, family/USB
layout, and the actual provider/RX/TX pipe routing. The same pure predicate gates
TX/rate/BA/RX consumers without a per-frame USB read. Normal readiness, firmware
version and mailbox changes do not alter identity; ordinary stop/restart remains
valid. AU has no power-epoch counter; this port does not claim one.

The kernel shares owner-first/live-health admission between every state
transition (including INIT/stop) and the independent EDCA callback. An invalid
MCU cannot receive media/rate H2C or MAC state programming before a late PHY
guard catches it. Rejection flushes host INIT/keys/port/rates immediately.
Only an unchanged usable adapter may receive bounded direct PHY/antenna cleanup;
foreign bindings and known unusable transports never dispatch PHY/power helpers.

Firmware operations and direct stop require the exact legacy MCUFWDL health
mask `(status & 0xc7) == 0xc6`. Failed/all-ones/download-enabled status clears
firmware/association authority. Direct stop then preserves ENXIO and sends no
H2C; unchanged ownership with a usable bus can perform bounded primary WOFF
antenna cleanup. USB removal forgets PHY/coex and flushes host keys/controlled
port without register/mailbox I/O. The 8821 primary descriptor excludes newer
LTE-indirect and MIMO-PS operations; accidental dispatch returns ENOTSUP.

Run `python3 scripts/test-rtl8821au-coex.py` for the real portable policy/RF-bus
model and `python3 scripts/test-rtl8812au-backend.py` for actual controller
methods against host/USB/PHY boundaries. The suites cover both antenna routes,
A2DP and 5 GHz policies, canonical H2C interleaving, MP ordering/deadlines,
300 inactive-BT cycles, immutable-owner mutations, direct unhealthy stop,
live PHY leases, 988 BT/non-BT ownership cases across 19 mutations and 13
entry surfaces, all seven kernel state/stop/EDCA surfaces over 252 fault
cases, healthy WMM-to-default EDCA with reserved-bit preservation, startup I/O
failures before/after effect, WPA controlled-port
and EAPOL behavior, 1SS VHT80, and host-only removal. The native KDK checks compile
the complete controller and the coex C unit. The strict C-unit build also
checks a 2048-byte frame bound. These are
software/native-build results; physical 8821 BT connection/audio/RF performance
certification has not been performed.
