// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <cmath>

namespace rtwui {
// Only monotonic timestamps are accepted. A non-finite clock must not create
// an always-due tight loop or hide a failed request indefinitely.
inline bool refreshDue(double now, double last, double periodSeconds) {
    if (!std::isfinite(now) || now < 0 || !std::isfinite(periodSeconds) ||
        periodSeconds <= 0) return false;
    return !std::isfinite(last) || last < 0 || now < last || now - last >= periodSeconds;
}
inline bool joinTimedOut(double now, double start) {
    return std::isfinite(now) && std::isfinite(start) &&
        start >= 0 && now >= start && now - start > 45.0;
}
inline double pollInterval(bool suspended, bool menuOpen, bool joining,
                           bool connected, bool adapterPresent, bool serviceAvailable) {
    if (suspended) return 0.0;
    if (menuOpen || joining) return 2.0;
    if (!serviceAvailable) return 15.0;
    if (!adapterPresent) return 8.0;
    return connected ? 4.0 : 2.0;
}
inline double pollTolerance(double interval) {
    return std::isfinite(interval) && interval > 0 ? std::fmin(0.5, interval * 0.15) : 0.0;
}
}
