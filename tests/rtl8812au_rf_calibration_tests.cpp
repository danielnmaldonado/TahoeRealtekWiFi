/* SPDX-License-Identifier: GPL-2.0-only
 * Offline test of RTL8812AU RF thermal diagnostic and factory baseline.
 * No hardware connection, temperature conversion or IQK is performed.
 */
#include "rtw/RFCalibration.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
unsigned long checks = 0UL;
#define CHECK(x) do { ++checks; if (!(x)) { \
    std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); \
    std::abort(); } } while (0)

struct Fake {
    std::array<uint8_t,4096> regs{};
    uint32_t first = (24U<<10U) | 0x3U;
    uint32_t second = (25U<<10U) | 0x3U;
    unsigned reads = 0U;
    unsigned writes = 0U;
    unsigned delays = 0U;
    unsigned sensor_reads = 0U;
    unsigned fail_read_at = 0U;
    unsigned fail_write_at = 0U;
    unsigned fail_wait_at = 0U;
    bool ignore_hssi_restore = false;

    Fake() {
        set(0x00F0U,4U,0x04411137U);
        set(0x0100U,2U,0x06FFU);
        set(0x0608U,4U,0xF0007B3FU);
        set(0x0800U,4U,0x03000000U);
        set(0x0808U,4U,0x30000000U);
        set(0x0834U,4U,0x00000001U);
        set(0x0838U,4U,0x13572460U);
        set(0x08B0U,4U,0xABCD9912U);
        set(0x0C00U,4U,4U);
    }
    void set(unsigned address,unsigned bytes,uint32_t value) {
        for (unsigned i=0U;i<bytes;++i)
            regs[address+i]=static_cast<uint8_t>(value>>(8U*i));
    }
    uint32_t get(unsigned address,unsigned bytes) const {
        uint32_t value=0U;
        for(unsigned i=0U;i<bytes;++i)
            value|=uint32_t(regs[address+i])<<(8U*i);
        return value;
    }
};

rtw_usb_error usb_in(void *ctx,const rtw_usb_setup *req,
                     uint8_t *out,uint16_t *actual) {
    auto& f=*static_cast<Fake*>(ctx);
    if (!req || !out || !actual ||
        req->bm_request_type!=0xC0U || req->b_request!=5U ||
        !(req->w_length==1U || req->w_length==2U || req->w_length==4U) ||
        unsigned(req->w_value)+req->w_length>f.regs.size())
        return RTW_USB_INVALID_ARGUMENT;
    ++f.reads;
    if (f.reads==f.fail_read_at) return RTW_USB_IO_ERROR;
    uint32_t value=f.get(req->w_value,req->w_length);
    if (req->w_value==0x0D04U || req->w_value==0x0D08U)
        value=f.sensor_reads++==0U ? f.first : f.second;
    for(unsigned i=0U;i<req->w_length;++i)
        out[i]=static_cast<uint8_t>(value>>(8U*i));
    *actual=req->w_length;
    return RTW_USB_OK;
}

rtw_usb_error usb_out(void *ctx,const rtw_usb_setup *req,
                      const uint8_t *bytes,uint16_t *actual) {
    auto& f=*static_cast<Fake*>(ctx);
    if (!req || !bytes || !actual ||
        req->bm_request_type!=0x40U || req->b_request!=5U ||
        req->w_length!=4U ||
        unsigned(req->w_value)+req->w_length>f.regs.size())
        return RTW_USB_INVALID_ARGUMENT;
    ++f.writes;
    if(f.writes==f.fail_write_at) return RTW_USB_IO_ERROR;
    const uint32_t value=uint32_t(bytes[0]) |
        (uint32_t(bytes[1])<<8U) |
        (uint32_t(bytes[2])<<16U) |
        (uint32_t(bytes[3])<<24U);
    if (!(f.ignore_hssi_restore && req->w_value==0x08B0U &&
          value==0xABCD9912U))
        std::memcpy(f.regs.data()+req->w_value,bytes,req->w_length);
    *actual=req->w_length;
    return RTW_USB_OK;
}

int wait_for_rf(void *ctx,uint32_t us) {
    auto& f=*static_cast<Fake*>(ctx);
    ++f.delays;
    return us==10U && f.delays!=f.fail_wait_at;
}

