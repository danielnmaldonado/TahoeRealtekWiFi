# Development validation — 0.14.0/build47, app 0.3.0/build42

This report accompanies the source and does not certify all
adapters or revisions in the catalog. Current status: development.

## Scope

- 270 records: 250 from the original package and 20 extensions with provenance retained.
- 231 verified numeric identities; 230 eligible for the 13 integrated backends.
- RTL8814B B814 has primary identification but no operational HAL.
- 39 unconfirmed/conflicting identities remain disabled.
- WPA2-Personal/CCMP station association, with integrity, replay and a controlled
  port in net80211 adapted for macOS.

Limits by family, width and band are in the README and USB matrix.
Numeric identity, an existing interface and compilation do not prove traffic.

## Available hardware evidence

On the connected RTL8814AU 2357:0106, the explicit radio/receive routine checked
channels 149/153/157/161/165: 40 MPDUs/beacons, zero format/CRC/ICV errors and
clean shutdown. There is an earlier record of reception on channels 36–48.
These tests are not WPA2 association, DHCP or download/upload measurement with
the build47 kext loaded.

The current query found neither a TahoeRealtekWiFi kext nor an RTWUSBController
service loaded. Validation of the new kext requires Clover on the next boot
and the corresponding adapters/revisions. Compilation does not restart the Mac
or modify SIP/AMFI.

## Software verification

The catalog, C reader, personality generation, matrix and app identification
passed nine tests together after confirming B814 and the two legacy aliases. The tests
use all 270 identities and their interface/configuration restrictions.

New components have tests of registers, actual firmware images,
transport failures before/after their effect, actual methods extracted
from the controller and negotiation through the imported net80211. Assertion
counts do not represent the number of certified adapters.

App 0.3.0/build42 passed all 14 native test executables, including the AppKit
menu, identification, transport and credential policies. The app and helper
were built and their ad-hoc signatures passed strict verification.
Credential and login tests use substitutes and do not change the user's
settings or Keychain.

Kext 0.14.0/build47 was compiled and linked with the project's 109 units,
230 personalities and a verified ad-hoc signature. The binary contains no external
references to private Apple80211/IO80211/AppleBCMWLAN classes. The 8710BU/8723BU IQK
tables have private per-module linkage, including in the generators.

The final run passed all 143 CTest tests in Release (50,20 s) and the same
143 with ASan/UBSan on host targets (59,93 s). The kernel does not receive this
instrumentation. The B/C bootstrap fixture observes authority capture
only after complete identification, preserving the actual
Boot/FW/OTP/features paths and 8.611 I/O failures.

Independent AU and B/C review ended with no outstanding reproducible failures
within the software scope. MLME states, shutdown and EDCA check ownership
and firmware before commands; fast TX/rate/BA authorization uses only
memory, including without Bluetooth. Tests include changes to transport,
board, ROM and pipes, invalid firmware, errors before/after their effect and local
key/port cleanup. The AU matrix contains 988 ownership cases, 252 firmware
cases and 1.600 initialization failures; the B/C matrix contains 2.268
state/EDCA cases with and without BT. Counts do not equate to certified adapters.

The 8822C watchdog HID list query was recovered from the primary source;
regression across the three families confirms H2C0x73 only on 8822C. CCX tests
check actual ACK, tokens and expiry; USB completion is not treated as
radio acknowledgment.

Imported C sources still emit conversion warnings. The full build
passed; separate tests of the Controller/new units also checked
stack frames limited to 2.048 bytes. No claim is made that the entire
imported tree is warning-free.

Packaging checks CRC, hashes of every file and permissions. Delivery
validation extracts the ZIP to another folder, builds tests, app,
kext and installer, verifies signatures and compares the installer's app/helper
payload with the built bundle. The kernel SDK remains a pinned external
dependency; it is not included in the source ZIP.

## Criteria still needed for release

Connection, reconnection, sleep, USB removal, DHCP, sustained traffic and Bluetooth
coexistence need hardware testing for each family/revision.
Conflicting identities require additional evidence; RTL8814B requires its own HAL,
firmware and PHY. The complete catalog must not be advertised as
270 physically certified adapters.
