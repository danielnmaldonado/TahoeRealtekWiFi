#ifndef RTW_RUNTIME_HOST_PLATFORM_HPP
#define RTW_RUNTIME_HOST_PLATFORM_HPP
#define RTW_NET80211_PREFIX_HPP
#define CTimeout RTWNet80211Timeout
#define _fWorkloop RTWNet80211WorkLoop
#define _fCommandGate RTWNet80211CommandGate
#include <sys/queue.h>
#include <algorithm>
#include <atomic>
#include <vector>
#include <mutex>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <strings.h>
using IOReturn=int;
enum {kIOReturnSuccess=0,kIOReturnError=1,kIOReturnNoMemory=2,kIOReturnBusy=3,kIOReturnBadArgument=4,kIOReturnNoResources=5};
inline std::atomic<int> host_live_objects{0},host_live_thread_calls{0};
inline bool host_fail_add_event=false;
inline bool host_fail_workloop=false,host_fail_gate=false,host_fail_thread_call=false;
inline unsigned host_add_event_calls=0,host_fail_add_at=0;
class OSObject {
    std::atomic<unsigned> references{1};
public:
    OSObject(){++host_live_objects;}virtual ~OSObject(){--host_live_objects;}
    virtual bool init(){return true;}
    void retain(){++references;}void release(){if(--references==0)delete this;}
};
#define OSDeclareDefaultStructors(type) public: type(); ~type() override;
#define OSDefineMetaClassAndStructors(type,base) type::type()=default;type::~type()=default;
#define OSDynamicCast(type,object) dynamic_cast<type*>(object)
inline void *IOMalloc(size_t n){return calloc(1,n);}inline void IOFree(void*p,size_t){free(p);}
using IOLock=std::mutex;inline IOLock *IOLockAlloc(){return new IOLock;}inline void IOLockFree(IOLock*p){delete p;}inline void IOLockLock(IOLock*p){p->lock();}inline void IOLockUnlock(IOLock*p){p->unlock();}
inline bool OSCompareAndSwapPtr(void*old,void*value,void*volatile*address){return __atomic_compare_exchange_n(address,&old,value,false,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST);}
class IOWorkLoop;
class IOEventSource:public OSObject {public:IOWorkLoop*workloop=nullptr;OSObject*owner=nullptr;bool enabled=false;void enable(){enabled=true;}void disable(){enabled=false;}IOWorkLoop*getWorkLoop(){return workloop;}};
class IOTimerEventSource:public IOEventSource {
public:using Action=void(*)(OSObject*,IOTimerEventSource*);Action action=nullptr;bool scheduled=false;
 static IOTimerEventSource*timerEventSource(OSObject*o,Action a){auto*t=new IOTimerEventSource;t->owner=o;t->action=a;return t;}
 void setTimeoutMS(int){scheduled=true;}void cancelTimeout(){scheduled=false;}
};
class IOWorkLoop:public OSObject {
public:std::recursive_mutex mutex;std::vector<IOEventSource*>events;bool in_gate=false;
 static IOWorkLoop*workLoop(){return host_fail_workloop?nullptr:new IOWorkLoop;}
 IOReturn addEventSource(IOEventSource*e){std::lock_guard<std::recursive_mutex>l(mutex);++host_add_event_calls;if(host_fail_add_event||host_add_event_calls==host_fail_add_at)return kIOReturnNoResources;e->retain();e->workloop=this;events.push_back(e);return 0;}
 void removeEventSource(IOEventSource*e){std::lock_guard<std::recursive_mutex>l(mutex);auto i=std::find(events.begin(),events.end(),e);if(i!=events.end()){events.erase(i);e->workloop=nullptr;e->release();}}
 bool dispatchOne(){std::lock_guard<std::recursive_mutex>l(mutex);for(auto*e:events){auto*t=dynamic_cast<IOTimerEventSource*>(e);if(!t||!t->enabled||!t->scheduled)continue;t->retain();t->scheduled=false;bool prior=in_gate;in_gate=true;t->action(t->owner,t);in_gate=prior;t->release();return true;}return false;}
};
class IOCommandGate:public IOEventSource {
public:using Action=IOReturn(*)(OSObject*,void*,void*,void*,void*);
 static IOCommandGate*commandGate(OSObject*o){if(host_fail_gate)return nullptr;auto*g=new IOCommandGate;g->owner=o;return g;}
 IOReturn runAction(Action action,void*a=nullptr,void*b=nullptr,void*c=nullptr,void*d=nullptr){std::lock_guard<std::recursive_mutex>l(workloop->mutex);bool prior=workloop->in_gate;workloop->in_gate=true;auto r=action(owner,a,b,c,d);workloop->in_gate=prior;return r;}
};
class CTimeout:public OSObject {OSDeclareDefaultStructors(CTimeout)};
using thread_call_param_t=void*;using thread_call_func_t=void(*)(void*,void*);
struct host_thread_call {thread_call_func_t function;void*argument;bool pending=false;};using thread_call_t=host_thread_call*;
inline std::mutex host_calls_lock;inline std::vector<thread_call_t>host_calls;
inline thread_call_t thread_call_allocate(thread_call_func_t f,void*a){if(host_fail_thread_call)return nullptr;++host_live_thread_calls;return new host_thread_call{f,a,false};}
inline bool thread_call_enter(thread_call_t c){std::lock_guard<std::mutex>l(host_calls_lock);if(c->pending)return true;c->pending=true;host_calls.push_back(c);return false;}
inline bool thread_call_free(thread_call_t c){if(c->pending)return false;--host_live_thread_calls;delete c;return true;}
inline void host_run_thread_calls(){for(;;){thread_call_t c;{std::lock_guard<std::mutex>l(host_calls_lock);if(host_calls.empty())return;c=host_calls.front();host_calls.erase(host_calls.begin());c->pending=false;}auto f=c->function;auto a=c->argument;f(a,nullptr);}}
#endif
