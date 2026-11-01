# Source provenance and reference material

This is the single human-readable source-origin guide. Pinned hashes and upstream
notice files are retained beside their components. Provenance imported from the
input archive identifies origins; it is not a claim that this review downloaded
or independently revalidated every upstream revision.

## Native multi-family ports (0.14.0 development)

The operating backends retain separate chipset contexts, wire descriptors,
firmware loaders, PHY/RFK and runtime state. Chip names or a USB ID alone do not
prove that a board can transmit. The per-component `*-Upstream.json` files
record the primary revision, imported functions/tables and local adaptations.

- AU boot/radio: `src/RTL8812AUBootstrap-Upstream.json` and
  `src/RTL8812AUPHY-Upstream.json`; BSD/FreeBSD notices remain beside the code.
- B/C operating radio and station: the pinned Linux rtw88 sources and
  `src/RTW88Station-Upstream.json`; original dual-license notices are retained.
- DU: `src/RTL8192DUBoot-Upstream.json` and `src/RTL8192DUPHY-Upstream.json`.
- FU,8710BU,8723BU: corresponding `src/*-Upstream.json` and full upstream
  GPL-2.0 license/notice files, with dedicated typed adapters.
- 8814AU: `src/RTL8814AUPHY-Upstream.json`, including per-channel factory-power
  bounds and measured RF calibration.

Bluetooth coexistence policy is imported from the pinned rtw88 source, with
separate typed AU and B/C hardware callbacks, registers, tables and context.
`src/RTL8821AUCoex-Upstream.json` and `src/RTW88Coex-Upstream.json` retain the
exact source pins. Shared policy does not imply shared chipset hardware state.
Physical coexistence validation is reported separately from these source ports.

Unmodified embedded firmware and complete redistribution notices are distributed
with SHA256 pins in `firmware/*-provenance.json`. The 8710BU vendor0/SMIC and
vendor2/UMC images have distinct exact signatures; neither is substituted for an
unknown or unsupported vendor. Their linux-firmware git object IDs and the
license object ID are checked by the generator and the source hash tests.
Generators consume explicit local upstream inputs; they do not alter the host
or silently fetch replacement firmware.

This section describes source provenance. Physical association, traffic and
board-revision coverage are recorded separately in the release readiness report.

## RTL8812AU thermal/IQK preflight references (0.13.14)

The opt-in read-only RF[0x42] *value* observation uses the Jaguar guarded
register read interface already attributed below; its temporary BB selector
writes are restored and independently verified. The RTL8812AU temperature
meter field is RF[0x42] bits 15:10, the factory reference is logical eFuse
offset 0xBA, and an eight-code-unit difference is a useful **advisory**
from the Realtek driver family, not a temperature in degrees Celsius:

- https://github.com/aircrack-ng/rtl8812au
- https://github.com/freebsd/freebsd-src/tree/main/sys/dev/rtwn/rtl8812a
- https://github.com/xen-proc/rtl8812au-macos/blob/main/rtl8812au/phy.py

The local thermal test does not trigger a new conversion, change power,
perform IQK/LCK/DPK calibration or authorize TX. No upstream code files,
firmware or closed-source blobs were newly imported.

## RTL8812AU RF monitor register reference

The guarded RF[0x18] LSSI/HSSI readback follows the GPL-2.0 vendor-derived
register sequence in the open-source `xen-proc/rtl8812au-macos` project:
https://github.com/xen-proc/rtl8812au-macos/blob/main/rtl8812au/phy.py

The reference identifies HSSI `0x8B0`, CCA guard `0x838`, RF PI mode
registers `0xC00`/`0xE00`, per-path PI reads `0xD04`/`0xD44`, and SI reads
`0xD08`/`0xD48`. It documents known unreliable RF path-B readback on its
board. This source release implements guarded readback in a separate
GPL-2.0-only component; it does not claim to have reproduced the reference
author's hardware results.

## OpenIntelWireless net80211 compatibility layer

Repository: https://github.com/OpenIntelWireless/itlwm
Pinned revision: `53c51c2cdd6e4b69beb91f310d74c53422b0f8bd` (2024-06-09,
upstream 2.4.0). This is a source pin, not a Tahoe runtime compatibility claim.

