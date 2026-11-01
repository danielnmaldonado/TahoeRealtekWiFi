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
}
#include <IOKit/IOCommandGate.h>

extern IOCommandGate *_fCommandGate;

struct network_header {
    char pad[0x48];
};

static IOReturn _if_input(OSObject *target, void *arg0, void *arg1, void *arg2, void *arg3)
{
    mbuf_t m;
    bool isEmpty = true;
    struct _ifnet *ifq = (struct _ifnet *)arg0;
    struct mbuf_list *ml = (struct mbuf_list *)arg1;
    uint64_t offered = 0;
    uint64_t immediate = 0;
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
            noIfnet = ifq->iface->getIfnet() == NULL;
            if (noIfnet) ++ifq->if_input_diag.noIfnetCalls;
        }
        isEmpty = false;
        ++offered;
        ++ifq->if_input_diag.offeredPackets;
        ifq->if_input_diag.offeredBytes += mbuf_pkthdr_len(m);
        immediate += ifq->iface->inputPacket(m, 0, IONetworkInterface::kInputOptionQueuePacket);
        if (ifq->netStat != NULL) {
            ifq->netStat->inputPackets++;
        }
    }
    if (!isEmpty) {
        ++ifq->if_input_diag.flushCalls;
        const uint64_t flushed = ifq->iface->flushInputQueue();
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

