# DFS station mode — build55/app47

macOS development and adaptation: MaLd0n.

## Channels and chipset families

RTL8192DU, RTL8812AU, RTL8814AU, RTL8821AU, RTL8821CU, RTL8822BU, and RTL8822CU gain DFS HT20 on channels 52/56/60/64 and 100/104/108/112/116/120/124/128/132/136/140. Existing non-DFS channels remain supported. The registered channel map depends on ROM/RFE, factory regulatory domain, calibration, and power limits. The 5 GHz-only OEM restrictions remain in place. Channel 144 stays disabled: missing primary AU/DU tables are not extrapolated from channel 140. DFS40/80 is not advertised. AU backends negotiate 20 MHz with wider-channel APs without reducing their existing non-DFS modes. The 2.4 GHz-only families do not gain DFS support.

App47 recognizes two new capability bits without changing the ABI v1 layout. Each bit requires its complete channel group to be present in the controller map. Neither an older driver nor a chipset catalog can authorize additional channels. SSID grouping continues to preserve usable, actually observed bands.

Read-only selector 5 adds an exact registered-channel bitmap covering 256 channels, with a 48-byte response, version, and zeroed reserved fields. All previous structures and selectors retain their layouts and semantics. This map avoids suppressing legal partial ranges in AU/DZ/KP/MY/RU without advertising prohibited channels. The app binds the map to the selected adapter and generation; an absent or invalid response invalidates those channels. Only `Unsupported` from an older driver permits fallback to complete-group capability bits. The model, reconnect logic, and descriptions share the same map. Reading the map neither tunes the radio nor grants TX permission.

## Discovery, association, and channel evacuation

The station scans passively. A complete ESS beacon must match the requested SSID, BSSID/TA, actually tuned channel, and PHY channel when available. The cache holds at most 32 observations; AUTH authorization expires after 30 seconds. Conflicting fields, incomplete/foreign frames, and stale observations cannot authorize TX. Authorization is bound to the BSS; it does not enable probe transmissions or globally clear `NO_IR`. ASSOC/RUN remain bound to the admitted BSSID/channel. WPA2/CCMP uses net80211.

Beacons from the already associated AP update power limits even with a hidden SSID; that update timestamp does not extend the visible AUTH-authorization lifetime. This also applies during bound AUTH/ASSOC before RUN. Saved hidden profiles retain directed discovery on 2.4 GHz when the current channel map permits it. An invalid map, lack of enabled 2.4 GHz channels, or a 5 GHz-only adapter prevents that fallback.

A valid CSA/ECSA from the associated AP immediately halts host and hardware transmission, brings down the link, and closes the controlled port. TX hold also covers subsequent TXPAUSE writes, including the RTL8192DU MAC1 address. The watchdog completes stop/drain before releasing the hold and attempting passive scanning, reassociation, and WPA2 again. The announced destination never programs RF directly. No deauthentication frame is sent on the evacuated channel, and teardown is not invoked recursively from RX. Removal, sleep, deleted profile, and USB failure prevent stale resumption.

## Regulatory domain and transmit power

The country comes from previously decoded family-specific factory fields. DU does not reuse OTP offsets from another chipset and instead uses the world baseline. An absent or unknown country also uses the world baseline; locale, IP location, and Country IE cannot override ROM-derived identity.

`DFSRegulatoryTables.inc` contains 183 domains derived from the official wireless-regdb, with complete 20 MHz coverage and mBm ceilings per channel. Rule edges and gaps are not extended. Before running the primary parser or writing tables, the generator verifies SHA-256 and Git blob IDs of all three pinned inputs.

Before AUTH/RFK, the DFS ceiling is the minimum of world 20 dBm, factory-country limits, and the AP Country-triplet ceiling when present, minus PowerConstraint. Values are rounded down to PHY units. Absolute bounds use per-path/per-rate-section SAR conversion and ROM/PG baselines from the primary sources; thermal compensation receives no extra margin from the reduced ceiling. An impossible bound refuses readiness/TX: AU preflights the plan, while DU/B-C may perform channel I/O while TX remains paused before detecting and latching the failure. There is no second relative reduction on a rate already limited. A stricter AP limit during RUN interrupts TX and requires renewed admission. These software controls do not replace EIRP, antenna-gain, or board-certification measurements.

## Primary sources and reproduction

- Pinned Linux revision `0e135910deec43ffe97eca0b6af1f8f847285b59`:
  [station mode does not require local CAC](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/net/wireless/chan.c#L1015),
  [station CSA](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/net/mac80211/spectmgmt.c#L220),
  [rtw88 regulatory rules](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/drivers/net/wireless/realtek/rtw88/regd.c#L336).
- [Official wireless-regdb](https://git.kernel.org/pub/scm/linux/kernel/git/wens/wireless-regdb.git), pinned revision `40095bd16e53d5c5682fd6e7e3dbbbfaf0fe5410`. Original ISC-licensed inputs are preserved in `vendor/wireless-regdb/`; hashes and provenance are recorded in `src/DFSRegulatory-Upstream.json`.
- Family-specific PHYs retain their `*-Upstream.json` files, generators, golden fixtures, and license notices. Tables are not copied from unrelated families.

```sh
python3 scripts/generate-dfs-regulatory.py --check
cmake -S . -B build-dfs -DCMAKE_BUILD_TYPE=Release
cmake --build build-dfs --parallel 4
ctest --test-dir build-dfs --output-on-failure --parallel 4
bash ./build.command --kext
bash ./app/build.command
```

This is station/client mode, not AP/hotspot, local CAC/radar, WPA3, or AX. The user's router's automatic channel had not yet been identified in the recorded test, and this delivery does not establish DFS as the cause of the missing network. The new kext must be loaded to validate association, DHCP, traffic, and CSA on hardware. RTL8814BU remains without station support.
