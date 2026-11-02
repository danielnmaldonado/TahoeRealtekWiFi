# Build and load through Clover

This project exposes the adapter as an Ethernet interface, with net80211
handling association and WPA2. The menu bar app controls the selected USB
interface. Loading the kext and installing the app helper are separate steps.

## Build

From the source root, run:

```sh
./build.command --kext
./app/build.command
```

The products are placed in `Release/`. The source includes the kext, app, helper,
tests and generators. The build scripts apply and verify ad-hoc signatures.

## EFI

Mount the EFI that actually boots the computer through Clover. Identify the
partition before copying: the `diskXs1` number may change between reboots.
Keep the previous kext backup outside `EFI/CLOVER/kexts/Other` to avoid
injecting two copies with the same identifier at the same time.

Copy the entire `Release/TahoeRealtekWiFi.kext` bundle to:

```text
EFI/CLOVER/kexts/Other/TahoeRealtekWiFi.kext
```

Clover must be configured to inject this kext. The new bundle will be used on
the next boot through the correct EFI. Copying it does not replace the driver
already loaded in memory. The build scripts do not restart the Mac, alter
Clover configuration or change SIP/AMFI.

On this computer, `kmutil load` was denied by the approval process, while
Clover is the loading method specified by the user. Reopening Security Settings
does not replace EFI injection.

## App

The app installer places `Tahoe Wi-Fi USB.app` in `/Applications`.
The package installs the app and its helper; the kext is copied separately to EFI.
When macOS requests helper approval, it is handled in Login Items.
This is separate from kernel extension approval.

Choose the adapter in the menu, refresh networks and enter the password in the app.
The password can be saved in macOS Keychain. No personal credentials are
needed to build the project.

## Checks after boot

```sh
kmutil showloaded --list-only
ioreg -r -c RTWUSBController -l
```

Check the version, build, family and interface of the selected adapter.
The loaded-kext record and the existence of `enX` alone do not demonstrate
WPA2, DHCP or traffic. Validation must check port authorization, IP address,
reception/transmission and the network path used by the test.
When wired Ethernet is also connected, a speed test may use the wired
interface.

## State of this tree

Build57 (driver 0.14.10) and app49 (0.3.7) are in development.
The 13 operational backends are integrated, with 262 eligible IDs. There are
263 verified identities. RTL8814B has its own HAL/USB and PHY, but station mode
awaits qualified NIC firmware/protocol and physical validation.
Seven records remain unconfirmed or conflicting.
Numeric identification does not establish every board revision.

The connected RTL8814AU passed physical radio and reception tests in both
non-DFS 5 GHz ranges. WPA2 association, DHCP and traffic with the new kext still
need validation after it is loaded. Other families require their corresponding
adapters to validate connection, traffic and coexistence.
Consult the README, matrix and validation report before advertising compatibility.

## DFS in build55

The app and kext must be rebuilt: app47 accepts the DFS groups that kext55
actually registers, without enabling channels in an older driver. The seven
dual-band backends include DFS HT20 on 52–64/100–140, subject to ROM/RFE, country
and power limits. Use Refresh networks for a real scan; the band seen in cache
does not establish the association band. Check the current channel in the details.
Channel 144 remains closed. An AP channel change stops TX and repeats association/WPA2.

Build57 still requires subsequent physical loading by the user. Software tests
do not establish the router's automatic 5 GHz channel or validate every
board/revision. Details are in DFS-STATION.md and VALIDATION-0.14.10.md.

## Recovery in build56

Build56 adds limited manual recovery for eligible RTL8814AU failures, with
draining, verified shutdown and reidentification. It also fixes power registration
through the IOKit family and preserves late initial-RX errors. App48 has already
been installed locally; kext56 was not loaded by the agent. There is no proof of
physical stability for this new kext. RTL8814AU remains HT20, with up to two streams.

## Two USB adapters and refresh in build57/app49

App49 was installed in Applications and successfully queried both real USB adapters.
The kernel was still using build56: AU on 5 GHz/channel 36 and EU on 2.4 GHz/channel 6,
both authorized and with working HTTPS tested separately per interface. The new
build57 must be built, its entire bundle copied to Clover, and loaded on the next boot.
App49 refreshes automatically for the selected, idle adapter; an authorized
connection keeps its cache to preserve the link. See VALIDATION-0.14.10.md.

## Preferences and resources — build58/app50

The current delivery uses driver0.14.11/build58 and app0.3.8/build50. The app restores
the physical adapter by identity, retains the network per adapter and handles late
enumeration. Resource and common-scan fixes require the new kext; building does
not replace the code loaded in the kernel. See VALIDATION-0.14.11.md and the
delivery's VERIFICATION.json.

## Audit of 14 adapters — build60/app51

Use driver0.14.13/build60 and app0.3.9/build51 for this delivery's regression checks.
The kext must be built and copied as a complete bundle, keeping a backup.
The new collector exports USB-STARTUP and AU stages even when the controller
never publishes an interface. See ADAPTER-TESTS-20261101.md for errors and
speed limits in each capture, and VALIDATION-0.14.13.md for the scope of
software tests and what still requires hardware.
