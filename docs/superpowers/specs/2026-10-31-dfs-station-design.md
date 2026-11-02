# DFS in USB client mode

The user requested DFS support using Linux, for all adapters with that capability.
Implement directly in the current tree, preserving the loaded connection and
delivering complete kext and app source. Do not alter EFI, load the kext, restart,
change the router or use personal credentials.

## Selected architecture

Port managed-station behavior without implementing AP mode. Linux
cfg80211_chandef_dfs_required returns zero for STATION: master CAC/radar is not
a client-adapter requirement. The client must scan passively, connect to the
observed BSS and obey channel evacuation announced by the AP. Do not remove
NO_IR globally after receiving a beacon or force a country from locale/IP.

Expand the seven dual-band backends: RTL8192DU, RTL8812AU, RTL8814AU,
RTL8821AU, RTL8821CU, RTL8822BU and RTL8822CU. New HT20 channels: 52/56/60/64 and
100/104/108/112/116/120/124/128/132/136/140. Channel144 remains closed in this
delivery: primary power rows are missing in AU/DU. Do not copy channel140 values.
Capabilities of 2.4GHz-only boards and 5GHz-only OEM profiles remain.

The backend channel list depends on ROM/RFE validation, power/calibration and
existing restrictions. Adding numbers alone is insufficient. AU8814 needs the
primary spur adjustment on140, cleanup on exit and the correct100–140 thermal
group. Other PHYs require their own OTP indices/limits. DFS starts in HT20;
existing wide modes on non-DFS channels remain. A wider AP advertisement must
negotiate 20MHz without causing association failure or advertising unimplemented
DFS40/80.

## Common station state

Passive reception must record a valid ESS beacon by BSSID, SSID and channel,
with bounded memory and time validity. DFS authentication may use only a BSS
recently observed on the received channel and authorized by the backend.
Malformed or foreign frames, an old association or contradictory fields do
not grant TX. Factory domain/power is not relaxed by a beacon.

Beacon/action CSA from the associated AP must stop TX as needed, drop the
link and start passive recovery without sending deauth on the evacuated channel.
The first implementation may repeat association/WPA2 after rescan instead of
switching RF while retaining keys. Resumption preserves the profile only while
the device and session remain valid. Teardown, sleep, USB failure and selecting
another adapter cancel tasks; do not run stop/restart recursively inside RX
analysis. Pause uses primary paths/registers verified for each family.

## App and ABI

Retain the v1 layout and reserved bytes. Add two independent capabilities:
DFS52–64 and DFS100–140, only when the registered map covers the complete group.
App47 accepts and describes the groups advertised by the driver. Old drivers
do not gain new channels through the chipset name. Do not advertise144 or AP mode.

Review detected countries with only part of the100–140 group (AU/DZ/KP/MY/RU).
Add a read-only query for the exact map of256 registered channels, with version
and zeroed reserved fields, without changing existing ABIv1 structures/selectors.
App47 uses this map for the list, selection/reconnection and description; an
old driver returns Unsupported and retains the conservative complete-capability policy.

## Domain and power cap

Port metadata from the pinned official wireless-regdb and verify hashes before
generating. Use the known factory country, otherwise world; never locale/IP
or a beacon override. Before AUTH/RFK, the effective limit is
min(world20dBm,ROMdomain,APCountrytripletWhenPresent) minus PowerConstraint.
Apply once through each PHY's absolute SAR, rounding down and without a second
relative reduction. Preserve factory thermal headroom, without extra margin
from reducing TXAGC. An impossible cap must not result in readiness/TX;
DU/B-C may detect it after paused channel I/O and must keep the failure closed.

## Evidence and delivery

Use behavioral tests RED on54 and GREEN on the implementation: every new channel,
power per RFE/ROM, spur140 transitions, HT20 on a wide AP, scanning without probes,
beacon-bound authentication and CSA/teardown/ownership. Compare RF/PHY against
pinned Linux/vendor sources and record hashes. Run Release/sanitizer suites,
SDK compilation, native app tests and a build of the delivered ZIP. Independent
review before freezing build55/app47. Physical tests require subsequent loading
by the user; do not claim every revision is certified or the cause of the current
network issue is proven.
