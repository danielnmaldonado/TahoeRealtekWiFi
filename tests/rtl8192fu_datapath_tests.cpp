#include "rtw/RTL8192FUDataPath.h"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <vector>

static unsigned long checks = 0;
#define CHECK(x) do { ++checks; if (!(x)) { \
    std::cerr << "FAIL " << __FILE__ << ':' << __LINE__ << ": " << #x << '\n'; \
    std::exit(1); } } while (0)
static void word(uint8_t *p, uint32_t v) {
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(v >> (8U*i));
}
static uint32_t word(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8U) |
           ((uint32_t)p[2] << 16U) | ((uint32_t)p[3] << 24U);
}
static std::vector<uint8_t> rx_frame(size_t count = 25, bool phy = false) {
    const size_t len = RTW_R92FU_RX_DESC_BYTES + (phy ? 32U : 0U) + count;
    std::vector<uint8_t> out((len + 7U) & ~size_t(7U), 0);
    word(out.data(), (uint32_t)count | (phy ? 0x04040000U : 0U));
    word(out.data()+8U, 0U);
    word(out.data()+12U, 4U);
    word(out.data()+16U, 0U);
    word(out.data()+20U, 0x01020304U);
    for (size_t i=0; i<count; ++i)
        out[RTW_R92FU_RX_DESC_BYTES+(phy?32U:0U)+i]=(uint8_t)(i+1U);
    return out;
}
static void rx_tests() {
    CHECK(rtw_r92fu_rate_valid(0,1));
    CHECK(rtw_r92fu_rate_valid(19,1) && !rtw_r92fu_rate_valid(20,1));
    CHECK(rtw_r92fu_rate_valid(27,2) && !rtw_r92fu_rate_valid(28,2));
    CHECK(!rtw_r92fu_rate_valid(53,1) && !rtw_r92fu_rate_valid(54,1));
    CHECK(!rtw_r92fu_rate_valid(63,2) && !rtw_r92fu_rate_valid(64,2));
    CHECK(!rtw_r92fu_rate_valid(4,3));
    CHECK(!rtw_r92fu_rate_valid(4,0));
    {
        auto data=rx_frame(25,true);
        rtw_r92fu_rx_iterator it{data.data(),data.size(),0,0};
        rtw_r92fu_rx_frame frame{};
        CHECK(rtw_r92fu_rx_next(&it,2,&frame)==RTW_R92FU_FRAME);
        CHECK(frame.phy && frame.phy_length==32);
        CHECK(frame.payload_length==25 && frame.payload[0]==1);
        CHECK(frame.timestamp==0x01020304U && frame.bandwidth_mhz==20);
        CHECK(!frame.is_c2h && frame.rate==4 && !frame.crc_error);
        CHECK(rtw_r92fu_rx_next(&it,2,&frame)==RTW_R92FU_END);
    }
    {
        auto data=rx_frame(24,true);
        const size_t header=RTW_R92FU_RX_DESC_BYTES;
        data.insert(data.begin()+header+32U,2U,0x00);
        word(data.data(),word(data.data()) | (2U<<24U));
        data.resize((data.size()+7U)&~size_t(7U));
        data[header]=0x9a;
        data[header+1U]=0x7b;
        rtw_r92fu_rx_iterator it{data.data(),data.size(),0,0};
        rtw_r92fu_rx_frame frame{};
        CHECK(rtw_r92fu_rx_next(&it,1,&frame)==RTW_R92FU_FRAME);
        CHECK(frame.phy==data.data()+header);
        CHECK(frame.phy[0]==0x9a && frame.phy[1]==0x7b);
        CHECK(frame.payload==data.data()+header+34U);
        CHECK(frame.payload[0]==1U);
    }
    {
        auto first=rx_frame();
        auto second=rx_frame(12);
        first.insert(first.end(),second.begin(),second.end());
        rtw_r92fu_rx_iterator it{first.data(),first.size(),0,0};
        rtw_r92fu_rx_frame frame{};
        CHECK(rtw_r92fu_rx_next(&it,1,&frame)==RTW_R92FU_FRAME);
        CHECK(frame.payload_length==25);
        CHECK(rtw_r92fu_rx_next(&it,1,&frame)==RTW_R92FU_FRAME);
        CHECK(frame.payload_length==12);
        CHECK(rtw_r92fu_rx_next(&it,1,&frame)==RTW_R92FU_END);
    }
    {
        std::array<uint8_t,32> bytes{};
        word(bytes.data(),2);
        word(bytes.data()+8,0x10000000U);
        bytes[24]=0x03; bytes[25]=0x7e;
        rtw_r92fu_rx_iterator it{bytes.data(),bytes.size(),0,0};
        rtw_r92fu_rx_frame frame{};
        CHECK(rtw_r92fu_rx_next(&it,1,&frame)==RTW_R92FU_FRAME);
        CHECK(frame.is_c2h && frame.c2h_id==3 && frame.c2h_sequence==0x7e);
    }
    {
        auto bytes=rx_frame(24);
        rtw_r92fu_rx_iterator it{bytes.data(),bytes.size(),0,0};
        rtw_r92fu_rx_frame frame{};
        word(bytes.data()+12U,54U);
        CHECK(rtw_r92fu_rx_next(&it,1,&frame)==RTW_R92FU_MALFORMED && it.cursor==0);
        word(bytes.data()+12U,4U);
        word(bytes.data(),24U|0x04000000U);
        CHECK(rtw_r92fu_rx_next(&it,1,&frame)==RTW_R92FU_MALFORMED);
        word(bytes.data(),24U);
        word(bytes.data()+16U,0x30U);
        CHECK(rtw_r92fu_rx_next(&it,1,&frame)==RTW_R92FU_MALFORMED);
        word(bytes.data()+16U,0U);
        it.length=30;
        CHECK(rtw_r92fu_rx_next(&it,1,&frame)==RTW_R92FU_TRUNCATED);
        it.length=bytes.size();
        it.cursor=it.length+1U;
        CHECK(rtw_r92fu_rx_next(&it,1,&frame)==RTW_R92FU_BAD_ARGUMENT);
        it.cursor=0;it.frames=RTW_R92FU_MAX_RX_FRAMES;
        CHECK(rtw_r92fu_rx_next(&it,1,&frame)==RTW_R92FU_TOO_MANY);
    }
    {
        auto bytes=rx_frame();
        rtw_r92fu_rx_iterator it{bytes.data(),bytes.size(),0,0};
        rtw_r92fu_rx_frame frame{};
        CHECK(rtw_r92fu_rx_next(nullptr,1,&frame)==RTW_R92FU_BAD_ARGUMENT);
        CHECK(rtw_r92fu_rx_next(&it,0,&frame)==RTW_R92FU_BAD_ARGUMENT);
        CHECK(rtw_r92fu_rx_next(&it,1,nullptr)==RTW_R92FU_BAD_ARGUMENT);
        it.length=RTW_R92FU_MAX_BULK+1;
        CHECK(rtw_r92fu_rx_next(&it,1,&frame)==RTW_R92FU_BAD_ARGUMENT);
    }
}
static rtw_r92fu_tx_options sample() {
    rtw_r92fu_tx_options o{};
    o.payload_bytes=100;
    o.sequence=0xabc;
    o.rate=4;
    o.rf_chains=2;
    o.data=1;
    o.bandwidth_mhz=20;
    o.retry_limit=5;
    o.ack_expected=1;
    return o;
}
static void tx_tests() {
    std::array<uint8_t,40> tx{};
    auto o=sample();
    CHECK(rtw_r92fu_tx_descriptor(&o,tx.data()));
    CHECK(rtw_r92fu_tx_descriptor_valid(tx.data(),2));
    CHECK((word(tx.data())&0xffff)==100);
    CHECK((word(tx.data())&0xff000000U)==0x0c000000U);
    CHECK((word(tx.data()+4U)&0x1f00U)==0);
    CHECK((word(tx.data()+4U)&0x1f0000U)==0x60000U);
    CHECK((word(tx.data()+16U)&0x00fc0000U)==(5U<<18));
    CHECK(word(tx.data()+32U)==0U);
    CHECK(word(tx.data()+36U)==0xabc000U);
    tx[0]^=1;
    CHECK(!rtw_r92fu_tx_descriptor_valid(tx.data(),2));
    CHECK(rtw_r92fu_tx_descriptor(&o,tx.data()));
    CHECK(!rtw_r92fu_tx_descriptor_valid(tx.data(),0));
    CHECK(!rtw_r92fu_tx_descriptor_valid(nullptr,2));
    CHECK(!rtw_r92fu_tx_descriptor(nullptr,tx.data()));
    for (auto byte:tx) CHECK(byte==0);
    CHECK(!rtw_r92fu_tx_descriptor(&o,nullptr));
    o.qos=1;o.tid=5;
    CHECK(rtw_r92fu_tx_descriptor(&o,tx.data()));
    CHECK(((word(tx.data()+4U)>>8U)&31U)==5U);
    CHECK(word(tx.data()+32U)==0U);
    CHECK(word(tx.data()+36U)==0xabc000U);
    o.eapol=1;o.firmware_rate=1;
    CHECK(!rtw_r92fu_tx_descriptor(&o,tx.data()));
    o.firmware_rate=0;
    CHECK(rtw_r92fu_tx_descriptor(&o,tx.data()));
    o.band_5ghz=1;o.rate=0;
    CHECK(!rtw_r92fu_tx_descriptor(&o,tx.data()));
    o=sample();o.rate=27;o.bandwidth_mhz=40;
    CHECK(rtw_r92fu_tx_descriptor(&o,tx.data()));
    CHECK(rtw_r92fu_tx_descriptor_valid(tx.data(),2));
    CHECK(!rtw_r92fu_tx_descriptor_valid(tx.data(),1));
    o.rate=44;o.bandwidth_mhz=80;o.band_5ghz=1;
    CHECK(!rtw_r92fu_tx_descriptor(&o,tx.data()));
    o.band_5ghz=0;
    CHECK(!rtw_r92fu_tx_descriptor(&o,tx.data()));
    o=sample();o.multicast=1;o.ack_expected=0;o.retry_limit=0;
    CHECK(rtw_r92fu_tx_descriptor(&o,tx.data()));
    CHECK((word(tx.data())&0x01000000U)!=0);
    o.retry_limit=1;
    CHECK(!rtw_r92fu_tx_descriptor(&o,tx.data()));
    o=sample();o.protection=2;o.rts_rate=4;
    CHECK(rtw_r92fu_tx_descriptor(&o,tx.data()));
    CHECK((word(tx.data()+12U)&0x3000U)==0x3000U);
    o=sample();o.short_preamble=1;
    CHECK(!rtw_r92fu_tx_descriptor(&o,tx.data()));
    o.rate=2;
    CHECK(rtw_r92fu_tx_descriptor(&o,tx.data()));
    o=sample();o.rate=19;o.short_gi=1;
    CHECK(rtw_r92fu_tx_descriptor(&o,tx.data()));
}
static void host_sequence_tests() {
    // net80211 supplies every sequence; no FU caller requests replacement.
    // Cover wrap as an ordered 4094,4095,0 sequence as well as 0x123.
    for(unsigned kind=0;kind<7;++kind)for(unsigned sequence:{0x123U,4094U,4095U,0U}) {
        auto o=sample();o.sequence=uint16_t(sequence);
        if(kind==0||kind==6)o.data=0;
        if(kind==2||kind==4)o.eapol=1;
        if(kind==3||kind==4){o.qos=1;o.tid=5;}
        if(kind>=5){o.multicast=1;o.ack_expected=0;o.retry_limit=0;}
        std::array<uint8_t,40> tx{};
        CHECK(rtw_r92fu_tx_descriptor(&o,tx.data()));
        CHECK(rtw_r92fu_tx_descriptor_valid(tx.data(),2));
        CHECK(word(tx.data()+32U)==0U);
        CHECK(word(tx.data()+36U)==sequence<<12U);
        unsigned check=0;for(unsigned i=0;i<32;i+=2)check^=unsigned(tx[i])|(unsigned(tx[i+1])<<8U);CHECK(check==0);
        // DW8/DW9 are outside the checksum. The sequence validator must
        // independently reject HW_SEQ and non-sequence DW9 bits.
        for(uint32_t mask:{1U,0x800U,0x01000000U}){auto bad=tx;word(bad.data()+36U,word(bad.data()+36U)|mask);CHECK(!rtw_r92fu_tx_descriptor_valid(bad.data(),2));}
        auto hardware=tx;word(hardware.data()+32U,0x8000U);word(hardware.data()+36U,0U);CHECK(!rtw_r92fu_tx_descriptor_valid(hardware.data(),2));
    }
    auto invalid=sample();invalid.sequence=4096;std::array<uint8_t,40> tx;tx.fill(0xa5);
    CHECK(!rtw_r92fu_tx_descriptor(&invalid,tx.data()));for(auto byte:tx)CHECK(byte==0);
}
static void fuzz_tests() {
    std::mt19937 gen(0x0128812aU);
    std::array<uint8_t,256> data{};
    for (unsigned trial=0;trial<50000U; ++trial) {
        for(auto &byte:data)byte=(uint8_t)gen();
        const size_t n=gen()%(data.size()+1U);
        rtw_r92fu_rx_iterator it{data.data(),n,0,0};
        rtw_r92fu_rx_frame out{};
        const auto result=rtw_r92fu_rx_next(&it,1U+(gen()%2U),&out);
        CHECK(result!=RTW_R92FU_FRAME || (out.payload>=data.data() &&
              out.payload_length<=n && (size_t)(out.payload-data.data())<=n-out.payload_length));
        CHECK(it.cursor<=it.length);
        if(result!=RTW_R92FU_FRAME && result!=RTW_R92FU_END)
            CHECK(it.cursor==0);
        rtw_r92fu_tx_options opts=sample();
        opts.rate=(uint8_t)gen();
        opts.rf_chains=(uint8_t)gen();
        opts.bandwidth_mhz=(uint8_t)gen();
        std::array<uint8_t,40> descriptor{};
        const int ok=rtw_r92fu_tx_descriptor(&opts,descriptor.data());
        if(ok) CHECK(rtw_r92fu_tx_descriptor_valid(descriptor.data(),opts.rf_chains));
    }
}
int main() {
    rx_tests(); tx_tests(); host_sequence_tests(); fuzz_tests();
    std::cout<<"RTL8192FU wire descriptor checks passed: "<<checks<<"\n";
}
