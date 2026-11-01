/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/FamilyBackend.h"
#include "rtw/FCS.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <random>
#include <vector>

static std::vector<uint8_t> make_configuration(const rtw_usb_wifi_profile& p) {
    std::vector<uint8_t> b = {
        9, 2, 32, 0, 1, p.configuration, 0, 0x80, 50,
        9, 4, p.interface_number, 0, 2, 0xff, 0xff, 0xff, 0,
        7, 5, 0x81, 2, 0x00, 2, 0,
        7, 5, 0x02, 2, 0x00, 2, 0
    };
    if (p.interface_match_flags & RTW_USB_MATCH_CLASS)
        b[14] = p.interface_class;
    if (p.interface_match_flags & RTW_USB_MATCH_SUBCLASS)
        b[15] = p.interface_subclass;
    if (p.interface_match_flags & RTW_USB_MATCH_PROTOCOL)
        b[16] = p.interface_protocol;
    return b;
}

int main() {
    unsigned known = 0, ready = 0, diagnostics = 0, unverified = 0;
    unsigned encoded = 0;
    const size_t count = rtw_usb_wifi_profile_count();
    assert(rtw_backend_family_count() == 14);
    assert(count == 270);
    assert(rtw_backend_for_family(RTW_WIFI_UNKNOWN) == nullptr);
    assert(rtw_backend_for_family(static_cast<rtw_wifi_family>(255)) == nullptr);
    assert(rtw_backend_family_at(14) == nullptr);

    for (size_t i = 0; i < rtw_backend_family_count(); ++i) {
        const auto* backend = rtw_backend_family_at(i);
        assert(backend);
        assert(backend == rtw_backend_for_family(backend->family));
        assert((backend->capabilities & RTW_BACKEND_CAP_USB_PREFLIGHT) != 0);
        assert(backend->reference_driver && backend->remaining_requirement);
        const bool wire = (backend->capabilities & RTW_BACKEND_CAP_RX_WIRE) != 0;
        assert(wire == (backend->rx_codec != RTW_BACKEND_RX_NONE));
        const bool header = (backend->capabilities &
            RTW_BACKEND_CAP_LEGACY_FIRMWARE_HEADER) != 0;
        const bool radio = (backend->capabilities &
            RTW_BACKEND_CAP_PASSIVE_CHANNEL_GEOMETRY) != 0;
        const bool expected_header =
            backend->family == RTW_WIFI_RTL8192CU ||
            backend->family == RTW_WIFI_RTL8188EU ||
            backend->family == RTW_WIFI_RTL8192EU ||
            backend->family == RTW_WIFI_RTL8192DU ||
            backend->family == RTW_WIFI_RTL8192FU ||
            backend->family == RTW_WIFI_RTL8723BU ||
            backend->family == RTW_WIFI_RTL8710BU;
        /* Passive geometry is available for every known radio family;
         * it does not imply RF writes or a functional station backend. */
        const bool expected_radio =
            rtw_wifi_radio_family_capabilities(backend->family).family_bands != 0;
        assert(header == expected_header && radio == expected_radio);
        assert(wire || (backend->capabilities & RTW_BACKEND_CAP_TX_WIRE) == 0);
        if (backend->capabilities & RTW_BACKEND_CAP_KEXT_STATION) {
            ++encoded;
            assert(wire && backend->rx_chains >= 1);
        }
        rtw_backend_rx_report rx{};
        assert(rtw_backend_rx_preflight(backend->family, nullptr, 0, &rx) ==
               (wire ? RTW_BACKEND_RX_VALID : RTW_BACKEND_RX_UNAVAILABLE));
        assert(rx.frames == 0 && rx.firmware_events == 0);
        const uint8_t junk[] = {0x11, 0x12};
        if (wire)
            assert(rtw_backend_rx_preflight(backend->family, junk,
                sizeof(junk), &rx) == RTW_BACKEND_RX_MALFORMED);
        else
            assert(rtw_backend_rx_preflight(backend->family, junk,
                sizeof(junk), &rx) == RTW_BACKEND_RX_UNAVAILABLE);
    }
    assert(encoded == 13);

    for (size_t i = 0; i < count; ++i) {
        const rtw_usb_wifi_profile *p = rtw_usb_wifi_profile_at(i);
        assert(p && rtw_backend_for_family(p->family));
        const std::vector<uint8_t> config = make_configuration(*p);
        rtw_backend_usb_plan plan{};
        auto status = rtw_backend_usb_preflight(
            p->vendor, p->product, p->match_bcd ? p->bcd_device : 0U,
            config.data(), config.size(), p->interface_number, &plan);
        if (!p->primary_verified) {
            assert(status == RTW_BACKEND_PREFLIGHT_UNVERIFIED);
            assert(plan.backend == nullptr && plan.profile == nullptr);
            ++unverified;
        } else {
            ++known;
            assert(plan.profile == p && plan.backend->family == p->family);
            assert(plan.endpoints.num_bulk_in == 1);
            assert(plan.endpoints.num_bulk_out == 1);
            assert(plan.endpoints.bulk_in[0] == 0x81);
            assert(plan.endpoints.bulk_out[0] == 0x02);
            if (rtw_usb_wifi_station_backend(p) == p->family) {
                assert(status == RTW_BACKEND_PREFLIGHT_STATION);
                assert(plan.station_backend_integrated && !plan.usb_preflight_only);
                ++ready;
            } else {
                assert(status == RTW_BACKEND_PREFLIGHT_DIAGNOSTICS);
                assert(!plan.station_backend_integrated && plan.usb_preflight_only);
                ++diagnostics;
            }
            auto bad = config;
            bad[2] = 0; bad[3] = 0;
            assert(rtw_backend_usb_preflight(p->vendor, p->product, 0,
                bad.data(), bad.size(), p->interface_number, &plan) ==
                RTW_BACKEND_PREFLIGHT_DESCRIPTOR_MISMATCH);
            assert(plan.profile == nullptr);
            bad = config;
            bad[21] = 3;
            assert(rtw_backend_usb_preflight(p->vendor, p->product,
                p->match_bcd ? p->bcd_device : 0, bad.data(), bad.size(),
                p->interface_number, &plan) == RTW_BACKEND_PREFLIGHT_BAD_ENDPOINTS);
            assert(plan.profile == nullptr);
            if (p->match_bcd) {
                assert(rtw_backend_usb_preflight(p->vendor, p->product,
                    p->bcd_device ^ 1u, config.data(), config.size(),
                    p->interface_number, &plan) ==
                    RTW_BACKEND_PREFLIGHT_DESCRIPTOR_MISMATCH);
            }
        }
    }
    assert(known == 263 && ready == 262 &&
           diagnostics == 1 && unverified == 7);

    const auto* p = rtw_usb_wifi_lookup(0x0bda, 0x8812, 0, 1, 0);
    assert(p && p->family == RTW_WIFI_RTL8812AU);
    auto b = make_configuration(*p);
    rtw_backend_usb_plan plan{};
    assert(rtw_backend_usb_preflight(0x1234, 0xffff, 0, b.data(), b.size(),
        0, &plan) == RTW_BACKEND_PREFLIGHT_NOT_CATALOGUED);
    assert(rtw_backend_usb_preflight(0x0bda, 0x8812, 0, b.data(), b.size(),
        0, nullptr) == RTW_BACKEND_PREFLIGHT_INVALID_ARGUMENT);
    assert(rtw_backend_usb_preflight(0x0bda, 0x8812, 0, nullptr, 0,
        0, &plan) == RTW_BACKEND_PREFLIGHT_INVALID_ARGUMENT);
    b[14] = 0x08;
    if (p->interface_match_flags & RTW_USB_MATCH_CLASS)
        assert(rtw_backend_usb_preflight(p->vendor, p->product, 0,
            b.data(), b.size(), 0, &plan) ==
            RTW_BACKEND_PREFLIGHT_DESCRIPTOR_MISMATCH);

    // The two preflight entry points must not overwrite caller-owned USB
    // descriptor/aggregate bytes when an output overlaps the input.
    // This is particularly important when inspecting a reused DMA buffer.
    {
        const auto *profile = rtw_usb_wifi_lookup(0x0bda, 0x8812, 0, 1, 0);
        assert(profile && profile->primary_verified);
        const auto usb_config = make_configuration(*profile);
        alignas(rtw_backend_usb_plan) uint8_t shared[256]{};
        std::memcpy(shared, usb_config.data(), usb_config.size());
        const auto status = rtw_backend_usb_preflight(profile->vendor,
            profile->product, profile->match_bcd ? profile->bcd_device : 0,
            shared, usb_config.size(), profile->interface_number,
            reinterpret_cast<rtw_backend_usb_plan*>(shared));
        assert(status == RTW_BACKEND_PREFLIGHT_INVALID_ARGUMENT);
        assert(std::memcmp(shared, usb_config.data(), usb_config.size()) == 0);
        // Partial alias must also be rejected before any output is cleared.
        std::memcpy(shared, usb_config.data(), usb_config.size());
        const auto partial = rtw_backend_usb_preflight(profile->vendor,
            profile->product, profile->match_bcd ? profile->bcd_device : 0,
            shared, usb_config.size(), profile->interface_number,
            reinterpret_cast<rtw_backend_usb_plan*>(shared + 8));
        assert(partial == RTW_BACKEND_PREFLIGHT_INVALID_ARGUMENT);
        assert(std::memcmp(shared, usb_config.data(), usb_config.size()) == 0);
    }
    {
        alignas(rtw_backend_rx_report) uint8_t shared[128]{};
        const uint8_t initial[] = {
            0x12,0x34,0x56,0x78,0x90,0xab,0xcd,0xef,
            0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88
        };
        std::memcpy(shared, initial, sizeof(initial));
        const auto status = rtw_backend_rx_preflight(RTW_WIFI_RTL8812AU,
            shared, sizeof(initial), reinterpret_cast<rtw_backend_rx_report*>(shared));
        assert(status == RTW_BACKEND_RX_INVALID_INPUT);
        assert(std::memcmp(shared, initial, sizeof(initial)) == 0);
        std::memcpy(shared, initial, sizeof(initial));
        const auto partial = rtw_backend_rx_preflight(RTW_WIFI_RTL8812AU,
            shared, sizeof(initial), reinterpret_cast<rtw_backend_rx_report*>(shared + 8));
        assert(partial == RTW_BACKEND_RX_INVALID_INPUT);
        assert(std::memcmp(shared, initial, sizeof(initial)) == 0);
    }

    {
        // DU carries one MPDU with FCS, not the CU aggregate stride.
        std::vector<uint8_t> wire(24+28);wire[0]=28;wire[12]=4;
        wire[24]=0x80;const auto fcs=rtw_fcs32(wire.data()+24,24);
        for(unsigned i=0;i<4;i++)wire[48+i]=uint8_t(fcs>>(8*i));
        rtw_backend_rx_report du{};
        assert(rtw_backend_rx_preflight(RTW_WIFI_RTL8192DU,wire.data(),wire.size(),&du)==RTW_BACKEND_RX_VALID);
        assert(du.frames==1&&du.management_or_data_frames==1);
        assert(rtw_backend_rx_preflight(RTW_WIFI_RTL8192DU,wire.data(),wire.size()-1,&du)==RTW_BACKEND_RX_MALFORMED&&!du.frames);
        wire.back()^=1;
        assert(rtw_backend_rx_preflight(RTW_WIFI_RTL8192DU,wire.data(),wire.size(),&du)==RTW_BACKEND_RX_BAD_RADIO_FRAME&&!du.frames);
    }
    {
        //8710BU uses its own24-byte descriptor decoder, no DU FCS handling.
        std::vector<uint8_t> wire(48);wire[0]=24;wire[12]=4;wire[24]=0x80;
        rtw_backend_rx_report report{};
        assert(rtw_backend_rx_preflight(RTW_WIFI_RTL8710BU,wire.data(),wire.size(),&report)==RTW_BACKEND_RX_VALID&&report.frames==1);
        wire[1]|=0x40; // Hardware CRC error must invalidate the whole candidate.
        assert(rtw_backend_rx_preflight(RTW_WIFI_RTL8710BU,wire.data(),wire.size(),&report)==RTW_BACKEND_RX_BAD_RADIO_FRAME&&!report.frames);
    }
    const uint8_t short_bytes[] = {1};
    rtw_backend_rx_report report{};
    assert(rtw_backend_rx_preflight(RTW_WIFI_RTL8192CU, short_bytes,
        sizeof(short_bytes), &report) == RTW_BACKEND_RX_MALFORMED);
    assert(rtw_backend_rx_preflight(RTW_WIFI_RTL8812AU, short_bytes,
        sizeof(short_bytes), &report) == RTW_BACKEND_RX_MALFORMED);
    assert(rtw_backend_rx_preflight(RTW_WIFI_RTL8812AU, nullptr, 1,
        &report) == RTW_BACKEND_RX_INVALID_INPUT);
    assert(rtw_backend_rx_preflight(RTW_WIFI_RTL8812AU, short_bytes,
        65537, &report) == RTW_BACKEND_RX_INVALID_INPUT);
    assert(rtw_backend_rx_preflight(RTW_WIFI_RTL8812AU, short_bytes,
        1, nullptr) == RTW_BACKEND_RX_INVALID_INPUT);

    std::mt19937 engine(0x2015cafe);
    std::uniform_int_distribution<int> byte_value(0, 255);
    for (unsigned i = 0; i < 20000; ++i) {
        std::vector<uint8_t> random(32);
        for (auto &v : random) v = static_cast<uint8_t>(byte_value(engine));
        size_t length = static_cast<size_t>(byte_value(engine)) % 33U;
        auto result = rtw_backend_usb_preflight(0x0bda, 0x8812, 0,
            random.data(), length, 0, &plan);
        assert(result != RTW_BACKEND_PREFLIGHT_STATION);
        if (result != RTW_BACKEND_PREFLIGHT_DIAGNOSTICS)
            assert(plan.profile == nullptr && plan.backend == nullptr);
        const rtw_wifi_family family = static_cast<rtw_wifi_family>(1U + (i % 14U));
        auto rx_result = rtw_backend_rx_preflight(family, random.data(),
            length, &report);
        if (rtw_backend_for_family(family)->rx_codec == RTW_BACKEND_RX_NONE)
            assert(rx_result == RTW_BACKEND_RX_UNAVAILABLE);
        else
            assert(rx_result == RTW_BACKEND_RX_VALID ||
                   rx_result == RTW_BACKEND_RX_MALFORMED ||
                   rx_result == RTW_BACKEND_RX_BAD_RADIO_FRAME);
    }
    std::printf("14 family definitions, %zu catalog profiles, %u station-enabled, "
                "%u USB-only families' profiles, %u unverified IDs; "
                "20000 descriptor/RX fuzz iterations passed\n",
                count, ready, diagnostics, unverified);
    return 0;
}
