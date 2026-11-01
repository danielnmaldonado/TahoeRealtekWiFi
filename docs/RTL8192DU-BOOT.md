# RTL8192DU USB boot primitives

`RTL8192DUBoot.h/.c` ports the DU USB power and 8051 download sequence from
Linux rtlwifi. It is standalone C with synchronous injected USB/wait callbacks,
no allocation, bounded polling and native tests. The kernel station bridge
now uses these primitives; boot alone never authorizes RF/TX or enables a USB
ID. Physical DU association/traffic validation remains outstanding.

## Source and family differences

The reference is Linux commit
`0e135910deec43ffe97eca0b6af1f8f847285b59`. Original GPL-2.0 notices are retained;
file hashes and symbol provenance are in
`src/RTL8192DUBoot-Upstream.json`. Primary files:

- [rtl8192du/hw.c](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/drivers/net/wireless/realtek/rtlwifi/rtl8192du/hw.c): `_rtl92du_init_mac`, `_rtl92du_init_power_on`.
- [rtl8192du/phy.c](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/drivers/net/wireless/realtek/rtlwifi/rtl8192du/phy.c): `rtl92du_phy_set_poweron`.
- [rtl8192du/fw.c](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/drivers/net/wireless/realtek/rtlwifi/rtl8192du/fw.c): `rtl92du_download_fw`.
- [rtl8192d/hw_common.c](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/drivers/net/wireless/realtek/rtlwifi/rtl8192d/hw_common.c): production chip/PROM mode and MAC layout.
- [rtl8192d/phy_common.c](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/drivers/net/wireless/realtek/rtlwifi/rtl8192d/phy_common.c): MAC/PHY mode and coexistence buffer boundary.
- [rtl8192d/fw_common.c](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/drivers/net/wireless/realtek/rtlwifi/rtl8192d/fw_common.c): download enable/disable, checksum, RDY and per-MAC firmware readiness.
- [efuse.c](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/drivers/net/wireless/realtek/rtlwifi/efuse.c): USB firmware page writer, **64/8/1-byte chunks**.
- [usb.c](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/drivers/net/wireless/realtek/rtlwifi/usb.c): vendor request `0x05`, `wValue=register`, `wIndex=0`, 1000 ms USB timeout. This port additionally rejects short completions, which the pinned Linux wrapper does not check.

This is not the EU boot implementation with substituted IDs. DU uses
`SYS_CFG & 0x000f0000` for production silicon; logical PROM endpoint-setting
bit0 chooses single-MAC/2T2R or dual-MAC/1T1R. Dual mode initially assigns
**MAC0 to 5 GHz and MAC1 to 2.4 GHz**. The USB MAC0 address is at `0x19`, MAC1
at `0x5b`. Neither is an EU PROM offset.

Firmware signatures are strictly `0x92d0..0x92d3`, with the existing family
header validator; raw/headerless images are deliberately refused. The payload
is at most 32768 bytes, in eight 4096-byte pages at `0x1000`. USB payloads are
not padded (the Linux dummy-byte path is PCI-only). The page selector preserves
the upper five bits of `MCUFWDL+2`; download enable clears bit3 as Linux does.
`MCUFWDL+1` is not cleared: bit7 is **MAC0_ON**. Checksum bit2 is polled, then
RDY bit1 is set. Startup polls bit0 at **`0x18` for MAC0 or `0x1a` for MAC1**;
the EU WINTINI bit6 and EU CPU reset are not part of this DU protocol.

## API and current boundaries

1. Obtain a real, immutable DU USB preflight plan and a logical (unpacked)
   PROM image. `rtw_92du_identify` checks the catalog/backend, endpoint geometry,
   production SYS_CFG, `0x8129` PROM ID, mode and selected unicast MAC. It performs
   no I/O. MAC1 can be decoded only for a dual-MAC board.
2. Claim and serialize **the whole USB device, including both interfaces**.
   `rtw_92du_boot_init` requires explicit experimental-I/O and exclusive-device
   options and a poll limit from 1 to 1000. It performs no I/O; these options
   are caller contracts, not OS ownership enforcement. Do not run two contexts
   concurrently or reuse one across detach/reattach.
