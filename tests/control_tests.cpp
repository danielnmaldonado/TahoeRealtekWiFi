/* SPDX-License-Identifier: GPL-2.0-only. No kernel or USB access. */
#include "RTWControl.h"
#include "RTWBandCapabilities.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

static unsigned checks = 0;
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); std::exit(1); } } while (0)

// Catches accepting unrecognized operations, short/long messages, or a stale ABI.
static void fixed_requests_reject_noncanonical_messages() {
    RTWControlRequest request{};
    request.version = 1;
    for (const uint32_t selector : {0U, 1U, 3U, 4U, 5U}) {
        CHECK(rtw_control_validate_request(selector, &request, 16) == RTW_CONTROL_VALID);
        CHECK(rtw_control_validate_request(selector, &request, 15) == RTW_CONTROL_BAD_SIZE);
        CHECK(rtw_control_validate_request(selector, &request, 17) == RTW_CONTROL_BAD_SIZE);
        CHECK(rtw_control_validate_request(selector, nullptr, 16) == RTW_CONTROL_BAD_SIZE);
        request.version = 2;
        CHECK(rtw_control_validate_request(selector, &request, 16) == RTW_CONTROL_BAD_VERSION);
        request.version = 1;
        for (unsigned i = 0; i < 3; ++i) {
            request.reserved[i] = 1;
            CHECK(rtw_control_validate_request(selector, &request, 16) == RTW_CONTROL_BAD_RESERVED);
            request.reserved[i] = 0;
        }
    }
    CHECK(rtw_control_validate_request(6, &request, 16) == RTW_CONTROL_BAD_SELECTOR);
    CHECK(rtw_control_validate_request(0xffffffffU, &request, 16) == RTW_CONTROL_BAD_SELECTOR);
    CHECK(rtw_control_input_size(6) == 0);
    CHECK(rtw_control_output_size(6) == 0);
    CHECK(rtw_control_output_size(5) == 48);
    CHECK(rtw_control_output_size(0) == 120);
    CHECK(rtw_control_output_size(1) == 16);
    CHECK(rtw_control_output_size(4) == 3096);
}

// Catches accepting hidden SSID bytes, reserved fields, empty/oversize SSIDs,
// and rejects neither valid binary SSIDs nor the complete 32-byte boundary.
static void join_accepts_only_bounded_canonical_ssids() {
    RTWControlJoin join{};
    join.version = 1;
    join.ssidLength = 3;
    join.ssid[0] = 'a'; join.ssid[1] = 0; join.ssid[2] = 0xff;
    std::memset(join.pmk, 0xa5, sizeof(join.pmk));
    CHECK(rtw_control_input_size(2) == 80);
    CHECK(rtw_control_validate_request(2, &join, 80) == RTW_CONTROL_VALID);
    CHECK(rtw_control_validate_request(2, &join, 79) == RTW_CONTROL_BAD_SIZE);
    CHECK(rtw_control_validate_request(2, &join, 81) == RTW_CONTROL_BAD_SIZE);
    join.ssid[3] = 1;
    CHECK(rtw_control_validate_request(2, &join, 80) == RTW_CONTROL_BAD_RESERVED);
    join.ssid[3] = 0;
    join.ssidLength = 0;
    CHECK(rtw_control_validate_request(2, &join, 80) == RTW_CONTROL_BAD_SSID);
    join.ssidLength = 33;
    CHECK(rtw_control_validate_request(2, &join, 80) == RTW_CONTROL_BAD_SSID);
    join.ssidLength = 32;
    std::memset(join.ssid, 0xff, sizeof(join.ssid));
    CHECK(rtw_control_validate_request(2, &join, 80) == RTW_CONTROL_VALID);
    for (unsigned i = 0; i < 2; ++i) {
        join.reserved[i] = 1;
        CHECK(rtw_control_validate_request(2, &join, 80) == RTW_CONTROL_BAD_RESERVED);
        join.reserved[i] = 0;
    }
    join.version = 0;
    CHECK(rtw_control_validate_request(2, &join, 80) == RTW_CONTROL_BAD_VERSION);
}

// Catches an unsafe aligned struct dereference at the external-message boundary.
static void request_validation_accepts_unaligned_message_storage() {
    RTWControlJoin join{};
    join.version = 1; join.ssidLength = 1; join.ssid[0] = 'x';
    uint8_t storage[81]{};
    std::memcpy(storage + 1, &join, 80);
    CHECK(rtw_control_validate_request(2, storage + 1, 80) == RTW_CONTROL_VALID);
}

int main() {
    // Each DFS capability grants only its exact channel group.
    const uint32_t dfsLow=1U<<11,dfsMid=1U<<12;
    for(unsigned ch=0;ch<256;++ch) {
        const bool low=ch>=52&&ch<=64&&ch%4==0;
        const bool mid=ch>=100&&ch<=140&&ch%4==0;
        CHECK(bool(rtw_control_channel_supported(ch,(1U<<10)|dfsLow))==low);
        CHECK(bool(rtw_control_channel_supported(ch,(1U<<10)|dfsMid))==mid);
    }
    const auto allDFS=[](uint8_t ch){return (ch>=52&&ch<=64&&ch%4==0)||(ch>=100&&ch<=140&&ch%4==0);};
    CHECK(rtw_station_5ghz_flags(allDFS)==(dfsLow|dfsMid));
    CHECK(rtw_station_5ghz_flags([&](uint8_t ch){return ch!=120&&allDFS(ch);})==dfsLow);
    CHECK(!rtw_control_channel_supported(144,(1U<<10)|dfsLow|dfsMid));
    // ABI v1 legacy flags retain 2.4 GHz; explicit 5-GHz-only boards deny it.
    const uint32_t explicitBands=1U<<10, twoGHz=1U<<9;
    CHECK(rtw_control_channel_supported(6,0));
    CHECK(!rtw_control_channel_supported(6,explicitBands|RTW_CONTROL_BAND_5GHZ));
    CHECK(rtw_control_channel_supported(36,explicitBands|RTW_CONTROL_BAND_5GHZ));
    CHECK(rtw_control_channel_supported(6,explicitBands|twoGHz));
    CHECK(!rtw_control_channel_supported(36,explicitBands|twoGHz));
    CHECK(!rtw_control_channel_supported(6,explicitBands));
    fixed_requests_reject_noncanonical_messages();
    join_accepts_only_bounded_canonical_ssids();
    request_validation_accepts_unaligned_message_storage();
    std::printf("%u control protocol checks passed\n", checks);
    return 0;
}
