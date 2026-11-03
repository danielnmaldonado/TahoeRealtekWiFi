# Connection-drop follow-up — kext 65 / app 55

## Observed on installed build 64

The installed driver was build64 and the app was build54. A read-only monitor
captured 60 samples over 121.8 seconds on a RTL8814AU. It observed three
authorized HT connections on 5 GHz/channel36 followed about 18.2 seconds later
by INIT, an invalid controlled port and a stopped radio. The driver recorded
`AUThermal` (stage19), `ETIMEDOUT` (60), then successful hardware recovery.

RX aggregate, CRC, transport recovery, TX USB, EAPOL MIC/replay and WPA recovery
timeout counters remained zero. The monitor requested STATUS and channel maps;
it did not request SCAN, JOIN or LEAVE. The installed app was reopened to observe
its existing saved-profile reconnection. Private addresses, credentials and raw
captures are excluded from the source package.

Stage19 covered both the PHY thermal/calibration operation and the following
RSSI mailbox command. The old capture does not distinguish these operations.
The existing deauthentication value1 is also initialized during authentication;
it does not establish that the AP sent a deauthentication frame.

## Foreground scan admission

An app can queue SCAN from an idle STATUS while the kernel completes connection
recovery before the command runs. The old dispatcher accepted that foreground
scan even in AUTH, ASSOC or RUN. The imported net80211 scan path cleans the BSS,
including its connection keys. This is a separate, reproducible defect in the
shared control path of all 13 enabled station families.

The driver now checks the live serialized state before backend recovery or
scanning. A healthy running AUTH/ASSOC/RUN returns Busy and preserves its keys,
controlled port and profile. Idle discovery, fault/stopped-radio recovery and
explicit JOIN/LEAVE retain their existing behavior. The app explains a deferred
refresh and keeps its current connection and network preference.

## Advisory RSSI commands

The pinned Linux rtw88 reference treats a busy RSSI mailbox as an unsuccessful
advisory update. The port previously converted mailbox readiness exhaustion into
a sticky fatal I/O error, which could stop the radio from a periodic callback.
The correction separates readiness Busy from failures of actual register reads
and writes. Busy leaves the mailbox contents, box index and hardware authority
unchanged; the existing periodic schedule can try again. Required association,
rate and media commands retain their original error handling.

The [13-family coverage matrix](RSSI-BUSY-COVERAGE-0.14.18.csv) records the
corrected advisory paths and the CU/EU/88EU paths that already deferred clean
mailbox contention. Software tests do not certify every physical board revision.

Actual bus errors, including a bus callback returning EBUSY or ETIMEDOUT, must
retain their sticky fault and reach teardown. A caller may defer EBUSY only when
the underlying hardware context has no latched error. No firmware bytes, command
format, RF power constraints, WPA2 rules or channel permissions are changed.

## Maintenance diagnostics

Build65 separates RTL8814AU maintenance phases:

| Field | Meaning |
| --- | --- |
| `RTWDiagAUPeriodicPhase` | 0=not run; 1=PHY/thermal; 2=RSSI H2C |
| `RTWDiagAUPeriodicResult` | Last operation result; a clean-context EBUSY is deferred |
| `RTWDiagAUPeriodicLastFailurePhase/Result` | Last fatal phase/result, retained after later success |
| `RTWDiagAUPeriodicCalls/PHYFailures` | Cumulative maintenance calls and PHY failures |
| `RTWDiagAUPeriodicRSSIAttempts/RSSIBusy/RSSIFailures` | Cumulative RSSI attempts, deferrals and hard failures |

These are scalar counters/gauges published through the existing diagnostic
snapshot. They add no periodic app task or hardware reads. They do not measure
calibration duration. Collector6 includes driver build, AU failure context,
recovery deltas and the separate periodic counters in `RTW-SUMMARY.txt`.

PHY IQK failures remain fatal. A reliable rollback of all path-banked calibration
coefficients has not been established; clearing that failure would incorrectly
retain unverified hardware state. A new build65 capture is required to determine
whether the observed stage19 failures were PHY failures or advisory RSSI busy.

## Validation boundary

The old failure was reproduced on hardware. New source behavior is verified
with host/native regression tests and the normal build tools. The replacement
kext and app have not been installed or loaded by this follow-up. Stability and
traffic with build65 still require the user's compiled kext at the next boot.
No GitHub, EFI, security policy or reboot changes were made.

Primary references: Linux rtw88
[`fw.c`](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/drivers/net/wireless/realtek/rtw88/fw.c)
and
[`rtw8814a.c`](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/drivers/net/wireless/realtek/rtw88/rtw8814a.c).
