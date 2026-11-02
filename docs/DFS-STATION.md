# Station DFS — build55/app47

Design and macOS adaptation: MaLd0n.

## Channels and families

RTL8192DU, RTL8812AU, RTL8814AU, RTL8821AU, RTL8821CU, RTL8822BU and RTL8822CU
receive DFS HT20 on 52/56/60/64 and
100/104/108/112/116/120/124/128/132/136/140. Existing non-DFS channels remain.
The registered map depends on ROM/RFE, factory domain, calibration and power.
OEM restrictions to 5 GHz only are preserved. Channel 144 remains closed:
missing primary AU/DU tables are not extrapolated from channel 140. DFS40/80
is not advertised. The AU path negotiates 20 MHz with wider APs; this change
does not reduce its non-DFS modes. Families limited to 2.4 GHz do not gain DFS.

App47 accepts two new capability bits, retaining the ABIv1 layout.
Each bit requires the complete group in the controller map; an old driver or
a chipset catalog does not authorize new channels. SSID grouping continues
to preserve the bands actually observed and usable.

Read-only selector 5 adds an exact map of 256 registered channels, with a
48-byte response, version and zeroed reserved fields. All previous structures
and selectors retain their size and meaning. This map avoids hiding valid
channels in countries with partial ranges (AU/DZ/KP/MY/RU), without advertising
prohibited channels. The app binds the map to the selected adapter and generation;
an error or missing response invalidates the channels, and only Unsupported from
an older driver permits fallback to complete-group bits. The model, reconnection
and description use the same map. The query neither tunes the radio nor grants TX permission.

## Discovery, association and channel evacuation

The client scans passively. A complete ESS beacon must match the desired SSID,
BSSID/TA, the channel actually tuned and the PHY channel when available.
The cache is limited to 32 observations; AUTH permission expires after 30 s.
Contradictory fields, incomplete/foreign frames and stale records do not grant
TX. Permission is bound to the BSS, without allowing probes or removing NO_IR
globally. ASSOC/RUN remain bound to the admitted BSSID/channel. WPA2/CCMP uses
net80211. Beacons from the already bound AP update power even with a hidden SSID;
the timestamp for that update does not renew visible AUTH permission. This also
applies during already bound AUTH/ASSOC, before RUN. Saved hidden profiles preserve
directed discovery on 2.4 GHz when the current map permits that band. An invalid
map, a map without active 2.4 GHz channels or a 5 GHz-only adapter does not enable
this fallback.

A valid CSA/ECSA from the associated AP immediately stops host and hardware
transmission, drops the link and closes the controlled port. The TX hold also
covers later TXPAUSE writes, including the 8192DU MAC1 address. The watchdog
performs a full stop/drain before releasing the hold and attempting passive
scanning, association and WPA2 again. The advertised destination does not program
RF directly. No deauth is sent on the evacuated channel, and shutdown is not
performed recursively within RX. Removal, sleep, a deleted profile and USB failure
prevent an improper restart.

## Country and power

The known country comes from factory fields already decoded per family.
DU does not reuse another chipset's OTP offsets and uses the world baseline.
A missing or unknown country also uses world; locale, IP and Country IE do not
replace ROM. `DFSRegulatoryTables.inc` contains 183 domains derived from the
official wireless-regdb, with complete 20 MHz coverage and an mBm cap per channel.
A rule boundary or gap is not extended. The generator verifies SHA256 and gitblob
for all three inputs before running the primary parser or writing the table.

Before AUTH/RFK, the DFS cap is the minimum of world 20 dBm, factory country and
the AP's Country triplet limit when present, minus PowerConstraint.
The value is rounded down to the PHY's unit. The absolute limit uses the SAR
conversion per path/rate section and ROM/PG baseline from the primary sources;
thermal compensation gains no extra headroom from the reduced cap. An impossible
limit refuses readiness/TX: AU performs plan preflight; DU/B-C may perform channel
I/O while still paused before detecting the failure and keeping it blocked.
There is no second relative reduction applied to an already limited rate. A more
restrictive AP limit during RUN stops TX and requires new admission. These
software controls do not replace EIRP measurement, antenna gain assessment or
board certification.

## Primary sources and reproduction

- Linux pinned at `0e135910deec43ffe97eca0b6af1f8f847285b59`:
  [STATION does not require local CAC](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/net/wireless/chan.c#L1015),
  [station CSA](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/net/mac80211/spectmgmt.c#L220),
  [rtw88 rules](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/drivers/net/wireless/realtek/rtw88/regd.c#L336).
- [Official wireless-regdb](https://git.kernel.org/pub/scm/linux/kernel/git/wens/wireless-regdb.git),
  revision `40095bd16e53d5c5682fd6e7e3dbbbfaf0fe5410`.
  Intact ISC inputs in `vendor/wireless-regdb/`; hashes and origin in
  `src/DFSRegulatory-Upstream.json`.
- The specific PHYs retain their `*-Upstream.json` files, generators,
  golden fixtures and license notices. Tables are not copied from another family.

```sh
python3 scripts/generate-dfs-regulatory.py --check
cmake -S . -B build-dfs -DCMAKE_BUILD_TYPE=Release
cmake --build build-dfs --parallel 4
ctest --test-dir build-dfs --output-on-failure --parallel 4
bash ./build.command --kext
bash ./app/build.command
```

This is client mode, without AP/hotspot, local CAC/radar, WPA3 or AX. The user's
network's automatic channel has not yet been identified, and this delivery does
not prove that DFS caused that network to be missing. The new kext must be loaded
to validate association, DHCP, traffic and CSA on hardware. RTL8814BU still has
no station mode.
