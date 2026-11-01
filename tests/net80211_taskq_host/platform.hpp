#ifndef RTW_TASKQ_HOST_PLATFORM_HPP
#define RTW_TASKQ_HOST_PLATFORM_HPP
/* Event-loop model for unit tests of the production taskq compatibility file.
 * This is not a replacement kernel implementation or a hardware test. */
#include <sys/queue.h>
#include <algorithm>
#include <vector>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

using IOReturn = int;
enum { kIOReturnSuccess = 0, kIOReturnError = 1,
       kIOReturnNoMemory = 2, kIOReturnBusy = 3 };
class OSObject {
    unsigned references = 1;
public:
    virtual ~OSObject() = default;
    void retain() { ++references; }
    void release() { if (--references == 0) delete this; }
};
#define OSDeclareDefaultStructors(type) public: type(); ~type() override;
#define OSDefineMetaClassAndStructors(type, base) type::type() = default; type::~type() = default;
#define OSDynamicCast(type, object) dynamic_cast<type *>(object)
inline void *IOMalloc(size_t size) { return calloc(1, size); }
inline void IOFree(void *pointer, size_t) { free(pointer); }

class IOWorkLoop;
class IOTimerEventSource : public OSObject {
public:
    using Action = void (*)(OSObject *, IOTimerEventSource *);
    OSObject *owner = nullptr;
    Action action = nullptr;
    bool scheduled = false;
    bool enabled = false;
    IOWorkLoop *workloop = nullptr;
    static IOTimerEventSource *timerEventSource(OSObject *owner, Action action) {
        auto *timer = new IOTimerEventSource;
        timer->owner = owner;
        timer->action = action;
        return timer;
    }
    void enable() { enabled = true; }
    void disable() { enabled = false; }
    void setTimeoutMS(int) { scheduled = true; }
    void cancelTimeout() { scheduled = false; }
};
class IOWorkLoop : public OSObject {
public:
    std::vector<IOTimerEventSource *> events;
    bool in_gate = false;
    IOReturn addEventSource(IOTimerEventSource *timer) {
        timer->retain();
        timer->workloop = this;
        events.push_back(timer);
        return kIOReturnSuccess;
    }
    void removeEventSource(IOTimerEventSource *timer) {
        auto found = std::find(events.begin(), events.end(), timer);
        if (found != events.end()) {
            events.erase(found);
            timer->workloop = nullptr;
            timer->release();
        }
    }
    bool dispatchOne() {
        for (auto *timer : events) {
            if (!timer->enabled || !timer->scheduled) continue;
            timer->retain();
            timer->scheduled = false;
            bool was_in_gate = in_gate;
            in_gate = true;
            timer->action(timer->owner, timer);
            in_gate = was_in_gate;
            timer->release();
            return true;
        }
        return false;
    }
};
class IOCommandGate : public OSObject {
public:
    using Action = IOReturn (*)(OSObject *, void *, void *, void *, void *);
    IOWorkLoop *workloop;
    explicit IOCommandGate(IOWorkLoop *loop) : workloop(loop) {}
    IOReturn runAction(Action action, void *a = nullptr, void *b = nullptr,
                       void *c = nullptr, void *d = nullptr) {
        bool was_in_gate = workloop->in_gate;
        workloop->in_gate = true;
        IOReturn result = action(this, a, b, c, d);
        workloop->in_gate = was_in_gate;
        return result;
    }
};
#endif
