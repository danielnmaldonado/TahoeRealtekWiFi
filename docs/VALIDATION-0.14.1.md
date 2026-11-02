# Development validation — 0.14.1/build48, app 0.3.2/build44

Project and macOS adaptation: MaLd0n. Status: development.
This report separates code evidence from hardware station validation.

## Catalog

There are 270 records, preserving the original package's 250 plus 20 extensions.
263 have verified primary identities; 262 are eligible for the 13 integrated
station backends. The 32 new confirmations use evidence retained in the
source register, with OEM and interface restrictions.

Five IDs still lack sufficient primary matches: 038b:0100,
058c:ff20, 0bda:8011, 0e0b:9071 and 1668:8105. IDs 13b1:0043 and 0bda:8192
have conflicting sources. The identity of an observed unit does not silently
change the catalog or authorize another revision.

Profiles 04bb:0959, 04bb:095a and 0bda:2003 are 5 GHz only according to the
OEM installation. The controller rejects 2.4 GHz before I/O; the app interprets
the explicit bands published by the driver, preserving compatibility
with the previous protocol.

## RTL8814BU

The source includes its own HALMAC/USB, OTP and codecs, imported from a pinned
primary revision. The component build, generator reproduction and
independent review passed with transport failures before and after their
effect, ownership checks and actual kernel stack limits.
The lifecycle link between PHY and HAL received separate static review
and local lease tests; no model review is hardware certification.

Its own PHY/RFK is integrated as a component blocked for station use. The station
path remains disabled: it still requires licensed NIC firmware and
station protocol qualification, factory/regulatory USB profile and
remaining tracking functions, along with corresponding hardware. See
[the component limits](RTL8814BU-STATUS.md).
A firmware signature resembling AU is not proof of compatibility.

## Software verification

The integrated suite with PHY and silicon discovery passed 147/147
CTest tests in Release and the same 147 with ASan/UBSan on host targets
(62,15 s in the last instrumented pass). Band tests include actual controller
methods and rejection of transmission on a blocked band without USB I/O.
App 0.3.2/build44 passed all 15 native executables, including the AppKit
build. The delivery flow extracts the ZIP to another folder, repeats the tests and
builds app, kext and installer; it checks signatures, hashes and the installer's
app/helper payload against products from that same extracted tree.
The generators reproduced 72 HALMAC files and 16 PHY artifacts with 159
actual functions and nine primary arrays. The PHY passed KernelSDK compilation
with a 2.048-byte stack frame limit.

Tests use models, firmware fixtures and actual extracted methods. They
do not equate to a set of physically certified adapters. ASan/UBSan
instrumentation belongs to host targets, not the kernel.

## Physical system and release

Two adapters were observed: RTL8814AU 2357:0106 and RTL8192EU 2357:0108.
The initial audit in this session found neither the kext loaded nor an
RTWUSBController service. Historical AU reception results do not prove
WPA2, DHCP or throughput with the new build. Traffic from integrated Ethernet is
not counted as USB adapter traffic.

Release validation requires actual loading, WPA2, DHCP, reconnection,
USB removal, sleep and sustained traffic on the relevant families/revisions.
Compilation does not change SIP/AMFI or restart the computer. Complete support
or certification of the 270 records is not claimed while this evidence is missing.

## Delivery

Packages and build sources are in this project's `releases` folder.
The Desktop is not used as a destination. Each new package must contain build
instructions, a file manifest and SHA-256; mandatory licenses and dependency
provenance remain with the source.