`vendor/itl80211` contains macOS-adapted OpenBSD net80211, mbuf/queue/timer glue,
bundled cryptography and Linux type headers. Intel HAL, PCI/DMA compatibility,
Intel firmware, AirPort classes and private AirPort headers are excluded.
`sources.json` is the production translation-unit list. `UPSTREAM-SHA256.json`
records imported originals; `LOCAL-SHA256.json` records the adapted snapshot.

Existing adaptations include prefixed/hidden compatibility symbols, command-gate
serialized task queues, bounded task-name copies, static OUI storage, validated
EAPOL headroom, safe key replacement/cleanup, erased session keys and removal of
SSIDs from kernel diagnostics. Current shared-runtime/controller ownership is
described in `ARCHITECTURE.md`; the old single-controller prose is superseded.

`vendor/itl80211/LICENSE.itlwm` preserves the upstream GPL text. Source headers
retain their individual GPL, BSD/ISC, MIT and public-domain notices. These notices
are not replaced by the project's top-level license.

Protocol fixtures trace to RFC 6070 section 2, RFC 3394 section 4.1 and the
upstream wlantest CCMP vector. Synthetic AP-side HMAC/SHA-1 values are generated
independently; Node/OpenSSL supplies the AES key-wrap fixture. Test-only
allocation/mbuf/work-loop substitutes do not establish kernel ABI compatibility.

## OpenBSD rtwn and USB HAL

The following provenance is retained from the supplied source. Statements about
previous compile/test runs are historical input records, not this review's result.
Current validation is reported only in `validation.json`.

```text
OpenBSD src commit: 3ff365285878f448ba65325e29c8d1e185af3e6d
Snapshot date: 2026-10-20T12:47:57Z
Repository: https://github.com/openbsd/src

Imported files:
  sys/dev/ic/rtwn.c -> rtwn.cpp
    CVS revision 1.61 (2025-08-20)
  sys/dev/ic/r92creg.h -> r92creg.h
    CVS revision 1.31 (2024-09-01)
  sys/dev/ic/rtwnvar.h -> rtwnvar.h
    CVS revision 1.16 (2023-04-28)
  sys/dev/usb/if_urtwn.c -> ../../kext/RTW8192EUHAL.cpp (USB HAL functions)
    CVS revision 1.114 (2025-06-18)

Copyright and permission notices remain in each source. The copied HAL and
common core originate from one coherent OpenBSD92E snapshot. Subsequent
local adaptations are recorded below. The build16 RTL8192EU IQK backend
r92e_iqk.inc is adapted from the separately pinned Linux implementation,
with its own GPL-2.0-only notice. Other register tables retain their
OpenBSD origin; FreeBSD and Realtek vendor sources were cross-checks.
UPSTREAM-SHA256.json records the pinned original OpenBSD sources before
adaptation. FREEBSD-REFERENCE-SHA256.json and LINUX-REFERENCE-SHA256.json
record the reference files consulted to cross-check timing/descriptor fields.
LINUX-IQK-REFERENCE-SHA256.json records the build16 IQK/descriptor references.

macOS adaptations:
  - Compile the common core as C++ against pinned itlwm net80211 compatibility.
  - _ifnet and mbuf_t replace OpenBSD-private interface/packet structures.
  - Management mbuf node ownership uses mbuf_pkthdr_rcvif.
  - ieee80211_ifattach accepts the IOEthernetController bridge.
  - USB-specific HAL uses synchronous controller callbacks; no endpoint opens.
  - DELAY is a controller callback, and transport errors remain sticky.
  - Attach checks all required callbacks and device eFuse identity.
  - I/O errors are checked after ROM/MAC/BB/RF/IQ/LC/channel programming.
  - Initial offered capabilities: station, channels1..11, legacy11b/g,
    20 MHz. Monitor and HT are not offered by this initial port.
  - net80211 retains software key/encryption/decryption/MIC/replay handling.
    Hardware crypto stays disabled (SECCFG=0); CAM helper code remains available
    as original source but is not installed as a key callback.
  - The workloop owns serialization; a reentrant busy ioctl returns EBUSY.
  - LC settle delay is 100000 us (100 ms), cross-checked against FreeBSD
    ebf1041fc854c531a27efd9f979e28d57306777d r92c_lc_calib and Linux
    f47d223627f80cb7a1eb14c585fe6cb35de5e984 rtl8723a_phy_lc_calibrate
    used by RTL8192EU; OpenBSD snapshot used only 100 us.
  - Host-only trace tests compile the actual HAL with a transport/type shim.
  - Local IQ comparison fix: require both TX/RX coefficients of every active
    chain to remain within tolerance, and reject 0xff failure sentinels in
    either run. The imported OpenBSD comparison overwrote each chain's
    result per coefficient, allowing the last coefficient to hide an earlier
    mismatch/failure. No RF tables or calibration register sequences changed.
    tests/iq_calib_core.py compiles the actual comparison routine for offline
    regression checks, including one/two-chain and tolerance boundaries.
  - Local RTL8192EU thermal decoding fix: RF 0x42 reports the six-bit value
    in bits 15:10, rather than the gen1 low-five-bit value. The 92EU threshold
    is four units, matching FreeBSD r92eu_attach/r88e_temp_read at the pinned
    reference commit above. The measurement trigger and IQ/LC register
    sequences remain unchanged; other chips keep their prior behavior.
    The same field is confirmed by the Realtek driver at
    c629cc972a0d878c930d19c4a9309c27c1e61f92, hal/phydm/halrf/halphyrf_ce.c
    and hal/phydm/halrf/rtl8192e/halrf_8192e_ce.c. tests/thermal_core.py
    compiles the actual thermal routine with offline RF input, covering
    six-bit samples, control-bit independence and threshold boundaries.

Firmware is supplied separately by the controller, validated before loading,
and must be allocated with the net80211 compatibility malloc so rtwn can free
it with the matching allocator. Firmware redistribution terms are separate
from the OpenBSD driver-code permission notices.

Validation completed without opening hardware:
  - rtwn.cpp and RTW8192EUHAL.cpp compile for x86_64 kernel C++14.
  - Offline HAL transport trace test exercises power branches/timeouts,
    1/2/3-endpoint DMA, LLT timeout, pinned MAC/BB/AGC writes, USB aggregation,
    firmware page chunking/bounds and sticky transport failures.

Hardware operation, real RF calibration results, association, WPA2 handshake,
Ethernet traffic, reconnect and sustained load still require validation on the
actual adapter and target macOS version. A successful compile or trace test
is not a claim that those hardware tests passed.
```

