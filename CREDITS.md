# Credits and third-party acknowledgements

**Project copyright (project-authored material):** © 2026 DNMTechLabs  
**Contact:** [dnmtechlabs@gmail.com](mailto:dnmtechlabs@gmail.com)

This project includes original macOS integration and adaptations alongside software,
firmware, specifications, and implementation references from independent upstream
projects. The project copyright does **not** replace or transfer any third-party
copyright. Original author notices, license terms, and source-specific provenance
remain applicable and are retained with their respective files.

## Linux and Realtek wireless driver credits

The TahoeRealtekWiFi source draws on the work of Linux wireless developers and
Realtek driver contributors. Depending on the component, this includes imported
or adapted implementation material, hardware tables and definitions, and reference
implementations. Credits include:

- [Linux kernel](https://github.com/torvalds/linux) wireless and Realtek drivers,
  including `rtw88`, `rtl8xxxu`, and `rtlwifi`.
- Realtek Corporation, for relevant hardware definitions, HALMAC components,
  driver material and separately licensed firmware.
- [RinCat/RTL88x2BU-Linux-Driver](https://github.com/RinCat/RTL88x2BU-Linux-Driver),
  for pinned Realtek HALMAC/chip identity source references.
- [lwfinger/rtw88](https://github.com/lwfinger/rtw88) and
  [lwfinger/rtl8xxxu](https://github.com/lwfinger/rtl8xxxu), for documented
  Linux driver references.
- [aircrack-ng/rtl8812au](https://github.com/aircrack-ng/rtl8812au), for
  documented RTL8812AU technical references.
- Linux firmware maintainers and contributors of the firmware images whose
  origins and redistribution conditions are tracked under `firmware/`.
- Linux wireless-regdb contributors, whose original license is retained under
  `vendor/wireless-regdb/LICENSE`.

Individual contributors, including Jes Sorensen and Larry Finger where credited
by the imported files, retain their own notices. Exact upstream revisions,
selected licenses, per-component attribution, and the distinction between
imported code and reference-only material are documented in
[docs/SOURCES.md](docs/SOURCES.md) and the `*-Upstream.json` files.

## Other upstream contributors (not Linux-derived)

- [OpenBSD](https://github.com/openbsd/src): `rtwn`, `net80211`, protocol and
  cryptographic code, with original BSD/ISC and other notices preserved.
- [FreeBSD](https://github.com/freebsd/freebsd-src): Realtek `rtwn` source and
  radio/descriptor reference implementations, credited according to each file.
- [OpenIntelWireless/itlwm](https://github.com/OpenIntelWireless/itlwm): the
  macOS-compatible `net80211` integration and related source adaptations.
- [xen-proc/rtl8812au-macos](https://github.com/xen-proc/rtl8812au-macos):
  documented Realtek RF register/readback references.
- [Acidanthera/MacKernelSDK](https://github.com/acidanthera/MacKernelSDK):
  the pinned macOS kernel build SDK fetched during compilation, not bundled
  into this repository.
- Apple: macOS frameworks, AppKit and SF Symbols as identified in the app.

## Lilu dependency status

[Lilu](https://github.com/acidanthera/Lilu) is a distinct Acidanthera project.
Inspection of this source and its build configuration did **not** find a Lilu
plugin, Lilu headers, Lilu libraries, or a required Lilu runtime dependency.
Using MacKernelSDK does not itself mean that this project uses Lilu. If Lilu
is incorporated into the project in the future, its authors and relevant
license notices must be credited accordingly.

## Licenses and source provenance

The GPL-2.0 project license does not supersede separately licensed upstream
components. In particular, preserve source-level copyright notices, SPDX
identifiers, permissions and conditions for BSD/ISC/MIT components, and the
separate firmware redistribution terms. Refer to:

- [LICENSE-GPL-2.0.txt](LICENSE-GPL-2.0.txt)
- [vendor/itl80211/LICENSE.itlwm](vendor/itl80211/LICENSE.itlwm)
- [vendor/wireless-regdb/LICENSE](vendor/wireless-regdb/LICENSE)
- [firmware/urtwn-license](firmware/urtwn-license) and
  [firmware/LICENCE.rtlwifi_firmware.txt](firmware/LICENCE.rtlwifi_firmware.txt)
- Component-specific license files under `src/`, with the full provenance in
  [docs/SOURCES.md](docs/SOURCES.md).

This acknowledgement file is a directory of credits, not a relicensing
statement or a claim of authorship over upstream code.
