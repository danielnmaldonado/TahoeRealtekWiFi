// SPDX-License-Identifier: GPL-2.0-only
#include "rtw/FamilyRX24.h"
#include "rtw/FamilyBackend.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

static void put32(std::vector<uint8_t>& buffer, size_t offset, uint32_t word) {
    for (size_t i = 0; i != 4; ++i)
        buffer[offset + i] = static_cast<uint8_t>(word >> (8U * i));
}
static bool modern(rtw_wifi_family family) {
    return family == RTW_WIFI_RTL8821CU || family == RTW_WIFI_RTL8822BU ||
           family == RTW_WIFI_RTL8822CU;
}
static void add(std::vector<uint8_t>& b, rtw_wifi_family family,
                unsigned payload = 28, unsigned phy = 0, unsigned shift = 0,
                unsigned rate = 4, unsigned bw = 0, bool event = false,
                bool crc = false, bool icv = false) {
    const size_t first = b.size();
    const size_t used = 24U + phy + shift + payload;
    const size_t padded = (used + 7U) & ~(size_t)7U;
    b.resize(first + padded);
    put32(b, first, payload | ((phy / 8U) << 16U) | (shift << 24U) |
              (crc ? 1U << 14U : 0U) | (icv ? 1U << 15U : 0U));
    put32(b, first + 8U, event && modern(family) ? 1U << 28U : 0U);
    put32(b, first + 12U, rate | ((event && !modern(family)) ? 1U << 14U : 0U));
    put32(b, first + 16U, bw << 4U);
    put32(b, first + 20U, 0x12345678);
    for (unsigned i = 0; i < payload; ++i)
        b[first + 24U + phy + shift + i] = static_cast<uint8_t>(1U + (i & 31U));
}
static void primaryPhyLayoutsKeepPaddingOutOfReports() {
    // Pinned rtw88/rx.c places SHIFT before PHY; rtl8xxxu/core.c captures
    // PHY immediately after the descriptor, then skips PHY plus SHIFT.
    for (auto family : {RTW_WIFI_RTL8192FU, RTW_WIFI_RTL8723BU,
                        RTW_WIFI_RTL8821CU, RTW_WIFI_RTL8822BU, RTW_WIFI_RTL8822CU}) {
        for (unsigned shift = 0; shift <= 3; ++shift) {
            std::vector<uint8_t> bytes;
            add(bytes, family, 28, 32, shift, 12);
            const size_t phyOffset = modern(family) ? 24U + shift : 24U;
            for (unsigned i = 0; i < 32; ++i) bytes[phyOffset + i] = 0xa5;
            const size_t paddingOffset = modern(family) ? 24U : 24U + 32U;
            for (unsigned i = 0; i < shift; ++i) bytes[paddingOffset + i] = 0xee;
            rtw_rx24_iterator iterator{};
            rtw_rx24_iterator_init(&iterator, bytes.data(), bytes.size());
            rtw_rx24_frame frame{};
            assert(rtw_rx24_next(&iterator, family, &frame) == RTW_RX24_FRAME);
            assert(frame.driver_info == bytes.data() + phyOffset);
            assert(frame.driver_info_length == 32 && frame.driver_info[0] == 0xa5 &&
                   frame.driver_info[31] == 0xa5);
            assert(frame.payload == bytes.data() + 24U + shift + 32U &&
                   frame.payload[0] == 1 && frame.payload[27] == 28);
        }
    }
    std::puts("RX24 primary PHY layouts: 20 family/SHIFT cases passed");
}
int main() {
    primaryPhyLayoutsKeepPaddingOutOfReports();
    constexpr std::array<rtw_wifi_family, 5> families = {
        RTW_WIFI_RTL8192FU, RTW_WIFI_RTL8723BU,
        RTW_WIFI_RTL8821CU, RTW_WIFI_RTL8822BU, RTW_WIFI_RTL8822CU
    };
    for (const auto family : families) {
        assert(rtw_backend_for_family(family)->rx_codec ==
               (modern(family) ? RTW_BACKEND_RX_RTW88 : RTW_BACKEND_RX_RTL8XXXU));
        assert(rtw_backend_for_family(family)->capabilities &
                 RTW_BACKEND_CAP_KEXT_STATION);
        assert(bool(rtw_backend_for_family(family)->capabilities &
                 RTW_BACKEND_CAP_TX_WIRE));
        std::vector<uint8_t> bytes;
        add(bytes, family, 28, 32, 1, 12, 0);
        add(bytes, family, 12, 0, 0, 4, 0, true);
        rtw_rx24_iterator it{};
        rtw_rx24_iterator_init(&it, bytes.data(), bytes.size());
        rtw_rx24_frame packet{};
        assert(rtw_rx24_next(&it, family, &packet) == RTW_RX24_FRAME);
        assert(packet.payload_length == 28 && packet.driver_info_length == 32);
        assert(packet.payload == bytes.data() + 24 + 32 + 1);
        assert(packet.timestamp == 0x12345678 && packet.rate == 12);
        assert(packet.is_firmware_event == 0 && packet.rf_chains >= 1);
        assert(rtw_rx24_next(&it, family, &packet) == RTW_RX24_FRAME);
        assert(packet.is_firmware_event == 1 && packet.payload_length == 12);
        assert(rtw_rx24_next(&it, family, &packet) == RTW_RX24_END);

        rtw_backend_rx_report report{};
        assert(rtw_backend_rx_preflight(family, bytes.data(), bytes.size(), &report) ==
               RTW_BACKEND_RX_VALID);
        assert(report.frames == 2 && report.firmware_events == 1);
        assert(report.management_or_data_frames == 1 && report.error_offset == 0);
        auto corrupted = bytes;
        corrupted[1] |= 0x40; // CRC
        assert(rtw_backend_rx_preflight(family, corrupted.data(), corrupted.size(),
               &report) == RTW_BACKEND_RX_BAD_RADIO_FRAME);
        assert(report.frames == 0 && report.error_offset == 0);
        corrupted = bytes;
        corrupted[1] |= 0x80; // ICV
        assert(rtw_backend_rx_preflight(family, corrupted.data(), corrupted.size(),
               &report) == RTW_BACKEND_RX_BAD_RADIO_FRAME);
        corrupted = bytes;
        const size_t second = (24U + 32U + 1U + 28U + 7U) & ~size_t(7);
        corrupted[second] = 0xff; corrupted[second + 1] = 0x3f;
        assert(rtw_backend_rx_preflight(family, corrupted.data(), corrupted.size(),
               &report) == RTW_BACKEND_RX_MALFORMED);
        assert(report.frames == 0 && report.error_offset == second);
        // Valid 8-byte-aligned DMA payload may omit final padding.
        bytes.resize(bytes.size() - 4U);
        rtw_rx24_iterator_init(&it, bytes.data(), bytes.size());
        assert(rtw_rx24_next(&it, family, &packet) == RTW_RX24_FRAME);
        assert(rtw_rx24_next(&it, family, &packet) == RTW_RX24_FRAME);
        // Truncated final USB payload must never be accepted.
        assert(rtw_backend_rx_preflight(family, bytes.data(), bytes.size() - 1,
               &report) == RTW_BACKEND_RX_MALFORMED);
        // 65 frames are not accepted in one aggregate.
        std::vector<uint8_t> many;
        for (unsigned i = 0; i < 65; ++i)
            add(many, family, 8);
        assert(rtw_backend_rx_preflight(family, many.data(), many.size(),
               &report) == RTW_BACKEND_RX_MALFORMED);
        // C2H and MPDU use different length bounds.
        std::vector<uint8_t> bad;
        add(bad, family, 3);
        assert(rtw_backend_rx_preflight(family, bad.data(), bad.size(),
               &report) == RTW_BACKEND_RX_MALFORMED);
        bad.clear();
        add(bad, family, 3, 0, 0, 4, 0, true);
        assert(rtw_backend_rx_preflight(family, bad.data(), bad.size(),
               &report) == RTW_BACKEND_RX_VALID);
        bad.clear();
        add(bad, family, 28, 0, 0, 127);
        assert(rtw_backend_rx_preflight(family, bad.data(), bad.size(),
               &report) == RTW_BACKEND_RX_MALFORMED);
        bad.clear();
        add(bad, family, 28, 0, 0, 4, 3);
        assert(rtw_backend_rx_preflight(family, bad.data(), bad.size(),
               &report) == RTW_BACKEND_RX_MALFORMED);
        bad.clear();
        add(bad, family, 28, 0, 0, 44);
        assert(rtw_backend_rx_preflight(family, bad.data(), bad.size(),
               &report) ==
            (modern(family) ? RTW_BACKEND_RX_VALID : RTW_BACKEND_RX_MALFORMED));
    }
    // A descriptor from an unimplemented family must not be reinterpreted.
    rtw_rx24_iterator it{};
    rtw_rx24_frame packet{};
    const uint8_t data[24] = {};
    rtw_rx24_iterator_init(&it, data, sizeof(data));
    assert(rtw_rx24_next(&it, RTW_WIFI_RTL8192DU, &packet) == RTW_RX24_UNSUPPORTED);
    assert(rtw_rx24_next(&it, RTW_WIFI_RTL8710BU, &packet) == RTW_RX24_UNSUPPORTED);
    assert(rtw_rx24_next(&it, RTW_WIFI_RTL8814BU, &packet) == RTW_RX24_UNSUPPORTED);
    assert(rtw_rx24_next(&it, RTW_WIFI_RTL8192FU, &packet) == RTW_RX24_MALFORMED);
    rtw_rx24_iterator_init(&it, nullptr, 1U);
    assert(rtw_rx24_next(&it, RTW_WIFI_RTL8192FU, &packet) == RTW_RX24_INVALID);
    rtw_rx24_iterator_init(&it, data, 65537U);
    assert(rtw_rx24_next(&it, RTW_WIFI_RTL8192FU, &packet) == RTW_RX24_INVALID);

    // Deterministic fuzz with bounded buffers; never dereference outside data.
    std::mt19937 gen(0x18a31u);
    std::uniform_int_distribution<int> octet(0, 255);
    std::vector<uint8_t> fuzz(200);
    unsigned iterations = 0;
    for (const auto family : families) {
        for (unsigned n = 0; n != 12000; ++n) {
            for (auto& byte : fuzz) byte = static_cast<uint8_t>(octet(gen));
            const size_t len = static_cast<size_t>(octet(gen)) % (fuzz.size() + 1);
            rtw_backend_rx_report report{};
            const auto status = rtw_backend_rx_preflight(
                family, fuzz.data(), len, &report);
            assert(status == RTW_BACKEND_RX_VALID ||
                   status == RTW_BACKEND_RX_MALFORMED ||
                   status == RTW_BACKEND_RX_BAD_RADIO_FRAME);
            assert(report.frames <= 64);
            ++iterations;
        }
    }
    std::printf("5 family-specific offline RX decoders: %u fuzz cases passed\n",
                iterations);
}
