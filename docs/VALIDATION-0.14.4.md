# Validation — 0.14.4/build51 and app 0.3.4/build46

Project and macOS adaptation: MaLd0n. Status: development.

## Evidence in build50

kmutil confirmed build50 loaded, UUID 2A9BFFFC-E0B2-34BC-BA34-EEF2B306D7EE.
RTL8814AU 2357:0106 had authorized WPA2, an active link and zero published USB errors
or recovery attempts. The user reported stability.
This is an observation from that session; it is not certification of sustained traffic.

The PHY measured values between −44 and −42 dBm, with recent samples and thousands of
valid readings. At these levels, three bars are the expected result of the
existing thresholds. Native AppKit tests distinguished the 1/2/3-bar images
and checked correct return to cached images. No rendering defect
or need to change RSSI calibration was found.

The router was described as Wi-Fi 6 with automatic channel/bands and the same SSID.
The actual 5 GHz channel was not reported and could not be obtained from the control
interface without administrative authentication. The agent did not request a password,
change the router or USB, install or restart.

## App changes

Tooltip, accessibility and signal row show the measured dBm reading and the
filtered value used for bars. Retained readings are marked stale;
expiry, disconnection and unavailability do not produce fictitious 0 dBm. Icon
width, thresholds and filtering were not changed.

When connected, Refresh networks previously only requested another cache
read. The explicit action now requests an actual search. The driver may need
to reassociate during this foreground scan; the profile remains in the same control
path. Periodic polls remain read-only without disruptive automatic
searches. Tests cover duplication, adapter, result and preferences.

## Radio and diagnostics

The review compared RTL8814AU against Linux pinned in the provenance manifest.
The HT20 path omitted the channel 153 NBI/CSI adjustment; it was ported and checked
against the final registers from primary code, including return to 149/157.
This omission does not demonstrate that the user's AP uses 153 or explain, by itself,
the absence of all 5 GHz networks.

Foreground scan already visits the nine implemented 5 GHz channels before
choosing a BSS. New telemetry distinguishes transitions/visits, beacon/probe
response frames by software channel and known PHY channel, as well as
mismatches between them and cache counts by band. Missing PHY channel
does not become an assumed physical measurement. No change is made to frame
delivery or USB controls to produce these diagnostics.

Only scalar counters are published, without SSID, BSSID, MAC address,
password, PMK, traffic data or packet content. Publication occurs at
state/query boundaries; there are no IORegistry mutations on each RX frame.

Authorized channels remain 1–11 and, in 5 GHz, 36/40/44/48 and 149/153/157/161/165,
according to backend/ROM. DFS remains outside the implementation. The router's Wi-Fi 6
name does not identify its channel or security options. No channels were unlocked
without implementation and no changes were made to WPA2/CCMP.

## Reading scan counters

RTWDiagScanState uses 0=Idle, 1=Active, 2=Complete, 3=Cancelled and 4=Failed.
Complete identifies scan exit to AUTH; it does not prove WPA2 authorization
or network access. RTWDiagScanVisited0–3 are 64-bit words: for channel C,
check word C/64 and bit C%64. A visit indicates programming accepted by the
backend, not proven RF reception. LastError is the transition error.

RTWDiagScanSoftware5Frames counts frames labeled by software channel;
RTWDiagScanPHY5Frames uses only decoded PHY channel. UnknownPHYFrames and
PHYChannelMismatches make the absence or mismatch of this information explicit.
RTWDiagScanCacheBSS5 counts cache records, not frames or distinct networks
by SSID. CurrentChannel is the current state channel. Do not confuse these values
with the band the router uses outside the observed channels.

## Verification and delivery

The suite passed 150/150 in Release and 150/150 with ASan/UBSan on the host.
The 15 native app suites also passed. The PHY executed 35.657 checks,
including the 22 reference records and 472 failure positions for channel 153;
portable telemetry passed 34 and controller/WPA 1.186 checks.
Detailed results and the native build of the delivered ZIP are recorded
in VERIFICATION.json. Tests of models, images or
registers do not replace observing networks and traffic on hardware.
The source contains app46 and kext51; the build script does not install or restart.

The previous hardware inventory and limits are preserved: 270 IDs,
263 primary identities, 262 IDs eligible for 13 station backends plus
separate restricted discovery 13b1:0043. RTL8814BU and unproven identities
remain in their documented states.
