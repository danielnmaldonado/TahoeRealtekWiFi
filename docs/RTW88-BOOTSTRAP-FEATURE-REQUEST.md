# B/C cold-boot hardware feature request

`bootstrap88` now writes `REG_C2HEVT (0x1a0) = 0xfd` after MAC power-on and
before firmware download/CPU boot, matching Linux rtw88
`main.c:rtw_chip_efuse_enable` and `fw.h:C2H_HW_FEATURE_DUMP` at pinned commit
`0e135910deec43ffe97eca0b6af1f8f847285b59`.

The firmware emits `C2H_HW_FEATURE_REPORT (0x19)` in response. Reading 0x19
after firmware boot cannot request the report retroactively. This blocked
the B/C path even when firmware and OTP initialization had succeeded.

`scripts/test-rtw88-bootstrap.py` compiles the actual controller bootstrap,
Boot, firmware downloader, MAC plan, OTP read/decoder and feature parser.
The hardware boundary models USB registers, IDDMA and the opaque MCU's
response: 0x19 is emitted only when 0xfd preceded download and CPU boot.
Neither bootstrap nor firmware loading is replaced by a success stub.

The ASan/UBSan regression covers all three B/C families, physical versus
active RF/NSS feature parsing, stale reports, request order and 8,611 I/O
failure positions. It passes 329,853 checks. Temporary-source modes
`--omit-request` and `--late-request` reproduce EBUSY with a running firmware
and valid OTP, proving that the request/order is necessary. Existing station
backend tests and real MacKernelSDK backend compilation also pass.

No device was opened, programmed, reset or loaded during these tests.
Add `scripts/test-rtw88-bootstrap.py` to the normal host test registration.
