//
//  _mbuf.cpp
//  itlwm
//
//  Created by qcwap on 2020/6/14.
//  Copyright © 2020 钟先耀. All rights reserved.
//
/*
* Copyright (C) 2020  钟先耀
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include <sys/_mbuf.h>
extern "C" {
#include <net/bpf.h>
#include <net/kpi_interface.h>
}
#include <IOKit/IOCommandGate.h>

extern IOCommandGate *_fCommandGate;

struct network_header {
    char pad[0x48];
};

static rtw_if_input_native_snapshot _if_input_native_snapshot(ifnet_t iface)
{
    rtw_if_input_native_snapshot snapshot{};
    snapshot.statsError = ENXIO;
    if (iface == NULL) return snapshot;
    snapshot.index = ifnet_index(iface);
    snapshot.supportedCapabilities = ifnet_capabilities_supported(iface);
    snapshot.enabledCapabilities = ifnet_capabilities_enabled(iface);
    ifnet_stats_param stats{};
    snapshot.statsError = ifnet_stat(iface, &stats);
    if (snapshot.statsError == 0) {
        snapshot.packetsIn = stats.packets_in;
        snapshot.bytesIn = stats.bytes_in;
        snapshot.errorsIn = stats.errors_in;
        snapshot.packetsOut = stats.packets_out;
        snapshot.bytesOut = stats.bytes_out;
        snapshot.errorsOut = stats.errors_out;
    }
    return snapshot;
}

static IOReturn _if_input(OSObject *target, void *arg0, void *arg1, void *arg2, void *arg3)
{
    mbuf_t m;
    bool isEmpty = true;
    struct _ifnet *ifq = (struct _ifnet *)arg0;
    struct mbuf_list *ml = (struct mbuf_list *)arg1;
    uint64_t offered = 0;
    uint64_t immediate = 0;
    uint64_t offeredBytes = 0;
    bool sampleNative = false;
    ifnet_t sampleIfnet = NULL;
    bool noIfnet = false;
    ++ifq->if_input_diag.calls;
    
    /* inputPacket takes ownership and requires a single, unlinked packet.
     * Detach it before the handoff; the interface may change nextpkt or free
     * the mbuf, so traversing that mbuf afterward is invalid. */
    while ((m = ml_dequeue(ml)) != NULL) {
        if (ifq->iface == NULL) {
            panic("%s ifq->iface == NULL!!!\n", __FUNCTION__);
            break;
        }
        if (m == NULL) {
            XYLog("%s m == NULL!!!\n", __FUNCTION__);
            continue;
        }
        //        XYLog("%s %d 啊啊啊啊 ifq->iface->inputPacket(m) hdr_len=%d len=%d\n", __FUNCTION__, __LINE__, mbuf_pkthdr_len(m), mbuf_len(m));
        if (isEmpty) {
            const auto nativeIfnet = ifq->iface->getIfnet();
            noIfnet = nativeIfnet == NULL;
            if (noIfnet) ++ifq->if_input_diag.noIfnetCalls;
            sampleNative = ifq->if_input_diag.flushCalls %
                RTW_IF_INPUT_NATIVE_SAMPLE_INTERVAL == 0;
            if (sampleNative) {
                sampleIfnet = nativeIfnet;
                ifq->if_input_diag.nativeBefore = _if_input_native_snapshot(nativeIfnet);
            }
        }
        isEmpty = false;
        ++offered;
        ++ifq->if_input_diag.offeredPackets;
        const auto packetBytes = mbuf_pkthdr_len(m);
        offeredBytes += packetBytes;
        ifq->if_input_diag.offeredBytes += packetBytes;
        immediate += ifq->iface->inputPacket(m, 0, IONetworkInterface::kInputOptionQueuePacket);
        if (ifq->netStat != NULL) {
            ifq->netStat->inputPackets++;
        }
    }
    if (!isEmpty) {
        ++ifq->if_input_diag.flushCalls;
        const uint64_t flushed = ifq->iface->flushInputQueue();
        if (sampleNative) {
            const auto nativeIfnet = ifq->iface->getIfnet();
            auto& diag = ifq->if_input_diag;
            diag.nativeAfter = _if_input_native_snapshot(nativeIfnet);
            ++diag.nativeSampleCount;
            diag.nativeSampleBatch = diag.flushCalls;
            diag.nativeSampleOfferedPackets = offered;
            diag.nativeSampleOfferedBytes = offeredBytes;
            diag.nativeSampleImmediateReturns = immediate;
            diag.nativeSampleFlushPackets = flushed;
            diag.nativeSampleIfnetStable = sampleIfnet != NULL && sampleIfnet == nativeIfnet;
        }
        ifq->if_input_diag.immediateReturns += immediate;
        ifq->if_input_diag.flushPackets += flushed;
        /* A queued inputPacket normally returns zero; a detached interface
         * instead frees the mbuf and returns one. A flush return is a queue
         * submission count, not proof that BSD accepted or counted packets.
         * Do not retry or free any packet after this ownership boundary. */
        if (immediate != 0 || flushed != offered) {
            ++ifq->if_input_diag.submissionMismatches;
            return noIfnet ? kIOReturnNotReady : kIOReturnIOError;
        }
        if (noIfnet) return kIOReturnNotReady;
    }
    return kIOReturnSuccess;
}

int if_input(struct _ifnet *ifq, struct mbuf_list *ml)
{
    return _fCommandGate->runAction((IOCommandGate::Action)_if_input, ifq, ml);
}

