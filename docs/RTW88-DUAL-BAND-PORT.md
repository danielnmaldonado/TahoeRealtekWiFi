# Other dual-band Realtek families — native port work,30/10/2026

This audit started from source build45/app40. The catalog still has270 USB
identities, with122 existing station-eligible profiles. A verified USB identity,
RX parser, TX encoder or firmware downloader alone does not make a station.

| Family | Catalog IDs / verified | Hardware bands / nominal PHY | Remaining station work |
|---|---:|---|---|
| RTL8192DU |5/5|2.4+5GHz,2T2R HT40|PHY/RF calibration and power, kernel lifecycle and net80211/WPA|
| RTL8821CU |12/5|2.4+5GHz,1T1R VHT80|Kernel hook integration, physical WPA2/IP/traffic validation and broader BT-coexistence policy|
| RTL8822BU |31/30|2.4+5GHz,2T2R VHT80|Kernel hook integration, physical WPA2/IP/traffic validation and broader BT-coexistence policy|
| RTL8822CU |2/2|2.4+5GHz,2T2R VHT80|Kernel hook integration, physical WPA2/IP/traffic validation and broader BT-coexistence policy|
| RTL8814BU candidate |1/0|Unverified in this project|Identity and a verified board HAL first; stays disabled|

Some adapters sold as RTL8812BU use USB IDs mapped by upstream rtw8822bu.c to
`rtw8822b_hw_spec`. The catalog represents those confirmed IDs as RTL8822BU;
this is not permission to guess the chip behind any similarly named adapter.
The RTL8812AU, RTL8821AU and RTL8814AU ports are separate work streams.
RTL8192CU, RTL8188EU and RTL8192EU remain2.4GHz hardware families.

## Implemented primitives

`RTW88USBDataPath` now emits real48-byte USB TX descriptors for RTL8821CU,
RTL8822BU and RTL8822CU. It encodes legacy/HT/VHT rate, sequence, aggregation,
RTS, firmware rate control and report fields from the pinned Linux rtw88
format. RTL8821C/8822B checksum the first32 bytes; RTL8822C checksums all48.
The builder validates family NSS/rate/width, rejects5GHz CCK, preserves output
on errors and rejects option/output aliasing. Security fields stay zero for
software-encrypted MPDUs. No USB submission is implied by a descriptor.

`RTW88Firmware` implements the actual Linux3081 reserved-page/IDDMA/CPU-start
flow behind bounded synchronous bus callbacks. It parses64-byte headers,
validates complete segment lengths and conservative memory windows, transfers
4096-byte chunks with TX48, appends an explicit zero for the USB512-byte
workaround, polls beacon-valid/IDDMA/firmware-ready, preserves LTE coexistence
register0x38 and restores saved registers on failure. Chip contexts are typed
for B/C families and never cast to the8814AU HAL. Caller-supplied scratch
storage avoids a4KB kernel-stack frame.

Board identification and chip-specific power-on must already have succeeded.
The new `RTW88Boot` bus adapter supplies the source-confirmed LTE coexistence
handshake; callers can pass its typed firmware context directly. The downloader performs no RF configuration or station attach and
is not called by automatic kext matching. Conservative memory ranges are port
bounds, not a claim about the largest RAM configuration on every silicon cut.
Header/length checks are not cryptographic firmware authentication.

## MAC/DMA setup added in the same work

`RTW88MAC` now computes each chip's real FIFO and reserved-page layout and
performs MAC/LLT/H2C/USB configuration through the typed B/C bus context.
RTL8821C uses512 TX pages and16KB RX FIFO; RTL8822B/C use2048 pages and24KB RX
FIFO. RTL8822C reserves16 driver and50 CSI pages, unlike8821C/8822B's8 driver
and zero CSI pages. Two/three/four bulk-out endpoints select the upstream
priority-page table and DMA mapping; invalid QoS TIDs8–15 are rejected.

The chip-specific MAC register sequences preserve read-modify-write masks.
They are generated from the complete pinned upstream functions (37 operations
for8821C,33 for8822B,75 for8822C), not copied from the8814AU table. The generator
checks upstream hashes before running those functions against deterministic
register models. Initialization checks identified/powered/running-firmware
preconditions and never sets a PHY-ready state. LLT timeout, H2C ring mismatch
or any I/O failure leaves MAC readiness false and disables CR.

## Board identity, power and OTP

`RTW88Boot` now validates an immutable, primary-verified USB catalog profile,
reads the real silicon cut/RF configuration and executes the exact per-chip
USB power-on/off sequences. The tables retain cut masks, including8822B cut-C
registers, rather than using an8814AU sequence. Unknown/test silicon and cut
masks fail closed. Power polls are bounded at the upstream20,000 attempts;
transfer failures stop the sequence and keep readiness false. Stop is a recovery
operation that preserves the original error.

