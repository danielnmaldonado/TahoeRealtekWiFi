/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/RTL8192DUStation.h"
#include "rtw/RTL8192DUUSB.h"
#include "rtl8192du_fixture.hpp"
using namespace du_test;
static rtw_92du_result bootstrap(Fixture& f,rtw_92du_mac& mac) {
    auto p=plan();f.boot.firmware_ready=0;
    auto result=rtw_92du_mac_init(&mac,&f.boot,&p);
    if(result==RTW_92DU_OK)result=rtw_92du_mac_prepare(&mac);
    if(result==RTW_92DU_OK){
        f.boot.firmware_ready=1;f.boot.firmware_version=38;f.bus.regs[0x80]=2;f.bus.regs[3]|=4;
        f.bus.regs[0x10c]|=15; // stale aggregation plus unrelated low control bits
        result=rtw_92du_mac_start(&mac);
        if(result==RTW_92DU_OK)assert((f.bus.regs[0x10c]&15)==11);
    }return result;
}
static void start(Fixture& f,rtw_92du_mac& mac) {
    assert(bootstrap(f,mac)==RTW_92DU_OK);
    assert(f.bus.llt[248]==0xff && f.bus.llt[255]==249 && f.bus.llt[249]==250);

    assert(f.bus.regs[0x522]==0xff && !f.bus.regs[0x680] && !f.bus.regs[0x423]);
    assert(!control_admitted(f.bus,8)&&!control_admitted(f.bus,10));
}
static void checked_cleanup(){Fixture f;rtw_92du_mac c{};start(f,c);f.boot.last_error=RTW_92DU_POLL_TIMEOUT;assert(rtw_92du_mac_stop_checked(&c)==RTW_92DU_OK&&f.boot.last_error==RTW_92DU_POLL_TIMEOUT&&!f.boot.power_ready);Fixture bad;rtw_92du_mac failed{};start(bad,failed);bad.bus.fail_at=bad.bus.io+1;assert(rtw_92du_mac_stop_checked(&failed)==RTW_92DU_IO_ERROR);bad.bus.fail_at=0;assert(rtw_92du_mac_stop_checked(&failed)==RTW_92DU_IO_ERROR);}
static void advisory_rssi(){for(unsigned box=0;box<4;++box){
 Fixture f;rtw_92du_mac c{};start(f,c);c.joined=c.rate_ready=1;c.next_h2c=uint8_t(box);
 f.bus.h2c_busy=15;const auto writes=f.bus.writes.size();
 assert(rtw_92du_mac_report_rssi(&c,58)==RTW_92DU_MCU_BUSY);
 assert(f.boot.last_error==RTW_92DU_OK&&c.joined&&c.rate_ready&&c.next_h2c==box&&f.bus.writes.size()==writes);
 f.bus.h2c_busy=0;assert(rtw_92du_mac_report_rssi(&c,58)==RTW_92DU_OK&&c.next_h2c==(box+1)%4);
 for(bool short_io:{false,true})for(unsigned position=1;position<=5;++position){
  Fixture bad;rtw_92du_mac m{};start(bad,m);m.joined=m.rate_ready=1;
  if(short_io)bad.bus.short_at=bad.bus.io+position;else bad.bus.fail_at=bad.bus.io+position;
  assert(rtw_92du_mac_report_rssi(&m,58)==RTW_92DU_IO_ERROR&&bad.boot.last_error==RTW_92DU_IO_ERROR&&!m.joined&&!m.rate_ready&&!m.next_h2c);}
 }std::puts("DU optional RSSI: all boxes preserve busy station/retry; failed/short mailbox reads and writes stay fatal");}
