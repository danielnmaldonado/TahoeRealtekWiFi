# RTL8710BU / RTL8188GU source port

This is a dedicated RTL8710BU implementation, with an opaque `rtw_r10bu`
context and separate controller adapter. It does not alias another chip's
hardware state or enable catalog entries itself. The integration owner controls
USB matching, capability publication, CMake and the generated kext project.

The primary implementation is Linux rtl8xxxu at immutable commit
`0e135910deec43ffe97eca0b6af1f8f847285b59`: [8710b.c](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/drivers/net/wireless/realtek/rtl8xxxu/8710b.c),
[8188f.c](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/drivers/net/wireless/realtek/rtl8xxxu/8188f.c) and
[core.c](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/drivers/net/wireless/realtek/rtl8xxxu/core.c).
`src/RTL8710BU-Upstream.json` pins all five input SHA256 values and 54 imported
functions. `scripts/generate-rtl8710bu.py --source <reference> --out <directory>`
reproduces the algorithms, register/constants headers, tables and manifest.
Original GPL notices and `src/RTL8710BU-LICENSE.txt` are retained. The separate
wire serializer retains the original BSD notices and specializes the source
`rxdesc24` / `fill_txdesc_v2` format to one RF path.

The implementation preserves the chip's actual differences:

- Scalar USB addresses below0x100 use the0x8000 aperture. SYSON registers and
  physical OTP use the0x806c command /0x80f8 address /0x809c data bridge, including
  bounded completion polls. No OTP programming command is emitted.
- Normal silicon/cut/vendor comes from SYSON CFG0; ROM revision from CFG2.
  Physical package byte0xf8 selects the U/S BB/RF tables. Only known0xee/0xfe
  packages are accepted; erased0xff uses the primary vendor-derived package
  inference. Unsupported test silicon, TSMC/unknown vendors and corrupt ROM
  fail before firmware/RF. The logical ROM has magic0x8195, MAC at0x11a,
  crystal/channel data at0xc8/0xc9 and one12-byte TX-power structure at0x20.
  EEPROM strap flags remain diagnostic, as the source always reads this OTP.
- Firmware uses its own0x8090 control register, required bit8 before download,
  the0x1000 paged window, four-byte writes and exact two-byte final tails.
  Identified vendor0 requires the shipped SMIC signature0x10b1; vendor2 requires
  UMC0x10b2. Both complete16.0 payloads are exercised byte-for-byte in host tests.
  The firmware includes, exact blobs, license and hashes remain documented in
  `firmware/rtl8710bu-firmware-provenance.json`.
- Actual power/MAC/DMA/LLT, package BB/AGC/RF, crystal, channel and HT20/40
  operations precede TX readiness. LC must complete and TX+RX IQK must contain
  measured nonzero compensation, rather than the source's fallback matrices.
  All needed ROM power arithmetic is validated before register packing.
- Hardware security is disabled, CAM clearing is polled, and every descriptor
  preserves net80211's management/data sequence in DW9 without HW_SEQ.
  RTS/CTS sets both source-required RTS and HW_RTS bits. Software CCMP,
  replay/MIC validation and the controlled port stay in imported net80211.
- One stream supports MCS0–7/SGI,20/40MHz on channels1–11. The kernel bridge
  derives its actual channel modes from board power prerequisites, removes
  VHT/HE modes, and requests AMPDU only for a live authorized RSN BA session.
  Firmware RA/media/RSSI and EDCA use the source gen2 command/layout formats.
  Thermal monitoring samples the actual meter; no new power-correction curve
  is invented. Stop follows the chip's own bounded RF/MCU/CM4-suspend sequence.

Tests execute the actual portable algorithms and kernel state/TX/RX methods
against isolated USB/host boundaries. They cover both firmware vendors, both
package tables,1/2/3 OUT pipes, HT20/40, controlled-port and software-crypto
boundaries, faults before/after hardware effects, stuck indirect/FW/LLT/CAM/LC/IQ
completion, imported channel-init/AUTO/HT negotiation and malformed wire input.
They do not certify physical RTL8710BU association, DHCP or throughput. No RTL8710BU
hardware was accessed during this implementation.
