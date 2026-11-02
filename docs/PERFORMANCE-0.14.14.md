# Performance and diagnostics — build61/app52

This round continues the audit of the 14 build55 captures. Historical results
remain in ADAPTER-TESTS-20261101.md and the associated CSVs. The new implementation
has not yet been loaded or physically validated by the agent.

| Families | Source change | Retained limits |
|---|---|---|
| RTL8192CU | HT20/MCS and SGI20, firmware RA/H2C mask, QoS, RX and BA agreements | Up to 2 streams depending on the board; no HT40/5 GHz |
| RTL8188EU | HT20/MCS and SGI20, AMRR adaptation through CCX reports, descriptors, QoS, RX and BA agreements | 1 stream; no HT40/5 GHz; I-cut unsupported |
| RTL8821CU | HT40 and VHT20/40/80 on non-DFS 5 GHz; consistent width/calibration/RA/RX/coexistence | 1 stream; 2.4 GHz and DFS in HT20; channel 165 at 20 MHz |
| RTL8822BU/CU | Same B/C modes, with masks limited to the chains and peer | Up to 2 streams depending on eFuse/coexistence; same band/width limits |
| RTL8812AU/RTL8821AU | Fix to OTP calibration offset and bits for VHT80 power | Missing/invalid ROM remains rejected; no invented values |

Wide B/C channels use non-DFS blocks 36–48 and 149–161. Every 20 MHz component
must be allowed by the profile and regulatory policy. VHT20 does not advertise
MCS9 for 1/2 streams. Transmit masks follow the AP's receive capabilities;
descriptors and C2H reports use the negotiated width.
A calibration or RA/media failure terminates link initialization.

On RTL8188EU, firmware is not marked as the rate controller. Adaptation uses
a list limited to negotiated rates, with real CCX feedback; USB responses
are not treated as RF success. The previous AMRR wrapper retains its API.

The AU fix uses the types and power function from the Linux revision already
pinned in the project. The 80 MHz fields occupy the high nibble of bytes 38/39
of the 42-byte power block, after the two-byte OFDM field. The golden generator
compiles the primary reference and derives the layout, without copying the
faulty formula. Vectors cover positive/negative values, differences between
80/160 MHz, 1/2 streams and RF paths. This does not establish the physical cause
of the 2357:0120 failure or infer a resolution for the missing 2.4 GHz band in
that capture.

## Capabilities and negotiation on the selected channel

Association and scan requests use the local capabilities permitted on the
target channel. The common path does not advertise HT40/SGI40 on channels
registered for 20 MHz only, or VHT on 2.4 GHz or channels without registered VHT.
Negotiation also respects the secondary-channel side and registered width
limit, including DFS20 and channel165. The global channel/capability catalog
remains intact for the next scan or band change. Tests execute the real request
generators and negotiation functions, without radio transmission.

## Native counters

The delivery boundary to IONetworkInterface records two native readings on
the first nonempty delivery and then every 1024 batches. These observations
include RX/TX packets/bytes/errors, index and capabilities, with separate validity.
They do not export pointers, addresses or frame contents; they do not alter
counters, capabilities, processing or packet ownership. The collector treats
these samples as values for one batch, without calculating deltas between
different samples.

The zero native RX count still has no confirmed cause. Captures show positive
Ethernet delivery; analysis of the installed binary confirms the input path,
but does not establish a supported option for correcting the count in macOS.
The instrumentation serves to distinguish delivery, identity and deferred counting.

## Validation and physical performance

The delivery's VERIFICATION.json records executed tests, native builds, exact
revisions and hashes. Software tests verify logic, descriptors, negotiation,
calibration and modeled failures; they do not measure download/upload. Review
by another person/agent is also limited to the code and models used.

Connection, sustained traffic, recovery and real rates still need measurement
on the corresponding adapters and revisions. This does not prevent this round's
source fixes; it prevents claiming that the pack has already been certified
on every board. RTL8814BU still has no enabled station mode; RTL8814AU remains HT20.
