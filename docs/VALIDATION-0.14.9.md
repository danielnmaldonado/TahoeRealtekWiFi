# Validation — 0.14.9/build56 and app0.3.6/build48

Project and macOS adaptation: MaLd0n. Development.

## Actual evidence

The ZIP from 01/11/2026 shows build55 and app47: RTL8814AU 2357:0106/en3 with
fresh publication, but scan/RX/profile at zero; RTL8192EU 2357:0108/en1 connected.
The USB error accompanied by disconnection belongs to EU. The 20 IPS files are
historical userspace test aborts, without proof of a panic in this kext.

App48 was installed and its executable compared by SHA256 to the built product.
The CLI successfully executed LIST/STATUS/MAP on ID 4294968492 and SCAN returned
0xe00002d8 in 0,765 ms. STATUS showed USBerror at zero and no radio/search started.
Queries are metadata; they do not demonstrate physical USB transport health.
Build55 exported neither core errno nor bootstrap phase, so the specific initial
error remains unknown. No RFK/firmware cause was invented.

## Fixes and verification

Tests of actual methods reproduced and fixed: a permanent latch after
eligible failure; poweroff before drain; bypass through startup/watchdog; loss
of mixed error; missing finalizer after callback; and RX provenance lost
between EBUSY and late completion. Cleanup error does not erase the original.

The IONetworkController family called the default Unsupported hook and stopped PM
before late driver registration. The new implementation registers states
through the policy maker, requests state 1 and stops PM outside the gate before releasing
resources. Missing PM in the snapshot is consistent with this defect, but does not prove
it caused the original AU failure.

The app CLI was validated through strict parser, actual/mocked XPC boundaries and a bounded
private log. FIFO and busy flock do not block commands. CodeIdentity and
helper authentication were preserved. No password file was created.

Invalid-request fixtures were writing simulated errors into the user's log.
CTest and app/test.command now isolate the process home before starting these
binaries. Verification found records in the test home and no PID from
these tests in the normal log. Actual STATUS from the installed app remained valid.

Final Release/sanitizer/ZIP counts, native build, signatures,
installer payload and hashes are in VERIFICATION.json alongside the delivery.
Independent reviews and their scope are in REVIEWS/.

## Limits

The agent did not load kext56. There is no hardware validation of recovery,
sleep/wake, WPA2/DHCP/traffic or stability of every adapter/revision in this
build. No EFI, reboot, router or macOS protection changes were made.
RTL8814AU remains HT20 with up to two streams; VHT/AC was not enabled. Channel 144 and
RTL8814BU station operation remain closed. Complete app/helper/kext source is supplied.