3. `rtw_92du_power_on` is cold-only. It rereads SYS_CFG and refuses either MAC
   already on before any write. The source sequence programs the MAC/PHY mode,
   current MAC band/on bits, other-MAC power-off wait, autoload wait, SPS/LDO,
   APFM, isolation, CR and coexistence buffer boundary. Poll timeouts that Linux
   only warns about are hard failures here. TX queues are paused both before
   power and after the MAC enable edge. There is no RF/BB table or bulk TX.
4. `rtw_92du_firmware_start` accepts a DU image only after successful power-on.
   It uses source-exact USB firmware chunking, never retries payload writes,
   and verifies checksum and the selected MAC's readiness. Existing MCU RDY
   firmware is reused only after the selected MAC is ready; this **does not
   authenticate its version against the supplied image**.

Unready running RAM firmware, stale CRC, active download or an already-set
MAC-ready flag without MCU RDY are refused. The Linux DU self-reset path is
not yet ported; the EU reset must not be substituted. On I/O, wait or poll
failure, the context is poisoned and firmware readiness is cleared. Cleanup
attempts to disable download and pause TX while preserving the original error
and failed register. If USB itself is broken, cleanup cannot be guaranteed;
physical reset/re-enumeration is required before another attempt. The module
does not guess a full device power-off sequence or reset another MAC.

`last_register`, `last_error`, `power_ready`, `firmware_ready`, `firmware_reused`
and `firmware_bytes` give bounded diagnostics. `firmware_bytes` counts fully
completed writes, not proof that a later startup succeeded. A short completion
is an error even when a prefix may already have reached hardware.

## Verification

`tests/rtl8192du_boot_tests.cpp` runs with a fake synchronous USB device. It
covers single/dual board decode, both initial bands and MAC addresses, cold
power ordering/state, TX pause after a modeled power/reset edge, 4096-byte
boundary and full eight-page images, exact odd-size USB payload, preservation
of MAC0_ON, DU per-MAC ready with EU bit6 absent, firmware reuse, stale-state
rejection, every bounded poll timeout, I/O and short-transfer injection at
every successful-path transfer, and delay failure. No physical USB writes
were made for this port.

An independent ASan+UBSan build can be run against the existing core library:

```sh
clang -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -fno-sanitize-recover=all -Iinclude -c src/RTL8192DUBoot.c \
  -o /tmp/RTL8192DUBoot.o
clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -fno-sanitize-recover=all -Iinclude tests/rtl8192du_boot_tests.cpp \
  /tmp/RTL8192DUBoot.o build/librtwcore.a -o /tmp/rtl8192du-boot-tests
/tmp/rtl8192du-boot-tests
```

## Required before a working Tahoe DU station

- DU-specific eFuse physical read/unpack, EEPROM/autoload state and board-cut,
  regulatory, antenna, thermal and 2.4/5 GHz TX-power decoding.
- Verified MAC/DMA/LLT, queue/aggregation layout and live USB RX/TX descriptor
  codecs; current catalog preflight must not be treated as a datapath backend.
- Actual DU BB/AGC/RF tables, channel/bandwidth switching, both band power
  groups, IQ/LO/LC calibration and temperature tracking. No TX power defaults
  or PHY registers are guessed by this module.
- Whole-device ownership and both-MAC lifecycle, warm firmware reset,
  power-off, sleep/wake, cancellation and detach-safe kext integration.
- net80211 station management, WPA2 key/CAM integration and 802.11n rate/
  aggregation control; app capability flags only after those paths work.
- Physical DU cold/warm boot, connect, WPA2 rekey, bidirectional traffic,
  throughput and recovery validation on both supported bands.

The five verified DU catalog identities remain diagnostic-only. Completing
this primitive does not make those adapters connect in Tahoe.

The PHY/MAC/kernel port has progressed beyond this initial boot note. See
[RTL8192DU-PHY.md](RTL8192DU-PHY.md) and
[RTL8192DU-KERNEL-INTEGRATION.md](RTL8192DU-KERNEL-INTEGRATION.md) for current
implemented stages, integration hooks and the remaining dual-interface gap.
