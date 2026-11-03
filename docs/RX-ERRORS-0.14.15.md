# RX error repair — build 62

This local source update investigates the supplied RTL8192EU `2357:0108`
capture and audits the RX paths of all 13 supported station families.

## Captured error accounting

The capture uses macOS Sequoia 15.0, driver 61 and app 53. Over about 50 seconds,
the controller error components sum to 97,870→97,881, bracketing the screenshot's
97,880. They comprise 16,585 aggregate errors, 335 CRC drops, 154 ICV drops,
80,787→80,798 admission rejections and 9 unexpected hardware-decryption drops.
Allocation/copy/missing-node drops and USB recovery errors are zero.

The only increase is 11 admission rejections, also matching net80211 input errors.
The old gate combines length/version/header checks, unsupported control frames
and BAR authorization/context/address checks. The capture cannot distinguish
these causes; it does not prove that 80,798 frames were damaged or foreign BARs.
Historical aggregate errors include 16,566 truncations and 19 malformed transfers.
Original USB buffers and effective aggregation-mode readback were not captured.
Native RX accounting remains zero despite 58 additional Ethernet handoff packets.

## Repair scope and validation

Receive capacity and aggregation-mode selection are checked per chipset rather
than assuming that a 16 KiB host request fits every USB aggregation setting.
Complete final payloads permit optional alignment padding; descriptor, PHY,
shift, payload and required FCS bytes must still be present. Station filtering
retains physical-address, multicast/broadcast and BSSID requirements, with scan
and monitor policy preserved. [The family matrix](RX-COVERAGE-0.14.15.csv)
records allocation, initial/replacement/recovery read sizes and codec paths.

The 32 KiB read policy covers RTL8192EU, RTL8812AU, RTL8821AU, RTL8814AU,
RTL8821CU, RTL8822BU and RTL8822CU. The other six retain their coherent
16 KiB allocation. RTL8192CU/RTL8188EU clear the stale USB aggregation selector
before selecting DMA aggregation; RTL8192DU clears stale aggregation enable
because its decoder handles a single unit. RTL8812AU/RTL8821AU also remove a
64-unit limit that could reject a complete aggregate within the supported
buffer size; the replacement bound derives from maximum buffer/minimum unit.
For each of the six newly expanded families, the fixed transfer-buffer budget
increases from 117,760 to 183,296 bytes per adapter (four RX and twelve TX
buffers). Allocation/reuse/teardown tests check these exact per-family totals
and every individual buffer capacity.

Regression tests exercise all 13 typed codecs through actual controller
allocation, initial submission, replacement and recovery. They check complete
payloads with absent/partial final alignment, incomplete wire/FCS rejection,
valid-prefix handling, and preservation of unrelated mode bits. These are
source corrections and offline checks, not measured hardware results.

[Station filter coverage](RX-FILTER-COVERAGE-0.14.15.csv) includes all 13 families.
The shared RTL8192CU/RTL8188EU/RTL8192EU station RUN path clears accept-all-unicast
and restores its control subtype policy after monitor use. The ten private
backends already use physical-address matching and retain their own register
layouts. Tests compile their real MAC/filter implementations; no legacy ACF bit
is copied into the newer AU/B/C bit-12 layout. Multicast, broadcast, associated
BSSID checks, scan and monitor behavior are preserved.

## Offline verification

The complete sanitizer-enabled host selection passed all 175 CTest entries.
Targeted checks include 7,784 controller/capacity cases across all 13 families,
11,010,068 admission-policy equivalence checks, independent AU aggregate-bound
checks and 31 collector cases. Exact resource tests cover buffer reuse, failure
rollback and teardown. Native compilation and clean-archive verification are
recorded with the local delivery; no hardware validation is implied.

## Rejection census

Admission decisions and error increments are preserved. `RTWDiagRXErrors`
exports the same cumulative value used by the app. `RTWDiagRXReject*` counters
split length, version, header, control, BAR authorization/context/flags/receiver/
transmitter and unsupported-rate rejection. They sum to `RTWDiagRXInvalidFrameDrops`.
`RTWDiagRXRejectSchema=1` identifies the new fields.

Last-rejection fields contain only scalar reason/type/subtype/version/length,
rate/security/PHY metadata, state, authorization and address-match booleans.
No packet contents, MAC addresses, SSIDs or credentials are exported.
Codes: 0=no sample,1=length,2=version,3=header,4=control,5=BAR authorization,
6=BAR context,7=BAR flags,8=BAR receiver,9=BAR transmitter,10=unsupported rate.
Collector revision 4 shows counter deltas and keeps last observations as gauges.

The supplied logs do not establish the exact live rejection subtype or the cause
of every historical truncation. A fresh capture after compiling and loading build 62
is needed to validate hardware behavior. No GitHub update, installation, driver
load, EFI/security change or reboot is performed by this source delivery.
