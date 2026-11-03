# Driver and app checkup — kext 64 / app 54

This source update preserves all 13 enabled station backends and the build63 RX
bounds, aggregation-mode, alignment and strict BlockAck accounting corrections.
The [family matrix](FAMILY-CHECKUP-0.14.17.csv) identifies each family's supported
bands/modes, fixed transfer-buffer requests, connection/security paths and the
limits of physical validation. RTL8814BU remains disabled.

## Connection and application changes

Automatic reconnect now waits while the driver is scanning, authenticating,
associating or already connected. A delayed Keychain callback checks the current
connection admission again before automatic JOIN. This prevents an old password
lookup from preempting a recovered link. Explicit manual requests remain valid.
Regression cases exercise the actual Main policies/callbacks, including 15 busy
and delayed-result cases, idle reconnect/discovery and manual positive controls.

Every successful STATUS still reads the current channel map. Map-only changes
continue to revoke stale menu actions immediately. Unchanged polls now perform
zero network-model rebuilds, and simultaneous band/map/scan changes rebuild once
instead of twice. Channel maps are validated with 32 masked-byte checks; model
operations validate their immutable map once. Public ABI, allowed channels,
invalid-map refusal and security eligibility remain unchanged.

Cancelled polls stop before their next registry or IPC boundary, including
initial connection/identity discovery traversals. Twelve cancellation cases
verify that old work stops and a replacement poll retains its own busy state.

Driver details is now an asynchronous, selectable, copyable report. It reads
only the selected driver's registry snapshot on request, with a fixed numeric
whitelist and a positive even publication generation checked before, inside and
after the snapshot. Stale or inconsistent snapshots export no counters. Adapter,
work/session, sleep and termination fences prevent late reports from appearing
for another device. No periodic diagnostics task was added.

The report separates validated filtered BlockAck from RX errors and shows the
available aggregate, CRC, ICV, admission, decrypt, allocation and recovery
categories. Old drivers display missing fields as `not reported`. Driver 64
publishes its immutable build number once during startup. Raw registry data,
SSIDs, MAC addresses, packet contents and peer bitmaps are not included.

The PHY capability inventory is regenerated from the compiled adapter catalog,
with exact JSON/CSV freshness regressions. This corrects stale documentation;
it does not create new hardware support. There are 270 catalog identities,
263 verified identities and 262 station-enabled identities.

## Measurements on the installed baseline

Measurements were taken before these source changes were installed. This machine
runs app 0.3.11/build53 and driver61 with a RTL8814AU connected on 5 GHz/channel36.

| Process | CPU during a 50-second idle window | RSS range | Physical footprint |
| --- | ---: | ---: | ---: |
| App53 | about 0.04% of one logical CPU | 63.4 MiB, stable | 34.7M as reported by vmmap |
| Helper53 | about 0.04% of one logical CPU | 5.4 MiB, stable | protected process; not measured |

CPU is the change in cumulative CPU time divided by elapsed monotonic time,
with the OS counter's centisecond precision. It is not an instantaneous Activity
Monitor percentage. Sampling was completed before that idle window. The native
app main-thread sample was waiting in its event loop. No unbounded app-owned
retention was identified in the audited bounded caches, queues and histories.
A 50-second stable RSS window does not establish an unlimited leak-free runtime.

The active interface matched the driver MAC and the default route. Ten ICMP
requests to the user's gateway had zero loss and 2.8 ms average round trip.
A verified HTTPS HEAD bound to the same interface returned HTTP200. No channel
scan, JOIN, disconnect or network reconfiguration was performed. These are
baseline connectivity checks, not a build64 throughput or hardware result.
Aggregate, CRC, ICV, recovery and TX USB error counters remained zero between
the same adapter's two registry snapshots.

## Controlled source benchmarks

Identical randomized fixture digests and sanitizer checks verify equivalent
channel-map/model behavior. Repeated isolated old/new workloads show roughly
14.5x faster valid-map validation and 23x faster grouping for the tested
64-BSS/eight-SSID case. The 64-unique-SSID case showed no supported improvement.
Validation/lookups allocate nothing; vector operations retain one allocation.
These are small CPU costs measured in nanoseconds/microseconds. They are not a
whole-app CPU or RAM reduction measured after installation.

The deterministic workload is included as `app/tests/model-benchmark.cpp`.
To reproduce it on macOS from the project directory (no device is accessed):

```sh
mkdir -p out
xcrun clang++ -std=c++17 -O2 -mmacosx-version-min=13.0 -Iapp -Ikext -Iinclude app/tests/model-benchmark.cpp -o out/model-benchmark
./out/model-benchmark
```

Fixed transfer payload requests remain 117,760 or 183,296 bytes per adapter,
depending on its required RX capacity. These totals exclude other driver, mbuf,
shared net80211 and IOKit allocations. Per-kext CPU cannot be isolated reliably
from the shared kernel process with these measurements. Reducing safe RX bounds
to save memory would reintroduce USB aggregate truncation.

## Validation scope

Actual native app tests cover connection preferences, switching, refresh,
credentials, async cancellation, new diagnostic privacy/coherence and model
work. Offline family tests cover startup/teardown, WPA2/CCMP/replay, RX/TX,
recovery, bands and regulatory admission using their real software paths and
host platform boundaries. Native compilation/signature verification and
clean-archive checks accompany the local delivery.

The new build is not installed by this source delivery. Physical loading,
connection, throughput and power/sleep behavior of build64 on each adapter and
board revision still require that hardware. All project documentation remains
English and the concise project README is preserved. No GitHub update occurs.
