/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/LegacyFirmwareStage.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

static std::vector<uint8_t> config_for(const rtw_usb_wifi_profile& p) {
    std::vector<uint8_t> cfg = {
        9, 2, 32, 0, 1, p.configuration, 0, 0x80, 50,
        9, 4, p.interface_number, 0, 2, 0xff, 0xff, 0xff, 0,
        7, 5, 0x81, 2, 0, 2, 0,
        7, 5, 0x02, 2, 0, 2, 0
    };
    if (p.interface_match_flags & RTW_USB_MATCH_CLASS)
        cfg[14] = p.interface_class;
    if (p.interface_match_flags & RTW_USB_MATCH_SUBCLASS)
        cfg[15] = p.interface_subclass;
    if (p.interface_match_flags & RTW_USB_MATCH_PROTOCOL)
        cfg[16] = p.interface_protocol;
    return cfg;
}
static rtw_backend_usb_plan usb_plan(rtw_wifi_family family) {
    for (size_t i = 0; i < rtw_usb_wifi_profile_count(); ++i) {
        const auto* p = rtw_usb_wifi_profile_at(i);
        if (!p || p->family != family || !p->primary_verified) continue;
        const auto cfg = config_for(*p);
        rtw_backend_usb_plan plan{};
        const auto status = rtw_backend_usb_preflight(
            p->vendor, p->product, p->match_bcd ? p->bcd_device : 0,
            cfg.data(), cfg.size(), p->interface_number, &plan);
        if (status == RTW_BACKEND_PREFLIGHT_DIAGNOSTICS || status == RTW_BACKEND_PREFLIGHT_STATION) return plan;
    }
    assert(false && "expected verified profile");
    return {};
}
static std::vector<uint8_t> firmware(uint16_t sig, size_t payload_size) {
    std::vector<uint8_t> data(32 + payload_size);
    data[0] = static_cast<uint8_t>(sig);
    data[1] = static_cast<uint8_t>(sig >> 8);
    data[4] = 1;
    data[5] = 2;
    for (size_t i = 32; i < data.size(); ++i)
        data[i] = static_cast<uint8_t>(i * 7U + 3U);
    return data;
}
struct Walk {
    size_t total = 0;
    size_t count = 0;
    uint8_t max_page = 0;
    bool cancel = false;
    uint16_t base = 0;
};
static int walk_chunk(void* context, uint8_t page, uint16_t address,
                      const uint8_t*, uint16_t len) {
    auto& w = *static_cast<Walk*>(context);
    assert(len >= 1 && len <= 254);
    assert(page < 8 && address >= w.base);
    assert(static_cast<unsigned>(address - w.base) + len <= 4096);
    assert(page >= w.max_page);
    w.max_page = page;
    ++w.count;
    w.total += len;
    return !w.cancel;
}

