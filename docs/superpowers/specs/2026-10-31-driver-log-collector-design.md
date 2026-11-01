# Driver log collector

Extend the existing standalone WiFi-Diagnostics.command so a double click can
capture the two Realtek adapters even when the RTL8814AU radio/USB has failed.
The user explicitly requested the command and previously authorized direct
source work without further design questions. No new UI or external dependency.

Use native Bash 3.2, macOS tools and system Perl (JSON::PP/POSIX core modules).
Default output: $HOME/Library/Logs/Tahoe-WiFi-USB/Diagnostics. Keep --output-dir.
Do not write to Desktop, modify interfaces, run speed tests, scan RF, install or
load drivers, change settings, read Keychain/passwords, or request admin by default.
Existing explicit --admin/--raw options keep their existing contracts.

Capture RTWUSBController before and after collection, retaining text and
allowlisted scalar JSON. Map each registry ID to family, VID:PID and BSD interface.
Summarize per-device monotonic counter deltas; distinguish reset, new and removed
controllers. Do not combine siblings or interpret AU's absent calibration
instrumentation as measured zero delay. Record current loaded driver and app.

Bound external command execution to 30 seconds, configurable1..120. Kill only
that command's child process group, preserve exit124 on timeout, continue with
available evidence and ZIP. Basic redaction must include data-form MAC addresses
and lower-case kernel ssid fields; it is not a guarantee for arbitrary logs.

Approaches: a full sysdiagnose would be slower and much broader; a new Python
collector would add a dependency/duplicate existing behavior. Extending the
existing command preserves its packaging/privacy contract and is the chosen
implementation judgment. Software review/testing does not certify USB hardware.
