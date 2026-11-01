# Correctness review across supported families

The user explicitly expanded the ongoing work to the whole project and other
supported adapters. Preserve the verified build51/app46 snapshot and work
directly in the authoritative source. No installed app, EFI, router, USB port
or credentials are changed by this source work.

Review all13 operational station families against their pinned primary sources
and run meaningful host/native regressions. Correct confirmed defects rather
than enabling unsupported modes by changing flags. Distinguish source/test
evidence from physical board/revision qualification.

Confirmed work for the next driver build:

- RTL8192EU gen2 TX fields: bandwidth/subchannel/short-GI belong in DW5. Preserve
  DW4 RTS/retry fields, distinguish HT40 packets from20MHz packets on an HT40
  radio, and keep gen1 CU/8188EU codec gates intact.
- RTL8812AU/RTL8821AU PHY status starts after descriptor SHIFT, per rtw88. Verify
  measured signal and backend RSSI consumers, not only the pointer itself.
- Generic FamilyRX24 uses the rtw88 shifted PHY layout for B/C and the rtl8xxxu
  unshifted PHY layout for legacy FU/8723BU. Do not apply one family layout to all.
- RTL8192FU must preserve host-managed80211 sequence numbers for management and
  non-QoS/EAPOL as well as QoS frames, according to the explicit caller contract.
- AU/B-C asynchronous coexistence must not lose the desired BT LNA request when
  an MP query is busy. Use bounded per-context deferred work, preserve protocol
  and acknowledgement semantics, and discard stale work on lifecycle changes.

Keep app46 signal/real-refresh changes, AU153 filtering and scan telemetry.
Keep current channel, board, firmware and identity gates. RTL8814BU remains
blocked unless its actual licensed NIC firmware/protocol and factory metadata
can be qualified; do not substitute AU/router firmware. No unavailable hardware
is declared validated from these tests.
