# Validation — driver 0.14.11/build58 and app 0.3.8/build50

Project and macOS adaptation: MaLd0n. Development delivery with complete
app/helper/kext source, tests, generators and licenses. The package's
VERIFICATION.json records exact results and final hashes.

## Adapter and network preference

The original cause was selection held only in memory by RegistryID and automatic
selection limited to a single adapter. With two USB devices after reboot, neither was
selected. Validated physical identity is now persisted, RegistryID is
resolved again and each adapter retains its last confirmed network.
Stored serial/MAC must be present and match; the port is a fallback
only when there is no durable identity. Identifiers appearing
later strengthen the preference in the same session. A different adapter or
ambiguity does not silently replace the preference.

A one-time network is remembered without saving its password. Automatic reconnection
still requires a saved Keychain profile and the option enabled. Disabling/re-enabling
the option invalidates old automatic queries and JOIN still queued.
Migration from older versions can adopt a single already-authorized link compatible
with the old network preference; two links require a choice. App selection
and macOS global service order are distinct states.

Tests execute actual Main methods with substituted IOKit/Keychain
boundaries: new IDs/order, delayed enumeration/identity, port change,
ambiguous identity, manual selection, per-adapter network, noninteractive credentials,
pause/disabled option and asynchronous cancellation. Both actual USB devices produced
valid identities with serial/MAC/VID/PID/port without logging their values.
This does not replace repeating boot and association on the machine with the new products.

## CPU and memory

Installed app49, menu closed, 16 samples over approximately 30 seconds:
`top` recorded CPU 0–0,1%, stable MEM memory at 39 MB, four threads.
Helper: 0%, 3.236 KB MEM, three threads. These are observations of that interval,
not a usage ceiling for all networks/operations. RSS and MEM from top use different
metrics and must not be added to kext memory.

The ISO8601 formatter was created/configured twice per exchange. A synchronized
cache retains the format and avoids repeated construction. Benchmark of
10.000 actual records: about 1,10–1,12 s CPU before, 20–24 ms after.
This gain applies to the logging routine; it does not mean the entire app became
50 times faster. The concurrent test also validates content/timestamps,
nonblocking FIFO/lock, permissions and private file limit.

Kext review found a B/C timer retained in the shared WorkLoop after
releasePlatform released the loop reference. Removal now happens first.
The actual teardown test retains a second simulated adapter and repeats 10.000
cycles; it measures ownership/retention, not only absence of a crash.

DMA payload requests are fixed/reused: 117.760 bytes per adapter,
or 183.296 on RTL8814AU. This excludes page rounding, descriptors,
mbufs, net80211 nodes, IOKit objects and each family's own heaps. It is not
total kext memory. CPU/memory of kernel_task are kernel aggregates;
they were not attributed exclusively to this driver.

## Actual system observed in this round

macOS 26.7.2/25G332, user-built kext build57,
UUID 4D759E33-4832-38A0-9518-54607CC0D327; app49 initially.
T9UH/RTL8814AU: en3, channel 36/5GHz, WPA2/CCMP authorized, USBerror 0.
RTL8192EU: en1, present and searching for 2.4GHz networks, without link/IP at that
moment, USBerror 0. LIST/STATUS/MAP passed on both. No new association was
forced to change this state. The default route uses en3; service en3 precedes en1.
An HTTPS GET bound to en3 returned HTTP200/577 bytes in approximately 65 ms.
This verifies data delivery and does not measure sustained speed.

The idle EU repeated channels with the same search generation. Infinite search comes
from the imported AP-search policy; this delivery's change separates searching
to list networks from searching for the JOIN profile. New radio validation
requires the user to load build58.

## Publication and limits

Generated build-*/out-* directories, SDK and Release are excluded from Git and the source
ZIP. A local or extracted SDK without Git is checked using a complete Headers/Library
manifest and revision when Git metadata exists. A divergent dependency
is rejected and preserved. Firmware, licenses and third-party notices remain.

The source contains no actual authentication data identified by the scan
performed; tests use fixtures only. This examination is limited by patterns and
is not mathematical proof that all private information is absent.

Documented matrix limits remain: RTL8814AU HT20/up to two streams,
RTL8814BU station closed, channel 144 closed and no WPA3/AX/AP mode. There is no
physical certification of all 262 eligible identities/revisions. Native RX
counters remain zero despite delivered traffic; Skywalk is active.
The exact cause of this discrepancy has not yet been demonstrated. There are no manual
increments or capability changes to conceal this observation.
