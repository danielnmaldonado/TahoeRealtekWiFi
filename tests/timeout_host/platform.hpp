#pragma once
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <vector>
using IOReturn=int;using UInt32=uint32_t;
enum {kIOReturnSuccess=0,kIOReturnError=1,kIOReturnBadArgument=2,kIOReturnNoMemory=3,kIOReturnNoResources=4};
inline int liveObjects=0;
inline bool failInit=false,failTimer=false,failAdd=false,failArm=false;
class OSObject {
    unsigned refs=1;
public:
    OSObject(){++liveObjects;}
    virtual ~OSObject(){--liveObjects;}
    virtual bool init(){return !failInit;}
    void retain(){++refs;}
    void release(){assert(refs);if(--refs==0)delete this;}
};
#define OSDeclareDefaultStructors(T) public: T()=default; ~T() override=default;
#define OSDynamicCast(T,o) dynamic_cast<T*>(o)
class IOWorkLoop;
class IOTimerEventSource:public OSObject {
public:
    using Action=void(*)(OSObject*,IOTimerEventSource*);
    OSObject* owner=nullptr;Action action=nullptr;IOWorkLoop* loop=nullptr;
    bool pending=false,enabled=false;
    static IOTimerEventSource* timerEventSource(OSObject* owner,Action action) {
        if(failTimer)return nullptr;
        auto* t=new IOTimerEventSource;t->owner=owner;t->action=action;return t;
    }
    void enable(){enabled=true;}
    void disable(){enabled=false;}
    IOReturn setTimeoutMS(UInt32){pending=!failArm;return failArm?kIOReturnNoResources:0;}
    void cancelTimeout(){pending=false;}
    IOWorkLoop* getWorkLoop(){return loop;}
};
class IOWorkLoop:public OSObject {
public:
    std::vector<IOTimerEventSource*> timers;
    IOReturn addEventSource(IOTimerEventSource* t){
        if(failAdd)return kIOReturnNoResources;
        t->loop=this;t->retain();timers.push_back(t);return 0;
    }
    void removeEventSource(IOTimerEventSource* t){
        auto i=std::find(timers.begin(),timers.end(),t);
        if(i!=timers.end()){timers.erase(i);t->loop=nullptr;t->release();}
    }
    void fire(){
        assert(!timers.empty());auto* t=timers.front();
        assert(t->pending&&t->enabled);
        t->pending=false;
        // Do not artificially retain: exercise production's self-free guard.
        t->action(t->owner,t);
    }
};
