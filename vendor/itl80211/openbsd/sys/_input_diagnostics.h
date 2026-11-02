#ifndef RTW_IF_INPUT_DIAGNOSTICS_H
#define RTW_IF_INPUT_DIAGNOSTICS_H

#include <stdint.h>

/* Cumulative native values, not inferred deltas. A failed read retains numeric
 * identity/capabilities but leaves counters zero; inspect statsError and the
 * containing sample count before interpreting them. No ifnet is retained. */
struct rtw_if_input_native_snapshot {
    uint64_t packetsIn, bytesIn, errorsIn;
    uint64_t packetsOut, bytesOut, errorsOut;
    uint32_t index, supportedCapabilities, enabledCapabilities;
    int32_t statsError;
};

/* Two native KPI reads on the first batch and once per 1024 further batches.
 * Keep observation work bounded in the receive path. The batch ordinal and
 * offered totals identify exactly which handoff the snapshots surround. */
#define RTW_IF_INPUT_NATIVE_SAMPLE_INTERVAL 1024U

/* Per-interface observations at the IONetworkInterface ownership boundary.
 * These are not BSD packet counters. All access is serialized by the shared
 * net80211 WorkLoop; no packet, address or key contents are retained. */
struct rtw_if_input_diagnostics {
    uint64_t calls;
    uint64_t offeredPackets;
    uint64_t offeredBytes;
    uint64_t immediateReturns;
    uint64_t flushCalls;
    uint64_t flushPackets;
    uint64_t submissionMismatches;
    uint64_t noIfnetCalls;
    uint64_t nativeSampleCount;
    uint64_t nativeSampleBatch;
    uint64_t nativeSampleOfferedPackets;
    uint64_t nativeSampleOfferedBytes;
    uint64_t nativeSampleImmediateReturns;
    uint64_t nativeSampleFlushPackets;
    uint64_t nativeSampleIfnetStable;
    struct rtw_if_input_native_snapshot nativeBefore;
    struct rtw_if_input_native_snapshot nativeAfter;
};

#endif
