# B/C Bluetooth coexistence

`RTW88Coex.c` implements the station policy for RTL8821CU, RTL8822BU and
RTL8822CU boards with positively identified Bluetooth/antenna ROM metadata.
It reuses the pinned generic Linux policy/types/wrappers also used by the AU
port. Each B/C chip has its own original antenna, PTA, TDMA, RF, RSSI, AFH and
counter routines/tables. No AU hardware context or guessed fixed policy is
used. Provenance and deterministic generation are in
`src/RTW88Coex-Upstream.json` and `scripts/generate-rtw88-coex.py`.

`RTW88CoexBackend.inc` supplies the real controller integration. Startup
follows firmware/MAC/PHY/RX/general-info initialization. Scan, band change,
association, media, RFK and idle/power-off events reach the policy. OS sleep
quiesces the station before setting the suspended flag, so normal stop can
return antenna ownership while USB is available. Removal only forgets policy
state and cancels its timer. A dedicated serialized timer services delayed
work, the two-second traffic/counter watchdog and ten-millisecond BT MP
deadlines during scan, association and RUN.

Firmware register commands use only `r88Core_.firmware.h2c_box`, shared with
station and RFK commands. Long H2C packets retain their separate packet
sequence. BT-info, firmware-debug, HID and MP C2H records are bounded before
policy entry. A completed BLE scan-type response updates that completed
request directly, including after another BT-info event; it does not recount
or resample an older event. Traffic uses actual interval counters and the
primary Mbps/EWMA conversion; chip routines supply actual CRC counters. The
scan AP count comes from the stack cache. The watchdog performs the primary
RTL8822C HID-list discovery; the other two families correctly omit that query.

RTL8822C MIMO power saving invokes the original `config_trx_mode` routine,
updates firmware RA and sends an HT SMPS action to an associated HT AP.
Factory RX capacity remains available for restoration. CCX report tokens are
shifted six-bit values in descriptor W6; token zero is valid. The existing
host 802.11 sequence remains in W9 and the frame header. V0/V1 reports must
match the pending token, AP and association. USB submission never establishes
AP acceptance. Only an actual matching ACK changes local SMPS state. A final
NACK or absent report at 500 ms quiesces the station; this fail-closed behavior
and retired-token protection are explicit bounded port adaptations.

Every policy I/O entry, including stop, checks retained board, transport,
PHY/MAC and boot-epoch authority. Stop also checks the live 3081 MCU readiness
format before issuing commands. The station TX gate checks retained ownership
without USB I/O. Invalid metadata, transport changes, failed calibration,
firmware loss and policy errors cannot promote a ready station.

The kernel also retains bootstrap transport/board/epoch authority for boards
without Bluetooth. All MLME states, independent EDCA, stop and rollback preflight that authority
before firmware teardown and recheck it at the final power gate. Lost ownership
performs host key/controlled-port/timer cleanup without firmware, PHY or boot
power dispatch. 2,268 actual-controller authority regressions cover BT and no-BT boards,
all seven actions and all three families on channels 6/36/165, malformed
ownership, MCU loss and status-read failure. The same pure ownership check
gates fast TX for both BT and no-BT boards without USB.

Validation commands:

```sh
python3 scripts/generate-rtw88-coex.py --source /path/to/pinned/rtw88 --check
python3 scripts/test-rtw88-coex.py
python3 scripts/test-rtw88-coex-backend.py
python3 scripts/test-rtw88-backend.py
python3 scripts/check-rtw88-backend-native.py
```

The tests execute the actual policy and controller methods, with isolated
host/USB/RF boundaries. They cover all three families, both antenna layouts,
channels 6/36/165, canonical mailbox interleaving, scan/link/RFK/timer teardown,
942 startup and 899 station-transition I/O failures, 5,888 CCX wire cases,
all 64 live controller tokens, BLE MP response/deadline cases and family-specific
watchdog HID discovery. The real
KDK compilation enforces a 2,048-byte stack-frame limit. The operating PHY
suite also executes real RTL8822C TRX path changes/restoration and rate guards.

Physical B/C Bluetooth coexistence, WPA2/IP and throughput validation remain
pending. These model/native checks are not radio or AP certification.
