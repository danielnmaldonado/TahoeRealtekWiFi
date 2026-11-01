# Validation — driver 0.14.8/build55 and app 0.3.5/build47

macOS development and adaptation: MaLd0n. Status: **development**.

This delivery extends build54 with DFS HT20 for seven dual-band backends. It preserves build53/54 fixes for AU RX 32 KiB buffers, TX AMPDU, C2H rate tracking, TX queue ownership, and RX accounting/observability. The catalog remains at 270 IDs, with 263 confirmed numeric identities and 262 eligible for the 13 backends. RTL8814BU and seven unconfirmed/conflicting entries do not gain static station enablement.

## Software-verified behavior

Tests reproduce rejection of the newly introduced channels in build54 and verify channel geometry; power calculations by ROM/RFE/path/rate; OTP indexing; channel-140 spur handling; channel-exit transitions; the 100–140 thermal group; I/O failures; and absolute SAR bounds. DFS operates at 20 MHz. The AU paths can associate to wider-channel APs by negotiating 20 MHz without advertising DFS40/80.

The common parser checks bounds, SSID/BSSID/channel, beacon freshness, Country, PowerConstraint, and CSA/ECSA. Tests of actual controller methods check admission before AUTH/RFK, queue holds, TXPAUSE retention on both DU MACs, deferred recovery, adapter isolation, and teardown cancellation. The combined local ceiling is applied exactly once. Factory country and wireless-regdb restrictions are tested across all seven paths, including a domain that disallows channel 100 but allows 52; beacons never relax regulatory rules. The generator rejects modified inputs before overwriting output.

App47 validates explicit capabilities and DFS descriptions, older-driver behavior, and the same SSID observed on both bands. ABI v1 layout and finite cache bounds are preserved. The additive exact-channel-map query covers partial regional ranges, including AU8/RU3, reserved fields, malformed responses, live socket use, and adapter/generation changes. Native RTL8814 scanning/authentication paths are tested on 24 5 GHz channels. An associated AP's CSA remains active when its SSID is hidden or changed, without granting fresh authorization. Rejected power plans preserve the active thermal ceiling; negative SAR values keep their precision and cannot become positive indices.

The original delivery's final Release/sanitizer suite, native build, app tests, independent review, ZIP extraction, and hash checks are recorded in `VERIFICATION.json`/`NATIVE-VERIFICATION.json` alongside that delivery. The ZIP includes matching full source, helper, generators, tests, and required licenses.

## Hardware limitations

The agent did not load build55 in the recorded validation. Earlier physical evidence belongs to build53 and is preserved in `VALIDATION-0.14.7.md`: RTL8814AU association at 2.4 GHz and short transfers. That does not validate DFS/CSA or every board/revision. The router's automatic 5 GHz channel remained unknown at the time. There is no claim that DFS caused the observed missing connection or that the new build already fixed it on physical hardware.

Channel 144 remains disabled; AU/DU power tables are not extrapolated. There is no AP mode, local CAC/radar, WPA3/AX, or RTL8814BU station support. Software power limits do not measure aggregate antenna gain/EIRP in hardware. Existing RFE, ROM, Bluetooth, and 5 GHz-only OEM restrictions remain. See [DFS-STATION.md](DFS-STATION.md) and [USB-WIFI-SUPORTE.txt](USB-WIFI-SUPORTE.txt).

The agent did not alter the EFI, loaded kext, installed app, credentials, or router. Rebuilding/replacing the complete bundle and booting from the correct EFI remains the user's physical step. After boot, verify build55, current channel, WPA2, DHCP, traffic via the USB interface, and AP channel transitions.
