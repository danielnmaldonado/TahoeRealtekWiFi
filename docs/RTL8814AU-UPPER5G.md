# RTL8814AU upper 5 GHz operating channels

The PHY operating path implements HT20 on 149, 153, 157, 161 and 165 in
addition to the existing 36, 40, 44 and 48. It uses actual per-path factory
power, rate offsets, regulatory limits, channel programming and calibration.
This is source and register-bus validation; physical connection and throughput
on each band still require the corresponding adapter and access point.

The primary source is Linux rtw88 commit
`0e135910deec43ffe97eca0b6af1f8f847285b59`, selected under BSD-3-Clause.
`src/RTL8814AUPHY-Upstream.json` records SHA256 for the chip, table, register,
common PHY and packed-ROM definitions. See `src/RTL8814AUPHY-LICENSE.txt`.

* `phy.c:rtw_get_channel_group` maps 149/153 to group 10, 157/161 to 11,
  and 165 to 12. `rtw_phy_get_5g_tx_power_index` supplies the HT20 signed
  OFDM/1SS/2SS delta interpretation. Each of four physical paths uses its
  own factory base at packed offset `18 + group`.
* The RFE0/RFE1 PG and per-domain power tables supply independent OFDM,
  HT1SS and HT2SS limits. A missing domain entry uses the world minimum;
  unset limit sentinels do not become transmit permission. Existing signed
  power clamping and the hardware TXAGC `+2` encoding are preserved.
* The actual imported `rtw8814a_switch_channel` programs CLKTRK `0x412`,
  RF18 high-band `0x501` and AGC table 3 on upper channels. Lower 5 GHz and
  2.4 GHz retain their respective settings. The upper band selects thermal
  table row `RTW_PWR_TRK_5G_3` for both RFEs and all four paths.
* Channel changes pause TX and clear readiness. Actual LOK/TXIQK/RXIQK
  completion is required before calibration releases the original TXPAUSE.
  Bus errors and calibration timeouts keep the transmitter paused.

## Integration contract

`rtw_8814au_phy_channel_supported(ctx, channel, 20)` is a pure local query
for attach and channel advertisement. It needs a validated board, supported
RFE/cut/path/stream geometry, the existing 2G factory prerequisites, a valid
factory base on all four physical paths for the requested 5G group, and the
corresponding OFDM/HT power limits. It does not require power, firmware, MAC
or PHY initialization and performs no bus I/O. Advertisement must apply this
query per channel; `supports_5g` only means at least one 5 GHz channel passes.
Regulatory/country policy remains a separate caller restriction.

Receive-only tuning accepts the same nine non-DFS 5 GHz channels but leaves
TX paused and calibration invalid. Operating tuning still requires the
per-channel factory query and `calibrate` before AUTH or other transmission.
DFS, HT40 and VHT80 remain outside this PHY station implementation.

## Reproduction and coverage

```
python3 scripts/generate-rtl8814au-phy.py --source /path/to/pinned/linux/rtw88
python3 scripts/generate-rtl8814au-upper5g-goldens.py --source /path/to/pinned/linux/rtw88
python3 scripts/test-rtl8814au-phy.py --out /tmp/rtl8814-phy
```

The second generator verifies source hashes, compiles unchanged primary
channel/group/packed-ROM/thermal-selector functions with register callbacks,
and derives committed golden values from primary PG/power tables. It never
imports the port or accesses hardware. The normal test consumes these
goldens without requiring a Linux checkout.

Validation covers 72 RFE/domain/channel power and RF rows, every positive
and negative thermal delta 0..29 on lower/upper 5 GHz for both RFEs, factory
rejection without I/O, 2G/lower/upper transitions, actual RFK readiness and
timeout recovery, stop, and all 165 bus failure positions in upper tuning.
The initial upper-channel test failed against the previous implementation;
the completed ASan/UBSan suite passes 30,281 checks. The PHY also compiles as
C and C++ against the real MacKernelSDK, with kernel stack limits enforced.
