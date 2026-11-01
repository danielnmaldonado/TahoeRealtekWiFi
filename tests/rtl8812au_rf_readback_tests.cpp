/* SPDX-License-Identifier: GPL-2.0-only
 * All tests use a synthetic register transport; no real RF hardware.
 */
#include "rtw/RFReadback.h"
#include <array>
#include <initializer_list>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
unsigned long checks=0;
#define CHECK(v) do { ++checks; if (!(v)) { \
    std::fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #v); \
    std::abort(); } } while (0)

struct Fake {
    std::array<uint8_t,4096> registers{};
    uint32_t a_first=0x07006U;
    uint32_t a_second=0x07006U;
    uint32_t b_first=0U;
    uint32_t b_second=0U;
    unsigned reads=0U;
    unsigned writes=0U;
    unsigned delays=0U;
    unsigned a_reads=0U;
    unsigned b_reads=0U;
    unsigned fail_read_at=0U;
    unsigned fail_write_at=0U;
    unsigned fail_wait_at=0U;
    bool ignore_disable=false;
    bool ignore_hssi_restore=false;
    Fake() {
        set(0x00F0U,4U,0x04411137U);
        set(0x0100U,2U,0x06FFU);
        set(0x0608U,4U,0xF0007B3FU);
        set(0x0800U,4U,0x03000000U);
        set(0x0808U,4U,0x30000000U);
        set(0x0834U,4U,0x00000001U);
        set(0x0838U,4U,0x13572460U);
        set(0x08B0U,4U,0xABCD9912U);
        set(0x0C00U,4U,4U);  /* path A: PI */
        set(0x0E00U,4U,0U);  /* path B: SI */
    }
    void set(unsigned address,unsigned width,uint32_t v) {
        for(unsigned i=0U;i<width;++i)
            registers[address+i]=static_cast<uint8_t>(v>>(8U*i));
    }
    uint32_t get(unsigned address,unsigned width) const {
        uint32_t value=0U;
        for(unsigned i=0U;i<width;++i)
            value|=uint32_t(registers[address+i])<<(8U*i);
        return value;
    }
};
rtw_usb_error fake_in(void* context,const rtw_usb_setup* req,
                      uint8_t* data,uint16_t* actual) {
    auto& fake=*static_cast<Fake*>(context);
    if (!req || !data || !actual || req->bm_request_type!=0xc0 ||
        req->b_request!=5 || req->w_index!=0 ||
        !(req->w_length==1 || req->w_length==2 || req->w_length==4) ||
        static_cast<unsigned>(req->w_value)+req->w_length>
        fake.registers.size())
        return RTW_USB_INVALID_ARGUMENT;
    ++fake.reads;
    if (fake.fail_read_at==fake.reads)
        return RTW_USB_IO_ERROR;
    uint32_t value=fake.get(req->w_value,req->w_length);
    if (req->w_value==0x0D04U || req->w_value==0x0D08U) {
        value=fake.a_reads++==0U?fake.a_first:fake.a_second;
    }
    if (req->w_value==0x0D44U || req->w_value==0x0D48U) {
        value=fake.b_reads++==0U?fake.b_first:fake.b_second;
    }
    for(unsigned i=0U;i<req->w_length;++i)
        data[i]=static_cast<uint8_t>(value>>(8U*i));
    *actual=req->w_length;
    return RTW_USB_OK;
}
rtw_usb_error fake_out(void* context,const rtw_usb_setup* req,
                       const uint8_t* data,uint16_t* actual) {
    auto& fake=*static_cast<Fake*>(context);
    if (!req || !data || !actual || req->bm_request_type!=0x40 ||
        req->b_request!=5 || req->w_index!=0 ||
        !(req->w_length==1 || req->w_length==2 || req->w_length==4) ||
        static_cast<unsigned>(req->w_value)+req->w_length>
        fake.registers.size())
        return RTW_USB_INVALID_ARGUMENT;
    ++fake.writes;
    if (fake.fail_write_at==fake.writes) return RTW_USB_IO_ERROR;
    const bool restoring_hssi=req->w_value==0x08B0U &&
        req->w_length==4U && fake.writes>=6U;
    if ((!fake.ignore_disable || req->w_value!=0x0100U) &&
        !(fake.ignore_hssi_restore && restoring_hssi))
        std::memcpy(&fake.registers[req->w_value],data,req->w_length);
    *actual=req->w_length;
    return RTW_USB_OK;
}
int fake_wait(void* context,uint32_t micros) {
    auto& fake=*static_cast<Fake*>(context);
    ++fake.delays;
    return micros==10U && fake.delays!=fake.fail_wait_at;
}
rtw_monitor_report ready() {
    rtw_monitor_report state{};
    state.monitor_registers_written=1U;
    state.rx_register_operations=7U;
    return state;
}
rtw_rf_readback_error run(Fake& fake,rtw_rf_readback_report& report,
                          bool allow=true) {
    rtw_usb_rw_transport bus{&fake,fake_in,fake_out};
    rtw_waiter timer{&fake,fake_wait};
    const rtw_monitor_report state=ready();
    return rtw_rf_readback_channel6_8812au(
        &bus,&timer,&state,allow?1:0,&report);
}
void success_and_readback_limits() {
    Fake fake{};
    const auto cca=fake.get(0x838U,4U);
    const auto selector=fake.get(0x8B0U,4U);
    rtw_rf_readback_report report{};
    CHECK(run(fake,report)==RTW_RF_READBACK_OK);
    CHECK(report.successful_samples==4U);
    CHECK(report.rf18_a_first==0x07006U && report.rf18_a_second==0x07006U);
    CHECK(report.rf18_b_first==0U && report.rf18_b_second==0U);
    CHECK(report.path_a_confirmed && !report.path_b_confirmed);
    CHECK(report.path_b_inconclusive && report.pi_mode_mask==1U);
    CHECK(report.bb_registers_restored);
    CHECK(fake.get(0x838U,4U)==cca);
    CHECK(fake.get(0x8B0U,4U)==selector);
    CHECK(fake.writes==7U && fake.delays==4U);
    fake.a_reads=0U;fake.b_reads=0U;
    fake.b_first=0x07006U;fake.b_second=0x07006U;
    CHECK(run(fake,report)==RTW_RF_READBACK_OK);
    CHECK(report.path_b_confirmed && !report.path_b_inconclusive);
    fake.b_first=0x000FFFFFU;fake.b_second=0U;
    CHECK(run(fake,report)==RTW_RF_READBACK_OK);
    CHECK(report.path_b_inconclusive);
}
void mode_selection_and_preamble() {
    Fake fake{};
    fake.set(0x0C00U,4U,0U);
    fake.set(0x0E00U,4U,4U);
    fake.set(0x0838U,4U,0x80808088U);
    rtw_rf_readback_report report{};
    CHECK(run(fake,report)==RTW_RF_READBACK_OK);
    CHECK(report.pi_mode_mask==2U);
    CHECK(fake.writes==5U); /* Guard already active; no unnecessary write. */
    CHECK(fake.get(0x0838U,4U)==0x80808088U);
    Fake denied{};
    CHECK(run(denied,report,false)==RTW_RF_READBACK_NOT_AUTHORIZED);
    CHECK(denied.reads==0U && denied.writes==0U);
    rtw_usb_rw_transport bus{&denied,fake_in,fake_out};
    rtw_waiter timer{&denied,fake_wait};
    rtw_monitor_report state=ready();
    state.ethernet_authorized=1U;
    CHECK(rtw_rf_readback_channel6_8812au(&bus,&timer,&state,1,&report)==
          RTW_RF_READBACK_MONITOR_UNVERIFIED);
    CHECK(denied.reads==0U && denied.writes==0U);
    state.ethernet_authorized=0U;
    denied.set(0x0608U,4U,0U);
    CHECK(rtw_rf_readback_channel6_8812au(&bus,&timer,&state,1,&report)==
          RTW_RF_READBACK_MONITOR_UNVERIFIED);
    CHECK(denied.writes==0U);
    CHECK(rtw_rf_readback_channel6_8812au(nullptr,&timer,&state,1,&report)==
          RTW_RF_READBACK_BAD_ARGUMENT);
    CHECK(rtw_rf_readback_channel6_8812au(&bus,&timer,&state,1,nullptr)==
          RTW_RF_READBACK_BAD_ARGUMENT);
}
void reject_bad_path_a_and_keep_cleanup() {
    for(uint32_t unexpected: {0U,0xFFFFFU,0x07007U,0x10706U,0x07106U}) {
        Fake fake{};
        const auto cca=fake.get(0x0838U,4U);
        const auto hssi=fake.get(0x08B0U,4U);
        fake.a_first=unexpected;
        rtw_rf_readback_report report{};
        CHECK(run(fake,report)==RTW_RF_READBACK_CHANNEL_MISMATCH);
        CHECK(!report.path_a_confirmed && report.bb_registers_restored);
        CHECK(fake.get(0x0838U,4U)==cca);
        CHECK(fake.get(0x08B0U,4U)==hssi);
    }
    Fake fake{};
    fake.a_first=0x7006U;fake.a_second=0x7007U;
    rtw_rf_readback_report report{};
    CHECK(run(fake,report)==RTW_RF_READBACK_CHANNEL_MISMATCH);
    CHECK(!report.path_a_confirmed);
}
void errors_and_restoration() {
    for(unsigned position: {1U,2U,3U,4U,5U}) {
        Fake fake{};
        const auto cca=fake.get(0x0838U,4U);
        const auto hssi=fake.get(0x08B0U,4U);
        fake.fail_write_at=position;
        rtw_rf_readback_report report{};
        CHECK(run(fake,report)==RTW_RF_READBACK_USB_ERROR);
        CHECK(report.bb_registers_restored);
        CHECK(fake.get(0x0838U,4U)==cca &&
              fake.get(0x08B0U,4U)==hssi);
    }
    for(unsigned position: {6U,7U}) {
        Fake fake{};
        fake.fail_write_at=position;
        rtw_rf_readback_report report{};
        CHECK(run(fake,report)==RTW_RF_READBACK_RESTORE_FAILED);
        CHECK(!report.bb_registers_restored);
        CHECK(fake.writes==7U); /* Both restorations were attempted. */
    }
    for(unsigned position: {1U,6U,7U,8U,9U,10U,11U,12U,13U,14U}) {
        Fake fake{};
        const auto cca=fake.get(0x0838U,4U);
        const auto hssi=fake.get(0x08B0U,4U);
        fake.fail_read_at=position;
        rtw_rf_readback_report report{};
        const auto result=run(fake,report);
        if (position<=6U)
            CHECK(result==RTW_RF_READBACK_MONITOR_UNVERIFIED);
        else
            CHECK(result==RTW_RF_READBACK_USB_ERROR);
        if(position>=11U)
            CHECK(report.bb_registers_restored);
        CHECK(fake.writes==0U || (fake.get(0x0838U,4U)==cca &&
              fake.get(0x08B0U,4U)==hssi));
    }
    for(unsigned position: {15U,16U}) {
        Fake fake{};
        fake.fail_read_at=position;
        rtw_rf_readback_report report{};
        CHECK(run(fake,report)==RTW_RF_READBACK_RESTORE_FAILED);
        CHECK(!report.bb_registers_restored);
        CHECK(!report.path_a_confirmed);
    }
    Fake ignored{};
    ignored.ignore_hssi_restore=true;
    rtw_rf_readback_report ignoredReport{};
    CHECK(run(ignored,ignoredReport)==RTW_RF_READBACK_RESTORE_FAILED);
    CHECK(!ignoredReport.path_a_confirmed);
    CHECK(!ignoredReport.bb_registers_restored);
    for(unsigned at=1U;at<=4U;++at) {
        Fake fake{};
        fake.fail_wait_at=at;
        rtw_rf_readback_report report{};
        CHECK(run(fake,report)==RTW_RF_READBACK_DELAY_FAILED);
        CHECK(report.bb_registers_restored);
    }
}
void rx_quiesce() {
    Fake fake{};
    rtw_usb_rw_transport bus{&fake,fake_in,fake_out};
    const auto state=ready();
    CHECK(rtw_monitor_quiesce_rx_channel6(&bus,&state)==RTW_MONITOR_OK);
    CHECK(fake.get(0x0100U,2U)==(0x06FFU & ~0x008AU));
    CHECK(rtw_monitor_quiesce_rx_channel6(&bus,&state)==RTW_MONITOR_OK);
    rtw_monitor_report denied{};
    CHECK(rtw_monitor_quiesce_rx_channel6(&bus,&denied)==RTW_MONITOR_NOT_AUTHORIZED);
    denied=state;denied.ethernet_authorized=1U;
    CHECK(rtw_monitor_quiesce_rx_channel6(&bus,&denied)==RTW_MONITOR_NOT_AUTHORIZED);
    Fake stuck{};
    stuck.ignore_disable=true;
    rtw_usb_rw_transport stuckBus{&stuck,fake_in,fake_out};
    CHECK(rtw_monitor_quiesce_rx_channel6(&stuckBus,&state)==RTW_MONITOR_REGISTER_MISMATCH);
    rtw_monitor_report partial{};
    partial.rx_register_operations=2U;
    Fake partialUsb{};
    rtw_usb_rw_transport partialBus{&partialUsb,fake_in,fake_out};
    CHECK(rtw_monitor_quiesce_rx_channel6(&partialBus,&partial)==RTW_MONITOR_OK);
    CHECK((partialUsb.get(0x0100U,2U)&0x008AU)==0U);
    Fake failed{};
    failed.fail_write_at=1U;
    rtw_usb_rw_transport failedBus{&failed,fake_in,fake_out};
    CHECK(rtw_monitor_quiesce_rx_channel6(&failedBus,&state)==RTW_MONITOR_USB_FAILED);
}
}
int main() {
    success_and_readback_limits();
    mode_selection_and_preamble();
    reject_bad_path_a_and_keep_cleanup();
    errors_and_restoration();
    rx_quiesce();
    std::printf("RTL8812AU RF readback: %lu mock-transport checks passed; "
                "no real USB or radio used\n",checks);
}
