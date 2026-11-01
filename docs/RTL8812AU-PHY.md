# RTL8812AU / RTL8821AU portable PHY

`RTL8812AUPHY.c` is the real register/USB PHY layer and distinct RF calibration
for the two Jaguar-A chip families. It is separate from the older fixed
B-cut/RFE3/channel 6 diagnostic table in `PHYProfile.c`.

The source is pinned Linuxrtw88 at
[f47d223627f80cb7a1eb14c585fe6cb35de5e984](https://github.com/torvalds/linux/tree/f47d223627f80cb7a1eb14c585fe6cb35de5e984/drivers/net/wireless/realtek/rtw88).
`RTL8812AUPHY-Upstream.json` records input hashes and port adaptations.
`scripts/generate-rtl8812au-phy.py --source /path/to/pinned/rtw88` checks these
hashes before extracting 41 register/RFK functions and the BB/AGC/RF/PG/limit
arrays. Generated files are supplied with the source; compiling requires no
network or generator invocation. The selected source license is BSD-3-Clause.

The strict ROM decoder uses the 0xB8-family layout, 0xD7 vs0x107 identityMAC,
8812 two-chain amplifier masks and8821 one-chain amplifier masks. The latter are
cross-checked with FreeBSD `r21a_rom.c`/`r12a_rom_image.h` at
[685dc743dc3b5645e34836464128e1c0558b404b](https://github.com/freebsd/freebsd-src/tree/685dc743dc3b5645e34836464128e1c0558b404b/sys/dev/rtwn).
The FreeBSD BSD-2-Clause notice is retained separately. For 8821 the caller must
supply the measured Bluetooth-function flag from the hardware register: an OTP
board option is not a substitute. Missing calibration/amplifier/crystal
bytes and unknown power indices are rejected before a channel change. The
primary USB-specific RFE0xff encoding selects RFE0, and0xff is also a valid
BB-swing byte: the two paths select its respective2-bit fields. Other
unknown RFE types remain rejected. The bootstrap-only parser's historical defaulted values therefore must
not be copied blindly into a PHY configuration.

The layer accepts allocation supplied by the caller plus vendor-control and
wait callbacks. `init` performs no hardware I/O. The caller must have identified
the exact chip/cut, stopped concurrent bus/RF work, powered the MAC, and booted
the correct firmware. The layer loads conditional per-board BB/AGC/RF tables,
uses ROM crystal and band-specific baseband swing, selects the real band/RFE,
tunes2.4 GHz HT20/40 or non-DFS5GHz HT20/40 andVHT20/40/80, and programs per-mode power
indices derived from ROM, per-rate PG values and the minimum of the pinned
FCC/MKK/ETSI limit rows. These limit tables do not grant regulatory permission;
the OS integration still has to authorize the selected channel and power.

8812 uses the physical OTP path count. A1T1R module loads only RF_A, applies
the source1T BB/path-B shutdown configuration, and never polls absent-B IQK.
Every present path still requires measured ready/fail/convergence. 8821 uses its own one-path
LOK/TXIQK/RXIQK sequence;8812 RF tables and IQK are never reused for it. Actual
measured convergence is exposed as TX/RX masks. The upstream 8821 code overwrote
measured `iqk_ready`/`tx_fail` with constants; this port keeps the measured bits
so a timeout or failed calibration cannot be reported as success. Signed 11-bit
IQK coefficient packing is defined through unsigned shifts. Calibration
restores the caller's page/CCA state, preserves paused TX through LCK, validates
backups before destructive RFK, and performs bounded best-effort restores after
I/O failures. TX remains paused until explicit `resume_tx` after a successful
full calibration; initialization/tuning/fallback coefficients cannot authorize
transmission. A failed transport remains latched and cannot resume TX.

Current bounds are8812 RFE0..3 with one or two physically identified paths,
8821 RFE0 with one path,2.4GHz1..11, and non-DFS5GHz
36/40/44/48/149/153/157/161/165. Mode-specific geometry and measured
thermal tracking are exercised separately in `rtl8812au_modes_tests.cpp`;
PHY success alone does not prove AP negotiation or throughput.

Observed-Bluetooth8821AU boards use the dedicated typed coex module described
in `RTL8821AU-COEX.md`. Preparation/calibration remain paused. Every TX resume
requires a matching, active coex lease whose immutable device/board/transport
identity still agrees with its bootstrap. The kernel starts the actual primary
PTA/antenna/TDMA policy before its first scan transmission.

BT-active8812AU remains rejected: its pinned chip descriptor supplies neither
coex callbacks nor coex tables. The8821 implementation does not authorize an
8812 board or provide a hardware-context cast. This is an explicit remaining
board limitation, not a claim that all possible8812 board variants were tested.

`tests/rtl8812au_phy_tests.cpp` uses a paged-register/RF/USB model. It covers
both bands, all supported RFE selectors, separate path counts, negative IQK
coefficients, independent power-register golden vectors, missing-ROM rejection,
measured-ready/TX-failure/RX-failure rejection, LCK timeout, and every I/O failure
position in a successful RFK sequence. These are software-model checks, not
physical adapter/RF or macOS-kext validation. Native station integration is
described separately in `RTL8812AU-8821AU-NATIVE-BRIDGE.md`; physical connection
and throughput certification remain separate work.

Run the isolated ASan/UBSan model with `python3 scripts/test-rtl8812au-phy.py`.
The mode suite additionally injects3083 channel/RFK/thermal I/O failure
positions across8812 two-path,8812 OTP1T1R and8821, including measured
calibration failures. Both-band tests exercise all256 BB-swing encodings.
Valid saturated CCK reports preserve100% RSSI, with the portable sample clipped
to-1dBm because the shared station cache reserves zero for unavailable data.
