# Typed AU bootstrap and MAC

`RTL8812AUBootstrap` and `RTL8812AUMAC` provide the actual pre-PHY USB hardware
steps for RTL8812AU and RTL8821AU. Their `rtw_r12au_boot` context is distinct
from the RTL8814AU and 88xxB/C contexts. The older RTL8812-only diagnostic API
remains available; the new context is the station-integration path for both AU
families. No support flag or native station attachment is added by these units.

The primary reference is Linux rtw88 at
[f47d223627f80cb7a1eb14c585fe6cb35de5e984](https://github.com/torvalds/linux/tree/f47d223627f80cb7a1eb14c585fe6cb35de5e984/drivers/net/wireless/realtek/rtw88).
The source manifest records hashes for `rtw88xxa.c`, both chip and table files,
`mac.c`, `usb.c`, `main.c`, the legacy firmware layout and station/security
register definitions. `generate-rtl8812au-bootstrap.py` rejects changed input
before extracting separate MAC tables, USB cold enable/disable flows and LPS
transitions. All imported power steps use the upstream ALL cut mask; PCI and
SDIO branches are excluded. Generated code and the BSD-3-Clause notice ship
with the source, so compilation does not need a download or generator run.

The exact family and real USB descriptors must be supplied by the outer USB
matcher. Initialization validates direction/address, distinct OUT endpoints,
2/3/4 endpoint count and the actual 64/512/1024-byte bulk packet size for
full/high/super speed. It performs no hardware operation. Per-QSEL endpoint
mapping preserves Linux's descriptor-order HIGH/NORMAL/LOW/EXTRA assignments;
the controller must use the returned endpoint, rather than a fixed bulk pipe.

Identification reads the actual 512-byte packed OTP and decodes the 512-byte
logical map. Only RTL8812AU additionally accesses physical bytes 1018..1021 for
NSS/VHT restrictions, with the corresponding 1024-byte address bound. RTL8821AU
never reads outside its 512-byte physical geometry. MAC/VID/PID offsets are
0xD7/0xD0 for 8812 and 0x107/0x100 for 8821. The strict PHY decoder supplies the
actual PA/LNA/RFE, crystal, thermal and power information, preserving genuine
one-chain restrictions. The 8821 Bluetooth-function flag comes from hardware
register 0x68 bit18, not a substituted OTP board option. USB identity comes from
the verified descriptor; an undocumented SYS_CFG bit is not used to invent a
USB-family check. OTP gate closure is attempted on every exit; a closing-write
failure receives one bounded retry while keeping the original error and an
invalid identity. The same closing-write regression was fixed in the older
RTL8812-only API.

Power-on refuses resident firmware for the cold path. It follows distinct chip
power sequences, the documented 8812 RF reset workaround and the 8821 LDO
branch. DMA/security clocks are prepared with MAC TX/RX disabled and queues
paused. The pre-firmware DMA phase initializes the entire 256-entry LLT: data
chain, end marker and reserved-page ring. 8812 uses 512-byte TX pages and
boundary247 (9 reserved pages); 8821 uses 256-byte pages and boundary248
(8 reserved pages). LLT command completion has a finite timeout.

The legacy8051 downloader accepts the actual NIC formats: 8812 signature950x,
category0x10 versus 8821 signature210x, category0x13, both function0 and a
32-byte header. It verifies the declared RAM-image length and bounds it to
8*4096 bytes. The corresponding MCU wrapper reset uses 0x1D bit3 for 8812 and
bit0 for 8821. The USB upload follows primary Linux's196/8/1-byte chunk scheme,
never crosses a 4096-byte aperture page and requires a fresh checksum. MCU
updates preserve the write-one-to-clear checksum status. Success requires the
complete firmware-ready mask, including RAM selection, rather than just the
initialization bit. Any upload/handshake failure disables download mode and
invalidates firmware/MAC readiness. The supplied firmware is header-validated;
its provenance and cryptographic authenticity are the caller/pack's job.

The actual primary firmware images were also exercised through the USB model:
8812 version52.14, payload26998 bytes in7 pages; 8821 version42.4,
payload31866 bytes in8 pages. This is byte-for-byte modeled transport validation,
not execution of the firmware on physical hardware. The images are separate
binary artifacts and are not embedded by these source modules.

Post-firmware MAC setup loads the exact family MAC table, programs real queue
pages and mappings, boundaries, RX FIFO size, USB burst/aggregation parameters,
EDCA/SIFS/retries, beacon/TSF state, ARFR tables and hardware sequence control.
The native path keeps security register0x680 at0 and clears the CAM, so CCMP is
owned by the host net80211 stack. RCR includes PHY status and encrypted
payload/MIC information but omits APP_FCS: the AU RX payload must not have a
second four-byte FCS removal. Station preparation writes actual local MAC,
BSSID, AID and association-dependent receive filters. It neither authenticates
nor publishes link, and it keeps TX paused for the caller's final PHY/host-state
commit. H2C uses four bounded legacy mailboxes, extended word before main word,
and advances its index only on success. A failed submission is not silently
retried. Power-off invalidates readiness first, then attempts bounded LPS,
CPU/MAC shutdown and the correct family card-disable sequence even after a
latched transport error. Queue-register pause bits can be reset by the pinned
LPS flow after MAC TX/RX has been disabled; they are not a power-ready signal.

The intended outer sequence is:

1. Verify USB family/descriptors and initialize the typed context.
2. Inspect the intended firmware image, identify/decode actual OTP.
3. Power on, prepare DMA/LLT, download matching firmware, initialize MAC.
4. Initialize `RTL8812AUPHY` using `context.board.phy`, the same vendor transport
   and waiter; load tables, tune the authorized channel, calibrate.
5. Prepare host station state and resume PHY TX only after successful RFK.
6. On failure stop native transfers, stop PHY TX and power off the typed core.

The vendor-control OUT callback must support real block writes up to196 bytes;
a scalar-register-only transport is insufficient. The caller owns USB
exclusivity and serializes every bus/MAC/PHY operation. These modules do not
supply BT coexistence scheduling, HT/VHT power programming, thermal tracking,
a native net80211 attachment or a physical-hardware validation result. The
separate native adapter is documented in `RTL8812AU-8821AU-NATIVE-BRIDGE.md`. The PHY
module still documents its own supported RFE, chain, channel and bandwidth
bounds; successful bootstrap/MAC setup does not broaden them.

Run `python3 scripts/test-rtl8812au-bootstrap.py` for the ASan/UBSan model.
Optional `--firmware-8812` and `--firmware-8821` paths additionally exercise real
matching firmware files. Tests cover both exact OTP geometries, all endpoint
counts/speeds, LLT topology, queue and station golden register values, all page
boundaries through32768 bytes, checksum/WINTINI/RAM-ready failures, timeouts,
mailbox order, overlap rejection and shutdown after failure. Each of8011 normal
I/O positions is failed individually. Another sweep reports ambiguous write
failures after the write has already taken effect; station and mailbox failure
paths are checked separately. These tests execute actual driver functions with
only the USB register/firmware-memory surface modeled.