## RTL8814AU bootstrap, board and MAC

```text
RTL8814AU bootstrap, board and MAC source provenance

Upstream: https://github.com/torvalds/linux
Pinned commit: 0e135910deec43ffe97eca0b6af1f8f847285b59
Directory: drivers/net/wireless/realtek/rtw88/
The local source inputs were verified against Git blob SHA-1 values from the
pinned upstream directory metadata before adaptation.

Inputs (Git blob SHA-1):
  reg.h: 08e9494977e06aa08fce2f040294b786d8574ed1
  mac.c: eaa928bab240c7e991175ee2f5958de05fa83069
  mac.h: b73af90ee1d7f94d5633a12b6405e94972b26d0d
  fw.c: 945fedcd375b62b4110ab631e87889bb84e2f16b
  fw.h: 48ad9ceab6ea128cfe5ae3e8ea512a89b2e59cde
  efuse.c: b85075cd68d0a433563be736b5c3f7f26ee5f958
  rtw8814a.c: fa02aa299b5e9925428f591a609db2d9f8627ca8
  rtw8814a.h: c57c7c8f915e2824da589adc3c2d13982a6c0f44
  rtw8814a_table.c: b9ce51a09fc85e7d192d67c0dcda16d0041a099f
  usb.c: 64e1c3420e0a8cf3326042ceb6da6940e4809076
  main.c: cd9254370fcc72c9f7bcca3361a848e22c1f733c

mac.c/mac.h: 3081 power/system setup, reserved FIFO and firmware DMA.
fw.c/fw.h: mailbox/packet transport and reserved-page handshake.
efuse.c: OTP read protocol and logical map format.
rtw8814a.c/.h: chip OTP layout, RF/board traits and MAC setup.
rtw8814a_table.c: complete USB card-enable/card-disable flows and MAC table.
usb.c: boot descriptors, USB burst and RX aggregation setup.
main.c: OTP erased-field defaults and board startup ordering.

Adaptations:
- USB-only portable C callbacks with positive sticky errno; register widths,
  masked values, firmware checksum and DMA handshake operations are explicit.
- Strict OTP bounds before header and two-byte word accesses; preserves mainline
  physical1024/logical512/protect0 geometry, not vendor protect16/bank geometry.
- OTP-only identify/read snapshots/restores access, clocks and bank registers;
  it does not execute card-enable, MAC/BB/RF startup or load firmware.
- Firmware validation checks signature, lengths, overflow-safe window bounds;
  packets use an explicit zero padding byte rather than an adjacent source byte.
- Failure paths restore saved registers and preserve the first transport error.
- Firmware H2C sequence is16 bits; mailbox ext word precedes its main word.
- Software crypto policy leaves MAC security enable clear; PHY readiness is
  separate from successful firmware/MAC bootstrap.
- No embedded firmware bytes are included in these source files.

License selection: BSD-3-Clause; see RTL8814AU-LICENSE.txt.
```

