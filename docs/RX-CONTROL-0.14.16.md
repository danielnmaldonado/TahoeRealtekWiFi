# RX control accounting — build 63

The new capture runs driver 62 and app 53 on macOS Sequoia 15.0. It was
created with collector revision 2. The two controller snapshots show
`RTWDiagRXErrors=560 -> 649`, exactly matching 89 additional
`RTWDiagRXRejectControl` events. Other rejection reasons, CRC, ICV, unexpected
hardware decryption, USB recovery and aggregate errors are all zero.
The maximum successful USB read is 25,138 bytes, within the corrected 32 KiB
RTL8192EU capacity. The screenshot's later value of 662 is outside the snapshot
interval and cannot be decomposed at that exact instant from this ZIP.

Both last-rejection samples show control subtype 9 (BlockAck), length 28,
version 0, flags 0, a local receiver and associated-AP transmitter, with an
authorized RUN association. This is consistent with ordinary compressed
BlockAck. The old capture did not retain BA Control, starting fragment or TID,
so it does not establish the complete validity of every rejected control frame.

## Source correction

The pre-stack gate only admits BAR control frames. It classified other controls
as input errors before net80211 could apply its ordinary control accounting.
The imported net80211 input path handles BAR specially; other controls normally
increment `is_rx_ctl` and exit without incrementing input errors.

The correction consumes only an ordinary single-TID BlockAck that passes the
new bounded classifier. It requires the exact control/version byte, zero FC1
flags, an authorized HT association, exact local/AP addresses, legal single-TID
BA Control fields, a zero starting fragment, and the matching complete basic or
compressed size. It does not deliver the bitmap, acknowledge TX packets, change
BA agreements, advance reorder windows or generate radio/USB operations.

The common receive path applies this correction to all 13 enabled station
families. Existing rate, CRC, ICV and security checks continue to take
precedence; rejected or unsupported controls retain their diagnostic path.
The existing admission API, BAR policy and protected data/replay handling remain
unchanged. Previous per-family capacity, aggregation-mode and alignment fixes
are retained; see [RX coverage](RX-COVERAGE-0.14.15.csv) and
[station filter coverage](RX-FILTER-COVERAGE-0.14.15.csv).

The corresponding local primary definitions are in
[`ieee80211_input.c`](../vendor/itl80211/openbsd/net80211/ieee80211_input.c)
(control subtype switch and normal `out` path) and
[`ieee80211.h`](../vendor/itl80211/openbsd/net80211/ieee80211.h)
(BA Control fields). The helper is in
[`RTL8192EUDataPath.c`](../src/RTL8192EUDataPath.c); its name reflects its
original module, while the controller invokes it for every supported family.

## Diagnostics

`RTWDiagRXRejectSchema=2` identifies the added control accounting:

- `RTWDiagRXBlockAckFiltered` counts validated, consume-only BlockAck frames
  separately from RX errors and Ethernet traffic.
- `RTWDiagRXRejectControlSubtype0` through `15` identify the subtypes of remaining
  control rejections. Their sum matches `RTWDiagRXRejectControl`.
- `RTWDiagRXRejectLastBAControl` and `RTWDiagRXRejectLastBAFragment` expose only
  scalar format metadata of the last rejected BlockAck. Unavailable values are
  65535 and 255, respectively. No bitmap, packet bytes or addresses are exported.

Collector revision 5 reports the new cumulative counter deltas and treats last
observations as gauges. The app remains build 53 and uses the driver's actual
RX error total. No existing error counter is cleared or a transport completion
reported as a wireless acknowledgement.

Regression checks exercise the classifier and exact controller accounting block
for all 13 families, including normal consume-only handling, unchanged BAR/data
admission and preservation of genuine error increments. Physical validation of
build 63 requires a fresh capture after compiling and loading the new kext.

## Offline results

The complete Release and sanitizer-enabled selections each pass all 177 CTest
entries. Targeted ASan/UBSan checks cover 5,036 classifier cases and 4,584 actual
controller-accounting cases across all 13 families. The final accounting tests
also compile against the immutable build62 baseline and fail first on its RX
error increment for an ordinary BlockAck. The original 11,010,068 admission
policy equivalence checks still pass. Collector revision 5 has 32 offline cases.
These results verify source behavior; they do not establish the runtime result
of build63 on the physical adapters.
