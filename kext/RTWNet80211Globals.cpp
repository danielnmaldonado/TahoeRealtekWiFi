#include "RTWNet80211Globals.hpp"
#include <sys/CTimeout.hpp>
#include <sys/_task.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOLocks.h>
#include <kern/thread_call.h>

OSDefineMetaClassAndStructors(RTWNet80211Timeout, OSObject)

__attribute__((visibility("hidden"))) IOWorkLoop *RTWNet80211WorkLoop = nullptr;
__attribute__((visibility("hidden"))) IOCommandGate *RTWNet80211CommandGate = nullptr;

/* This owner is independent of every USB controller. A controller can detach
 * while its sibling continues to use compatibility timers and systq. */
class RTWNet80211RuntimeOwner : public OSObject {
    OSDeclareDefaultStructors(RTWNet80211RuntimeOwner)
public:
    bool init() override { return OSObject::init(); }
    IOWorkLoop *loop = nullptr;
    IOCommandGate *gate = nullptr;
    thread_call_t cleanupCall = nullptr;
    unsigned leases = 0;
    bool closing = false;
};
OSDefineMetaClassAndStructors(RTWNet80211RuntimeOwner, OSObject)

namespace {
/* The module-lifetime lock outlives individual runtimes, including waiters
 * racing the last release. Never free it while another acquire may use it. */
IOLock *lifecycleLock = nullptr;
RTWNet80211RuntimeOwner *runtime = nullptr;

IOLock *copyLifecycleLock()
{
    IOLock *lock = __atomic_load_n(&lifecycleLock, __ATOMIC_ACQUIRE);
    if (lock)
        return lock;
    IOLock *candidate = IOLockAlloc();
    if (!candidate)
        return nullptr;
    lock = nullptr;
    if (!__atomic_compare_exchange_n(&lifecycleLock, &lock, candidate, false,
                                     __ATOMIC_RELEASE, __ATOMIC_ACQUIRE)) {
        IOLockFree(candidate);
        return lock;
    }
    return candidate;
}

IOReturn drainSystemQueue(OSObject *, void *, void *, void *, void *)
{
    /* A thread-call worker enters the gate from outside the WorkLoop. Any
     * task callback, including one that released the final lease itself,
     * has returned before this action runs. destroy therefore finishes its
     * deferred owner/timer release before the compatibility globals clear. */
    taskq_destroy(systq);
    return kIOReturnSuccess;
}

void cleanupRuntime(thread_call_param_t argument, thread_call_param_t)
{
    auto *owner = static_cast<RTWNet80211RuntimeOwner *>(argument);
    owner->gate->runAction(drainSystemQueue);

    IOLock *lock = copyLifecycleLock();
    IOLockLock(lock);
    RTWNet80211CommandGate = nullptr;
    RTWNet80211WorkLoop = nullptr;
    runtime = nullptr;
    IOLockUnlock(lock);

    owner->loop->removeEventSource(owner->gate);
    owner->gate->release();
    owner->gate = nullptr;
    /* Release a WorkLoop outside its own thread: free may wait for that
     * thread to exit. The runtime kept this reference through queue drain. */
    owner->loop->release();
    owner->loop = nullptr;
    thread_call_free(owner->cleanupCall); /* SDK permits own-callback free. */
    owner->cleanupCall = nullptr;
    owner->release(); /* published runtime reference */
    owner->release(); /* cleanup worker reference */
}

void discardRuntime(RTWNet80211RuntimeOwner *owner)
{
    if (owner->gate) {
        if (owner->loop && owner->gate->getWorkLoop() == owner->loop)
            owner->loop->removeEventSource(owner->gate);
        owner->gate->release();
    }
    if (owner->loop)
        owner->loop->release();
    if (owner->cleanupCall)
        thread_call_free(owner->cleanupCall);
    owner->release();
}
}

IOReturn RTWNet80211Acquire(IOWorkLoop **retainedLoop)
{
    if (!retainedLoop)
        return kIOReturnBadArgument;
    *retainedLoop = nullptr;
    IOLock *lock = copyLifecycleLock();
    if (!lock)
        return kIOReturnNoMemory;
    IOLockLock(lock);
    if (runtime) {
        if (runtime->closing) {
            IOLockUnlock(lock);
            return kIOReturnBusy;
        }
        ++runtime->leases;
        runtime->loop->retain();
        *retainedLoop = runtime->loop;
        IOLockUnlock(lock);
        return kIOReturnSuccess;
    }

    /* No runtime exists, so no stack callback can acquire this lock while
     * first-time event sources and the system queue are initialized. */
    auto *owner = new RTWNet80211RuntimeOwner;
    if (!owner) {
        IOLockUnlock(lock);
        return kIOReturnNoMemory;
    }
    if (!owner->init()) {
        owner->release();
        IOLockUnlock(lock);
        return kIOReturnNoMemory;
    }
    owner->loop = IOWorkLoop::workLoop();
    owner->gate = IOCommandGate::commandGate(owner);
    owner->cleanupCall = thread_call_allocate(cleanupRuntime, owner);
    if (!owner->loop || !owner->gate || !owner->cleanupCall) {
        discardRuntime(owner);
        IOLockUnlock(lock);
        return kIOReturnNoMemory;
    }
    if (owner->loop->addEventSource(owner->gate) != kIOReturnSuccess) {
        discardRuntime(owner);
        IOLockUnlock(lock);
        return kIOReturnNoResources;
    }
    RTWNet80211WorkLoop = owner->loop;
    RTWNet80211CommandGate = owner->gate;
    taskq_init();
    if (!taskq_is_initialized(systq)) {
        RTWNet80211CommandGate = nullptr;
        RTWNet80211WorkLoop = nullptr;
        discardRuntime(owner);
        IOLockUnlock(lock);
        return kIOReturnNoResources;
    }
    owner->leases = 1;
    runtime = owner;
    owner->loop->retain();
    *retainedLoop = owner->loop;
    IOLockUnlock(lock);
    return kIOReturnSuccess;
}

void RTWNet80211Release()
{
    IOLock *lock = copyLifecycleLock();
    if (!lock)
        return;
    IOLockLock(lock);
    if (!runtime || runtime->closing || !runtime->leases) {
        IOLockUnlock(lock);
        return;
    }
    if (--runtime->leases == 0) {
        runtime->closing = true;
        runtime->retain();
        thread_call_enter(runtime->cleanupCall);
    }
    IOLockUnlock(lock);
}
