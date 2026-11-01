# RTL8723BU station review corrections

The register/rate corrections below use actual core/controller execution with
fake USB and OS boundaries. No physical RTL8723BU test is implied.

* **BAR admission:** HT RUN programs control subtype 8 (`RXFLTMAP1=0x0100`),
  rather than subtype 10 / PS-POLL. It also keeps `RCR_ACCEPT_CTRL_FRAME`
  clear: the pinned rtl8xxxu register documentation explicitly states that a
  raised accept-all bit bypasses the subtype map. INIT/non-HT clears the map.
  The shared controller continues to validate peer, address, BA session and
  controlled-port state before software reorder.
* **Sparse MCS:** any nonempty subset of the supported MCS0..7 is accepted.
  Requiring MCS0 was stricter than the actual imported
  `ieee80211_node_supports_ht` contract. Core tests sweep all 255 masks,
  both SGI choices and both legacy masks; real imported HT negotiation also
  sweeps all 255 masks. Actual station H2C and TX descriptors exercise sparse
  masks including `0xfe` with MCS0 disabled. Empty intersections still fail.
* **QoS ownership:** cached AP EDCA is selected only when both global F_QOS
  and the selected peer's NODE_QOS are present. The actual imported 11B/11G
  default table supplies the fallback. Sixteen combinations of mode, slot,
  local QoS and peer QoS check all four actual programmed AC registers.
* **MCU health:** 8051 readiness requires `(MCUFWDL & 0xc7)==0xc6`, including
  download-enable bit 0 clear. A `0xc7` stale-ready value is rejected. The
  callback invalidates firmware/calibration/rate readiness on health or
  transport failure. All 256 register-byte states and every thermal/RSSI
  tick I/O failure position are exercised.

## EDCA width finding: retain the RTL8723B full-word write

The review's proposed `0x07ffffff` RMW relies on a newer 3081 register layout.
It is not established for RTL8723B. The chipset's vendor HAL explicitly uses
full `rtw_write32` for every `HW_VAR_AC_PARAM_*`; mainline rtl8xxxu's
`conf_tx` likewise packs its `u16` TXOP at bit 16 and writes the full word.
Its register header defines shift 16, without an 11-bit TXOP mask.

Therefore this port retains the primary full-word replacement. The earlier
borrowed `0x7ff` input restriction is removed, preserving the primary `u16`
parameter range. Register goldens test TXOP `0xffff` followed by zero and
verify that old high TXOP bits are actually replaced. No claim that these
bits are reserved on RTL8723B is made.

Primary sources:

* [rtl8xxxu RX subtype-map semantics](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/drivers/net/wireless/realtek/rtl8xxxu/regs.h#L852)
* [rtl8xxxu conf_tx](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/drivers/net/wireless/realtek/rtl8xxxu/core.c#L6847)
* [RTL8723B vendor EDCA setters](https://github.com/lwfinger/rtl8723bu/blob/af3a408d6399655b0db23c2c8720436ca725ca47/hal/rtl8723b_hal_init.c#L5380)
* [RTL8723B vendor EDCA field offsets](https://github.com/lwfinger/rtl8723bu/blob/af3a408d6399655b0db23c2c8720436ca725ca47/include/hal_com_reg.h#L1417)

The vendor file Git blob SHA1s are respectively
`2863f0c656b24029f677e0aa4e16114bcc44ec3b` and
`d7d8dd45c4994c6efe1d5c1019934e842448b30a`. These files corroborate register
policy; no additional vendor algorithm or firmware bytes were imported.

The 8723 host runner now owns its OS boundary shapes. Its stack flags,
cipher enum and default EDCA values are imported from the actual shipped
net80211 sources, independently of other families' mutable test fixtures.
