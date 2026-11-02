# Validation — 0.14.3/build50 and app 0.3.3/build45

Project and macOS adaptation: MaLd0n. Status: development.

## Failure observed in build49

Kext49 was confirmed by kmutil, UUID 88C75B61-85DE-39D2-AEA1-972C0546A64A;
the installed app was 0.3.3/build45. On RTL8814AU 2357:0106, IORegistry recorded
two RX attempts, one completed, one failed and zero exhaustions. This is neither
an old loaded copy nor an attempt limit already reached.
RTL8192EU 2357:0108 still had authorized WPA2.

At 23:01:31.118 and 23:01:31.994, the USB log recorded transaction error 0xe00002ed,
zero bytes, on AU RX endpoint 0x81. At 23:01:31.995, there was also an error on
control endpoint 0x00. The old counters did not distinguish clearStall failure
from refill failure. The nearby control error is consistent
with clearStall failure, but the log does not identify the request.

The AU connection was at 480 Mbps through a USB 2.0 hub. The same log shows USB
errors on the other adapter and keyboard. This does not determine physical or
firmware origin; testing a direct port separates some of these variables.

## Recovery fix

Build49 made the first clear or rearm failure fatal, even with
budget remaining. Also, synchronous rejection of an RX submission
could latch usbError before its completion entered recovery.

Build50 preserves clearStall(true), cancels/drains previous generations and
allows new spaced attempts only for transient NotResponding during
this recovery. The limit is three endpoint recovery attempts,
a minimum 100 ms interval and a total two-second deadline. The deadline is
checked before and after synchronous Apple calls; a blocked call
cannot be interrupted by this driver's watchdog. Reads from a
partially rearmed attempt must be canceled and drained before the
next one. The consecutive RX error budget, reset only after useful RX,
and stop/sleep/unplug controls still apply.

A direct search requested by the app returns Busy during recovery, before
changing channel or state. Scan, calibration and B/C coexistence timers
defer their work until recovery ends. JOIN and LEAVE can still
cancel the session deliberately.

Register requests and transmissions are not replayed by this
policy. Different errors, partial transfer or persistent failure follow
the fatal path. Scalar diagnostics distinguish clear, drain and refill. In IORegistry,
RTWDiagRXRecoveryStage uses 0=Idle, 1=Clear, 2=RetryWait, 3=Drain, 4=Refill,
5=Complete, 6=Failed and 7=Cancelled. FailureStage preserves the terminal stage;
ClearResult and RefillResult identify individual results. ClearAttempts,
ClearFailures, RefillFailures, TransportRetries and TransportExhausted show the
budget actually used, without SSID or key material.

## Primary references

The Apple contract recommends synchronizing the toggle with withRequest=true and
explains that non-timeout bulk errors leave the pipe halted:
[Apple ClearStall](https://developer.apple.com/documentation/usbdriverkit/iousbhostpipe/clearstall).
The adaptation was also compared against IOUSBHostPipe.h in the actual SDK 26.5.

Vendor Linux resubmits RX after EPROTO and counts consecutive errors, but the
Linux host controller performs its own recovery. This is not equivalent to replacing
clearStall(true) with false on macOS:
[RTL8814AU RX](https://github.com/aircrack-ng/rtl8814au/blob/892641430c24308e7ae193d746b25b600ef227d8/os_dep/linux/usb_ops_linux.c#L806),
[Linux xHCI](https://github.com/torvalds/linux/blob/2a500a04d6f969318c05f46aedb6e73fc1963a00/drivers/usb/host/xhci-ring.c#L2558).

## Verification and limits

Tests execute actual controller methods with modeled USB boundaries,
including a halted pipe, synchronous rejection, transient clear error, partial
refill, deadline, budget, ordering, cancellation and fatal errors. Software verification passed 149/149 Release tests and 149/149 with
ASan/UBSan on the host, plus the 15 native app suites. Recovery executed
2.105 checks; control/WPA 1.050; ordering 258; coexistence checked
pause/resume across the six family/antenna profiles. Detailed results and the
native build from the delivered ZIP are in the delivery's
VERIFICATION.json. These tests do not prove actual RF/USB stability.

Build49's passive search and observed-band fixes are preserved.
5 GHz channels remain limited to 36/40/44/48 and 149/153/157/161/165; DFS is not
implemented. Inventory 270, 263 primary identities, 262 IDs eligible for the 13
station backends, plus separate restricted discovery 13b1:0043. RTL8814BU and
unproven identities remain within the documented limits.

The user must load kext50 to check the drop on Tahoe.
The source includes app45 and kext50; compilation does not install, change EFI or restart.