struct Usb {
    std::array<uint8_t, 256> registers{};
    size_t writes = 0;
    size_t reads = 0;
    size_t data_bytes = 0;
    int fail_after_write = -1;
    bool fail_read = false;
    bool short_write = false;
    bool clear_checksum = true;
    bool checksum_after_upload = true;
    bool never_ready = false;
    bool waiting_checksum = false;
    bool downloading = false;
    bool cpu_reset_observed = false;
};
static rtw_usb_error read(void* context, const rtw_usb_setup* request,
                          uint8_t* bytes, uint16_t* actual) {
    auto& state = *static_cast<Usb*>(context);
    if (!actual || !request || !bytes) return RTW_USB_INVALID_ARGUMENT;
    *actual = 0;
    ++state.reads;
    if (state.fail_read) return RTW_USB_IO_ERROR;
    assert(request->bm_request_type == 0xc0 && request->b_request == 5);
    assert(request->w_value + request->w_length <= state.registers.size());
    if (state.waiting_checksum && request->w_value == 0x80 &&
        state.checksum_after_upload)
        state.registers[0x80] |= 4;
    for (size_t i = 0; i < request->w_length; ++i)
        bytes[i] = state.registers[request->w_value + i];
    *actual = request->w_length;
    return RTW_USB_OK;
}
static rtw_usb_error write(void* context, const rtw_usb_setup* request,
                           const uint8_t* bytes, uint16_t* actual) {
    auto& state = *static_cast<Usb*>(context);
    if (!actual || !request || !bytes) return RTW_USB_INVALID_ARGUMENT;
    *actual = 0;
    assert(request->bm_request_type == 0x40 && request->b_request == 5);
    ++state.writes;
    if (state.fail_after_write > 0 &&
        state.writes == static_cast<size_t>(state.fail_after_write))
        return RTW_USB_IO_ERROR;
    if (request->w_value >= 0x1000) {
        if (state.short_write) {
            *actual = request->w_length - 1;
            return RTW_USB_OK;
        }
        state.data_bytes += request->w_length;
        *actual = request->w_length;
        return RTW_USB_OK;
    }
    assert(request->w_value + request->w_length <= state.registers.size());
    for (unsigned i = 0; i < request->w_length; ++i) {
        const size_t addr = request->w_value + i;
        const uint8_t prior = state.registers[addr];
        state.registers[addr] = bytes[i];
        if (addr == 0x80) {
            if ((bytes[i] & 4U) && state.clear_checksum)
                state.registers[addr] &= static_cast<uint8_t>(~4U);
            if ((bytes[i] & 1U) != 0U) state.downloading = true;
            if ((bytes[i] & 1U) == 0U && state.downloading) {
                state.downloading = false;
                state.waiting_checksum = true;
            }
            if ((bytes[i] & 2U) != 0U && (prior & 2U) == 0U) {
                state.cpu_reset_observed = true;
                if (!state.never_ready) state.registers[0x80] |= 0x40U;
            }
        }
    }
    *actual = request->w_length;
    return RTW_USB_OK;
}
static int sleep_us(void*, uint32_t delay) {
    return delay >= 1 && delay <= 10000;
}
int main() {
    struct Case {
        rtw_wifi_family family;
        uint16_t signature;
        uint16_t window;
        bool boot;
    };
    const Case cases[] = {
        {RTW_WIFI_RTL8192FU, 0x92f1, 0x4000, false},
        {RTW_WIFI_RTL8723BU, 0x5301, 0x1000, false},
        {RTW_WIFI_RTL8710BU, 0x10b1, 0x1000, false}
    };
    unsigned cases_checked = 0;
    for (const auto& c : cases) {
        auto usb = usb_plan(c.family);
        for (size_t length : {size_t(1), size_t(254), size_t(4096),
                              size_t(4097), size_t(32768)}) {
            auto file = firmware(c.signature, length);
            rtw_legacy_fw_plan plan{};
            if(usb.station_backend_integrated){
                assert(rtw_legacy_fw_prepare(&usb,file.data(),file.size(),&plan)==RTW_LEGACY_FW_IDENTITY_UNVERIFIED);
                // Exercise only immutable offline legacy geometry after native
                // activation. The ordinary native preflight is refused above;
                // FU hardware boot remains independently forbidden below.
                usb.station_backend_integrated=0;usb.usb_preflight_only=1;
            }
            assert(rtw_legacy_fw_prepare(&usb, file.data(), file.size(),
                                          &plan) == RTW_LEGACY_FW_OK);
            assert(plan.family == c.family &&
                   plan.firmware_window_register == c.window &&
                   plan.boot_experimental_available == static_cast<uint8_t>(c.boot));
            assert(plan.pages == (length + 4095) / 4096);
            Walk walked{}; walked.base = c.window;
            size_t transfers = 0;
            assert(rtw_legacy_fw_walk(&plan, walk_chunk, &walked, &transfers)
                   == RTW_LEGACY_FW_OK);
            assert(walked.total == length && walked.count == transfers);
            assert(walked.max_page + 1 == plan.pages);
            Walk cancelled{}; cancelled.base = c.window; cancelled.cancel = true;
            assert(rtw_legacy_fw_walk(&plan, walk_chunk, &cancelled, nullptr)
                   == RTW_LEGACY_FW_CANCELLED);
            auto corrupted = plan;
            corrupted.pages = 9;
            assert(rtw_legacy_fw_walk(&corrupted, walk_chunk,
                   &walked, nullptr) == RTW_LEGACY_FW_BAD_ARGUMENT);
            rtw_legacy_fw_report result{};
            Usb fake{};
            rtw_usb_rw_transport bus{&fake, &read, &write};
            rtw_waiter timer{nullptr, &sleep_us};
            rtw_legacy_fw_options opts{1, 1, 4, 10};
            assert(rtw_legacy_fw_boot_experimental(&plan, &bus, &timer,
                   &opts, &result) ==
                   (c.boot ? RTW_LEGACY_FW_OK : RTW_LEGACY_FW_UNSUPPORTED));
            if(!c.boot){
                assert(fake.writes==0&&fake.reads==0&&fake.data_bytes==0);
                if(c.family==RTW_WIFI_RTL8192FU||c.family==RTW_WIFI_RTL8723BU){
                    assert(rtw_legacy_fw_capability(c.family)==1);
                    auto forged=plan;forged.boot_experimental_available=1;
                    assert(rtw_legacy_fw_boot_experimental(&forged,&bus,&timer,&opts,&result)==RTW_LEGACY_FW_BAD_ARGUMENT);
                    assert(fake.writes==0&&fake.reads==0&&fake.data_bytes==0);
                }
            }
            if (c.boot) {
                assert(result.firmware_ready && result.checksum_seen &&
                       result.download_disabled && fake.cpu_reset_observed);
                assert(result.bytes_written == length &&
                       fake.data_bytes == length &&
                       result.pages_written == plan.pages);
                fake = {}; opts.confirm_unstable_hardware = 0;
                assert(rtw_legacy_fw_boot_experimental(&plan, &bus, &timer,
                       &opts, &result) == RTW_LEGACY_FW_PERMISSION_REQUIRED);
                assert(fake.writes == 0);
                opts.confirm_unstable_hardware = 1;
                opts.exclusive_interface_claimed = 0;
                assert(rtw_legacy_fw_boot_experimental(&plan, &bus, &timer,
                       &opts, &result) == RTW_LEGACY_FW_PERMISSION_REQUIRED);
                assert(fake.writes == 0);
                opts.exclusive_interface_claimed = 1;
                fake = {}; fake.registers[0x80] = 0x80;
                assert(rtw_legacy_fw_boot_experimental(&plan, &bus, &timer,
                       &opts, &result) == RTW_LEGACY_FW_BUSY);
                assert(fake.writes == 0);
                fake = {}; fake.short_write = true;
                assert(rtw_legacy_fw_boot_experimental(&plan, &bus, &timer,
                       &opts, &result) == RTW_LEGACY_FW_IO_ERROR);
                assert(!result.firmware_ready && result.download_disabled);
                fake = {}; fake.checksum_after_upload = false;
                assert(rtw_legacy_fw_boot_experimental(&plan, &bus, &timer,
                       &opts, &result) == RTW_LEGACY_FW_CHECKSUM_TIMEOUT);
                assert(!result.firmware_ready && result.download_disabled);
                fake = {}; fake.never_ready = true;
                assert(rtw_legacy_fw_boot_experimental(&plan, &bus, &timer,
                       &opts, &result) == RTW_LEGACY_FW_READY_TIMEOUT);
                assert(!result.firmware_ready && result.download_disabled);
                fake = {}; fake.clear_checksum = false;
                fake.registers[0x80] = 4;
                assert(rtw_legacy_fw_boot_experimental(&plan, &bus, &timer,
                       &opts, &result) == RTW_LEGACY_FW_BUSY);
                assert(result.download_disabled);
                fake = {}; fake.fail_read = true;
                assert(rtw_legacy_fw_boot_experimental(&plan, &bus, &timer,
                       &opts, &result) == RTW_LEGACY_FW_IO_ERROR);
                assert(fake.writes == 0);
                fake = {}; fake.fail_after_write = 5;
                assert(rtw_legacy_fw_boot_experimental(&plan, &bus, &timer,
                       &opts, &result) == RTW_LEGACY_FW_IO_ERROR);
                assert(!result.firmware_ready);
            }
            ++cases_checked;
        }
        auto file = firmware(c.signature, 32769);
        rtw_legacy_fw_plan plan{};
        assert(rtw_legacy_fw_prepare(&usb, file.data(), file.size(),
               &plan) == RTW_LEGACY_FW_CAPACITY_EXCEEDED);
        file = firmware(c.signature, 48);
        file[0] = 0xFF; file[1] = 0xFF;
        assert(rtw_legacy_fw_prepare(&usb, file.data(), file.size(),
               &plan) == RTW_LEGACY_FW_BAD_IMAGE);
    }
    auto unknown = usb_plan(RTW_WIFI_RTL8822CU);
    auto incorrect = firmware(0x2301, 40);
    rtw_legacy_fw_plan unused{};
    assert(rtw_legacy_fw_prepare(&unknown, incorrect.data(), incorrect.size(),
           &unused) == (unknown.station_backend_integrated?RTW_LEGACY_FW_IDENTITY_UNVERIFIED:RTW_LEGACY_FW_UNSUPPORTED));
    assert(unused.payload == nullptr);
    assert(std::strlen(rtw_legacy_fw_status_name(RTW_LEGACY_FW_OK)) == 2);
    std::printf("legacy firmware staging: %u family/size cases and failure-path "
                "simulations passed\n", cases_checked);
}