The PHY origin/hash list remains in `src/RTL8814AUPHY-Upstream.json`.
Its separately preserved selected license is `src/RTL8814AUPHY-LICENSE.txt`.

## Firmware

RTL8192EU origin record:

```text
OpenBSD src 3ff365285878f448ba65325e29c8d1e185af3e6d
sys/dev/microcode/urtwn/microcode.h array rtl8192eu
https://github.com/openbsd/src/blob/3ff365285878f448ba65325e29c8d1e185af3e6d/sys/dev/microcode/urtwn/microcode.h
SHA256 c6e5beeeb2a716ce417a9ede5974d0876b18ebf0368c195da8a181f6ba6d9b4b
Length 31818
Firmware redistribution uses separate Realtek license in urtwn-license.
```

Legacy images retain exact byte counts and SHA-256 digests in
`firmware/legacy-firmware-provenance.json`. The RTL8814AU image retains its pinned
linux-firmware source, byte count, version and digest in
`firmware/rtl8814au-firmware-provenance.json`. The data arrays in this release are
unchanged from the input release.

Firmware is redistributed under its own terms:
`firmware/urtwn-license` and `firmware/LICENCE.rtlwifi_firmware.txt`.
Driver source licensing does not replace firmware licensing.

## Build dependency

MacKernelSDK source: https://github.com/acidanthera/MacKernelSDK
Pinned commit: `3f750085caa17ec3a7880f11c11bf4f48cd6a164`.
It is a separately fetched native build dependency, not included in this archive.

## Interface and signing references

The previous project recorded Linux application workflow references in
`docs/LINUX-APP-REFERENCE-SHA256.json`; these are reference metadata, not a
runtime dependency or copied macOS executable.

Apple signing references consulted during this review:
https://developer.apple.com/library/archive/technotes/tn2206/_index.html
https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution
https://developer.apple.com/developer-id/

Inside-out nested signing and recursive verification are distinct operations.
Ad-hoc signing is not the Developer ID/notarization distribution path.

## License locations

`LICENSE-GPL-2.0.txt` covers project files marked GPL-2.0-only.
`vendor/itl80211/LICENSE.itlwm` preserves that component's supplied license text.
`src/RTL8814AU-LICENSE.txt` and `src/RTL8814AUPHY-LICENSE.txt` preserve the
selected source-component notices. The firmware terms are separate as above.
All per-source copyright/permission headers remain intact. Distinct legal notices
are intentionally not deleted as if they were redundant build instructions.

## Retained RTL8192EU calibration decision

The inherited 92E IQK implementation follows the coherent pinned Linux
`rtl8xxxu/8192e.c` profile rather than mixing that revision with vendor constants.
One documented difference is that Linux writes PI_A during chain B's RX phase,
where the consulted Realtek vendor revision writes PI_B. This release does not
change that selection or claim it is physically optimal for every adapter.

TX and RX/TXIMR phases are distinct for chains A/B, retries are bounded and
candidate comparison uses signed ten-bit coefficients. Accepted-coefficient
validity is separate from value: `0x00ff` is possible data and the local invalid
sentinel is `0xffff`. A non-error register read cannot independently prove that
a plausible sample is fresh; host traces cannot validate analog RF quality.

