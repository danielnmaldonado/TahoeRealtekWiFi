# RTL8812AU standalone USB bootstrap

`src/RTL8812AUBoot.c` adds actual synchronous register/OTP/firmware/H2C
operations behind the existing USB transport callbacks. It is an intermediate
hardware layer for a future kernel station backend. It is not called by
`RTWUSBController`, does not enable a USB personality and does not claim Wi-Fi
connectivity or 5 GHz radio operation.

The board parser follows the pinned Linux `rtw88xxa_efuse` layout: channel
plan at **0xB8**, crystal at 0xB9, thermal at 0xBA, amplifier/RFE inference,
country code, both RF-path power-index blocks, USB MAC at 0xD7, and physical
OTP capability hints at **1018..1021**. Raw cut encoding is adjusted as Linux
8812A does. The former experimental channel-6 identity helper remains
unchanged. Parsing an RFE number is not proof that this source has its tables.

OTP reads explicitly grant ELDR/ANA8M/loader clocks, read 512 packed OTP bytes
and the four separate USB-capability bytes, decode the logical map, and close
the access gate on success or failure. These register accesses read OTP; they
never program OTP cells.

Cold MAC power uses the exact USB-selected commands from Linux
`card_enable_flow_8812a`, with bounded polls. It pauses queues, enables only
DMA/protocol/security clocks and leaves **MACTXEN/MACRXEN clear**. Resident
firmware causes `EBUSY`; a future hot-reset implementation must account for
both MAC and RF state rather than treating a second cold enable as a reset.

The firmware operation reuses the existing tested RTL8812AU 8051 downloader:
32-byte header validation, 4096-byte pages, at most254-byte USB writes,
checksum clearing/checking, 8051 reset and firmware-ready polling. It sets
only firmware state, not radio state. Caller-supplied firmware must come from
a trusted source; header validation is not cryptographic authentication.

The H2C primitive waits at most3ms for one of four legacy firmware mailboxes,
writes its extended word before its main word, and rotates only after both
writes succeed. It constructs no station or security commands.

Regression coverage in `tests/rtl8812au_boot_tests.cpp` exercises real source
against a USB-register boundary model, with ASan/UBSan: ROM/RFE/NSS, aliased
output rejection, OTP clock/access cleanup, injected USB failures, cold power
and resident-firmware refusal, no RF-register writes, 8500-byte/3-page firmware
transfer, checksum/init timeouts, failure cleanup and mailbox rotation/busy
polling. C11 compilation with conversion/shadow/pedantic warnings also passes.
These are host tests; **no RTL8812AU hardware was available in this session**.

Remaining before a kernel station can use this layer: complete LLT/DMA/MAC
initialization for each USB topology, BB/AGC/RF/power-limit tables, channel/band
switching, IQK/LCK and thermal tracking, net80211 attach/state/RSSI/rate wiring,
TX/RX transport integration, WPA2/DHCP/traffic and board-specific testing.
RTL8821AU requires its separate ROM layout, power flow,1T PHY and coexistence.

Exact source revisions and digests are in
`src/RTL8812AUBoot-Upstream.json`; BSD notice is in
`src/RTL8812AUBoot-LICENSE.txt`. No system installation or security setting was
changed by these additions.
