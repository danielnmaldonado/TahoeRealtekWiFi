# Persistence after boot and resources — build58/app50

Request: retain the selected adapter/network after reboot with two USB adapters; check resources and prepare publishable source. The user authorized direct execution in the source.

## Architecture and choices

Volatile selection by RegistryID must continue to guard session operations. The persistent preference must use validated physical identity and resolve the RegistryID again on each enumeration; never reuse a number from another boot. Restore only an unambiguous match, retain the preference when an adapter is slow to appear and respect manual selection, reconnection pause and Keychain. The preferred network must not depend on saving a password.

Rejected alternatives: persisting RegistryID fails on reboot; selecting the first adapter may connect the wrong hardware; forcing global service order does not fix app selection. The current macOS route must be observed separately.

Resources: measure the real app/helper at idle; treat kernel_task as an aggregate, not exclusive kext consumption. Fix only observed defects: a formatter created per record and a coexistence timer retained after releasing WorkLoop. Preserve already validated polling intervals and radio deadlines.

Publication: complete source, verifiable SDK dependency, isolated tests, no generated artifacts/credentials; retain required licenses and provenance. Physical results are limited to the hardware present and state actually observed. The package must not claim universal certification or resolve the native RX discrepancy without evidence.
