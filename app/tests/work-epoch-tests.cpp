// SPDX-License-Identifier: GPL-2.0-only
#include "WorkEpoch.hpp"
#include <atomic>
#include <cassert>
#include <cstdio>
#include <thread>
#include <vector>

int main() {
    rtwui::WorkEpoch work;
    assert(work.snapshot()==0);
    const auto beforeSleep=work.snapshot();
    work.invalidate();
    assert(!work.matches(beforeSleep));
    assert(work.matches(work.snapshot()));
    // Every invalidation linearizes on the same atomic epoch; concurrent readers
    // may observe either side, but must never revive a token already canceled.
    constexpr unsigned workers=4, repetitions=25000;
    std::vector<std::thread> threads;
    for (unsigned worker=0; worker<workers; ++worker) {
        threads.emplace_back([&] {
            for (unsigned i=0; i<repetitions; ++i) {
                const auto old=work.snapshot();
                work.invalidate();
                assert(!work.matches(old));
            }
        });
    }
    for (auto& thread:threads) thread.join();
    assert(work.snapshot()==1+workers*repetitions);
    assert(!work.matches(beforeSleep));
    std::puts("Work epoch: 100,000 concurrent invalidations; stale tokens rejected");
}
