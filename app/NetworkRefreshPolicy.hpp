// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "PollPolicy.hpp"
#include "ReconnectPolicy.hpp"

namespace rtwui {
struct NetworkRefreshPolicy {
    double lastFinishedAt=-1;
    RetryGate retry;
    bool due(double now) {
        if(!refreshDue(now,lastFinishedAt,30.0))return false;
        // Match reconnect's bounded rounds: three failures, then five quiet
        // minutes. Switching adapters must not restart a failed round.
        if(retry.failures>=3) {
            if(now>=retry.lastAt&&now-retry.lastAt<300.0)return false;
            retry.reset();
        }
        return retry.due(now);
    }
    void finished(double now) {lastFinishedAt=now;retry.reset();}
    void failed(double now) {retry.fail(now);}
};
}
