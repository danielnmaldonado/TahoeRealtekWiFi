// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <atomic>
#include <cstdint>

namespace rtwui {
// Cancellation never waits for the worker. Every queued operation checks its
// captured epoch before I/O and again before applying a main-thread result.
class WorkEpoch {
public:
    uint64_t snapshot() const noexcept { return epoch_.load(std::memory_order_acquire); }
    bool matches(uint64_t token) const noexcept { return token == snapshot(); }
    void invalidate() noexcept { epoch_.fetch_add(1, std::memory_order_acq_rel); }
private:
    std::atomic<uint64_t> epoch_{0};
};
}
