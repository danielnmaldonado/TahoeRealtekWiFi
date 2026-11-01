#include "rtw/RTL8192CDataPath.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); std::exit(1); } } while (0)
static uint32_t word(const uint8_t *p) { return uint32_t(p[0]) | (uint32_t(p[1])<<8) | (uint32_t(p[2])<<16) | (uint32_t(p[3])<<24); }
static void put(uint8_t *p,uint32_t w) { for(unsigned i=0;i<4;i++)p[i]=uint8_t(w>>(i*8)); }
static void checksum(const uint8_t *p) { unsigned sum=0;for(unsigned i=0;i<32;i+=2)sum^=unsigned(p[i])|(unsigned(p[i+1])<<8);CHECK(sum==0); }
static rtw_92e_tx_options options() { rtw_92e_tx_options o{};o.data=1;o.raid=4;o.rate=11;o.sequence=0x123;return o; }
static void tx_vectors() {
    // Catches the gen2 offset/sequence/rate/RAID layout being reused for CU.
    std::array<uint8_t,34> storage{};storage.front()=0xa5;storage.back()=0x5a;
    auto *d=storage.data()+1;auto o=options();
    CHECK(rtw_92c_make_tx_descriptor(1500,&o,d));
    CHECK(word(d)==0x8c2005dcU);CHECK(word(d+4)==0x00040040U);
    CHECK(word(d+8)==0);CHECK(word(d+12)==0x01230000U);
    CHECK(word(d+16)==0x00000108U);CHECK(word(d+20)==0x0001ff0bU);
    CHECK(word(d+24)==0);CHECK(d[30]==0&&d[31]==0);checksum(d);
    CHECK(storage.front()==0xa5&&storage.back()==0x5a);
    CHECK((word(d+4)&0x00c00000U)==0);CHECK((word(d+16)&0x80U)==0);
    for(unsigned rate=0;rate<12;rate++) {
        o.rate=uint8_t(rate);o.sequence=uint16_t(rate*101);o.qsel=uint8_t(rate%8);
        CHECK(rtw_92c_make_tx_descriptor(24,&o,d));
        CHECK((word(d+20)&63)==rate);CHECK(word(d+12)==uint32_t(o.sequence)<<16);
        CHECK(((word(d+4)>>8)&31)==o.qsel);checksum(d);
    }
    o=options();o.data=0;o.multicast=1;o.qsel=0x12;o.raid=6;o.rate=0;o.sequence=4095;
    CHECK(rtw_92c_make_tx_descriptor(32,&o,d));
    CHECK(word(d)==0x8d200020U);CHECK(word(d+4)==0x00061200U);
    CHECK(word(d+8)==0);CHECK(word(d+12)==0x0fff0000U);
    CHECK(word(d+16)==0x100U);CHECK(word(d+20)==0x001a0000U);checksum(d);
    o=options();o.raid=6;o.rate=3;o.short_preamble=1;
    CHECK(rtw_92c_make_tx_descriptor(4096,&o,d));CHECK(word(d+16)==0x01000100U);checksum(d);
    // CU firmware owns data rate selection only after H2C MACID_CONFIG.
    o=options();o.firmware_rate=1;
    CHECK(rtw_92c_make_tx_descriptor(1500,&o,d));CHECK(word(d+16)==8U);
    CHECK(rtw_92c_tx_report_request(0,0,d));CHECK(word(d+4)==0x40040U);checksum(d);
    CHECK(!rtw_92c_tx_report_request(1,0,d)); // 88E requires host-selected rates.
    o.data=0;o.qsel=0x12;CHECK(!rtw_92c_make_tx_descriptor(100,&o,d));
    for(unsigned bad=0;bad<18;bad++) {
        o=options();size_t length=24;
        switch(bad) {
        case 0:length=23;break;case 1:length=4097;break;case 2:o.rate=12;break;
        case 3:o.raid=3;break;case 4:o.raid=7;break;case 5:o.sequence=4096;break;
        case 6:o.ampdu=1;break;case 7:o.density=1;break;case 8:o.max_agg=1;break;
        case 9:o.multicast=1;break;case 10:o.qsel=8;break;case 11:o.short_preamble=2;break;
        case 12:o.data=2;break;case 13:o.firmware_rate=2;break;
        case 14:o.bw40=1;break;case 15:o.channel_bw40=1;break;
        case 16:o.primary_above=1;break;case 17:o.short_gi=1;break;
        }
        std::memset(d,0xa5,32);CHECK(!rtw_92c_make_tx_descriptor(length,&o,d));CHECK(d[0]==0xa5&&d[31]==0xa5);
    }
    o=options();CHECK(!rtw_92c_make_tx_descriptor(24,nullptr,d));CHECK(!rtw_92c_make_tx_descriptor(24,&o,nullptr));
}
static void protection_and_reports() {
    std::array<uint8_t,32>d{};auto o=options();CHECK(rtw_92c_make_tx_descriptor(100,&o,d.data()));
    CHECK(rtw_92c_tx_protection(1,d.data()));CHECK(word(d.data()+16)==0x3108U);checksum(d.data());
    // Gen1 CTS-to-self needs HWRTSEN as well as CTS2SELF, unlike gen2.
    CHECK(rtw_92c_tx_protection(2,d.data()));CHECK(word(d.data()+16)==0x2908U);checksum(d.data());
    CHECK(rtw_92c_tx_protection(0,d.data()));CHECK(word(d.data()+16)==0x108U);checksum(d.data());
    auto before=d;CHECK(!rtw_92c_tx_protection(3,d.data()));CHECK(d==before);CHECK(!rtw_92c_tx_protection(0,nullptr));
    CHECK(rtw_92c_tx_report_request(1,1,d.data()));CHECK(word(d.data()+4)==0x00040000U);
    CHECK(word(d.data()+8)==0x00090000U);checksum(d.data());
    CHECK(rtw_92c_tx_report_request(1,0,d.data()));CHECK(word(d.data()+8)==0x10000U);checksum(d.data());
    CHECK(rtw_92c_tx_report_request(0,0,d.data()));CHECK(word(d.data()+4)==0x40040U);CHECK(word(d.data()+8)==0);checksum(d.data());
    before=d;CHECK(!rtw_92c_tx_report_request(0,1,d.data()));CHECK(d==before);
    CHECK(!rtw_92c_tx_report_request(2,0,d.data()));CHECK(!rtw_92c_tx_report_request(1,2,d.data()));
    CHECK(!rtw_92c_tx_report_request(1,1,nullptr));
    for(unsigned bad=0;bad<4;bad++) {
        o=options();CHECK(rtw_92c_make_tx_descriptor(100,&o,d.data()));
        if(bad==0)d[3]|=1;else if(bad==1)d[4]|=1;else if(bad==2)d[5]=0x12;else d[16]&=uint8_t(~1U);
        if(bad==3)d[17]&=uint8_t(~1U); // DRVRATE bit8.
        before=d;CHECK(!rtw_92c_tx_report_request(1,1,d.data()));CHECK(d==before);
    }
}
static void rx_vectors() {
    // Literal 24-byte CU RX descriptor: info16 + shift2 + payload30.
    std::array<uint8_t,320>b{};put(b.data(),0x0e52001eU);put(b.data()+8,0x10020000U);
    put(b.data()+12,0x0000000bU);put(b.data()+16,0x30U);put(b.data()+20,0x12345678U);
    b[42]=0x08;b[71]=0x7e;
    // Second normal packet begins at128, not gen2's8-byte boundary.
    put(b.data()+128,0x00000018U);put(b.data()+140,0x00000244U);b[152]=0x80;
    rtw_rx_iterator it{};rtw_92c_rx_frame f{};rtw_rx_iterator_init(&it,b.data(),176);
    CHECK(rtw_92c_rx_next(&it,0,&f)==RTW_PACKETS_OK);
    CHECK(f.packet.mpdu==b.data()+42&&f.packet.mpdu_length==30);CHECK(f.packet.mpdu[29]==0x7e);
    CHECK(f.packet.phy_info==b.data()+24&&f.packet.phy_info_length==16);
    CHECK(f.security_code==5&&f.sw_decrypt_required==1&&f.ht==0&&f.report_type==0);
    CHECK(f.packet.rate_index==11&&f.packet.bandwidth_code==0&&f.packet.timestamp==0x12345678U);
    CHECK(f.packet.phy_status_present==1&&it.cursor==128&&it.frames==1);
    // DW2 bit28 is gen1 reserved, never a gen2 C2H selector.
    CHECK(rtw_92c_rx_next(&it,1,&f)==RTW_PACKETS_OK);CHECK(f.report_type==0&&f.ht==1);
    CHECK(f.packet.rate_index==4&&f.packet.bandwidth_code==1&&f.packet.mpdu==b.data()+152);
    CHECK(rtw_92c_rx_next(&it,1,&f)==RTW_PACKETS_DONE);
    for(unsigned type=1;type<=3;type++) {
        b.fill(0);put(b.data(),8);put(b.data()+12,uint32_t(type)<<14);b[24]=0x80;b[31]=0x55;
        rtw_rx_iterator_init(&it,b.data(),32);CHECK(rtw_92c_rx_next(&it,1,&f)==RTW_PACKETS_OK);
        CHECK(f.report_type==type&&f.packet.mpdu==b.data()+24&&f.packet.mpdu_length==8);
        rtw_rx_iterator_init(&it,b.data(),32);CHECK(rtw_92c_rx_next(&it,0,&f)==RTW_PACKETS_OK);CHECK(f.report_type==0);
    }
    b.fill(0);put(b.data(),0x0800c018U);put(b.data()+12,4);rtw_rx_iterator_init(&it,b.data(),48);
    CHECK(rtw_92c_rx_next(&it,1,&f)==RTW_PACKETS_OK);CHECK(f.packet.crc_error&&f.packet.icv_error&&f.sw_decrypt_required);
    for(unsigned size=0;size<48;size++) {
        rtw_rx_iterator_init(&it,b.data(),size);auto r=rtw_92c_rx_next(&it,1,&f);
        CHECK(r==(size==0?RTW_PACKETS_DONE:RTW_PACKETS_TRUNCATED));CHECK(it.frames==0);
    }
    put(b.data()+12,12);rtw_rx_iterator_init(&it,b.data(),48);
    CHECK(rtw_92c_rx_next(&it,1,&f)==RTW_PACKETS_MALFORMED);CHECK(it.cursor==0&&f.packet.mpdu==nullptr);
    put(b.data()+12,0x4000);put(b.data(),0x10018);rtw_rx_iterator_init(&it,b.data(),56);
    CHECK(rtw_92c_rx_next(&it,1,&f)==RTW_PACKETS_MALFORMED); // Report cannot carry PHY words.
    b.fill(0);rtw_rx_iterator_init(&it,b.data(),320);CHECK(rtw_92c_rx_next(&it,1,&f)==RTW_PACKETS_DONE);
    b[31]=1;rtw_rx_iterator_init(&it,b.data(),32);CHECK(rtw_92c_rx_next(&it,1,&f)==RTW_PACKETS_MALFORMED);
    put(b.data(),24);rtw_rx_iterator_init(&it,b.data(),48);it.frames=RTW_MAX_RX_FRAMES;
    CHECK(rtw_92c_rx_next(&it,1,&f)==RTW_PACKETS_TOO_MANY);
    CHECK(rtw_92c_rx_next(nullptr,1,&f)==RTW_PACKETS_BAD_ARGUMENT);
    CHECK(rtw_92c_rx_next(&it,1,nullptr)==RTW_PACKETS_BAD_ARGUMENT);
    CHECK(rtw_92c_rx_next(&it,2,&f)==RTW_PACKETS_BAD_ARGUMENT);
    rtw_rx_iterator_init(&it,nullptr,1);CHECK(rtw_92c_rx_next(&it,1,&f)==RTW_PACKETS_BAD_ARGUMENT);
}
static void ccx_reports() {
    // OpenBSD r88e_tx_rpt_ccx, not gen2's event ID+sequence header.
    std::array<uint8_t,8>b{{0x80,0x40,0x03,0x34,0x12,0x0b,0x20,0x00}};rtw_88e_tx_report r{};
    CHECK(rtw_88e_parse_tx_report(b.data(),8,&r));CHECK(r.packet_ok==1);
    CHECK(r.report.event_sequence==0&&r.report.retries==3&&r.report.queue_time==0x1234&&r.report.final_rate==11);
    CHECK(!r.report.retry_over&&!r.report.lifetime_expired);
    b[1]=0;b[2]=0xc2;CHECK(rtw_88e_parse_tx_report(b.data(),8,&r));CHECK(!r.packet_ok);
    CHECK(r.report.retry_over&&r.report.lifetime_expired&&r.report.retries==2);
    b[2]=0;CHECK(rtw_88e_parse_tx_report(b.data(),8,&r));CHECK(!r.packet_ok&&!r.report.retry_over&&!r.report.lifetime_expired);
    for(unsigned length=0;length<16;length++)if(length!=8)CHECK(!rtw_88e_parse_tx_report(b.data(),length,&r));
    for(unsigned bad=0;bad<5;bad++) { auto c=b;
        if(bad==0)c[1]=1;else if(bad==1)c[1]=0x80;else if(bad==2)c[5]=12;else if(bad==3)c[6]=0x80;else c[1]=0x40,c[2]=0x80;
        CHECK(!rtw_88e_parse_tx_report(c.data(),8,&r));CHECK(r.packet_ok==0&&r.report.queue_time==0);
    }
    CHECK(!rtw_88e_parse_tx_report(nullptr,8,&r));CHECK(!rtw_88e_parse_tx_report(b.data(),8,nullptr));
}
static void bounded_inputs() {
    // Malformed lengths/offsets never advance or publish an out-of-range span.
    // Actual production decoder runs on deliberately unaligned input.
    std::array<uint8_t,513>b{};uint32_t state=0x2ea94115U;
    for(unsigned cycle=0;cycle<4096;cycle++) {
        for(auto &v:b){state=state*1664525U+1013904223U;v=uint8_t(state>>24);}
        size_t size=cycle%512;rtw_rx_iterator it{};rtw_92c_rx_frame f{};
        rtw_rx_iterator_init(&it,b.data()+1,size);auto result=rtw_92c_rx_next(&it,int(cycle&1),&f);
        CHECK(it.cursor<=size&&it.frames<=1);
        if(result==RTW_PACKETS_OK) {
            CHECK(it.frames==1&&it.cursor>0);
            CHECK(f.packet.mpdu>=b.data()+1&&f.packet.mpdu<=b.data()+1+size);
            CHECK(f.packet.mpdu_length<=size_t(b.data()+1+size-f.packet.mpdu));
            CHECK(f.packet.phy_info>=b.data()+1&&f.packet.phy_info<=b.data()+1+size);
            CHECK(f.packet.phy_info_length<=size_t(b.data()+1+size-f.packet.phy_info));
        } else if(result!=RTW_PACKETS_DONE) {
            CHECK(it.cursor==0&&it.frames==0&&f.packet.mpdu==nullptr&&f.packet.mpdu_length==0);
        }
    }
    rtw_rx_iterator bad{};rtw_92c_rx_frame f{};
    bad.buffer=b.data();bad.length=1;bad.cursor=2;
    CHECK(rtw_92c_rx_next(&bad,0,&f)==RTW_PACKETS_BAD_ARGUMENT);
}
int main() { tx_vectors();protection_and_reports();rx_vectors();ccx_reports();bounded_inputs();std::printf("gen1 datapath: %u checks passed\n",checks); }
