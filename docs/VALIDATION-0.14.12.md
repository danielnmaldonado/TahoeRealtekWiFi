# Validation — 0.14.12/build59, app0.3.8/build50

Project/macOS adaptation: MaLd0n. Development delivery.

The only behavioral change since build58 is the boolean property
IOBuiltin=Yes on IOEthernetInterface. The setter is in configureInterface,
after type/statistics validation and before ether_ifattach and publication.
There is no write to the USB provider or to IOPrimaryInterface, BSD names or routes.
The instruction is in the common path for all families publishing an interface.

App50 and all radio, association, WPA2, RX/TX and recovery paths
retain the previous source. Evidence from this round is in the delivery's
VERIFICATION.json. Native compilation/signing validate use of the SDK setter;
IOBuiltin on hardware can only be checked after loading build59.
The delivery does not replace the in-memory kext or restart the computer.

To check after boot, examine each RTWUSBController's child IOEthernetInterface
in IORegistry: IOBuiltin should be a boolean Yes.
Previous limits remain in VALIDATION-0.14.11.md and README.
