# Validation — 0.14.10/build57 and app 0.3.7/build49

Project and macOS adaptation: MaLd0n. Development.

## Actual system checked

On 01/11/2026, the user had already loaded their build56. Loaded UUID:
3B3FC579-9C48-3D8F-B104-89C9D314100D. Initially installed app: 0.3.6/build48.
Current IDs AU 4294968519/en3 and EU 4294968559/en1 replace those from the previous boot.

Two actual LIST/STATUS/MAP queries per adapter passed, approximately 5,11 s
between samples. Both were present and enabled, with radio/RUN and WPA2/CCMP
PTKDONE/authorized port/IP. AU: channel 36/−46 dBm; EU: channel 6/−44 dBm. Driver PM
was in state 1. High-speed USB 480 Mb/s; AU direct, EU on USB2 hub.
IPv4/IPv6 default routes used AU/en3; Ethernet en0 was inactive.
USB/TX/RX errors and recovery failures did not increase during sampling.

Two bounded HTTPS GETs, bound separately to en1 and en3 without a proxy,
returned HTTP 200 and 577 bytes each, about 60 ms. This demonstrates data delivery
on both, in addition to association/IP. It is not a speed, persistence or fresh-attempt WPA2
test; the agent did not change password, AP or association.

## Native RX accounting

The driver offered and flushed Ethernet queues, but ifnet_stat/netstat continued
to show zero cumulative RX on both interfaces. Both HTTPS tests worked.
Review of Apple IONetworkingFamily/XNU did not prove a driver defect;
there are distinct accounting paths, including Skywalk. A later ifconfig -m
read confirmed options/capabilities 0x400 (CHANNEL_IO) on both interfaces,
matching IFCAP_SKYWALK in the SDK. This alone does not prove where accounting
was omitted. Build57 publishes supported/active capabilities and SkywalkEnabled
for investigation. There is no artificial manual increment, feature disabling
or speculative replacement of the IOKit handoff. The discrepancy remains open.

## App and common recovery

App49 replaces cache reading with an actual search when the selected USB is
idle with no fresh RF completion. Validity 30 s, timeout 30 s, retries at 4/8 s and a 300 s pause
after three failures. State is per RegistryID, generation and work epoch. An authorized link,
JOIN, credentials, USB error and unavailable service/session block autoSCAN.
Missing RADIO on an interface not yet started does not block the first search.
UI retains the menuNeedsUpdate contract; the model is refreshed on completion.

PM and operational RX recovery were already common. Initial RX validation now
reaches all 13 backends; four valid reads are required before success,
with a clearStall budget, generation-based drain and preservation of late error.
Common stop also tracks the generation of pending RX/TX: a late USB error
does not disappear upon cancellation and short TX does not become success. Metadata is checked
before freeing the slot, including when the callback arrives after preflight.
Bootstrap recovery uses verified shutdown and per-family reidentification;
the implementation and exact reviews/tests accompany the delivery.

## Delivery limits

Build57 still requires loading and validation on hardware. It does not equate
to certification of all 262 identities/revisions. RTL8814AU remains HT20 and up to
two streams, without VHT/AC. Channel 144 and RTL8814BU station operation remain closed.
Software counts, native products, payload, hashes and closed reviews
are in VERIFICATION.json and REVIEWS alongside the pack. Complete source supplied.
No reboot, EFI, router, Keychain or macOS protection was changed.
