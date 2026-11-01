//
//  _task.cpp
//  itlwm
//
//  Created by qcwap on 2020/3/1.
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

/* TahoeRealtekWiFi adaptation: queue work on the controller work loop.
 * The upstream kernel-thread backend could enter net80211 concurrently with
 * USB completions. Public operations use the same command gate as the stack. */
#include <sys/_task.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/IOWorkLoop.h>

extern IOWorkLoop *_fWorkloop;
extern IOCommandGate *_fCommandGate;

struct taskq;
class RTWNet80211TaskSource : public OSObject {
    OSDeclareDefaultStructors(RTWNet80211TaskSource)
public:
    struct taskq *queue;
    static void fire(OSObject *, IOTimerEventSource *);
};
OSDefineMetaClassAndStructors(RTWNet80211TaskSource, OSObject)

struct taskq {
    struct task_list pending;
    RTWNet80211TaskSource *owner;
    IOTimerEventSource *timer;
    bool running;
    bool executing;
    bool heap_allocated;
};
static struct taskq system_queue;
struct taskq *const systq = &system_queue;

static void clear_pending(struct taskq *queue)
{
    struct task *work;
    while ((work = TAILQ_FIRST(&queue->pending)) != nullptr) {
        TAILQ_REMOVE(&queue->pending, work, t_entry);
        work->t_flags &= ~TASK_ONQUEUE;
    }
}

static void release_queue(struct taskq *queue)
{
    if (queue->timer) {
        queue->timer->cancelTimeout();
        queue->timer->disable();
        _fWorkloop->removeEventSource(queue->timer);
        queue->timer->release();
        queue->timer = nullptr;
    }
    if (queue->owner) {
        queue->owner->queue = nullptr;
        queue->owner->release();
        queue->owner = nullptr;
    }
    if (queue->heap_allocated)
        IOFree(queue, sizeof(*queue));
}

static void run_one(struct taskq *queue)
{
    struct task *work = TAILQ_FIRST(&queue->pending);
    if (!queue->running || work == nullptr)
        return;
    TAILQ_REMOVE(&queue->pending, work, t_entry);
    work->t_flags &= ~TASK_ONQUEUE;
    void (*function)(void *) = work->t_func;
    void *argument = work->t_arg;
    queue->executing = true;
    function(argument);
    queue->executing = false;
}

void RTWNet80211TaskSource::fire(OSObject *object, IOTimerEventSource *timer)
{
    auto *source = OSDynamicCast(RTWNet80211TaskSource, object);
    if (source == nullptr || source->queue == nullptr)
        return;
    /* Keep both event objects alive if a callback tears its queue down. */
    source->retain();
    timer->retain();
    struct taskq *queue = source->queue;
    run_one(queue);
    if (!queue->running)
        release_queue(queue);
    else if (!TAILQ_EMPTY(&queue->pending))
        timer->setTimeoutMS(0);
    timer->release();
    source->release();
}

static IOReturn initialize_queue(OSObject *, void *arg0, void *, void *, void *)
{
    auto *queue = (struct taskq *)arg0;
    if (queue->running)
        return kIOReturnSuccess;
    if (queue->executing)
        return kIOReturnBusy;
    TAILQ_INIT(&queue->pending);
    queue->executing = false;
    queue->owner = new RTWNet80211TaskSource;
    if (queue->owner == nullptr)
        return kIOReturnNoMemory;
    queue->owner->queue = queue;
    queue->timer = IOTimerEventSource::timerEventSource(queue->owner,
                                                    &RTWNet80211TaskSource::fire);
    if (queue->timer == nullptr) {
        queue->owner->release();
        queue->owner = nullptr;
        return kIOReturnNoMemory;
    }
    IOReturn result = _fWorkloop->addEventSource(queue->timer);
    if (result != kIOReturnSuccess) {
        queue->timer->release();
        queue->timer = nullptr;
        queue->owner->release();
        queue->owner = nullptr;
        return result;
    }
    queue->timer->enable();
    queue->running = true;
    return kIOReturnSuccess;
}

