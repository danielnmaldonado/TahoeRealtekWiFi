// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <stdint.h>

namespace rtw {
// Updating an unchanged OSNumber allocates and publishes another registry
// object. Preserve the original 64-bit wire representation, including after
// a property is externally replaced by an object with a different width.
inline bool diagnosticCounterNeedsWrite(bool numberPresent, unsigned widthBits,
                                        uint64_t previous, uint64_t current) noexcept {
    return !numberPresent || widthBits != 64 || previous != current;
}
} // namespace rtw
