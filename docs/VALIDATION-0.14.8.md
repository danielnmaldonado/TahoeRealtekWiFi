# Validation — 0.14.8/build55 and app0.3.5/build47

Project and macOS adaptation: MaLd0n. Status: development.

This delivery extends build54 with DFS HT20 for the seven dual-band backends.
It preserves fixes 53/54 for AU RX32KiB, TXAMPDU, trackingC2H, TX queue
ownership and RX accounting/observation. The catalog remains at 270 IDs,
263 confirmed identities and 262 eligible for the 13 backends. RTL8814BU and the seven
unconfirmed/conflicting records receive no static enablement.

## Behaviors verified in software

Tests reproduce rejection of new channels in build54 and check geometry,
power by ROM/RFE/path/rate, OTP indices, spur140 and exit transitions,
thermal group 100–140, I/O failures and absolute SAR limits. DFS operates at 20 MHz;
AU paths accept wider APs through 20 negotiation without advertising DFS40/80.

The common parser checks bounds, SSID/BSSID/channel, beacon freshness, Country,
PowerConstraint and CSA/ECSA. Tests of actual Controller methods check
admission before AUTH/RFK, queue blocking, retention of TXPAUSE on both DU MACs,
deferred recovery, adapter isolation and cancellation during teardown.
The combined local cap is applied once. Factory country and wireless-regdb
limits are tested across the seven paths, including a domain prohibiting 100
and allowing 52; no rule is expanded by a beacon. The generator refuses modified input
before overwriting output.

App47 validates explicit capabilities and DFS descriptions, an old driver and the same
SSID observed in both bands. ABIv1 layout and finite cache limits are
preserved. The additive exact-map query covers partial regional ranges,
including AU8/RU3, reserved values, invalid responses, actual socket and adapter/generation
changes. Native8814 search/authentication are tested on 24 channels at 5 GHz;
CSA of an associated AP remains active when its SSID is hidden or changes, without granting
new authorization. Rejected power plans preserve the active thermal cap;
negative SAR retains precision and cannot be converted to a positive index.
The final execution of Release/sanitizer suites, native build,
app tests, independent review, ZIP extraction and hashes is recorded in
`VERIFICATION.json`/`NATIVE-VERIFICATION.json` alongside the delivery. The ZIP includes
complete corresponding source, helper, generators, tests and mandatory licenses.

## Hardware limits

The agent did not load build55. Previous hardware evidence belongs to
build53 and is preserved in VALIDATION-0.14.7.md: 2.4 GHz association and short
transfers on RTL8814AU. It does not validate DFS/CSA or every adapter/revision. The automatic
router's 5 GHz channel remains unknown. There is no claim that DFS was
the observed cause or that the new build has already resolved physical association.

Channel 144 remains closed without extrapolating AU/DU power. There is no AP mode,
local CAC/radar, WPA3/AX or RTL8814BU enablement. Software limits do not measure
antenna gain/aggregate EIRP on hardware. Existing RFE, ROM,
Bluetooth and OEM 5 GHz restrictions remain. See DFS-STATION.md and USB-WIFI-SUPPORT.txt.

The agent did not change EFI, the loaded kext, installed app, credentials or router.
Building/replacing the entire bundle and booting from the correct EFI remains
the user's hardware step. After boot, check build55, current channel, WPA2,
DHCP, traffic through the USB interface and the AP channel transition.
