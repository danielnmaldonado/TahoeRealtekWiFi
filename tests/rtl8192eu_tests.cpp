/* SPDX-License-Identifier: GPL-2.0-only. RTL8192EU portable no-hardware tests. */
#include "rtw/RTL8192EU.h"
#include "rtw/USBIDs.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static size_t checks=0;
#define CHECK(x) do {++checks; if (!(x)) {std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);std::abort();}} while(0)
namespace {
constexpr uint16_t VID=RTW_8192EU_USB_VID,PID=RTW_8192EU_USB_PID;
struct Mock {
    uint32_t cfg=0x04011234U;
    uint32_t fsm=0x00400000U;
    uint32_t cfg2=0x00000113U;
    uint32_t fw=0x000604C6U;
    unsigned read_count=0;
    unsigned fail_at=0;
    bool invalid_setup=false;
    bool short_transfer=false;
};
rtw_usb_error read(void* c,const rtw_usb_setup* setup,uint8_t* data,uint16_t* actual) {
    auto& m=*static_cast<Mock*>(c);
    m.read_count++;
    if (!setup || !data || !actual || setup->bm_request_type!=0xc0 ||
        setup->b_request!=0x05 || setup->w_index!=0 || setup->w_length!=4 ||
        setup->timeout_ms==0 || setup->timeout_ms>3000) {
        m.invalid_setup=true;return RTW_USB_INVALID_ARGUMENT;
    }
    if (m.fail_at==m.read_count) return RTW_USB_IO_ERROR;
    uint32_t value=0;
    switch(setup->w_value) {
        case RTW_8192EU_REG_SYS_CFG: value=m.cfg; break;
        case RTW_8192EU_REG_HPON_FSM: value=m.fsm; break;
        case RTW_8192EU_REG_SYS_CFG2: value=m.cfg2; break;
        case RTW_8192EU_REG_MCU_FW_DL: value=m.fw; break;
        default: m.invalid_setup=true;return RTW_USB_INVALID_REGISTER;
    }
    for(unsigned i=0;i<4;++i) data[i]=(uint8_t)(value>>(8*i));
    *actual=(uint16_t)(m.short_transfer?3:4);
    return RTW_USB_OK;
}
void id_and_probe() {
    CHECK(rtw::identifyChip(VID,PID)==rtw::ChipFamily::RTL8192EU_Candidate);
    CHECK(rtw::identifyChip(0x0bda,0x8812)==rtw::ChipFamily::RTL8812AU);
    CHECK(rtw::identifyChip(0x2357,0x0001)==rtw::ChipFamily::NotRealtek);
    CHECK(rtw::identifyChip(0x0bda,0xbeef)==rtw::ChipFamily::UnknownRealtek);
    CHECK(std::strstr(rtw::toString(rtw::identifyChip(VID,PID)),"RTL8192EU")!=nullptr);
    rtw_8192eu_identity id{};
    CHECK(rtw_8192eu_decode_identity(VID,PID,0x04011234,0,0x00400000,0,&id)==RTW_8192EU_OK);
    CHECK(id.is_8191eu_bonding==1 && id.inferred_tx_paths==1 && id.silicon_cut==1);
    CHECK(id.foundry_code==0 && id.vid==VID);
    CHECK(rtw_8192eu_decode_identity(VID,PID,0x04011234,0,0,0,&id)==RTW_8192EU_OK);
    CHECK(id.inferred_tx_paths==2 && !id.is_8191eu_bonding);
    CHECK(rtw_8192eu_decode_identity(0x0bda,0x8812,0x04011234,0,0,0,&id)==RTW_8192EU_WRONG_USB_ID);
    CHECK(!id.sys_cfg && !id.pid);
    CHECK(rtw_8192eu_decode_identity(VID,PID,0,0,0,0,&id)==RTW_8192EU_IMPLAUSIBLE_REGISTER);
    CHECK(rtw_8192eu_decode_identity(VID,PID,0xffffffffU,0,0,0,&id)==RTW_8192EU_IMPLAUSIBLE_REGISTER);
    CHECK(rtw_8192eu_decode_identity(VID,PID,0x00811234U,0,0,0,&id)==RTW_8192EU_TEST_SILICON);
    CHECK(rtw_8192eu_decode_identity(VID,PID,0x04011234,UINT32_MAX,0,0,&id)==RTW_8192EU_IMPLAUSIBLE_REGISTER);
    CHECK(rtw_8192eu_decode_identity(VID,PID,0x04011234,0,UINT32_MAX,0,&id)==RTW_8192EU_IMPLAUSIBLE_REGISTER);
    CHECK(rtw_8192eu_decode_identity(VID,PID,0x04011234,0,0,UINT32_MAX,&id)==RTW_8192EU_IMPLAUSIBLE_REGISTER);
    CHECK(rtw_8192eu_decode_identity(VID,PID,0,0,0,0,nullptr)==RTW_8192EU_BAD_ARGUMENT);

    Mock m{};
    const rtw_usb_transport bus{&m,&read};
    CHECK(rtw_8192eu_probe_registers(&bus,VID,PID,&id)==RTW_8192EU_OK);
    CHECK(id.sys_cfg==m.cfg && id.sys_cfg2==m.cfg2 && id.hpon_fsm==m.fsm && id.mcu_fw_dl==m.fw);
    CHECK(m.read_count==4 && !m.invalid_setup);
    m.read_count=0;
    CHECK(rtw_8192eu_probe_registers(&bus,0x0bda,0x8812,&id)==RTW_8192EU_WRONG_USB_ID);
    CHECK(m.read_count==0 && id.sys_cfg==0);
    for(unsigned failure=1;failure<=4;++failure) {
        m.read_count=0;m.fail_at=failure;
        CHECK(rtw_8192eu_probe_registers(&bus,VID,PID,&id)==RTW_8192EU_USB_READ_FAILED);
        CHECK(m.read_count==failure && !id.sys_cfg);
    }
    m.fail_at=0; m.short_transfer=true; m.read_count=0;
    CHECK(rtw_8192eu_probe_registers(&bus,VID,PID,&id)==RTW_8192EU_USB_READ_FAILED);
    CHECK(m.read_count==1);
    CHECK(rtw_8192eu_probe_registers(nullptr,VID,PID,&id)==RTW_8192EU_BAD_ARGUMENT);
    CHECK(rtw_8192eu_probe_registers(&bus,VID,PID,nullptr)==RTW_8192EU_BAD_ARGUMENT);
}
void efuse() {
    std::array<uint8_t,512> logical{};logical.fill(0xff);
    logical[0]=0x29;logical[1]=0x81;
    logical[0xb8]=0x12;logical[0xb9]=0xfd;logical[0xba]=0x77;
    logical[0xc1]=0x04;
    constexpr uint8_t mac[6]={0x02,0x25,0x37,0x81,0x92,0xe0};
    std::memcpy(logical.data()+0xd7,mac,6);
    rtw_8192eu_efuse_identity id{};
    CHECK(rtw_8192eu_parse_logical_efuse(logical.data(),logical.size(),&id)==RTW_8192EU_OK);
    CHECK(id.rtl_id==0x8129 && !std::memcmp(id.mac,mac,6));
    CHECK(id.channel_plan==0x12 && id.crystal_cap==0x3d && id.thermal_meter==0x77 && id.rf_board_option==4);
    CHECK(rtw_8192eu_parse_logical_efuse(logical.data(),511,&id)==RTW_8192EU_BAD_ARGUMENT);
    CHECK(!id.rtl_id);
    logical[0]=0x12;
    CHECK(rtw_8192eu_parse_logical_efuse(logical.data(),logical.size(),&id)==RTW_8192EU_BAD_EFUSE);
    logical[0]=0x29;
    logical[0xd7]=0x03;
    CHECK(rtw_8192eu_parse_logical_efuse(logical.data(),logical.size(),&id)==RTW_8192EU_BAD_EFUSE);
    std::memcpy(logical.data()+0xd7,mac,6);
    std::memset(logical.data()+0xd7,0,6);
    CHECK(rtw_8192eu_parse_logical_efuse(logical.data(),logical.size(),&id)==RTW_8192EU_BAD_EFUSE);
    std::memcpy(logical.data()+0xd7,mac,6);
    CHECK(rtw_8192eu_parse_logical_efuse(nullptr,512,&id)==RTW_8192EU_BAD_ARGUMENT);
    // Packed map with extended header blocks 23,24,26,27 and valid MAC span.
    const std::array<uint8_t,27> packed={
        0x0e,0x29,0x81,
        0xef,0x2c,0x12,0xfd,0x77,0xff,
        0x0f,0x3e,0xff,0x04,
        0x4f,0x37,0xff,0x02,
        0x6f,0x30,0x25,0x37,0x81,0x92,0xe0,0xff,0xff,0xff
    };
    CHECK(rtw_8192eu_parse_physical_efuse(packed.data(),packed.size(),&id)==RTW_8192EU_OK);
    CHECK(!std::memcmp(id.mac,mac,6) && id.rtl_id==0x8129);
    CHECK(id.channel_plan==0x12 && id.crystal_cap==0x3d && id.rf_board_option==4);
    CHECK(rtw_8192eu_parse_physical_efuse(packed.data(),0,&id)==RTW_8192EU_BAD_ARGUMENT);
    CHECK(rtw_8192eu_parse_physical_efuse(packed.data(),4,&id)==RTW_8192EU_BAD_EFUSE);
    CHECK(rtw_8192eu_parse_physical_efuse(packed.data(),packed.size(),nullptr)==RTW_8192EU_BAD_ARGUMENT);
}
void firmware() {
    std::vector<uint8_t> data(32+4096,0);
    data[0]=0xe1;data[1]=0x92;data[2]=1;data[3]=2;
    data[4]=0x22;data[5]=0x00;data[6]=0x0c;
    data[12]=0x00;data[13]=0x10;data[16]=0x78;data[17]=0x56;
    data[32]=0xab;
    rtw_8192eu_fw_info fw{};
    CHECK(rtw_8192eu_inspect_firmware(data.data(),data.size(),&fw)==RTW_8192EU_OK);
    CHECK(fw.signature==0x92e1 && fw.major_version==34 && fw.minor_version==12);
    CHECK(fw.payload_length==4096 && fw.payload==data.data()+32 && fw.payload[0]==0xab);
    CHECK(fw.reported_ramcode_size==4096 && fw.svn_index==0x5678);
    data[1]=0x95;
    CHECK(rtw_8192eu_inspect_firmware(data.data(),data.size(),&fw)==RTW_8192EU_BAD_FIRMWARE);
    CHECK(!fw.signature);
    data[1]=0x92;
    data[0]=0xc0;
    CHECK(rtw_8192eu_inspect_firmware(data.data(),data.size(),&fw)==RTW_8192EU_BAD_FIRMWARE);
    data[0]=0xe1;
    CHECK(rtw_8192eu_inspect_firmware(data.data(),32,&fw)==RTW_8192EU_BAD_FIRMWARE);
    data.resize(32+32769);
    CHECK(rtw_8192eu_inspect_firmware(data.data(),data.size(),&fw)==RTW_8192EU_OVERSIZED_FIRMWARE);
    data.resize(32+4096);
    data[12]=0xff;data[13]=0xff;
    CHECK(rtw_8192eu_inspect_firmware(data.data(),data.size(),&fw)==RTW_8192EU_BAD_FIRMWARE);
    CHECK(rtw_8192eu_inspect_firmware(nullptr,0,&fw)==RTW_8192EU_BAD_ARGUMENT);
    CHECK(rtw_8192eu_inspect_firmware(data.data(),data.size(),nullptr)==RTW_8192EU_BAD_ARGUMENT);
}
void rx() {
    std::array<uint8_t,48> transfer{};
    // 24byte descriptor, 8byte PHY, 3byte frame, aligned to 8 => 40 bytes.
    const uint32_t word0=3U | (1U<<16U) | (4U<<20U) | (1U<<23U) | (1U<<26U) | (1U<<27U);
    const uint32_t word1=1U<<27U;
    const uint32_t word2=0x00000021U | (1U<<28U);
    auto put32=[](uint8_t* d,uint32_t v) {for(unsigned i=0;i<4;++i)d[i]=uint8_t(v>>(i*8U));};
    put32(transfer.data(),word0);
    put32(transfer.data()+4,word1);
    put32(transfer.data()+8,word2);
    put32(transfer.data()+12,0x04);
    transfer[24]=0xfa;transfer[32]=0x80;transfer[33]=0x01;transfer[34]=0x00;
    rtw_rx_iterator it{};
    rtw_rx_iterator_init(&it,transfer.data(),40);
    rtw_8192eu_rx_frame frame{};
    CHECK(rtw_8192eu_rx_next(&it,&frame)==RTW_PACKETS_OK);
    CHECK(frame.report_is_c2h==1 && frame.security_code==4 && frame.sw_decrypt_required==1);
    CHECK(frame.qos==1 && frame.fragmented==1 && frame.sequence==0x21 && frame.fragment_number==0);
    CHECK(frame.payload.mpdu_length==3 && frame.payload.phy_info_length==8);
    CHECK(frame.payload.mpdu[0]==0x80 && frame.payload.phy_info[0]==0xfa);
    CHECK(rtw_8192eu_rx_next(&it,&frame)==RTW_PACKETS_DONE);
    CHECK(rtw_8192eu_rx_next(&it,nullptr)==RTW_PACKETS_BAD_ARGUMENT);
    std::array<uint8_t,96> random{};
    uint32_t seed=0x71e29a0bU;
    for(unsigned i=0;i<20000;++i) {
        for(auto& b:random){seed^=seed<<13U;seed^=seed>>17U;seed^=seed<<5U;b=uint8_t(seed);}
        rtw_rx_iterator_init(&it,random.data(),i%(random.size()+1U));
        const auto result=rtw_8192eu_rx_next(&it,&frame);
        CHECK(result!=RTW_PACKETS_BAD_ARGUMENT);
        CHECK(it.cursor<=it.length);
    }
}
}
int main(){id_and_probe();efuse();firmware();rx();
    std::printf("rtl8192eu-tests: %zu assertions, read-only/mock/offline, no hardware\n",checks);}
