/* SPDX-License-Identifier: GPL-2.0-only. No hardware or kernel loading. */
#include "rtw/USBProbe.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static unsigned checks=0;
#define CHECK(x) do {++checks; if (!(x)) {std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);std::exit(1);}} while (0)

namespace {
struct Bus {
    std::array<uint16_t,4> addresses{};
    std::array<uint32_t,4> values{0x00043234U,0x00400000U,0x00000113U,0x000604c6U};
    unsigned calls=0;
    unsigned fail_at=0;
    bool short_transfer=false;
    bool malformed_request=false;
};

rtw_usb_error read_register(void* context,const rtw_usb_setup* setup,
                            uint8_t* bytes,uint16_t* actual) {
    auto& bus=*static_cast<Bus*>(context);
    const unsigned index=bus.calls++;
    if (!setup || !bytes || !actual || index>=4 ||
        setup->bm_request_type!=0xc0 || setup->b_request!=5 ||
        setup->w_index!=0 || setup->w_length!=4 ||
        setup->timeout_ms!=1000) {
        bus.malformed_request=true;
        return RTW_USB_INVALID_ARGUMENT;
    }
    bus.addresses[index]=setup->w_value;
    if (bus.fail_at==bus.calls) return RTW_USB_IO_ERROR;
    const uint32_t value=bus.values[index];
    for (unsigned i=0;i<4;++i) bytes[i]=static_cast<uint8_t>(value>>(8U*i));
    *actual=bus.short_transfer ? 3U : 4U;
    return RTW_USB_OK;
}

void empty_report(const rtw_usb_probe_report& report) {
    CHECK(report.family==RTW_USB_PROBE_FAMILY_NONE);
    CHECK(report.rtl8812au.sys_cfg==0);
    CHECK(report.rtl8192eu.vid==0 && report.rtl8192eu.pid==0);
    CHECK(report.rtl8192eu.sys_cfg==0 && report.rtl8192eu.sys_cfg2==0);
    CHECK(report.rtl8192eu.hpon_fsm==0 && report.rtl8192eu.mcu_fw_dl==0);
}

// Catches dispatching 8192EU through the 8812AU silicon-profile decoder.
void rtl8192eu_uses_its_own_register_protocol() {
    Bus bus;
    const rtw_usb_transport transport{&bus,&read_register};
    rtw_usb_probe_report report{};
    CHECK(rtw_usb_probe_read_only(&transport,0x2357,0x0108,&report)==RTW_USB_PROBE_OK);
    CHECK(report.family==RTW_USB_PROBE_FAMILY_RTL8192EU);
    CHECK(report.rtl8192eu.vid==0x2357 && report.rtl8192eu.pid==0x0108);
    CHECK(report.rtl8192eu.sys_cfg==0x00043234U);
    CHECK(report.rtl8192eu.sys_cfg2==0x00000113U);
    CHECK(report.rtl8192eu.hpon_fsm==0x00400000U);
    CHECK(report.rtl8192eu.mcu_fw_dl==0x000604c6U);
    CHECK(report.rtl8192eu.silicon_cut==3 && report.rtl8192eu.foundry_code==1);
    CHECK(report.rtl8192eu.is_8191eu_bonding==1 && report.rtl8192eu.inferred_tx_paths==1);
    CHECK(report.rtl8812au.sys_cfg==0);
    CHECK(bus.calls==4 && !bus.malformed_request);
    CHECK((bus.addresses==std::array<uint16_t,4>{0x00f0,0x00ec,0x00fc,0x0080}));
}

// Catches probing another product before an exact VID/PID allowlist check.
void unsupported_ids_never_issue_vendor_requests() {
    constexpr uint16_t ids[][2]={{0x2357,0x0109},{0x0bda,0x0108},
        {0x0bda,0x818b},{0x2357,0x8812},{0,0},{0xffff,0xffff}};
    for (const auto& id:ids) {
        Bus bus;
        const rtw_usb_transport transport{&bus,&read_register};
        rtw_usb_probe_report report;
        std::memset(&report,0xa5,sizeof(report));
        CHECK(rtw_usb_probe_read_only(&transport,id[0],id[1],&report)==RTW_USB_PROBE_UNSUPPORTED_ID);
        CHECK(bus.calls==0);
        empty_report(report);
    }
}

// Catches losing the existing 8812AU B-cut TSMC USB research path.
void rtl8812au_keeps_its_single_read_and_profile() {
    Bus bus;
    bus.values[0]=0x04001000U;
    const rtw_usb_transport transport{&bus,&read_register};
    rtw_usb_probe_report report{};
    CHECK(rtw_usb_probe_read_only(&transport,0x0bda,0x8812,&report)==RTW_USB_PROBE_OK);
    CHECK(report.family==RTW_USB_PROBE_FAMILY_RTL8812AU);
    CHECK(report.rtl8812au.sys_cfg==0x04001000U);
    CHECK(report.rtl8812au.cut==1 && report.rtl8812au.is_usb==1);
    CHECK(report.rtl8192eu.sys_cfg==0);
    CHECK(bus.calls==1 && bus.addresses[0]==0x00f0 && !bus.malformed_request);
    constexpr uint32_t invalid[]={0U,0xffffffffU,0x04000000U,0x04002000U,
        0x00001000U,0x04801000U,0x04081000U};
    for (const auto cfg:invalid) {
        Bus bad;
        bad.values[0]=cfg;
        const rtw_usb_transport bad_transport{&bad,&read_register};
        CHECK(rtw_usb_probe_read_only(&bad_transport,0x0bda,0x8812,&report)==RTW_USB_PROBE_UNSUPPORTED_PROFILE);
        CHECK(bad.calls==1);
        empty_report(report);
    }
}

// Catches continuing after failed/short I/O and returning stale identity.
void io_failures_stop_without_publishing_partial_identity() {
    for (unsigned at=1;at<=4;++at) {
        Bus bus;
        bus.fail_at=at;
        const rtw_usb_transport transport{&bus,&read_register};
        rtw_usb_probe_report report;
        std::memset(&report,0xa5,sizeof(report));
        CHECK(rtw_usb_probe_read_only(&transport,0x2357,0x0108,&report)==RTW_USB_PROBE_READ_FAILED);
        CHECK(bus.calls==at && !bus.malformed_request);
        empty_report(report);
    }
    for (const bool rtl8192eu:{false,true}) {
        Bus bus;
        bus.short_transfer=true;
        const rtw_usb_transport transport{&bus,&read_register};
        rtw_usb_probe_report report{};
        CHECK(rtw_usb_probe_read_only(&transport,rtl8192eu ? 0x2357 : 0x0bda,
            rtl8192eu ? 0x0108 : 0x8812,&report)==RTW_USB_PROBE_READ_FAILED);
        CHECK(bus.calls==1);
        empty_report(report);
    }
    Bus bus;
    bus.fail_at=1;
    const rtw_usb_transport transport{&bus,&read_register};
    rtw_usb_probe_report report{};
    CHECK(rtw_usb_probe_read_only(&transport,0x0bda,0x8812,&report)==RTW_USB_PROBE_READ_FAILED);
    CHECK(bus.calls==1);
    empty_report(report);
}

// Catches accepting implausible/test 8192EU silicon from a matching USB ID.
void rtl8192eu_rejects_invalid_identity_after_reads() {
    constexpr uint32_t invalid[]={0U,0xffffffffU,0x00843234U};
    for (const auto cfg:invalid) {
        Bus bus;
        bus.values[0]=cfg;
        const rtw_usb_transport transport{&bus,&read_register};
        rtw_usb_probe_report report{};
        CHECK(rtw_usb_probe_read_only(&transport,0x2357,0x0108,&report)==RTW_USB_PROBE_UNSUPPORTED_PROFILE);
        CHECK(bus.calls==4);
        empty_report(report);
    }
}

void invalid_arguments_do_not_touch_transport() {
    Bus bus;
    const rtw_usb_transport transport{&bus,&read_register};
    const rtw_usb_transport no_reader{&bus,nullptr};
    rtw_usb_probe_report report{};
    CHECK(rtw_usb_probe_read_only(nullptr,0x2357,0x0108,&report)==RTW_USB_PROBE_BAD_ARGUMENT);
    CHECK(rtw_usb_probe_read_only(&no_reader,0x2357,0x0108,&report)==RTW_USB_PROBE_BAD_ARGUMENT);
    CHECK(rtw_usb_probe_read_only(&transport,0x2357,0x0108,nullptr)==RTW_USB_PROBE_BAD_ARGUMENT);
    CHECK(bus.calls==0);
    empty_report(report);
}
}

int main() {
    rtl8192eu_uses_its_own_register_protocol();
    unsupported_ids_never_issue_vendor_requests();
    rtl8812au_keeps_its_single_read_and_profile();
    io_failures_stop_without_publishing_partial_identity();
    rtl8192eu_rejects_invalid_identity_after_reads();
    invalid_arguments_do_not_touch_transport();
    std::printf("USB probe: %u checks passed (no hardware).\n",checks);
}
