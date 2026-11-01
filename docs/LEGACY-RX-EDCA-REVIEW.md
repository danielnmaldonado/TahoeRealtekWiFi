# RTL8192EU receive filter and FU/8710BU EDCA review

Source audit and offline/native verification, 2026-10-30. No USB operation,
kext load, installation, or physical throughput measurement was performed.
Pinned commits, source blob identities and line ranges are recorded in
[LEGACY-RX-EDCA-PRIMARY.json](LEGACY-RX-EDCA-PRIMARY.json).

## RTL8192EU: retain the subtype filter during HT RUN

The actual inherited `rtwn_newstate()` wrote control subtype 8/BAR in
`RXFLTMAP1=0x0100`, but then raised `RCR_ACF` for HT. That latter operation
enabled general control reception rather than preserving the subtype filter.
It now leaves ACF low for both HT and legacy RUN, including when a previous
register value had ACF set. HT retains the BAR map, and INIT closes it.

This was verified for the exact RTL8192E core, rather than inferred from a
different chipset. The pinned Linux `rtl8192eu_fops` uses the rtl8xxxu register
map whose [RX filter documentation](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/drivers/net/wireless/realtek/rtl8xxxu/regs.h#L852-L864)
requires the corresponding general frame-accept bit to stay low for subtype
filtering. The [RTL8192EU vendor definitions](https://github.com/Mange/rtl8192eu-linux-driver/blob/c629cc972a0d878c930d19c4a9309c27c1e61f92/include/hal_com_reg.h#L1135-L1136)
also distinguish ACF from the ADF path used for BAR/BA, so enabling ACF is not
required for BAR. Its [control map definition](https://github.com/Mange/rtl8192eu-linux-driver/blob/c629cc972a0d878c930d19c4a9309c27c1e61f92/include/hal_com_reg.h#L1636-L1637)
explicitly assigns PS-POLL to bit 10; BAR uses bit 8.

The actual-source HT state regression failed on the old ACF behavior before
the fix. It exercises legacy RUN, HT RUN with a stale ACF bit, and HT to INIT.
Software CCMP and the controlled-port checks were not changed; the existing
actual controller WPA recovery and TX boundary suites also pass.

## RTL8192FU and RTL8710BU: complete legacy EDCA words

The two typed legacy setters borrowed `TXOP<=0x7ff` and an `0x07ffffff`
read/modify/write from the newer rtw88 register definition. Their own primary
drivers instead accept a `u16` WMM TXOP and replace the complete EDCA DWORD.
The previous setter both rejected valid upper TXOP bits and retained stale
upper bits when a new TXOP was smaller.

The evidence is independent for each chip:

* RTL8192F vendor [WMM input and assembly](https://github.com/kelebek333/rtl8192fu-dkms/blob/76c37ba3b7a4bd83de93c5dc0769611f45aa25d0/core/rtw_wlan_util.c#L1531-L1596)
  uses `u16 TXOP` at bit 16. Its [chip-specific setter](https://github.com/kelebek333/rtl8192fu-dkms/blob/76c37ba3b7a4bd83de93c5dc0769611f45aa25d0/hal/rtl8192f/rtl8192f_hal_init.c#L4700-L4715)
  writes every complete AC DWORD.
* RTL8710B vendor [WMM input and assembly](https://github.com/lwfinger/rtl8188gu/blob/9dec8292ae13c3a0dbf53eb07da6585427c8db7a/core/rtw_wlan_util.c#L1557-L1622)
  and [chip-specific setter](https://github.com/lwfinger/rtl8188gu/blob/9dec8292ae13c3a0dbf53eb07da6585427c8db7a/hal/rtl8710b/rtl8710b_hal_init.c#L3630-L3645)
  implement the same full-word contract.
* Pinned Linux [rtl8xxxu_conf_tx](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/drivers/net/wireless/realtek/rtl8xxxu/core.c#L6847-L6885)
  independently writes the full DWORD without narrowing the TXOP field for
  these chips.

Both setters now match that contract. AIFS/ECW validation is retained, along
with live firmware health checks and first-error propagation. Failure after
a device may have accepted a word does not replay it or continue to a later
AC. This change establishes the primary software programming contract; it
does not infer reserved-bit widths for another chipset. AU and newer B/C
setters were not modified.

## Verification

`python3 scripts/test-legacy-usb-edca.py` compiles the actual two C cores with
ASan/UBSan and models only USB/RF hardware. Each family passes 917,598 checks:
all 65,536 TXOP values on all 4 AC registers with both slot settings, a maximum
TXOP to zero transition, invalid inputs before register writes, and all 6 I/O
positions failing before/after effects. Separate frozen old-cap and old-RMW
source variants both fail this final regression for each family.

Additional passing checks:

* Actual inherited EU HT, 1,558 TX/BA checks and 790 controller WPA recovery
  checks.
* Complete FU core and wire suite, 20,626 before/after-effect I/O faults and
  149,933 wire checks.
* Actual FU and8710BU adapters, controlled port/EAPOL/software-crypto/BA/TX,
  with respectively 1,839/568 AUTH and 41 RUN failure positions per family.
* Both full controller adapters, both changed C modules and the complete
  inherited `rtwn.cpp` compile against the real MacKernelSDK. The changed
  modules and inherited core also pass the 2,048-byte stack-frame limit.

These checks validate source behavior and native compilation. Physical
association, radio behavior and throughput still require corresponding USB
hardware and a loaded build containing the changes.
