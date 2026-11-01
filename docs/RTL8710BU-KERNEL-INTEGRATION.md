# RTL8710BU controller integration

Include `rtw/RTL8710BU.h` at controller-header file scope and
`RTW8710BUBackendMembers.inc` in its private section. Include
`RTW8710BUBackend.inc` in the controller translation unit. The backend itself
includes `RTL8710BUFirmware.inc`; do not include the arrays twice.

Compile `src/RTL8710BU.c` and `src/RTL8710BUDataPath.c`. Their generated `.inc`
files are dependencies, not translation units. Register the tests
`rtl8710bu_tests.cpp`, `rtl8710bu_datapath_tests.cpp`,
`scripts/test-rtl8710bu-backend.py` and `scripts/test-rtl8710bu-negotiation.py`.
The first two link rtwcore; the scripts compile their actual C dependency sets.

Route the family through the following concrete hooks, preserving existing
shared lifecycle/transfer/PAE behavior:

- `is8710BUFamily`, `attach8710BU`, `start8710BU`, `stop8710BU`,
  `release8710BUStorage`; free storage only after the shared drain condition.
- `state8710BU`, `set8710BUChannel`, `refresh8710BUModes`,
  `channel8710BUModes` and `peer8710BUOperating`. Leave INIT's generic key/port
  flush reachable after USB/HAL failure. RUN media/RA is not authorization.
- Dispatch TX directly to `send8710BUFrame`, RX decoding to
  `decode8710BUUSBFrame`, PHY signal to `sample8710BURSSI`, and firmware events
  to `handle8710BUC2H`. Its RCR omits APP_FCS: preserve the decoded MPDU length,
  with no extra four-byte strip. One-path RX rejects MCS8–15/VHT.
- Use `uses8710BUFirmwareRate`, `uses8710BUHighThroughput` and
  `accepts8710BUReceiveRate` in shared authorization/rate/BAR gates. Accept
  raw HT rates12–19 only with a live matching profile. Do not use EU/AU/CU
  hardware initializers, descriptors or host AMRR to control this PHY.
- Run `tick8710BU` in the calibration timer, quiescing on real errors.
  Install `update8710BUEDCA` and `ampdu8710BUTXStart` alongside shared receive-BA
  callbacks. Its hardware aggregation requires an authorized live RSN BA;
  EAPOL is transmitted individually while the port is closed.
- Actual rates are in `buRate_`, operating channel/width in `buOperating_`, and
  board vendor/package/cut in `buBoard_`. Radio status is obtained with
  `rtw_r10bu_status_read(bu_,...)`. This family has one TX/RX path, 2.4 GHz only
  and no VHT/HE; do not publish either 5 GHz group flag.

The primary transport is endpoint0 vendor request5 with strict exact lengths;
firmware writes are bounded to four bytes. Vendor selection uses identified
silicon, not the USB marketing name. Only the verified catalog identities
0bda:2005 and0bda:b711 should enter this family; matching IDs alone never prove
radio or station readiness.

Native checks require no USB access or installation:

```
python3 scripts/test-rtl8710bu-backend.py
python3 scripts/test-rtl8710bu-negotiation.py
python3 scripts/check-rtl8710bu-backend-kdk.py --out /tmp/bu-controller.o
```

The last command compiles the complete controller against actual macOS
kernel/IOUSB headers with a 2048-byte stack-frame limit, using only temporary
include overlays if shared routing has not yet been integrated.
