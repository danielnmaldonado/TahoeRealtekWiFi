// SPDX-License-Identifier: GPL-2.0-only
#include "PollPolicy.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>

int main() {
    using namespace rtwui;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    assert(refreshDue(0,-1,8));
    assert(!refreshDue(7.99,0,8));
    assert(refreshDue(8,0,8));
    assert(refreshDue(10,20,8));
    assert(!refreshDue(100,99,6));
    assert(refreshDue(106,99,6));
    assert(!refreshDue(nan,0,8));
    assert(!refreshDue(inf,0,8));
    assert(!refreshDue(-1,0,8));
    assert(!refreshDue(8,0,0));
    assert(!refreshDue(8,0,-1));
    assert(!refreshDue(8,0,nan));
    assert(!refreshDue(8,0,inf));
    assert(refreshDue(8,nan,8));
    assert(refreshDue(8,inf,8));
    assert(!joinTimedOut(100,-1));
    assert(!joinTimedOut(145,100));
    assert(joinTimedOut(145.01,100));
    assert(!joinTimedOut(90,100));
    assert(!joinTimedOut(inf,100));
    assert(!joinTimedOut(100,nan));
    assert(!joinTimedOut(-1,-2));
    for (unsigned mask=0; mask<64; ++mask) {
        const bool suspended=mask&1, menu=mask&2, joining=mask&4;
        const bool connected=mask&8, present=mask&16, service=mask&32;
        const double interval=pollInterval(suspended,menu,joining,connected,present,service);
        const double expected=suspended?0:(menu||joining)?2:!service?15:!present?8:connected?4:2;
        assert(interval==expected);
        assert(pollTolerance(interval)>=0);
        assert(pollTolerance(interval)<=0.5);
        assert(pollTolerance(interval)<=interval);
    }
    assert(pollTolerance(nan)==0);
    assert(pollTolerance(inf)==0);
    assert(pollTolerance(-1)==0);
    // Static scheduling budget, not a measurement of real CPU or battery use.
    assert(60/pollInterval(false,false,false,true,true,true)==15);
    assert(60/pollInterval(false,true,false,true,true,true)==30);
    std::puts("Polling: 64 state combinations, invalid clocks, bounded tolerance and timeouts passed");
}
