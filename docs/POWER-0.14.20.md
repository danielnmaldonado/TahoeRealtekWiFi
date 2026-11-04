# Screen idle and power recovery — driver 67, app 56

## Observed and established

The user reports that the adapter remains listed when the connection fails
after screen idle. On November 4, the currently loaded build 66 was connected
using RTL8814AU on channel 52. Before the preceding reboot, the app continued
STATUS polling, with a maximum observed gap of 2.37 seconds, while nine JOIN
and seven SCAN requests returned `kIOReturnTimeout` (`0xe00002d6`). No LEAVE
request was recorded in that retained window. This does not establish the
initial fault or prove that a screen event caused a USB power transition.

A separate, reproducible recovery defect exists in the shared controller.
Apple's IOEthernetInterface calls the controller's disable operation before
an unusable power state, then enables it again after power returns when the
BSD interface remains administratively up. The old controller treated that
disable exactly like an explicit disconnect: it erased the selected SSID,
configured PMK and connection intent. Its Sleep arm erased them again. Wake
only cleared the sleeping flag, leaving reconnection dependent on the app and
another Keychain read.

## Controller policy

OS interface disable and power-down still quiesce the radio, cancel transfers
and stop timers. They preserve the configured connection profile for the
normal interface enable path after power returns. Operational pairwise/group
keys and authorization are still cleared by teardown; restoration must perform
a fresh WPA2 association and key exchange.

Explicit app LEAVE remains a manual radio-off request. It erases the profile
and prevents a later OS interface-enable notification from restarting the
radio. A valid explicit JOIN or SCAN can resume it. A new status flag exposes that
per-adapter intent without changing the control ABI layout. Automatic app
refresh and reconnect respect that flag; other adapters remain discoverable.
Shutdown and USB removal
retain their existing cleanup and profile erasure. The policy is shared by
all 13 enabled station families; it does not relax USB error handling.

A LEAVE received after Sleep has already stopped the hardware performs only
host-side profile erasure. Repeating the AU hardware stop while power was
unavailable previously latched an unrecoverable cleanup failure. The regression
test exercises the real AU stop/control path and retains pending-drain evidence;
awake LEAVE still stops the hardware normally.

A second AU regression reproduces a busy firmware mailbox during stop. The
old stop sent a media-disconnect H2C before powering off, latched a timeout
and prevented a later enable even when the mailbox became available. The
stop path now clears host media/rate state before INIT, omitting only that
pre-power-off notification. INIT register writes, session-key cleanup, transfer
drain, PHY stop and checked power-off remain. Normal association/media commands
and real USB/register failures retain their error behavior.

The same busy-mailbox condition was reproduced in the full-stop coexistence
paths of RTL8821AU, RTL8821CU, RTL8822BU and RTL8822CU with Bluetooth active.
Their permanent teardown now retains scoreboard clearing, the WOFF antenna
handoff and power-off register operations, and omits runtime IPS dynamic
RF/TDMA commands to firmware that will be reset. Live notifications, LPS,
watchdog policy and mailbox APIs are unchanged. Tests cover both antenna
configurations, every busy mailbox, restart, and direct/indirect register
failures before and after their effect; real errors remain sticky.

The existing awake power request remains in force. Parent power-down is still
honored. No system power settings, sleep veto, display-sleep assertion or USB
idle override is introduced. The PM wake callback does not initiate new USB
traffic; existing network-interface enable performs normal initialization.

## App and collection

Screen-power notifications are handled separately from system sleep and user
switching. Screen idle preserves connection work and manual pause. Screen wake
requests fresh read-only adapter/status/cache data through the existing poll
path. Actual system sleep and inactive-user protections remain applicable.

App lifecycle metadata uses the existing bounded private control log. It records
screen/system/session transitions without SSIDs, passwords or packet payloads.
Collector 7 includes current power policy, sleep assertions and available macOS
power history, with the normal command deadline. The summary includes sleeping,
running, enabled, radio-ready, present, core-error and manual-radio-off values.
These are last-published state values, not a count or proof of power events.
Inspect the captured event timestamps and publication freshness.

## Validation boundary

Verification completed with 180 registered host targets passing, plus native
AppKit lifecycle/manual-radio tests under AddressSanitizer and UBSan. Native
kext, app and installer builds succeeded; app, helper and kext ad-hoc signatures
were verified. The installer container is unsigned.

Actual controller and AppKit code is exercised in host/native regression tests.
Hardware/backend calls are test boundaries; these tests do not simulate an
electrical USB power transition. The original intermittent screen-idle failure
has not been reproduced on the replacement build. Loading the replacement and
observing screen idle, real sleep/wake and manual disconnect remain necessary
to validate that physical sequence on each board.

Primary references:

- [Apple IOEthernetInterface power handling](https://github.com/apple-oss-distributions/IONetworkingFamily/blob/main/IOEthernetInterface.cpp)
- [Apple IOService power requests](https://github.com/apple-oss-distributions/xnu/blob/main/iokit/IOKit/IOService.h)
- [Apple screen-wake notification](https://developer.apple.com/documentation/appkit/nsworkspace/screensdidwakenotification)
