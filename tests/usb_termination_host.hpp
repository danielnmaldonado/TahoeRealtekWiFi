// SPDX-License-Identifier: GPL-2.0-only
#pragma once
// Strict host doubles for the *extracted* USB termination and close methods.
// Real IOKit and a detached physical USB provider cannot be simulated here.
#include <cassert>
#include <cstdint>
#include <mutex>

using IOReturn=int;
using UInt32=uint32_t;
constexpr IOReturn kIOReturnSuccess=0;
constexpr UInt32 kIOMessageServiceIsTerminated=0x110;
constexpr UInt32 kIOMessageServiceIsRequestingClose=0x111;

struct IOService {virtual ~IOService()=default;};
struct RTWUSBController;
struct IOLock {std::mutex mutex;};
inline void IOLockLock(IOLock* lock){lock->mutex.lock();}
inline void IOLockUnlock(IOLock* lock){lock->mutex.unlock();}

struct IOWorkLoop {
    unsigned references=1;
    void retain(){++references;}
    void release(){assert(references>1);--references;}
};
struct IOUSBHostInterface:IOService {
    RTWUSBController* owner=nullptr;
    unsigned references=1,closeCalls=0;
    bool synchronousStop=true;
    void retain(){++references;}
    void release(){assert(references>1);--references;}
    void close(RTWUSBController* client);
};
struct MockSuper:IOService {
    unsigned superMessages=0;
    IOReturn message(UInt32,IOService*,void*){++superMessages;return kIOReturnSuccess;}
};
struct IOCommandGate {
    RTWUSBController* owner=nullptr;
    bool held=false,failNext=false;
    unsigned references=1;
    void retain(){++references;}
    void release(){assert(references>1);--references;}
    IOReturn runAction(IOReturn (*)(RTWUSBController*,void*),void* operation);
};
struct RTWUSBController:MockSuper {
    enum{Terminated=7};
    IOUSBHostInterface*usb_=nullptr;
    bool usbOpened_=false,present_=false,closing_=false,gateAvailable=true;
    bool stopped=false,platformReleased=false;
    IOService *duExtra_=nullptr;unsigned duExtraCloses=0;
    bool is8192DUUSBProvider(IOService*p)const{return p&&p==duExtra_;}
    void close8192DUProviders(){assert(!gate.held);if(duExtra_){duExtra_=nullptr;++duExtraCloses;}}
    unsigned references=1,quiesces=0;
    IOLock lock;
    IOLock*lifecycleLock_=&lock;
    IOCommandGate gate;
    IOWorkLoop loop;
    RTWUSBController(){gate.owner=this;}
    void retain(){++references;}
    void release(){assert(references>1);--references;}
    IOCommandGate* copyGate(IOWorkLoop**lease){
        *lease=nullptr;
        if(!gateAvailable)return nullptr;
        gate.retain();loop.retain();*lease=&loop;return &gate;
    }
    static IOReturn gated(RTWUSBController*self,void*operation){
        assert(reinterpret_cast<uintptr_t>(operation)==Terminated);
        self->closing_=true;self->present_=false;++self->quiesces;
        return kIOReturnSuccess;
    }
    void connect(IOUSBHostInterface* provider){
        assert(!usb_);usb_=provider;usbOpened_=present_=true;provider->owner=this;
    }
    void closeUSBProvider();
    IOReturn message(UInt32,IOService*,void*);
    void releasePlatform(){
        assert(!gate.held);
        closeUSBProvider();close8192DUProviders();
        IOLockLock(lifecycleLock_);
        usb_=nullptr;usbOpened_=present_=false;
        IOLockUnlock(lifecycleLock_);
        platformReleased=true;
    }
    void stop(IOService*provider){
        assert(provider==usb_);
        assert(!gate.held);
        stopped=true;
        releasePlatform();
    }
};
inline IOReturn IOCommandGate::runAction(IOReturn(*action)(RTWUSBController*,void*),void* operation){
    assert(!held);held=true;
    const IOReturn result=failNext?-1:action(owner,operation);
    held=false;failNext=false;
    return result;
}
inline void IOUSBHostInterface::close(RTWUSBController* client){
    assert(client==owner);
    assert(!client->gate.held);    // The critical deadlock regression.
    assert(!client->usbOpened_); // Must be claimed before a reentrant stop.
    if(client->quiesces)assert(client->closing_&&!client->present_);
    assert(client->references>1||!synchronousStop);
    ++closeCalls;
    if(synchronousStop)client->stop(this);
}