The implementation restores preserved control state and exits IQK mode. Reading
individual RF LUT addresses is not a snapshot of the entire analog LUT, so the
project does not promise restoration of a complete analog image. The exact pins
and source hashes remain in `vendor/rtwn/LINUX-IQK-REFERENCE-SHA256.json`.


## USB aliases added in 0.13.9

Only numeric identity/family/matching facts were added from the following primary
driver tables. No additional radio source, firmware or binary driver was imported.
Per-profile locations and revision metadata are in the single catalog.

- pvaret/rtl8192cu-fixes, commit
  `f0dfbb46a891820b27942ba3e213af83f2452957`, `os_dep/linux/usb_intf.c`,
  `RTL8192C_USB_IDS` under `CONFIG_RTL8192C`:
  https://github.com/pvaret/rtl8192cu-fixes/blob/f0dfbb46a891820b27942ba3e213af83f2452957/os_dep/linux/usb_intf.c
- torvalds/linux, named release `v6.18`,
  `drivers/net/wireless/realtek/rtlwifi/rtl8192cu/sw.c`,
  `rtl8192c_usb_ids` / `rtl92cu_hal_cfg`:
  https://github.com/torvalds/linux/blob/v6.18/drivers/net/wireless/realtek/rtlwifi/rtl8192cu/sw.c
- torvalds/linux, named release `v6.18`,
  `drivers/net/wireless/realtek/rtw88/rtw8814au.c`,
  `rtw_8814au_id_table` / `rtw8814a_hw_spec`:
  https://github.com/torvalds/linux/blob/v6.18/drivers/net/wireless/realtek/rtw88/rtw8814au.c

The Linux entries use a named release rather than an invented commit hash.
The new C/CU profiles add a project restriction to a vendor-specific interface,
configuration 1/interface 0; those restrictions are not falsely attributed to
the source USB_DEVICE entries. The AU source explicitly restricts interface
class/subclass/protocol to 0xff/0xff/0xff. New aliases do not inherit a fictitious
legacy plist bundle or bcdDevice value.

The RTL8192C table confirms `2357:0100` and `0eb0:9071`. It does not verify the
different legacy identities `038b:0100` and `0e0b:9071`; those remain separate
disabled records. Similar names or transposed digits are not identity evidence.

The primary OpenBSD USB/core source was also inspected for RTL8188F integration:
https://github.com/openbsd/src/blob/3ff365285878f448ba65325e29c8d1e185af3e6d/sys/dev/usb/if_urtwn.c
Existing partial F-family branches do not provide an assembled station backend
in this project. This release does not register them as operational support.

## RTL8812AU / RTL8821AU wire-field references (0.13.10)

The new portable parser/serializer was written for this project using hardware
field definitions from the FreeBSD rtwn driver, primarily the following
BSD-2-Clause and ISC licensed source references. No upstream source files were
incorporated wholesale. Their original attribution and license requirements
remain with the authors.

- https://github.com/freebsd/freebsd-src/blob/main/sys/dev/rtwn/rtl8812a/r12a_tx_desc.h
- https://github.com/freebsd/freebsd-src/blob/main/sys/dev/rtwn/rtl8812a/r12a_rx_desc.h
- https://github.com/freebsd/freebsd-src/blob/main/sys/dev/rtwn/rtl8812a/usb/r12au_tx_desc.h
- https://github.com/freebsd/freebsd-src/blob/main/sys/dev/rtwn/rtl8812a/usb/r12au_rx.c
- https://github.com/freebsd/freebsd-src/blob/main/sys/dev/rtwn/rtl8192c/r92c_rx_desc.h
- https://github.com/freebsd/freebsd-src/blob/main/sys/dev/rtwn/rtl8812a/r12a_tx.c

Those reference formats do not establish compatibility with each Realtek
board revision. A descriptor can be syntactically correct while firmware, USB
endpoints, EFUSE board data, RF calibration, channel setup and association are
all unsupported. No third-party firmware or station implementation was added.


## RTL8812AU / RTL8821AU RF and 802.11 station references (0.13.11)