rtw_hal_report good_boot() {
    rtw_hal_report out{};
    out.last_success=RTW_HAL_MAC_PREPARED;
    out.sys_cfg=0x04411137U;
    out.identity.eeprom_vendor=0x0BDAU;
    out.identity.eeprom_product=0x8812U;
    out.identity.rfe_type=3U;
    out.identity.thermal_meter=24U;
    out.identity.thermal_meter_valid=1U;
    return out;
}
rtw_monitor_report good_monitor() {
    rtw_monitor_report out{};
    out.monitor_registers_written=1U;
    return out;
}
rtw_rf_readback_report good_rf() {
    rtw_rf_readback_report out{};
    out.path_a_confirmed=1U;
    out.bb_registers_restored=1U;
    return out;
}
rtw_rf_cal_error run(Fake& f,rtw_rf_cal_report& observed,
                     bool authorized=true,
                     rtw_hal_report boot=good_boot(),
                     rtw_monitor_report monitor=good_monitor(),
                     rtw_rf_readback_report rf=good_rf()) {
    rtw_usb_rw_transport bus{&f,usb_in,usb_out};
    rtw_waiter waiter{&f,wait_for_rf};
    return rtw_rf_thermal_preflight_8812au(
        &bus,&waiter,&boot,&monitor,&rf,authorized?1:0,&observed);
}

void valid_and_factory_baseline() {
    Fake f{};
    const auto old_cca=f.get(0x0838U,4U);
    const auto old_hssi=f.get(0x08B0U,4U);
    rtw_rf_cal_report report{};
    CHECK(run(f,report)==RTW_RF_CAL_OK);
    CHECK(report.samples_completed==2U);
    CHECK(report.first_code==24U && report.second_code==25U);
    CHECK(report.averaged_code==25U);
    CHECK(report.baseline_valid && report.stable && report.factory_code==24U);
    CHECK(report.drift_codes==1U && !report.delta_exceeds_iqk_threshold);
    CHECK(report.bb_registers_restored);
    CHECK(!report.iqk_performed && !report.tx_authorized);
    CHECK(f.writes==5U && f.delays==2U);
    CHECK(f.get(0x0838U,4U)==old_cca && f.get(0x08B0U,4U)==old_hssi);
    CHECK(f.sensor_reads==2U);
    /* Repeated sample may be stale: validity never implies calibration. */
    Fake drift{};
    drift.first=33U<<10U;
    drift.second=34U<<10U;
    CHECK(run(drift,report)==RTW_RF_CAL_OK);
    CHECK(report.drift_codes==10U && report.delta_exceeds_iqk_threshold);
    CHECK(!report.iqk_performed && !report.tx_authorized);
    Fake no_factory{};
    auto boot=good_boot();
    boot.identity.thermal_meter_valid=0U;
    CHECK(run(no_factory,report,true,boot)==RTW_RF_CAL_FACTORY_UNAVAILABLE);
    CHECK(!report.baseline_valid && report.stable && report.bb_registers_restored);
    CHECK(!report.tx_authorized);
    /* Even a dishonest upstream validity flag cannot make saturation valid. */
    Fake saturated_factory{};
    auto saturated_boot=good_boot();
    saturated_boot.identity.thermal_meter=63U;
    saturated_boot.identity.thermal_meter_valid=1U;
    CHECK(run(saturated_factory,report,true,saturated_boot)==
          RTW_RF_CAL_FACTORY_UNAVAILABLE);
    CHECK(!report.baseline_valid && !report.delta_exceeds_iqk_threshold);
    Fake mode{};
    mode.set(0x0C00U,4U,0U);     /* SI instead of PI. */
    mode.set(0x0838U,4U,0x80808088U); /* CCA was already guarded. */
    CHECK(run(mode,report)==RTW_RF_CAL_OK);
    CHECK(mode.writes==3U && mode.sensor_reads==2U);
    CHECK(mode.get(0x0838U,4U)==0x80808088U);
}

void unavailable_or_unstable() {
    for (uint32_t code : {0U,0xFFFFFU,(63U<<10U), (24U<<10U)}) {
        Fake f{};
        f.first=code;
        f.second=0U;
        rtw_rf_cal_report report{};
        CHECK(run(f,report)==RTW_RF_CAL_SENSOR_UNAVAILABLE);
        CHECK(report.bb_registers_restored && !report.stable);
        CHECK(!report.tx_authorized);
    }
    Fake unstable{};
    unstable.first=10U<<10U;unstable.second=20U<<10U;
    rtw_rf_cal_report report{};
    CHECK(run(unstable,report)==RTW_RF_CAL_SENSOR_UNSTABLE);
    CHECK(report.bb_registers_restored && !report.stable);
    CHECK(report.first_code==10U && report.second_code==20U);
    CHECK(!report.tx_authorized);
    Fake stable_edge{};
    stable_edge.first=10U<<10U;stable_edge.second=12U<<10U;
    CHECK(run(stable_edge,report)==RTW_RF_CAL_OK);
    CHECK(report.averaged_code==11U);
}