void taskq_init(void)
{
    if (_fWorkloop && _fCommandGate)
        _fCommandGate->runAction(initialize_queue, systq);
}

bool taskq_is_initialized(const struct taskq *queue)
{
    return queue && queue->running;
}

struct taskq *taskq_create(const char *, unsigned int, int, unsigned int)
{
    if (!_fWorkloop || !_fCommandGate)
        return nullptr;
    auto *queue = (struct taskq *)IOMalloc(sizeof(struct taskq));
    if (!queue)
        return nullptr;
    bzero(queue, sizeof(*queue));
    queue->heap_allocated = true;
    if (_fCommandGate->runAction(initialize_queue, queue) != kIOReturnSuccess) {
        IOFree(queue, sizeof(*queue));
        return nullptr;
    }
    return queue;
}

static IOReturn destroy_queue(OSObject *, void *arg0, void *, void *, void *)
{
    auto *queue = (struct taskq *)arg0;
    if (!queue->running)
        return kIOReturnSuccess;
    queue->running = false;
    clear_pending(queue);
    queue->timer->cancelTimeout();
    queue->timer->disable();
    /* A callback can destroy itself without waiting on its own work loop. */
    if (!queue->executing)
        release_queue(queue);
    return kIOReturnSuccess;
}

void taskq_destroy(struct taskq *queue)
{
    if (queue && _fCommandGate)
        _fCommandGate->runAction(destroy_queue, queue);
}

void task_set(struct task *work, void (*function)(void *), void *argument,
              const char *name)
{
    work->t_func = function;
    work->t_arg = argument;
    work->t_flags = 0;
    strlcpy(work->name, name ? name : "", sizeof(work->name));
}

static IOReturn add_work(OSObject *, void *arg0, void *arg1, void *, void *)
{
    auto *queue = (struct taskq *)arg0;
    auto *work = (struct task *)arg1;
    if (!queue->running || (work->t_flags & TASK_ONQUEUE))
        return kIOReturnError;
    work->t_flags |= TASK_ONQUEUE;
    TAILQ_INSERT_TAIL(&queue->pending, work, t_entry);
    queue->timer->setTimeoutMS(0);
    return kIOReturnSuccess;
}

int task_add(struct taskq *queue, struct task *work)
{
    if (!queue || !work || !work->t_func || !_fCommandGate)
        return 0;
    return _fCommandGate->runAction(add_work, queue, work) == kIOReturnSuccess;
}

static IOReturn delete_work(OSObject *, void *arg0, void *arg1, void *, void *)
{
    auto *queue = (struct taskq *)arg0;
    auto *work = (struct task *)arg1;
    if (!queue->running || !(work->t_flags & TASK_ONQUEUE))
        return kIOReturnError;
    TAILQ_REMOVE(&queue->pending, work, t_entry);
    work->t_flags &= ~TASK_ONQUEUE;
    if (TAILQ_EMPTY(&queue->pending))
        queue->timer->cancelTimeout();
    return kIOReturnSuccess;
}

int task_del(struct taskq *queue, struct task *work)
{
    if (!queue || !work || !_fCommandGate)
        return 0;
    return _fCommandGate->runAction(delete_work, queue, work) == kIOReturnSuccess;
}

static IOReturn drain_queue(OSObject *, void *arg0, void *, void *, void *)
{
    auto *queue = (struct taskq *)arg0;
    if (queue->executing)
        return kIOReturnBusy;
    while (queue->running && !TAILQ_EMPTY(&queue->pending))
        run_one(queue);
    if (!queue->running && queue->owner)
        release_queue(queue);
    return kIOReturnSuccess;
}

void taskq_barrier(struct taskq *queue)
{
    if (queue && _fCommandGate)
        _fCommandGate->runAction(drain_queue, queue);
}

void taskq_del_barrier(struct taskq *queue, struct task *work)
{
    task_del(queue, work);
}