The adapter supplies the real0x1700/0x1704/0x1708 LTE indirect handshake to
`RTW88Firmware`. OTP reading follows the source-confirmed powered/running-FW
stage.8821C has512 physical/512 logical bytes;8822B has1024/768;8822C has512/768,
with protected tails96/96/124 bytes. The decoder bounds every header/pair and
excludes the protected tail. USB MAC addresses come from0x107 for8821C/8822B
and0x157 for8822C. Thermal/RFE/package and power-index parsing is per chip,
including8821C's RFE2/4 two-gigahertz power-table remapping.

The read path disables2.5V programming voltage at0x37 bit7 for8821C/8822B,
or0x29 bit0 for8822C, selects Wi-Fi bank0 and restores prior bank/voltage
settings after success or failure. There is no OTP programming API. These
three upstream chip ops have no additional `efuse_grant` callback; the port
therefore does not invent8814AU-style0xcf grant writes. An incomplete read,
invalid MAC or unsupported RFE never becomes a valid board.

## Conditional PHY boot tables and RF transport

`RTW88PHY` now performs the real BB/RF reset, condition-selected MAC/BB/AGC/RF
boot tables,8821C BTG table where appropriate,8822C RFK initialization table
and crystal-cap programming. It includes147,288 exact pinned table words.
The8822C RF-B/RF-A load order and each table's physical path are preserved.
Direct RF reads and8821C/8822B SIPI writes are implemented, along with8822C's
source-confirmed direct/SIPI mix (register0 uses SIPI). Masks, paths, register
bounds and delays are checked; any failed operation aborts the load.

`tables_loaded` only records completion of these tables. It is deliberately
not a PHY-ready or station flag. The operating stage below performs TRX/RFE,
transmit-power and IQK/DAC/DPK procedures. Source-level progress does not
establish successful calibration on physical hardware.

The explicit sequence is `boot_init` → `boot_identify` → `boot_power_on` →
`fw_download` → `boot_read_board` → `mac_initialize` → `phy_load_boot_tables`.
The boot context must remain at a stable address because its bus adapter
points back to the context. No step is automatically attached to a new USB
personality.

## Evidence and limits

The source references are pinned to Linux commit
`0e135910deec43ffe97eca0b6af1f8f847285b59`. Each local source blob was checked
against its upstream git blob identity. Exact URLs/hashes and externally
retrieved firmware fixture hashes are recorded in
`src/RTW88USBDataPath-Upstream.json`. Firmware blobs and redistribution notices are now embedded by the parent
packaging work; their immutable hashes are independently verified.

TX tests compare complete literal management and VHT2SS descriptor fixtures,
including the chip-dependent checksum difference and exact reserved/H2C control
defaults; check every rate code for
1T1R; and reject invalid lengths, fields, overlaps and unsupported families.
Firmware tests exercise three simulated chip boot traces, injected write
failures, bounded DMA/CPU timeouts, segment fragmentation, explicit zero
padding, invalid headers and memory regions. These are host tests of the
production functions, not evidence of physical B/C hardware operation.

Boot tests execute real production power/OTP/board code against deterministic
register responses, inject232 I/O failures, and check voltage restoration,
power/OTP/LTE timeouts, protected-tail boundaries and cut-specific sequences.
PHY selection is compared against84 trace hashes generated by executing the
complete pinned Linux `check_positive`/`rtw_parse_tbl_phy_cond` functions.
Separate tests check literal RF wire writes, actual table execution, malformed
branches/registers, prerequisites and injected transport failures.

Operational dual-band Wi-Fi for these families still requires kernel lifecycle
wiring, actual USB submission, net80211 association/security integration and
tests on the corresponding hardware. No new station flag or USB personality
was enabled by these source ports.

## Station MAC and firmware commands

`RTW88Station` implements port0 MAC/BSSID/AID, RX filters, software CCMP
configuration, passive/active scan TX pause, association timing, EDCA and
HT20 rate messages for the three B/C families. Actual TX stream count is a
required argument, rather than a family maximum inferred as board capability.
The pure HT20 codec is shared with8814A because Linux uses the same H2C
layout; device contexts remain distinct. No VHT or wider-channel mode is
advertised by this module.

A board PHY/regulatory callback must report the selected channel as RX-tuned
or TX-calibrated/permitted. AUTH/ASSOC/RUN require the latter. Passive SCAN
keeps every TX queue paused. RUN does not authorize the net80211 controlled
port, install keys, publish carrier or submit frames. Error paths clear
station/MAC readiness and attempt bounded quiescence.

`rtw88_fw_h2c` owns the single mailbox cursor in the firmware context, shared
by station, coexistence and RFK. It polls HMETFR at most31 times with a3ms explicit delay budget, writes EX before MAIN
and advances only after successful writes. Firmware startup resets the cursor.
The station tests cover all three families, both bands,1S/2S HT20, passive
RX-only readiness, mailbox wrapping/sharing, timeouts and150 I/O failures.
Kernel integration and physical WPA2/IP remain separate work.


## Operating PHY, power and measured calibration