void authorization_and_failures() {
    Fake denied{};
    rtw_rf_cal_report report{};
    CHECK(run(denied,report,false)==RTW_RF_CAL_NOT_AUTHORIZED);
    CHECK(denied.reads==0U && denied.writes==0U);
    Fake profile{};
    auto boot=good_boot();
    boot.identity.rfe_type=2U;
    CHECK(run(profile,report,true,boot)==RTW_RF_CAL_PROFILE_UNVERIFIED);
    CHECK(profile.reads==0U && profile.writes==0U);
    Fake unverified_rf{};
    auto rf=good_rf();rf.path_a_confirmed=0U;
    CHECK(run(unverified_rf,report,true,good_boot(),good_monitor(),rf)==
          RTW_RF_CAL_PROFILE_UNVERIFIED);
    CHECK(unverified_rf.reads==0U && unverified_rf.writes==0U);
    Fake mismatch{};
    mismatch.set(0x0608U,4U,0U);
    CHECK(run(mismatch,report)==RTW_RF_CAL_PROFILE_UNVERIFIED);
    CHECK(mismatch.writes==0U);
    /* Monitor (six reads) or snapshot (three reads) failures: no writes. */
    for(unsigned position=1U;position<=9U;++position) {
        Fake f{};
        f.fail_read_at=position;
        const auto error=run(f,report);
        if (position<=6U) CHECK(error==RTW_RF_CAL_PROFILE_UNVERIFIED);
        else CHECK(error==RTW_RF_CAL_USB_ERROR);
        CHECK(f.writes==0U);
    }
    for(unsigned position=10U;position<=11U;++position) {
        Fake f{};
        const auto old_cca=f.get(0x0838U,4U);
        const auto old_hssi=f.get(0x08B0U,4U);
        f.fail_read_at=position;
        CHECK(run(f,report)==RTW_RF_CAL_USB_ERROR);
        CHECK(report.bb_registers_restored && !report.tx_authorized);
        CHECK(f.get(0x0838U,4U)==old_cca && f.get(0x08B0U,4U)==old_hssi);
    }
    for(unsigned position=12U;position<=13U;++position) {
        Fake f{};
        f.fail_read_at=position;
        CHECK(run(f,report)==RTW_RF_CAL_RESTORE_FAILED);
        CHECK(!report.bb_registers_restored && !report.tx_authorized);
    }
    for(unsigned position=1U;position<=5U;++position) {
        Fake f{};
        f.fail_write_at=position;
        auto error=run(f,report);
        if(position>=4U) CHECK(error==RTW_RF_CAL_RESTORE_FAILED);
        else CHECK(error==RTW_RF_CAL_USB_ERROR);
        CHECK(!report.tx_authorized);
        if(position<=3U) CHECK(report.bb_registers_restored);
    }
    for(unsigned position=1U;position<=2U;++position) {
        Fake f{};
        f.fail_wait_at=position;
        CHECK(run(f,report)==RTW_RF_CAL_DELAY_FAILED);
        CHECK(report.bb_registers_restored && !report.tx_authorized);
    }
    Fake corrupted{};
    corrupted.ignore_hssi_restore=true;
    CHECK(run(corrupted,report)==RTW_RF_CAL_RESTORE_FAILED);
    CHECK(!report.bb_registers_restored && !report.tx_authorized);
    /* Invalid pointers must not perform I/O. */
    rtw_usb_rw_transport bus{&denied,usb_in,usb_out};
    rtw_waiter timer{&denied,wait_for_rf};
    auto good=good_boot(); auto mon=good_monitor(); auto channel=good_rf();
    CHECK(rtw_rf_thermal_preflight_8812au(
        &bus,&timer,&good,&mon,&channel,1,nullptr)==RTW_RF_CAL_BAD_ARGUMENT);
    CHECK(rtw_rf_thermal_preflight_8812au(
        nullptr,&timer,&good,&mon,&channel,1,&report)==RTW_RF_CAL_BAD_ARGUMENT);
    CHECK(rtw_rf_thermal_preflight_8812au(
        &bus,&timer,&good,&mon,nullptr,1,&report)==RTW_RF_CAL_BAD_ARGUMENT);
}
} // namespace
int main() {
    valid_and_factory_baseline();
    unavailable_or_unstable();
    authorization_and_failures();
    std::printf("RTL8812AU thermal preflight: %lu checks passed "
                "(mock only; IQK and TX always disabled)\n",checks);
}