int main() {advisory_rssi();checked_cleanup();
    for(uint8_t index:{uint8_t(0),uint8_t(1)}) {
        for(auto status:std::array<std::array<uint8_t,2>,6>{{{{2,1}},{{6,1}},{{0,1}},{{3,1}},{{2,0}},{{0xff,0xff}}}}) {
            Fixture live(index!=0,index);rtw_92du_mac c{};rtw_92du_usb_map mapped{};
            const auto raw=live.boot.bus;assert(rtw_92du_usb_map_init(&mapped,&raw,index)==RTW_USB_OK);
            live.boot.bus=*rtw_92du_usb_bus(&mapped);live.boot.firmware_ready=1;
            c.boot=&live.boot;c.out_pipes=1;c.started=c.rate_ready=c.joined=1;
            const unsigned base=index?0x4000:0;
            live.bus.regs[base+0x80]=status[0];live.bus.regs[base+(index?0x1a:0x18)]=status[1];
            live.bus.regs[base+(index?0x18:0x1a)]=1;live.bus.regs[base+0x81]=0xff;
            const bool expected=(status[0]&3)==2 && (status[1]&1);
            assert(rtw_92du_mac_health(&c)==(expected?RTW_92DU_OK:RTW_92DU_IO_ERROR));
            assert(bool(live.boot.firmware_ready)==expected&&bool(c.rate_ready)==expected&&bool(c.joined)==expected);
            if(!expected)assert(live.bus.regs[base+0x522]==0xff);
        }
        for(bool short_io:{false,true})for(unsigned fault:{1U,2U}) {
            Fixture live(index!=0,index);rtw_92du_mac c{};rtw_92du_usb_map mapped{};
            const auto raw=live.boot.bus;assert(rtw_92du_usb_map_init(&mapped,&raw,index)==RTW_USB_OK);
            live.boot.bus=*rtw_92du_usb_bus(&mapped);live.boot.firmware_ready=1;
            c.boot=&live.boot;c.out_pipes=1;c.started=c.rate_ready=c.joined=1;
            const unsigned base=index?0x4000:0;
            live.bus.regs[base+0x80]=2;live.bus.regs[base+(index?0x1a:0x18)]=1;
            const auto before=live.bus.io;
            if(short_io)live.bus.short_at=before+fault;else live.bus.fail_at=before+fault;
            assert(rtw_92du_mac_health(&c)==RTW_92DU_IO_ERROR);
            assert(!live.boot.firmware_ready&&!c.rate_ready&&!c.joined&&live.bus.io<=before+3);
        }
    }
    {
        // Normal commands remain poisoned, but bounded power cleanup must
        // clear our MAC ownership so the next clean context is not MCU_BUSY.
        Fixture failed;rtw_92du_mac c{};start(failed,c);
        failed.bus.regs[0x81]=0x80;failed.bus.h2c_busy=15;
        const uint8_t command=0;
        assert(rtw_92du_h2c(&c,2,&command,1)==RTW_92DU_POLL_TIMEOUT);
        const auto before=failed.bus.writes.size();
        assert(rtw_92du_mac_stop(&c)==RTW_92DU_POLL_TIMEOUT);
        assert(failed.bus.writes.size()>before && !(failed.bus.regs[0x81]&0x80));
        assert(failed.bus.regs[0x1c]==0x0e && failed.bus.regs[0x11]==0x23);
        assert(!c.started&&!c.prepared&&!failed.boot.power_ready&&!failed.boot.firmware_ready);
        assert(failed.boot.last_error==RTW_92DU_POLL_TIMEOUT);
        failed.bus.put(0xf0,failed.identity.board.sys_cfg);failed.bus.power_handshake=true;
        rtw_92du_boot fresh{};const auto bus=failed.boot.bus;const auto waiter=failed.boot.waiter;
        const rtw_92du_boot_options options{1,1,10};
        assert(rtw_92du_boot_init(&fresh,&failed.identity.board,&bus,&waiter,&options)==RTW_92DU_OK);
        assert(rtw_92du_power_on(&fresh)==RTW_92DU_OK&&fresh.power_ready);
        assert(failed.boot.last_error==RTW_92DU_POLL_TIMEOUT); // old command remains poisoned
    }
    for(uint8_t index:{uint8_t(0),uint8_t(1)}) {
        Fixture owned(index!=0,index);rtw_92du_mac c{};rtw_92du_usb_map mapped{};
        const auto raw=owned.boot.bus;assert(rtw_92du_usb_map_init(&mapped,&raw,index)==RTW_USB_OK);
        owned.boot.bus=*rtw_92du_usb_bus(&mapped);owned.boot.firmware_ready=1;
        c.boot=&owned.boot;c.out_pipes=1;c.started=c.prepared=1;
        const unsigned base=index?0x4000:0;
        const unsigned own=base+(index?0x53:0x81),other=base+(index?0x81:0x53);
        owned.bus.regs[own]=index?0xa1:0xa5;owned.bus.regs[other]=index?0x55:4;
        owned.bus.regs[base+0x80]=2;owned.boot.last_error=RTW_92DU_POLL_TIMEOUT;
        assert(rtw_92du_mac_stop(&c)==RTW_92DU_POLL_TIMEOUT);
        assert(owned.bus.regs[own]==(index?0xa0:0x25)&&owned.bus.regs[other]==(index?0x55:4));
        assert(owned.bus.regs[base+0x1c]==0x0e&&!c.started&&!owned.boot.power_ready);
    }
    {
        Fixture failed;rtw_92du_mac c{};start(failed,c);failed.bus.regs[0x81]=0x80;
        failed.bus.fail_at=failed.bus.io+1; // initial pause fails; later independent cleanup still runs
        assert(rtw_92du_mac_stop(&c)==RTW_92DU_IO_ERROR);
        assert(!(failed.bus.regs[0x81]&0x80)&&failed.bus.regs[0x1c]==0x0e&&!c.started);
    }
    for(bool permanent:{false,true}) {
        Fixture inaccessible;rtw_92du_mac c{};start(inaccessible,c);inaccessible.bus.regs[0x81]=0x80;
        const auto before=inaccessible.bus.io,rf=inaccessible.bus.rf_writes.size();
        if(permanent)inaccessible.bus.fail_from=before+1;
        else inaccessible.bus.fail_at=before+2; // unreadable sibling never means sibling OFF
        assert(rtw_92du_mac_stop(&c)==RTW_92DU_IO_ERROR);
        assert(inaccessible.bus.regs[0x81]==0x80&&inaccessible.bus.rf_writes.size()==rf);
        assert(!inaccessible.bus.regs[0x1c]&&!c.started&&!inaccessible.boot.power_ready);
        assert(inaccessible.bus.io<=before+3);
    }
    {
        // The catalog plan describes interface0. Applying its endpoint count
        // to MAC1 can silently program the wrong reserved queues/data pipes.
        Fixture secondary(true,1);rtw_92du_mac c{};auto primary=plan();
        const auto before=secondary.bus.io;
        assert(rtw_92du_mac_init(&c,&secondary.boot,&primary)==RTW_92DU_IDENTITY_UNVERIFIED);
        assert(secondary.bus.io==before);
    }
    Fixture f;rtw_92du_mac mac{};start(f,mac);
    assert(mac.prepared && mac.started && !mac.rate_ready && !mac.joined);
    assert((Bus::word(f.bus.regs.data()+0x608,4)&0xf0000000U)==0xf0000000U);
    assert(!f.bus.regs[0x6a4] && !f.bus.regs[0x6a5]);
    // DU uses two-byte extension at 0x88, unlike newer 4-byte extensions.
    const uint8_t data[]={0x12,0x34,0x56,0x78,0x9a};
    assert(rtw_92du_h2c(&mac,6,data,5)==RTW_92DU_OK);
    assert(f.bus.regs[0x88]==0x12 && f.bus.regs[0x89]==0x34);
    assert(f.bus.regs[0x1d0]==0x86 && f.bus.regs[0x1d1]==0x56 && f.bus.regs[0x1d3]==0x9a);
    assert(mac.next_h2c==1);
    const auto before=f.bus.io;
    assert(rtw_92du_h2c(&mac,6,data,6)==RTW_92DU_BAD_ARGUMENT && f.bus.io==before);
    f.bus.h2c_busy=2;
    assert(rtw_92du_h2c(&mac,6,data,5)==RTW_92DU_POLL_TIMEOUT && !mac.rate_ready && !mac.joined);
    {
        Fixture active;rtw_92du_mac c{};start(active,c);active.bus.regs[0x53]=1;
        const auto radio_writes=active.bus.rf_writes.size();
        assert(rtw_92du_mac_stop(&c)==RTW_92DU_MCU_BUSY && active.bus.rf_writes.size()==radio_writes);
        assert(active.bus.regs[0x53]==1 && !c.started && active.bus.regs[0x522]==0xff);
    }
    for(uint8_t channel:{uint8_t(1),uint8_t(36),uint8_t(165)}) {
        Fixture radio;rtw_92du_mac c{};start(radio,c);
        assert(rtw_92du_phy_load_tables(&radio.boot)==RTW_92DU_OK);
        assert(rtw_92du_phy_set_channel(&radio.boot,channel)==RTW_92DU_OK);
        assert(rtw_92du_phy_calibrate(&radio.boot)==RTW_92DU_OK);
        rtw_92du_peer_rates peer{uint16_t(channel>14?0xff0:0xfff),0xffff,1,1,3};
        rtw_92du_rate_config rates{};
        assert(rtw_92du_build_rate_config(&radio.boot,channel,&peer,&rates)==RTW_92DU_OK);
        assert(rates.initial_rate==27 && rates.ht && rates.sgi20 && rates.raid==(channel>14?1:0));
        assert(channel<14 || !(rates.mask&15));
        rtw_92du_bss bss{};bss.channel=channel;bss.bssid[0]=2;bss.bssid[5]=4;
        bss.aid=42;bss.beacon_interval=100;bss.basic_rates=channel>14?0x150:0xf;bss.short_slot=1;
        rtw_92du_edca_ac edca[4]{};for(auto& ac:edca){ac.aifsn=2;ac.ecwmin=3;ac.ecwmax=4;ac.txop=5;}
        assert(rtw_92du_mac_edca(&c,channel,0,edca)==RTW_92DU_OK);
        assert(radio.bus.regs[0x51b]==(channel>14?9:20));
        assert(Bus::word(radio.bus.regs.data()+0x508,4)==(channel>14?0x00054322U:0x00054332U));
        const auto edca_before=radio.bus.io;edca[3].ecwmin=15;edca[3].ecwmax=2;
        assert(rtw_92du_mac_edca(&c,channel,0,edca)==RTW_92DU_BAD_ARGUMENT && radio.bus.io==edca_before);
        assert(rtw_92du_mac_set_state(&c,RTW_92DU_ST_RUN,&bss)==RTW_92DU_WRONG_STATE);
        assert(rtw_92du_mac_set_state(&c,RTW_92DU_ST_AUTH,&bss)==RTW_92DU_OK);
        assert(rtw_92du_phy_activate(&radio.boot)==RTW_92DU_OK && rtw_92du_phy_tx_active(&radio.boot));
        assert(rtw_92du_mac_set_rates(&c,&rates)==RTW_92DU_OK && c.rate_ready);
        assert(rtw_92du_mac_set_state(&c,RTW_92DU_ST_RUN,&bss)==RTW_92DU_OK && c.joined);
        assert(control_admitted(radio.bus,8)&&!control_admitted(radio.bus,10));
        assert(!(Bus::word(radio.bus.regs.data()+0x608,4)&((1U<<12)|1U)));
        assert((Bus::word(radio.bus.regs.data()+0x608,4)&0xc0U)==0xc0U&&!memcmp(c.bssid,bss.bssid,6));
        const rtw_92du_peer_rates legacy{peer.legacy,0,0,0,3};rtw_92du_rate_config legacy_rates{};
        assert(rtw_92du_build_rate_config(&radio.boot,channel,&legacy,&legacy_rates)==RTW_92DU_OK);
        assert(rtw_92du_mac_set_rates(&c,&legacy_rates)==RTW_92DU_OK);
        assert(rtw_92du_mac_set_state(&c,RTW_92DU_ST_RUN,&bss)==RTW_92DU_OK);
        assert(!control_admitted(radio.bus,8)&&!control_admitted(radio.bus,10));
        assert(rtw_92du_mac_set_rates(&c,&rates)==RTW_92DU_OK);
        assert(rtw_92du_mac_set_state(&c,RTW_92DU_ST_RUN,&bss)==RTW_92DU_OK&&control_admitted(radio.bus,8));
        assert((radio.bus.regs[0x102]&3)==2 && radio.bus.regs[0x6a4]==0xff);
        assert(rtw_92du_mac_report_rssi(&c,58)==RTW_92DU_OK);
        assert(rtw_92du_mac_set_state(&c,RTW_92DU_ST_INIT,nullptr)==RTW_92DU_OK && !c.joined && !c.rate_ready);
        assert(!control_admitted(radio.bus,8)&&!control_admitted(radio.bus,10));
        assert(!(radio.bus.regs[0x102]&3) && !radio.bus.regs[0x6a4]);
        assert(!rtw_92du_phy_tx_active(&radio.boot) && radio.bus.regs[0x522]==0xff);
        assert(rtw_92du_mac_stop(&c)==RTW_92DU_OK && !radio.boot.firmware_ready && !radio.boot.power_ready);
        assert(!c.started && !c.prepared && radio.bus.regs[0x1c]==0xe && radio.bus.regs[0x11]==0x23);
    }
    size_t startup_io=0,failures=0;
    {Fixture healthy;rtw_92du_mac c{};assert(bootstrap(healthy,c)==RTW_92DU_OK);startup_io=healthy.bus.io;}
    for(bool short_io:{false,true})for(size_t fault=1;fault<=startup_io;++fault) {
        Fixture failed;rtw_92du_mac c{};
        if(short_io)failed.bus.short_at=fault;else failed.bus.fail_at=fault;
        assert(bootstrap(failed,c)==RTW_92DU_IO_ERROR);
        assert(!c.started&&!c.rate_ready&&!c.joined&&failed.bus.regs[0x522]==0xff);
        assert(failed.bus.io<=fault+2);const auto stopped=failed.bus.io;
        assert(rtw_92du_h2c(&c,6,data,5)!=RTW_92DU_OK&&failed.bus.io==stopped);++failures;
    }
    for(bool llt:{false,true}) {
        Fixture stalled;rtw_92du_mac c{};
        if(llt)stalled.bus.llt_timeout=true;else stalled.bus.cam_timeout=true;
        assert(bootstrap(stalled,c)==RTW_92DU_POLL_TIMEOUT&&!c.started&&!c.rate_ready);
        assert(stalled.bus.regs[0x522]==0xff&&stalled.bus.io<startup_io+30);
    }
    {
        Fixture stuck;rtw_92du_mac c{};start(stuck,c);stuck.bus.cpu_stuck=true;
        const auto stop_begin=stuck.bus.io;
        assert(rtw_92du_mac_stop(&c)==RTW_92DU_OK&&!c.started&&stuck.bus.regs[3]==0x54);
        assert(stuck.bus.io-stop_begin<180);
    }
    std::printf("RTL8192DU station bootstrap: %zu USB operations, %zu I/O/short faults verified\n",startup_io,failures);
    puts("RTL8192DU LLT/MAC/H2C/HT20 rates/firmware JOIN/disconnect/power-off passed");
}
