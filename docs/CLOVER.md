# Building and loading with Clover

This project exposes the adapter as an Ethernet interface. net80211 handles association and WPA2, while the menu-bar app controls the selected USB interface. Loading the kext and installing the app helper are separate steps.

## Build

From the source root, run:

```sh
./build.command --kext
./app/build.command
```

Products are written to `Release/`. The source includes the kext, app, helper, tests, and generators. Build scripts apply and verify ad-hoc signatures.

## EFI

Mount the EFI partition actually used to boot through Clover. Identify the partition before copying: the `diskXs1` identifier may change across reboots. Keep a backup of the previous kext outside `EFI/CLOVER/kexts/Other` to avoid simultaneously injecting two copies with the same bundle identifier.

Copy the entire `Release/TahoeRealtekWiFi.kext` bundle to:

```text
EFI/CLOVER/kexts/Other/TahoeRealtekWiFi.kext
```

Clover must be configured to inject this kext. The new bundle will be used on the next boot from the correct EFI. Copying does not replace the driver already loaded in memory. Build scripts do not reboot the Mac, alter Clover configuration, or change SIP/AMFI.

On the recorded test computer, `kmutil load` was denied pending approval; Clover is the loading method selected by the user. Reopening Security settings does not replace EFI injection.

## App

The app installer places `Tahoe Wi-Fi USB.app` in `/Applications` and installs its helper. The kext is copied separately to the EFI. If macOS requests helper approval, it appears under Login Items. This is not kernel-extension approval.

Select an adapter in the menu, refresh available networks, and enter the password in the app. The password can be saved to macOS Keychain. No personal credentials are needed to compile the project.

## Checks after boot

```sh
kmutil showloaded --list-only
ioreg -r -c RTWUSBController -l
```

Check the version, build, family, and interface of the selected adapter. A loaded kext or the existence of `enX` alone does not prove WPA2, DHCP, or actual traffic. Validation must confirm controlled-port authorization, IP address, RX/TX, and the network interface used by the test. With wired Ethernet connected, a speed test might run through the wired interface.

## Status of this tree

Build55 (driver 0.14.8) and app47 (0.3.5) are under development. The 13 operational backends are integrated, with 262 eligible USB IDs and 263 verified numeric identities. RTL8814B has its own HAL/USB and PHY components, but its station path still requires qualified NIC firmware/protocol and physical validation. Seven records remain unconfirmed or conflicting. Numeric identity does not prove support for every board revision.

The connected RTL8814AU passed recorded physical radio/receive tests on the two non-DFS 5 GHz channel ranges. WPA2 association, DHCP, and traffic with the new kext still need validation after that kext has been loaded. Other families require their respective adapters to test connectivity, traffic, and coexistence. Consult the README, adapter matrix, and validation report before making compatibility claims.

## DFS in build55

Both the app and the kext must be rebuilt. App47 accepts the DFS channel groups actually registered by kext55, without enabling channels on an older driver. Seven dual-band backends include DFS HT20 on 52–64/100–140, subject to ROM/RFE, country, and power limits. Use **Refresh networks** for a real scan; a band observed in cache does not establish the association band. Check the current channel in the details. Channel 144 remains disabled. AP channel changes stop TX and trigger reassociation/WPA2.

Build55 still requires a subsequent physical load on the user's machine. Software tests do not identify the router's automatic 5 GHz channel or validate all boards/revisions. See [DFS-STATION.md](DFS-STATION.md) and [VALIDATION-0.14.8.md](VALIDATION-0.14.8.md).
