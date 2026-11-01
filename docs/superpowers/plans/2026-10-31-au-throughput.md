# RTL8814AU throughput implementation plan

> Execute delegated independent fixes, then root integration/verification.

**Goal:** fix confirmed AU receive-size, PHY-rate authority and TXAMPDU gaps.
**Architecture:** per-family RX sizing; native C2H owns thermal tracking rate;
complete peer/BA/wire TXAMPDU support using existing common HT flow.
**Tech stack:** existing C/C++ kext/core/net80211; native/offline sanitizers.
**Spec:** ../specs/2026-10-31-au-throughput-design.md

- [x] Capture new-port baseline, bound transfers to en3, separate USB faults.
- [x] Primary20KiB versus16KiB RX mismatch and real split RED reproduced.
- [x] RX owner: centralized AU32KiB allocation/read/recovery and regressions;
  preserve all other families and USB budgets.
- [x] Real Controller→PHY thermal RED:55checks/8expectedfailures; minimal fix
  removes packet-side tracking write.55native+2328sharedHT/TX checks GREEN.
- [x] AMPDU owner: primary contracts, meaningful RED, then complete bounded port;
  peer-factor cap, per-TID BA, descriptor fields and exclusions/cleanup.
- [x] Independent reviews clean; integrated Release153/153 and host sanitizer153/153 pass; native SDK object checks pass.
- [x] Version driver53, retain app46 and update exact evidence.
- After source freeze, extracted-source/native product builds and ZIP integrity
  are recorded externally in the delivery verification JSON files.

Do not infer active traffic from unchanged IORegistry properties. No hardware
or installed-system mutations by agents; physical tests of new code remain next.
