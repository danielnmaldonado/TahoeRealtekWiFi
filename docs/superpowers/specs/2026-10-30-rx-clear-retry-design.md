# RX endpoint recovery after a transient control failure

The user requests direct correction of repeated RTL8814AU USB failure, with complete app and driver source for local compilation. Continue in the existing source as requested; no EFI changes or reboot.

Build49/app45 are confirmed live. The AU reports two RX recoveries, one completed and one failed, not budget exhaustion. The second RX transaction error is followed by an endpoint-zero transaction error. Build49 treats the first failed clearStall or refill as permanently fatal; current counters cannot distinguish these stages. A separate source race faults eligible synchronous RX submissions before their completion can enter recovery.

Retain clearStall(true), including device/host toggle synchronization recommended by Apple. Retry only the same idempotent endpoint recovery after transient NotResponding, with at most three attempts, timed spacing and the existing two-second whole recovery deadline. Drain every old/partial read generation before refill, preserve normal payload ordering and cancellation. Keep removal, other errors and all TX/vendor requests terminal as before. Do not use host-only clearing or replay data/register writes.

Add scalar stage/results to distinguish clear, drain and refill outcomes. Test actual controller methods with halted-pipe behavior and scripted transient recovery outcomes. Existing app45 remains compatible and is included in the source delivery. Physical stability and dual-band validation require loading the new kext; software tests alone do not establish them.
