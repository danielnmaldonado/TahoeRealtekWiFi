# AU operating modes implementation

The user requested continued work directly in source and explicitly declined
another confirmation flow. The parent delegated the complete AU PHY/runtime
and backend extension; shared controller/profile/CMake edits stay with parent.

1. Extend the existing typed AU PHY with explicit primary/center/width/mode.
   Preserve legacy entry points. Test geometry, actual OTP HT/VHT signed
   deltas, per-rate PG BCD references, worldwide-minimum source limits and
   TXAGC register words before implementing. Reuse extracted per-chip channel
   and IQK/LCK algorithms for20/40/80; require actual RFK before TX resume.
2. Extract pinned chip/RFE thermal swing tables and port measured thermal
   averaging, bounded power correction and RFK thresholds. Tests cover missing
   ROM, hot/cold extremes, I/O failures and preserving TX pause on failure.
3. Extend AU runtime RA_INFO0x40 and TX40 for negotiated HT/VHT/NSS/BW/SGI,
   using source masks and preserving fixed basic-rate management/EAPOL.
   Test every supported MCS shape and negative combinations before coding.
4. Extend the concrete net80211 AU adapter while preserving existing hook
   signatures. Advertise only fully implemented modes, negotiate from actual
   peer/channel fields, retune/recalibrate when operating width changes and
   retain controlled-port/software-CCMP checks. Add isolated actual-method
   traces for both families, widths, failures and ownership.
5. Run host sanitizers, strict native C and whole-controller KDK compilation;
   supply parent integration deltas. Physical validation stays with parent.

Reference: Linuxrtw88 f47d223627f80cb7a1eb14c585fe6cb35de5e984,
rtw88xxa.c, rtw8812a.c, rtw8821a.c, phy.c, main.c, main.h, fw.c/fw.h,
tx.c/tx.h and pinned per-chip tables. No guessed board flags or firmware ABI.
