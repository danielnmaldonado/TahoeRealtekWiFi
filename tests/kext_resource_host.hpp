// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "rtw/RTL8192EUDataPath.h"
#include "rtw/USBRXCapacity.h"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <vector>

inline unsigned resourceLiveEvents=0,resourceLiveBuffers=0,resourceAllocationCalls=0;
inline unsigned resourceFailAllocationAt=0,resourceRuntimeReleases=0;
inline size_t resourceBufferBytes=0;
using IOInterruptState=unsigned;
using IOSimpleLock=unsigned;
using IOLock=unsigned;
inline IOInterruptState IOSimpleLockLockDisableInterrupt(IOSimpleLock*) {return 0;}
inline void IOSimpleLockUnlockEnableInterrupt(IOSimpleLock*,IOInterruptState) {}
inline void IOSimpleLockFree(IOSimpleLock*p) {delete p;}
inline void IOLockLock(IOLock*) {}
inline void IOLockUnlock(IOLock*) {}
inline void IOFree(void*p,size_t) {free(p);}
inline void RTWNet80211Release() {++resourceRuntimeReleases;}
inline void thread_call_free(void*p) {free(p);}
enum {kIODirectionIn=1,kIODirectionOut=2};

class IOBufferMemoryDescriptor {
    std::vector<uint8_t> bytes_;
public:
    unsigned completions=0;
    explicit IOBufferMemoryDescriptor(size_t n):bytes_(n) {++resourceLiveBuffers;resourceBufferBytes+=n;}
    ~IOBufferMemoryDescriptor() {--resourceLiveBuffers;resourceBufferBytes-=bytes_.size();}
    static IOBufferMemoryDescriptor*withCapacity(size_t n,int) {
        if(++resourceAllocationCalls==resourceFailAllocationAt)return nullptr;
        return new IOBufferMemoryDescriptor(n);
    }
    size_t getCapacity() const {return bytes_.size();}
    void*getBytesNoCopy() {return bytes_.data();}
    void complete() {++completions;}
    void release() {delete this;}
};
class IOWorkLoop;
class IOEventSource {
    unsigned references_=1;
public:
    IOWorkLoop*loop=nullptr;
    bool enabled=true,scheduled=false;
    IOEventSource() {++resourceLiveEvents;}
    virtual ~IOEventSource() {assert(!loop);--resourceLiveEvents;}
    void retain() {++references_;}
    void release() {assert(references_);if(!--references_)delete this;}
    void disable() {enabled=false;}
    void cancelTimeout() {scheduled=false;}
    IOWorkLoop*getWorkLoop() const {return loop;}
};
using IOCommandGate=IOEventSource;
using IOTimerEventSource=IOEventSource;
using IOInterruptEventSource=IOEventSource;
class IOWorkLoop {
    unsigned references_=1;
public:
    std::vector<IOEventSource*> events;
    ~IOWorkLoop() {assert(events.empty());}
    void retain() {++references_;}
    void release() {assert(references_);if(!--references_)delete this;}
    void addEventSource(IOEventSource*e) {assert(!e->loop);e->loop=this;e->retain();events.push_back(e);}
    void removeEventSource(IOEventSource*e) {
        auto i=std::find(events.begin(),events.end(),e);
        if(i==events.end())return;
        events.erase(i);e->loop=nullptr;e->release();
    }
};
struct IOUSBHostPipe {void release() {delete this;}};
struct IOEthernetInterface {void release() {delete this;}};
inline void rtw88_phy_radio_invalidate(unsigned*p) {*p=0;}

class RTWUSBController {
public:
    enum {RXSlots=4,TXCapacity=4352};
    struct Transfer {
        RTWUSBController*owner=nullptr;
        IOBufferMemoryDescriptor*memory=nullptr;
        IOUSBHostPipe*pipe=nullptr;
        unsigned index=0;
        bool prepared=false;
    } transfers_[RTW_TRANSFER_SLOTS];
    rtw_wifi_family family_=RTW_WIFI_RTL8192EU;
    rtw_transfer_ledger ledger_{};
    IOSimpleLock*completionLock_=new IOSimpleLock;
    IOLock*lifecycleLock_=nullptr;
    bool retainedForPending_=false,runtimeLease_=true;
    void*cleanupCall_=nullptr;
    IOWorkLoop*loop_=nullptr;
    IOCommandGate*gate_=nullptr;
    IOTimerEventSource*scanTimer_=nullptr,*calibTimer_=nullptr,*watchdogTimer_=nullptr;
    IOInterruptEventSource*completionEvent_=nullptr;
    IOEthernetInterface*netif_=nullptr;
    void*stats_=nullptr,*usb_=nullptr;
    bool usbOpened_=false,present_=false;
    IOUSBHostPipe*rxPipe_=nullptr,*txPipes_[4]{};
    void*acPHYStorage_=nullptr;
    size_t acPHYSize_=0;
    struct {void*phy=nullptr;} acCore_;
    IOTimerEventSource*r88CoexTimer_=nullptr;
    unsigned r88PHY_=0;
    static constexpr size_t R88StorageBytes=64;
    void*r88Storage_=nullptr;
    bool r88BoardKnown_=false;
    int allocateTransfers();
    void freeTransfers();
    void releasePlatform();
    void release88Storage();
    void clear88Discovery(bool) {}
    void coex88Stop() {if(r88CoexTimer_)r88CoexTimer_->cancelTimeout();}
    void closeUSBProvider() {usbOpened_=false;}
    void detachInterface(IOEthernetInterface*,bool) {}
    void release8723Storage() {}
    void release8710BUStorage() {}
    void release8192FUStorage() {}
    void release8192DUStorage() {}
    void release8812Storage() {}
};
