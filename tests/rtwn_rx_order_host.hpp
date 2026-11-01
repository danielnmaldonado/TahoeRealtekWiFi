#pragma once
#include "rtw/RTL8192EUDataPath.h"
#include "rtw/USBRXCapacity.h"
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

using IOReturn = int;
using IOInterruptState = unsigned;
enum { kIOReturnSuccess = 0, kIOReturnNotReady = -1, kIOReturnAborted = -2,
       kIOReturnUnderrun = -3, kIOReturnIOError = -4, kIOReturnNotResponding = -5, IFF_RUNNING = 1 };
inline IOInterruptState IOSimpleLockLockDisableInterrupt(void *) { return 0; }
inline void IOSimpleLockUnlockEnableInterrupt(void *, IOInterruptState) {}
struct IOInterruptEventSource {
    unsigned signals = 0;
    void interruptOccurred(void *, void *, int) { ++signals; }
};
struct IOUSBHostPipe {};
struct HostMemory {
    uint8_t payload = 0;
    unsigned completions = 0;
    void complete() { ++completions; }
    void *getBytesNoCopy() { return &payload; }
};
struct HostIfnet {
    unsigned if_flags = IFF_RUNNING, if_snd = 0, starts = 0;
};
struct rtwn_softc {
    struct { HostIfnet ic_if; } sc_ic;
    unsigned sc_tx_timer = 1;
    int core_error = 0;
};
inline int rtwn_io_error(rtwn_softc *p) { return p->core_error; }
inline void ifq_clr_oactive(unsigned *p) { *p = 0; }
inline void rtwn_start(HostIfnet *p) { ++p->starts; }

class RTWUSBController {
public:
    enum { RXSlots = 4, RXCapacity = 16384 };
    rtw_wifi_family family_=RTW_WIFI_RTL8192EU;
    struct Transfer {
        HostMemory storage;
        HostMemory *memory = &storage;
        IOUSBHostPipe *pipe = nullptr;
        unsigned index = 0;
        uint32_t submitted = 0, actual = 0, generation = 0;
        IOReturn result = kIOReturnNotReady;
        bool prepared = false;
    };
    std::array<Transfer, RTW_TRANSFER_SLOTS> transfers_;
    rtw_transfer_ledger ledger_{};
    // Recovery has a separate real-controller suite; these cases assert ordinary ordering.
    struct {bool pending=false;unsigned consecutive=0;IOReturn lastError=kIOReturnSuccess;} rxRecovery_;
    uint64_t rxPackets_=0;
    bool beginRXRecovery(IOReturn error) {if(error==kIOReturnNotResponding)std::abort();return false;}
    void advanceRXRecovery() {std::abort();}
    void failRXRecovery() {std::abort();}
    rtwn_softc core_;
    struct Stats { unsigned inputErrors = 0, outputErrors = 0, outputPackets = 0; } statsValue_;
    Stats *stats_ = &statsValue_;
    IOInterruptEventSource event_;
    IOInterruptEventSource *completionEvent_ = &event_;
    IOUSBHostPipe pipe_;
    IOUSBHostPipe *rxPipe_ = &pipe_;
    void *completionLock_ = nullptr;
    bool running_ = true, stopping_ = false, present_ = true, attached_ = true;
    bool radioReady_ = false, enabled_ = false, completeOnResubmit = false;
    IOReturn usbError_ = kIOReturnSuccess;
    unsigned references = 1, rxErrors_ = 0, txErrors_ = 0, txPackets_ = 0, quiesces = 0;
    uint64_t txUSBTransferErrors_ = 0;
    unsigned outputWakes = 0;
    void serviceTXOutputQueue() { ++outputWakes; }
    uint8_t nextPayload = 4;
    std::vector<unsigned> delivered;
    RTWUSBController() {
        for (unsigned i = 0; i < transfers_.size(); ++i) transfers_[i].index = i;
    }
    void retain() { ++references; }
    void release() { if (!references) std::abort(); --references; }
    bool hasFreeTX() const {
        return running_ && !stopping_ && usbError_ == kIOReturnSuccess &&
            (ledger_.pending & 0xfff0U) != 0xfff0U;
    }
    void fault(IOReturn e) { usbError_ = e; }
    void quiesce(bool) { ++quiesces; running_ = false; }
    int submit(unsigned index, IOUSBHostPipe *pipe, uint32_t length) {
        if (!running_ || stopping_ || usbError_ || !rtw_transfer_begin(&ledger_, index)) return 1;
        auto &t = transfers_[index];
        ++t.generation; t.pipe = pipe; t.submitted = length; t.actual = 0;
        t.result = kIOReturnNotReady; t.prepared = true; retain();
        if (completeOnResubmit && index < RXSlots) finish(index, nextPayload++);
        return 0;
    }
    void finish(unsigned index, uint8_t payload, IOReturn result = kIOReturnSuccess) {
        auto &t = transfers_[index]; t.storage.payload = payload;
        recordCompletion(&t, result, index < RXSlots ? 1 : t.submitted, t.generation);
    }
    void receive(const uint8_t *p, size_t n) { if(n){++rxPackets_;delivered.push_back(*p);} }
    void recordCompletion(Transfer *, IOReturn, uint32_t, uint32_t);
    void drainCompletions(bool);
    void scheduleDeferredCleanup() {}
};
