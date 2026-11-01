// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace rtwui {
// Monotonic, bounded backoff. A rollback (test clock, reboot) resets the wait.
struct RetryGate {
    unsigned failures=0;
    double nextAt=0,lastAt=-1;
    bool due(double now) const {
        return std::isfinite(now)&&now>=0&&
            (lastAt<0||now<lastAt||now>=nextAt);
    }
    void fail(double now) {
        if(!std::isfinite(now)||now<0)return;
        failures=std::min(failures+1,6U);
        lastAt=now;
        nextAt=now+static_cast<double>(std::min(2U<<failures,60U));
    }
    void reset() {failures=0;nextAt=0;lastAt=-1;}
};
struct AutoJoinPolicy {
    RetryGate retry;
    bool paused=false;
    bool mayAttempt(double now,bool available,bool fresh,bool connected,
                    bool joining,bool credentialBusy,bool suspended) {
        if(paused||suspended||!available||!fresh||connected||joining||
           credentialBusy||!std::isfinite(now)||now<0)return false;
        // A missing router or temporarily locked Keychain must not disable
        // reconnection forever. Three failed joins are followed by five quiet
        // minutes, then another bounded round without any password dialog.
        if(retry.failures>=3) {
            if(now>=retry.lastAt&&now-retry.lastAt<300.0)return false;
            retry.reset();
        }
        return retry.due(now);
    }
    void reset() {retry.reset();}
};
// Keychain runs off the main thread. An automatic JOIN must not be started
// from a delayed password lookup after the user disabled reconnection or
// explicitly paused it. Manual credential requests remain independent.
inline bool credentialJoinStillAllowed(bool automatic, bool enabled, bool paused) {
    return !automatic || (enabled && !paused);
}
inline bool recoverSoleAdapter(bool wasPresent,unsigned previousCount,
                               unsigned currentCount) {
    // Do not switch to a different remaining adapter when one of two is removed.
    // Recover after a zero-device gap or a sole device's registry ID changes.
    return !wasPresent&&currentCount==1&&previousCount<=1;
}
}