`RTW88PHYOperating` ports a212-function dependency closure from the same pinned
Linux source, with62 referenced objects and each RFE-specific power-by-rate,
regulatory limit and thermal-track table. It uses an explicit B/C private
context; it does not cast an8814AU HAL. The generated function source hashes,
input git blobs, output hashes and checked adaptations are recorded in the
provenance JSON and reproduced by `generate-rtw88-operating.py`.

The implementation executes8822B/C TRX configuration,8822B RFE finalization,
8821C RF-set switching, per-chip2.4/5GHz20/40/80MHz channel tuning, factory
TX-power differences, region/world power limits and real TXAGC writers.
Negative factory differences are retained through the final clamp; unknown
factory base indices cannot authorize maximum transmit power. The default
regulatory domain is worldwide. Channel geometry validation is not regulatory
permission: the station caller still applies country/channel/DFS restrictions
and device bandwidth capabilities before calling the operating API.

Firmware IQK is implemented for all three chips.8822C also executes RF trims,
ADC/DAC calibration,TXGAPK,DPK and LC calibration, including per-path saved
register restoration. IQK requires a fresh firmware completion and checks
failure status. DAC requires actual valid sample counts,stable sample range
and measured residuals. DPK requires successful paths rather than a constant
ready value. Polls preserve source timings and have finite deadlines; an
additional per-stage500,000-operation/120-second delay budget prevents a bad
transition or transport from keeping the kernel indefinitely busy. Failure
attempts primary register restoration, then per-chip bounded power-off and
clears MAC/table/radio readiness while preserving the original error. Actual
hardware reconfiguration additionally reads the live 3081 MCU status using the
primary `FW_READY_MASK/FW_READY` format (`0xcfff/0xc078`). The same check guards
every TXPAUSE restoration, including imported RFK cleanup. Firmware loss or a
failed status read latches the original error and revokes firmware/MAC/PHY
readiness. Failure before reconfiguration causes no RF writes; failure after
pausing TX cannot restore an enabled pause mask. Cached readiness, status and
per-frame rate metadata remain pure. The actual-PHY suite covers 252 health,
clock-selector, I/O and mid-operation-loss cases across all three families and
both bands.

`boot_read_board` now retains the complete physical OTP bytes, including the
protected tail needed for8822C RF trim reads. A logical-only parser cannot
claim this cache is valid. The power epoch changes on identify/power/stop and
invalidates old calibrated state; reloading PHY tables invalidates it too.

After the boot/table sequence, the caller allocates64KiB of8-byte-aligned,
nonoverlapping scratch at a stable address, then calls `radio_initialize` →
`radio_tune` → `radio_calibrate`. No operation may race station TX,RFK or a
firmware command; the controller must serialize them. The scratch remains
alive until `radio_invalidate`. Invalidate before a firmware-only restart,
USB removal or freeing scratch. The USB bulk callback receives the complete
48-byte TX descriptor plus32-byte IQK H2C payload on queue19. RFK uses the
canonical `rtw88_fw_h2c` mailbox cursor; IQK packet sequence is separate from
that mailbox cursor and can be shared explicitly with startup commands.

AUTH/ASSOC/RUN must require `rtw88_phy_radio_ready()` and an exact match of the
reported primary/center/width/index tuple. `channel_ready` alone permits
receive tuning, not TX authorization. The station must additionally report
actual MLME association through `radio_set_associated` for segmented IQK;
calibration readiness never means association. A serialized watchdog calls
`radio_thermal_track` for real per-chip thermal power/DPK tracking. TX reports
feed actual descriptor rates through `radio_set_tx_rate`; tune starts with the
band's basic rate until feedback arrives. Thermal
failure clears readiness and powers down instead of leaving a stale link.
The station module currently negotiates HT20; a PHY width function does not
by itself advertise40/80MHz or VHT in the application.

Operating tests exercise successful3-chip TRX/power/channel/calibration and
thermal paths, literal IQK descriptor/header fields, fresh versus stale IQK,
ADC/DAC sample and RFK timeouts, negative factory-power clamping, invalid
geometry, power-epoch invalidation and21 injected IO/status failures, including saved LTE-grant restoration. They run
against modeled registers and actual production functions with ASan/UBSan.
They are not physical B/C association,WPA2 or traffic validation.


## Native kernel B/C backend

`RTW88Backend.inc` now supplies real attach/start,state transitions,HT20 RA,
TX48 submission,software CCMP/controlled-port checks,RX FCS handling,
page0/page1 RSSI,C2H rate feedback,thermal watchdog and stop/suspend/unplug
methods. Actual C2H19 hardware-feature reads cap active antennas and NSS,
separately from physical RF registers. General/PHYDM startup packets and IQK
share one long-packet sequence; mailbox commands share the firmware cursor.
The exact controller integration hooks and build registration are in
`RTW88-KERNEL-INTEGRATION.md`. Source integration and modeled test success
still do not establish physical WPA2/IP or throughput on B/C hardware.
