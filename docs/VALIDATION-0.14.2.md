# Validation — 0.14.2/build49 and app 0.3.3/build45

Project and macOS adaptation: MaLd0n. Status: development.

## Hardware evidence that prompted the fix

Build48 was found loaded. The RTL8192EU 2357:0108 had an authorized
WPA2 port. The RTL8814AU 2357:0106, RFE1/cut1, initialized and received frames;
after USB reconnection it associated in 2.4 GHz according to the user, but dropped again.
In two sessions, the USB host log recorded input endpoint 0x81
returning 0xe00002ed (NotResponding / transaction error), with zero bytes.
This event was immediately converted into a fatal error latched by the driver.
The log identifies the trigger; it does not determine its electrical or firmware origin.

## USB reception

Recovery accepts only NotResponding with zero bytes on an active station.
It clears halt with clearStall(true), blocks new RX while draining the four
old generations, and only then rearms buffers. Abort callbacks do not consume
the budget. Three consecutive attempts and two seconds bound the operation;
a failure in halt clear, drain, refill or budget returns to the fatal path.
Useful RX resets the sequence. TX, other errors, stop, sleep and unplug retain their
controls. Diagnostics publish attempts, completions, failures and endpoint.

The policy was compared against error handling in the
[Linux RTL8814AU driver](https://github.com/aircrack-ng/rtl8814au/blob/892641430c24308e7ae193d746b25b600ef227d8/os_dep/linux/usb_ops_linux.c#L735)
and the IOUSBHostPipe halt/abort contract in the actual Apple SDK headers.
The implementation and host tests traverse the register of 13 operational
families; this does not physically certify every adapter.

## Dual-band search

The code could select the SSID in 2.4 GHz at the end of the active round without
visiting passive 5 GHz channels. Foreground search with SCANALLBAND now
consumes enabled passive channels before selection and preserves the cache.
DU, 8812AU/8821AU, 8814AU and 8821CU/8822BU/8822CU advertise this capability.
The test executes actual scan/selection functions and the actual registration blocks
of the seven backends across five band masks/profiles.

Only implemented channels and adapters remain authorized. DFS remains
out of scope; in 5 GHz the groups are 36/40/44/48 and 149/153/157/161/165.
The app shows observed usable bands; the AP is chosen automatically.
A 5 GHz network on a channel outside these groups requires additional driver work.

## Verification and delivery

Operational RX and scan tests use actual extracted methods, modeled
USB/register boundaries and ASan/UBSan. Full results for integration,
native build, extracted source and installer payload are recorded in the
delivery's VERIFICATION.json. Models do not replace RF, WPA2, DHCP and sustained
traffic on physical adapters/revisions.

The inventory remains at 270 IDs: 263 primary identities, 262 eligible and
separate restricted discovery for 13b1:0043. Five unproven and two conflicting
IDs receive no static enablement. RTL8814BU remains without an enabled station;
its limits are in RTL8814BU-STATUS.md.

Build49 must be loaded to validate the fixes on Tahoe. Source,
app and kext are delivered together in project releases, with manifest and SHA-256.
Compilation does not automatically restart or change SIP/AMFI.
