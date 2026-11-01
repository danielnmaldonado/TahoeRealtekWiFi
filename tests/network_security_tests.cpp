// SPDX-License-Identifier: GPL-2.0-only
#include "rtw/NetworkSecurity.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    unsigned long long checks = 0;
    const auto classify = [&checks](const rtw_network_security &x) {
        ++checks;
        return rtw_network_security_classify(&x);
    };
    const rtw_network_security open{0,0,0,0,0,0};
    assert(classify(open) == RTW_NETWORK_OPEN);
    assert(rtw_network_security_classify(nullptr) == RTW_NETWORK_UNSUPPORTED);

    rtw_network_security valid{1,1,1,1,1,0};
    assert(classify(valid) == RTW_NETWORK_WPA2_PSK_CCMP);
    for (unsigned field = 0; field != 5; ++field) {
        auto candidate=valid;
        auto *members = reinterpret_cast<uint8_t *>(&candidate);
        members[field] = 0;
        assert(classify(candidate) == RTW_NETWORK_UNSUPPORTED);
    }
    valid.pmf_required = 1;
    assert(classify(valid) == RTW_NETWORK_UNSUPPORTED);
    valid.pmf_required = 0;

    // Exhaustive boolean combinations; the rule must neither misclassify a
    // privacy-only WEP network nor authorize an RSN configuration without
    // group CCMP or with mandatory protected management frames.
    for (unsigned mask = 0; mask < 64; ++mask) {
        const rtw_network_security c {
            static_cast<uint8_t>((mask >> 0) & 1),
            static_cast<uint8_t>((mask >> 1) & 1),
            static_cast<uint8_t>((mask >> 2) & 1),
            static_cast<uint8_t>((mask >> 3) & 1),
            static_cast<uint8_t>((mask >> 4) & 1),
            static_cast<uint8_t>((mask >> 5) & 1)
        };
        const bool isOpen = mask == 0;
        const bool wpa2 = (mask & 31U) == 31U && !(mask & 32U);
        const auto expected = isOpen ? RTW_NETWORK_OPEN :
            wpa2 ? RTW_NETWORK_WPA2_PSK_CCMP : RTW_NETWORK_UNSUPPORTED;
        assert(classify(c) == expected);
    }
    std::cout << "Network security policy passed " << checks
              << " classified configurations\n";
}
