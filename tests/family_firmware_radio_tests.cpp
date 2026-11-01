// SPDX-License-Identifier: GPL-2.0-only
#include "rtw/FamilyRadioPolicy.h"
#include "rtw/USBWiFiFirmware.h"
#include "rtw/FamilyBackend.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

static void set_signature(std::vector<uint8_t>& b, uint16_t signature) {
    b[0] = static_cast<uint8_t>(signature);
    b[1] = static_cast<uint8_t>(signature >> 8U);
}
int main() {
    struct Case { rtw_wifi_family family; uint16_t signature; };
    constexpr std::array<Case, 7> signatures{{
        {RTW_WIFI_RTL8192CU, 0x88c1},
        {RTW_WIFI_RTL8188EU, 0x88e1},
        {RTW_WIFI_RTL8192EU, 0x92e1},
        {RTW_WIFI_RTL8192DU, 0x92d2},
        {RTW_WIFI_RTL8192FU, 0x92f1},
        {RTW_WIFI_RTL8723BU, 0x5301},
        {RTW_WIFI_RTL8710BU, 0x10b1},
    }};
    for (const auto& c : signatures) {
        std::vector<uint8_t> image(32 + 192, 0U);
        set_signature(image, c.signature);
        image[4] = 9; image[12] = 192; image[16] = 7;
        rtw_8192eu_fw_info info{};
        assert(rtw_usb_wifi_inspect_firmware(c.family, image.data(),
               image.size(), &info));
        assert(info.signature == c.signature && info.major_version == 9 &&
               info.payload == image.data() + 32 && info.payload_length == 192 &&
               info.reported_ramcode_size == 192 && info.svn_index == 7);
        // Firmware from a different chip cannot pass.
        for (const auto& other : signatures) {
            if (other.family != c.family)
                assert(!rtw_usb_wifi_inspect_firmware(other.family,
                       image.data(), image.size(), &info));
        }
        // The caller must not receive a dangling payload on failure.
        image[12] = 193;
        assert(!rtw_usb_wifi_inspect_firmware(c.family, image.data(),
               image.size(), &info));
        assert(info.payload == nullptr && info.payload_length == 0);
        image[12] = 0; image[13] = 0;
        assert(!rtw_usb_wifi_inspect_firmware(c.family, image.data(),
               32, &info));
        assert(!rtw_usb_wifi_inspect_firmware(c.family, nullptr,
               image.size(), &info));
        assert(!rtw_usb_wifi_inspect_firmware(c.family, image.data(),
               image.size(), nullptr));
        image[0] = 0xff; image[1] = 0xff;
        assert(!rtw_usb_wifi_inspect_firmware(c.family, image.data(),
               image.size(), &info));
    }
    for (uint16_t sig = 0x92d0; sig <= 0x92d3; ++sig) {
        std::vector<uint8_t> b(33); set_signature(b, sig);
        rtw_8192eu_fw_info info{};
        assert(rtw_usb_wifi_inspect_firmware(RTW_WIFI_RTL8192DU, b.data(),
               b.size(), &info));
    }
    {
        std::vector<uint8_t> b(33); set_signature(b, 0x92d4);
        rtw_8192eu_fw_info info{};
        assert(!rtw_usb_wifi_inspect_firmware(RTW_WIFI_RTL8192DU, b.data(),
               b.size(), &info));
    }
    constexpr std::array<rtw_wifi_family, 6> radio{
        RTW_WIFI_RTL8192DU, RTW_WIFI_RTL8192FU, RTW_WIFI_RTL8723BU,
        RTW_WIFI_RTL8821CU, RTW_WIFI_RTL8822BU, RTW_WIFI_RTL8822CU
    };
    for (const auto family : radio) {
        rtw_r12au_channel_plan plan{};
        assert(rtw_family_radio_prepare(family, 6, 6, 20, &plan) ==
               RTW_FAMILY_RADIO_OK);
        assert(plan.passive_only && plan.primary_frequency_mhz == 2437 &&
               plan.is_5ghz == 0 && plan.rf_chains >= 1);
        assert(rtw_family_radio_prepare(family, 6, 6, 80, &plan) ==
               RTW_FAMILY_RADIO_UNSUPPORTED);
        assert(plan.primary_channel == 0); // fail-closed output
        assert(rtw_family_radio_prepare(family, 0, 0, 20, &plan) ==
               RTW_FAMILY_RADIO_UNSUPPORTED);
        assert(rtw_family_radio_prepare(family, 14, 14, 20, &plan) ==
               RTW_FAMILY_RADIO_UNSUPPORTED);
        assert(rtw_family_radio_prepare(family, 52, 52, 20, &plan) ==
               (family == RTW_WIFI_RTL8192FU || family == RTW_WIFI_RTL8723BU ?
                RTW_FAMILY_RADIO_UNSUPPORTED : RTW_FAMILY_RADIO_OK));
        if (family != RTW_WIFI_RTL8192FU && family != RTW_WIFI_RTL8723BU) {
            assert(plan.passive_only && plan.dfs_required);
            assert(rtw_family_radio_prepare(family, 36, 42, 80, &plan) ==
               (family == RTW_WIFI_RTL8192DU ?
                RTW_FAMILY_RADIO_UNSUPPORTED : RTW_FAMILY_RADIO_OK));
        }
    }
    rtw_r12au_channel_plan plan{};
    assert(rtw_family_radio_prepare(RTW_WIFI_RTL8814BU, 6, 6, 20, &plan) ==
           RTW_FAMILY_RADIO_UNSUPPORTED);
    assert(rtw_family_radio_prepare(RTW_WIFI_RTL8192FU, 6, 6, 20,
           nullptr) == RTW_FAMILY_RADIO_INVALID_ARGUMENT);
    // Fuzz firmware headers and radio requests over all catalog families.
    std::mt19937 gen(0xd018);
    std::uniform_int_distribution<int> value(0, 255);
    std::vector<uint8_t> blob(800);
    unsigned iterations = 0;
    for (int i = 0; i < 35000; ++i) {
        for (auto& b : blob) b = static_cast<uint8_t>(value(gen));
        const auto family = static_cast<rtw_wifi_family>(
            1U + static_cast<unsigned>(i) % 14U);
        const size_t len = static_cast<size_t>(value(gen)) % 801U;
        rtw_8192eu_fw_info info{};
        const int ok = rtw_usb_wifi_inspect_firmware(family, blob.data(), len, &info);
        assert(ok == 0 || ok == 1);
        if (!ok) assert(info.payload == nullptr && info.payload_length == 0);
        if (ok) assert(info.payload_length == len - 32U);
        const auto status = rtw_family_radio_prepare(
            family, static_cast<uint8_t>(value(gen)),
            static_cast<uint8_t>(value(gen)),
            static_cast<uint8_t>(value(gen)), &plan);
        assert(status == RTW_FAMILY_RADIO_OK ||
               status == RTW_FAMILY_RADIO_UNSUPPORTED);
        if (status == RTW_FAMILY_RADIO_OK) assert(plan.passive_only);
        ++iterations;
    }
    std::printf("7 firmware headers and 6 passive radio families: %u fuzz cases passed\n",
                iterations);
}
