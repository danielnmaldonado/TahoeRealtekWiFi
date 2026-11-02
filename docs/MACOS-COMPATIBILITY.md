# macOS package compatibility

Scope: kext 0.14.14/build61, app 0.3.11/build53 and collector revision 3, Intel
x86_64. This is a development package. This change broadens the target systems
for the app/helper/installer; it does not change the radio or prove hardware compatibility.

| System | Darwin | Package target | Loading, WPA2 and traffic in this version |
| --- | --- | --- | --- |
| Ventura 13 | 22 | Yes, minimum 13.0 | Pending on hardware |
| Sonoma 14 | 23 | Yes | Pending on hardware |
| Sequoia 15 | 24 | Yes | Pending on hardware |
| Tahoe 26 | 25 | Yes | Pending for this release |

The 14 historical logs were captured on macOS 12.7.6 with kext55 and app48.
They do not validate this package on the four systems in the table. Limits
by chipset, revision, OTP, power and channels remain documented in the
[support list](USB-WIFI-SUPPORT.txt) and [re-audit of the 14 logs](RE-AUDIT-14-20261102.md).
The two boards without demonstrated operation, zero native RX counters and the
CCMP events in the old capture remain outstanding; changing the minimum system version does not
resolve these issues.

## What was prepared

- AppKit and helper compiled with `-mmacosx-version-min=13.0`, including
  test builds. `LSMinimumSystemVersion` and the `.pkg` minimum are 13.0.
- Availability errors enabled at compile time to detect unguarded
  calls to APIs introduced after Ventura. `SMAppService`, service
  approval and login startup continue using APIs already present in the project.
- The packager inspects version commands in the app and helper Mach-O
  executables. Changing only the plist does not allow a binary requiring
  macOS 26 to be packaged as compatible with 13.
- Kext61 code remains unchanged. The project retains deployment target 11.0 and the
  existing `OSBundleLibraries` minimums. The kext continues to publish Ethernet
  and uses its own net80211; it does not need an IO80211/AirPort variant for
  each system. Deployment target 11.0 is a build setting: the kext's Mach-O
  does not contain a minimum-version command equivalent to those of applications.
- The name, bundle IDs, control protocol, per-adapter/network preferences and
  Keychain items are preserved. No user data needs migration
  for this change to the minimum system version.

## Evidence and limits

The available build environment is macOS 26.7.2, Xcode 26.6 and SDK 26.5.
The application minimum is 13.0; this allows generating a binary for Ventura using
a newer SDK. It is not an execution on Ventura. Builds with older Xcode/SDK versions
still lack their own testing. Exact build, test, signature
and manifest results are recorded in `VERIFICATION.json` beside the release ZIP.

In the static audit, the 419 declared KPI imports used by kext61
appear in Apple's official export lists for the XNU versions corresponding to
Darwin 22/23/24/25. The remaining imports are from the USB/Ethernet families. Symbol
presence does not prove C++ class layout, kernel behavior, installation
or USB stability on each version; these stages need hardware testing.

Apple states that the combination of USB and IONetworkingFamily KPIs has been
unsupported since Big Sur. This release continues with the project's kext model; the audit
does not turn it into DriverKit or a driver approved for standard installation.
Primary sources: [kernel extensions and alternatives](https://developer.apple.com/support/kernel-extensions/),
[SMAppService](https://developer.apple.com/documentation/servicemanagement/smappservice).

## Verification and hardware testing

To build the kext, app and tests from the complete source:

```sh
bash ./build.command --all
bash ./app/test.command
bash ./app/package.command --no-build
python3 ./app/check-macos-compatibility.py --json \
  "Release/Tahoe Wi-Fi USB.app/Contents/MacOS/TahoeUSBWiFi" \
  "Release/Tahoe Wi-Fi USB.app/Contents/Helpers/TahoeWiFiHelper"
```

Each system in the table still requires verification of actual loading,
scan/refresh, selection between two USB adapters, WPA2/DHCP, sustained traffic on 2.4/5 GHz
where applicable, reboot, sleep and USB removal/reconnection. Running
collector revision 3 before and after allows errors and counters to be tied to the same
session. A throughput measurement requires a real network test; a link rate or
successful compilation cannot replace it.

This release does not install the app, replace the loaded kext or modify EFI,
security policies or router settings. Stable publication depends on
the validation above and the outstanding issues from re-audit of the 14 logs.
