/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/RTL8192DUUSB.h"
#include "rtw/RTL8192DUStation.h"
#include "rtl8192du_fixture.hpp"
using namespace du_test;
static std::vector<uint8_t> configuration() {
    auto p=plan();return {9,2,62,0,2,p.profile->configuration,0,0x80,50,
        9,4,0,0,2,0xff,0xff,0xff,0,7,5,0x81,2,0,2,0,7,5,2,2,0,2,0,
        9,4,1,0,3,0xff,0xff,0xff,0,7,5,0x84,2,0,2,0,7,5,5,2,0,2,0,7,5,6,2,0,2,0};
}
int main() {
    const auto primary=plan();const auto cfg=configuration();
    for(uint8_t index:{uint8_t(0),uint8_t(1)}) {
        Fixture f(true,index);rtw_usb_endpoints endpoints{};
        assert(rtw_92du_usb_interface_endpoints(&primary,&f.identity.board,cfg.data(),cfg.size(),&endpoints)==RTW_92DU_OK);
        assert(endpoints.interface_number==index && endpoints.num_bulk_in==1 && endpoints.num_bulk_out==index+1);
        assert(endpoints.bulk_in[0]==(index?0x84:0x81) && endpoints.bulk_out[0]==(index?5:2));
        rtw_92du_mac mac{};f.boot.firmware_ready=0;
        assert(rtw_92du_mac_init_for_interface(&mac,&f.boot,&primary,cfg.data(),cfg.size())==RTW_92DU_OK);
        assert(mac.out_pipes==index+1 && !f.bus.io);
        assert(rtw_92du_mac_prepare(&mac)==RTW_92DU_OK);
        assert(f.bus.llt[122]==255 && f.bus.llt[127]==123);
        f.boot.firmware_ready=1;
        assert(rtw_92du_mac_start(&mac)==RTW_92DU_OK);
        const uint8_t logical_station[]={2,0x11,0x22,0x33,0x44,0x55};
        const auto factory=f.identity.board;
        assert(rtw_92du_mac_set_address(&mac,logical_station)==RTW_92DU_OK);
        assert(!memcmp(f.bus.regs.data()+0x610,logical_station,6) && !memcmp(mac.station_address,logical_station,6));
        assert(!memcmp(&factory,&f.identity.board,sizeof(factory)) && !memcmp(f.boot.board.mac,factory.mac,6));
        uint8_t invalid[6]{};const auto before=f.bus.io;
        assert(rtw_92du_mac_set_address(&mac,invalid)==RTW_92DU_BAD_ARGUMENT && f.bus.io==before);
        invalid[0]=1;
        assert(rtw_92du_mac_set_address(&mac,invalid)==RTW_92DU_BAD_ARGUMENT && f.bus.io==before);
        assert(rtw_92du_phy_load_tables(&f.boot)==RTW_92DU_OK);
        assert(rtw_92du_phy_set_channel(&f.boot,index?1:36)==RTW_92DU_OK);
        assert(rtw_92du_phy_calibrate(&f.boot)==RTW_92DU_OK && rtw_92du_phy_activate(&f.boot)==RTW_92DU_OK);
        const auto active=f.bus.io;
        assert(rtw_92du_mac_set_address(&mac,logical_station)==RTW_92DU_WRONG_STATE && f.bus.io==active);
    }
    const auto board=rom(true,1).board;
    for(size_t truncated=0;truncated<cfg.size();++truncated) {
        rtw_usb_endpoints ep{};
        assert(rtw_92du_usb_interface_endpoints(&primary,&board,cfg.data(),truncated,&ep)!=RTW_92DU_OK);
    }
    // Secondary class, alt, interface identity, address collision and count
    // are independent physical facts, not assertions borrowed from profile0.
    for(unsigned offset:{4U,34U,35U,37U,43U,50U,57U}) {
        auto bad=cfg;bad[offset]=offset==4?1:offset==34?0:offset==35?1:offset==37?0:offset==43?0x81:2;
        rtw_usb_endpoints ep{};
        assert(rtw_92du_usb_interface_endpoints(&primary,&board,bad.data(),bad.size(),&ep)!=RTW_92DU_OK);
    }
    auto forged=primary;forged.endpoints.bulk_out[0]=3;rtw_usb_endpoints ep{};
    assert(rtw_92du_usb_interface_endpoints(&forged,&board,cfg.data(),cfg.size(),&ep)==RTW_92DU_IDENTITY_UNVERIFIED);
    {
        auto bad=cfg;bad[2]+=7;bad[36]++;bad.insert(bad.end(),{7,5,0x81,3,8,0,1});
        // An interrupt endpoint may not alias the other MAC's bulk address.
        assert(rtw_92du_usb_interface_endpoints(&primary,&board,bad.data(),bad.size(),&ep)==RTW_92DU_BAD_PROM);
    }
    // Any partial station-address write poisons TX and never changes host
    // address metadata to suggest a complete hardware address was accepted.
    for(unsigned fault=1;fault<=6;++fault) {
        Fixture f;rtw_92du_mac mac{};f.boot.firmware_ready=0;
        assert(rtw_92du_mac_init(&mac,&f.boot,&primary)==RTW_92DU_OK);
        assert(rtw_92du_mac_prepare(&mac)==RTW_92DU_OK);f.boot.firmware_ready=1;
        assert(rtw_92du_mac_start(&mac)==RTW_92DU_OK);
        const auto before=mac;f.bus.short_at=f.bus.io+fault;const uint8_t address[]={2,1,2,3,4,5};
        assert(rtw_92du_mac_set_address(&mac,address)==RTW_92DU_IO_ERROR);
        assert(!mac.rate_ready && !mac.joined && f.bus.regs[0x522]==255 && !memcmp(mac.station_address,before.station_address,6));
    }
    puts("RTL8192DU two-interface endpoint proofs, one-stream radio domains and stable station address verified");
}
