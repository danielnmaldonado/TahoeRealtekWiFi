# All-family correctness implementation plan

**Goal:** Correct primary-source-proven TX/RX/coexistence defects across the
supported families and deliver complete, independently verified source.

**Spec:** ../specs/2026-10-31-all-families-design.md

- [x] Review legacy six-family, DU/AU and B/C paths against pinned sources.
- [x] EU: independently reproduce wrong gen2 fields, correct codec/native TX,
  verify HT20/40, SGI, 20-in40 and untouched RTS/retry fields; reject gen1 HT fields.
- [x] AU: correct shifted PHY offsets and verify datapath/backend signal users.
- [x] Generic RX24: RED→GREEN across 20 family/SHIFT layouts; preserve legacy layout.
- [x] FU: reproduce/correct host-sequence overrides with checksum/policy coverage.
- [x] Coexistence: reproduce busy-MP dropped LNA; retry bounded latest per-device
  work with correct ACK, timeout, cancellation and firmware ownership handling.
- [x] Independently review the combined fixes, resolve important findings, and
  complete fresh Release, sanitizer and native app/kext checks.
- [x] Bump driver52, retain app46, update compatibility/evidence documentation.
- After source freeze, clean extracted-source and native product builds, archive
  signatures and manifests are recorded in the delivery VERIFICATION.json and
  NATIVE-VERIFICATION.json; these records are outside the hashed source archive.

The delivered verification record must explicitly retain physical and firmware
limitations; tests do not certify every USB ID or commercial board revision.
