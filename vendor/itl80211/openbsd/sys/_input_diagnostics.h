#ifndef RTW_IF_INPUT_DIAGNOSTICS_H
#define RTW_IF_INPUT_DIAGNOSTICS_H

#include <stdint.h>

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
};

#endif
