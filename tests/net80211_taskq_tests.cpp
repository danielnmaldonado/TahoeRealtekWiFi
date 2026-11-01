#include "net80211_taskq_host/platform.hpp"
#include <sys/_task.h>
#include <stdio.h>

IOWorkLoop *_fWorkloop;
IOCommandGate *_fCommandGate;
static unsigned checks;
static int calls;
static void require(bool condition, const char *description) {
    ++checks;
    if (!condition) { fprintf(stderr, "FAIL: %s\n", description); exit(1); }
}
static void increment(void *) {
    require(_fWorkloop->in_gate, "task executes on workloop gate");
    ++calls;
}
static void destroy_self(void *queue) {
    increment(nullptr);
    taskq_destroy((taskq *)queue);
}
int main() {
    taskq_init();
    require(!taskq_is_initialized(systq), "init without globals stays unavailable");
    auto *workloop = new IOWorkLoop;
    auto *gate = new IOCommandGate(workloop);
    _fWorkloop = workloop;
    _fCommandGate = gate;
    taskq_init();
    taskq_init();
    require(taskq_is_initialized(systq), "queue initialized");
    require(workloop->events.size() == 1, "repeated init keeps one timer");

    task first = {}, second = {};
    char *short_name = (char *)malloc(2);
    short_name[0] = 'x'; short_name[1] = 0;
    task_set(&first, increment, nullptr, short_name);
    free(short_name);
    require(strcmp(first.name, "x") == 0, "short task name copied safely");
    task_set(&second, increment, nullptr, "second");
    require(task_add(systq, &first) == 1, "first enqueue");
    require(task_add(systq, &first) == 0, "duplicate enqueue rejected");
    require(calls == 0, "enqueue remains deferred");
    require(task_pending(&first), "pending bit set");
    require(task_del(systq, &first) == 1, "queued task cancelled");
    require(!task_pending(&first), "cancel clears pending bit");
    require(!workloop->dispatchOne(), "cancelled queue has no callback");

    require(task_add(systq, &first) == 1, "enqueue first again");
    require(task_add(systq, &second) == 1, "enqueue second");
    require(workloop->dispatchOne(), "first dispatch");
    require(calls == 1 && !task_pending(&first) && task_pending(&second),
            "only first task runs per event");
    require(workloop->dispatchOne(), "second dispatch");
    require(calls == 2 && !task_pending(&second), "second task completes");
    require(!workloop->dispatchOne(), "idle queue does not spin");

    require(task_add(systq, &first) == 1, "enqueue before destroy");
    taskq_destroy(systq);
    require(!taskq_is_initialized(systq), "destroy marks queue unavailable");
    require(!task_pending(&first), "destroy clears queued task bit");
    require(workloop->events.empty(), "destroy removes timer source");
    require(!workloop->dispatchOne() && calls == 2, "no callback after destroy");
    require(task_add(systq, &first) == 0, "destroyed queue rejects enqueue");
    taskq_destroy(systq);
    taskq_init();
    require(taskq_is_initialized(systq), "system queue can initialize after detach");

    taskq *temporary = taskq_create("temporary", 1, 0, 0);
    require(temporary != nullptr, "create extra queue");
    task self = {}, never = {};
    task_set(&self, destroy_self, temporary, "destroy self");
    task_set(&never, increment, nullptr, "must be cancelled");
    require(task_add(temporary, &self) == 1 && task_add(temporary, &never) == 1,
            "enqueue self-destroy and following task");
    require(workloop->dispatchOne(), "self-destroy dispatch");
    require(calls == 3 && !task_pending(&never), "self-destroy cancels following task");
    require(workloop->events.size() == 1, "self-destroy releases extra event source");
    require(!workloop->dispatchOne(), "self-destroy has no later callback");

    require(task_add(systq, &first) == 1, "enqueue before barrier");
    taskq_barrier(systq);
    require(calls == 4 && !task_pending(&first), "barrier drains work on gate");
    taskq_destroy(systq);
    _fCommandGate = nullptr;
    _fWorkloop = nullptr;
    gate->release();
    workloop->release();
    printf("PASS imported taskq workloop lifecycle model (%u assertions)\n", checks);
}