The offline channel geometry and station frame parser are locally authored
code following publicly documented 802.11 management element layouts.
FreeBSD rtwn RTL8812AU and RTL8821AU reference implementations were
consulted for family-specific RF limits and the separation of channel
selection, RF calibration, TX power, and association:

- https://github.com/freebsd/freebsd-src/blob/main/sys/dev/rtwn/rtl8812a/r12a_chan.c
- https://github.com/freebsd/freebsd-src/blob/main/sys/dev/rtwn/rtl8821a/r21a_chan.c
- https://github.com/freebsd/freebsd-src/blob/main/sys/dev/rtwn/rtl8812a/r12a_init.c
- https://github.com/freebsd/freebsd-src/blob/main/sys/dev/rtwn/rtl8812a/usb/r12au_attach.c

No upstream source files or firmware were imported for this increment. The
new geometry and join modules contain no register writes, transmit authorizer,
country policy or cryptographic key handling; these references do not
constitute tested PHY support on Tahoe.

## Multi-family transport references (0.13.17)

- Linux Wireless Realtek driver overview:
  https://wireless.docs.kernel.org/en/latest/en/users/drivers/rtl819x.html
- Linux `rtl8xxxu` source reference for RTL8192FU and RTL8723BU:
  https://github.com/lwfinger/rtl8xxxu
- Linux `rtw88` source reference for RTL8821CU, RTL8822BU and RTL8822CU:
  https://github.com/lwfinger/rtw88
- Linux staging RTL8192DU driver provenance:
  https://wireless.docs.kernel.org/en/latest/en/users/drivers/rtl819x.html

These are architectural/source references only. No Linux kernel code was
copied into this revision; macOS PHY/RF and station functionality is not
implied by a reference to Linux driver support.


## 0.13.18 multi-family wire-format and firmware references

- Linux rtw88 `rx.h` (RX 24-byte descriptor fields):
  https://github.com/torvalds/linux/blob/master/drivers/net/wireless/realtek/rtw88/rx.h
- Linux rtw88 `usb.c` (8-byte USB RX aggregate alignment):
  https://github.com/torvalds/linux/blob/master/drivers/net/wireless/realtek/rtw88/usb.c
- Linux rtw88 `rtw8821c.c`, `rtw8822b.c`, `rtw8822c.c`
  (RX descriptor size 24; board-specific firmware and radio initialization):
  https://github.com/torvalds/linux/tree/master/drivers/net/wireless/realtek/rtw88
- Linux rtl8xxxu `core.c`, `8192f.c`, `rtl8xxxu.h`
  (RX descriptor parsing, 32-byte firmware header, 8192FU hardware):
  https://github.com/torvalds/linux/tree/master/drivers/net/wireless/realtek/rtl8xxxu
- Linux rtl8192d `fw_common.h` (92D0-92D3 firmware header signatures):
  https://github.com/torvalds/linux/blob/master/drivers/net/wireless/realtek/rtlwifi/rtl8192d/fw_common.h

The new portable C translation units are original code implementing bounded
validation against these formats. No claim is made to complete the Linux
hardware drivers' radio boot, RF calibration, association or WPA2 paths.


## 0.13.19 — legacy firmware staging references

- Linux `rtl8xxxu/core.c`: firmware page selection, 8051 startup,
  MCUFWDL checksum/ready polling and USB vendor transport:
  https://github.com/torvalds/linux/blob/master/drivers/net/wireless/realtek/rtl8xxxu/core.c
- Linux `rtl8xxxu/8192f.c`: RTL8192FU-specific firmware RAM window:
  https://github.com/torvalds/linux/blob/master/drivers/net/wireless/realtek/rtl8xxxu/8192f.c
- Linux `rtl8xxxu/rtl8xxxu.h`: MCU firmware registers and RTL8710BU
  SoC-specific control layout:
  https://github.com/torvalds/linux/blob/master/drivers/net/wireless/realtek/rtl8xxxu/rtl8xxxu.h

The C firmware planner/experimental downloader is a new adaptation of the
GPL-2.0-only Linux hardware sequence, not a wholesale Linux driver port.
Its format checking is not firmware signature verification by a trusted key.
The optional USB tool and synthetic device harness were developed for this
project. Vendor USB control writes and hardware initialization have not been
validated against physical hardware. The upstream GPL notice remains retained.
