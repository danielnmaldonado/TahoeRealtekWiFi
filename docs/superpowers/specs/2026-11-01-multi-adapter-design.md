# Refresh per adapter and USB family recovery

Request: check both real USB adapters, scan networks automatically when needed
on adapter changes and apply recent fixes to every eligible backend.
Execution continues directly in the source, as instructed by the user.

The current app changes the adapter and reads SCAN_RESULTS without necessarily
starting SCAN. We will use scan state per RegistryID: cache with unreliable
provenance or expired cache on a selected, idle adapter requires a real coalesced
scan. Automatic scanning must not interrupt an active association. Hotplug, wake,
rapid switching, late results and failure have explicit invalidation/limits.
An uninitialized radio does not mean a disabled radio. No new credentials or JOIN.

Alternatives considered: a global SCAN timer would interrupt links and mix
adapters; cache alone retains the defect; a policy per adapter and state meets
the request and preserves existing epochs. The implementation choice is the agent's.

PM registration and runtime RX recovery are already common to all 13 backends.
Initial reception validation, draining and the clearStall budget move to the same
set, preserving capacity/type and original errors. UNKNOWN/8814BU do not gain
station mode. Full bootstrap retry requires a verified family-specific stop,
callback drain and immutable identity. AU routines will not be copied to another
chip. Other families receive their own stops and reidentification; the legacy
path requires primary power-off before enabling retry for a persistent error.

Tests of real methods failing before the fix, retry bounds, out-of-order callbacks,
ID/epoch changes and two independent controllers. The delivery will contain
complete source, builds from the ZIP and evidence of physical limitations.
No reboot or changes to EFI, router firmware or system protections.
