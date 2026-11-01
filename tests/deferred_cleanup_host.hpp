// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "rtw/RTL8192EUDataPath.h"
#include <cassert>
#include <cstdint>
#include <cstdio>

using IOInterruptState=unsigned;
using thread_call_param_t=void*;
using IOReturn=int;
constexpr IOReturn kIOReturnSuccess=0;
inline IOInterruptState IOSimpleLockLockDisableInterrupt(void*) {return 0;}
inline void IOSimpleLockUnlockEnableInterrupt(void*,IOInterruptState) {}
inline void IOLog(const char*) {}
struct IOWorkLoop {
    unsigned references=1;
    void retain(){++references;}
    void release(){assert(references);--references;}
};
struct RTWUSBController;
struct IOCommandGate {
    RTWUSBController* owner=nullptr;
    unsigned references=1;
    bool held=false;
    void retain(){++references;}
    void release(){assert(references);--references;}
    IOReturn runAction(IOReturn (*action)(RTWUSBController*,void*),void* operation);
};
struct ThreadCall {
    bool pending=false;
    unsigned enqueues=0;
};
using thread_call_t=ThreadCall*;
inline bool thread_call_enter(thread_call_t call) {
    const bool prior=call->pending;
    if(!prior){call->pending=true;++call->enqueues;}
    return prior;
}
struct Core {
    struct {unsigned ic_if=0;} sc_ic;
    unsigned detaches=0;
};
inline void rtwn_detach(Core* c,int){++c->detaches;}
inline void ether_ifdetach(unsigned* p){++*p;}
struct RTWUSBController {
    enum {Shutdown=1,TeardownReady=2};
    bool closing_=false,retainedForPending_=false,teardownReady_=false;
    bool cleanupQueued_=false,attached_=true,macReady_=true,gateAvailable=true;
    unsigned references=1,quiesces=0,platformReleases=0;
    void* completionLock_=this;
    rtw_transfer_ledger ledger_{};
    Core core_;
    ThreadCall call;
    thread_call_t cleanupCall_=&call;
    IOWorkLoop loop;
    IOCommandGate gate;
    RTWUSBController(){gate.owner=this;}
    void retain(){++references;}
    void release(){assert(references);--references;}
    void quiesce(bool erase){assert(erase);++quiesces;}
    IOCommandGate* copyGate(IOWorkLoop** lease) {
        *lease=nullptr;
        if(!gateAvailable)return nullptr;
        gate.retain();loop.retain();*lease=&loop;return &gate;
    }
    void releasePlatform() {
        assert(!gate.held); // The Ethernet child must detach outside the gate.
        if(retainedForPending_)return;
        assert(rtw_transfer_drained(&ledger_));
        ++platformReleases;cleanupCall_=nullptr;gateAvailable=false;
    }
    static IOReturn gated(RTWUSBController* self,void* operation) {
        if(reinterpret_cast<uintptr_t>(operation)==Shutdown)self->shutdownPlatform();
        else {
            assert(reinterpret_cast<uintptr_t>(operation)==TeardownReady);
            self->teardownReady_=true;self->scheduleDeferredCleanup();
        }
        return kIOReturnSuccess;
    }
    void shutdownPlatform();
    void scheduleDeferredCleanup();
    void markTeardownReady();
    static void deferredCleanup(thread_call_param_t,thread_call_param_t);
    void runWorker() {
        assert(call.pending);call.pending=false;
        deferredCleanup(this,nullptr);
    }
};
inline IOReturn IOCommandGate::runAction(
    IOReturn (*action)(RTWUSBController*,void*),void* operation) {
    assert(!held);held=true;auto result=action(owner,operation);held=false;return result;
}
