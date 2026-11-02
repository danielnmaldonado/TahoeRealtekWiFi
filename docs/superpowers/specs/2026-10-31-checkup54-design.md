# Build53 checkup fixes

The user requested fixes for the problems found, directly in the source. The
current connection must remain usable; do not install, reload or restart the driver.

## Evidence

Build53 loaded, RTL8814AU2357:0106/en3, WPA2 authorized on channel6.
Real 21498-byte reads, without USB errors or truncated aggregates. During the
checkup, 90/90 pings responded; short transfers used the USB interface.
TXQueueDrops12132 and TXLinkDrops5 did not increase, but a full queue discards
packets instead of signaling pressure to the output queue. Netstat reports RX0
despite received traffic. Seven new input errors have no exported cause.
HT/BA diagnostics are not published on AU because calibration returns early.

## Approach

Fix TX admission using the IOKit stall/retry and ownership contract, verify
accounting at the Ethernet delivery point and complete diagnostics under the
controller gate. Rejected approaches: increasing queues to hide losses; zeroing
counters; interpreting Ethernet54Mb/s metadata as RF rate; classifying the seven
errors without evidence. The RX cause will be reproduced before changing code.

Preserve the control ABI, AU 32KiB buffers, thermal C2H, RF policies, keys,
WPA2/CCMP, USB recovery and other families. Publish numeric scalars only;
no packet or key data. Do not attribute seven drops to a specific failure
without a counter demonstrating it. The Ethernet medium must declare a speed
only when the backend has a valid PHY rate; do not invent capability from throughput.

## Verification and delivery

Each fix will have a behavioral test failing on53 and passing on the implementation,
covering ownership, full queues, termination, multiple adapters and consistent
publication. Run Release/sanitizer suites, SDK compilation and the native app.
Independent review before freezing. Deliver build54 with complete app and kext
source, a manifest and compilation from the ZIP itself. Physical validation
of54 requires loading by the user.
