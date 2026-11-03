/* SPDX-License-Identifier: BSD-3-Clause */
#include "rtw/RTL8814AUDataPath.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#define BIT(n) (UINT32_C(1)<<(n))
#define GENMASK(h,l) ((UINT32_MAX>>(31U-(h)))&(UINT32_MAX<<(l)))
#include "fixtures/rtl8814au_txampdu_primary.h"

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr,"line %d: %s\n",__LINE__,#x); std::exit(1); } } while (0)
#ifndef RTW_8814AU_GEN2_BASELINE
static uint32_t get32(const uint8_t *p) {
    return uint32_t(p[0]) | (uint32_t(p[1])<<8) | (uint32_t(p[2])<<16) | (uint32_t(p[3])<<24);
}
#endif

/* Literal rtw88/tx.h 8814A fixture: LS, offset40, no OWN/FIRST, MACID0,
 * RATEID_BG6, firmware rate, rate54M11, software sequence0x123 in DW9.
 * Checksum is XOR of the first16 little-endian words ONLY (0x0441). */
static const uint8_t fw_legacy_tx[40]={
    0x64,0x00,0x28,0x04, 0x00,0x00,0x06,0x00,
    0,0,0,0, 0,0,0,0, 0x0b,0,0,0, 0,0,0,0,
    0,0,0,0, 0x41,0x04,0,0, 0,0,0,0, 0x00,0x30,0x12,0x00
};

#ifdef RTW_8814AU_GEN2_BASELINE
int main() {
    uint8_t d[40];
    CHECK(rtw_tx_make_descriptor(100,11,0,0,d)==RTW_PACKETS_OK);
    /* RED must expose the actual previous gen2 descriptor, not a mock. */
    CHECK(std::memcmp(d,fw_legacy_tx,40)==0);
    return 0;
}
#else
static rtw_8814au_rate_config config(uint16_t legacy=0xfff,bool ht=false,
    unsigned chains=2,uint8_t m0=0xff,uint8_t m1=0xff,bool sgi=false) {
    rtw_8814au_rate_config c{};
    const uint8_t mcs[2]={m0,m1};
    CHECK(rtw_8814au_build_rate_config(legacy,mcs,chains,ht,sgi,sgi,&c));
    return c;
}
static rtw_8814au_tx_options options() {
    rtw_8814au_tx_options o{};
    o.data=1;o.rate=11;o.firmware_rate=1;o.sequence=0x123;
    return o;
}
static void test_rates_and_h2c() {
    auto c=config();
    CHECK(c.mask==0xfff && c.rate_id==6 && !c.ht && !c.sgi20 && !c.tx_streams);
    CHECK(c.initial_rate==11 && c.init_ra_level==1);
    uint8_t low[8],high[8];
    CHECK(rtw_8814au_build_ra_commands(&c,1,low,high));
    const uint8_t legacy_low[8]={0x40,0x00,0x26,0x40,0xff,0x0f,0,0};
    const uint8_t legacy_high[8]={0x46,0x00,0x26,0x40,0,0,0,0};
    CHECK(!std::memcmp(low,legacy_low,8));CHECK(!std::memcmp(high,legacy_high,8));
    c.init_ra_level=0;
    CHECK(rtw_8814au_build_ra_commands(&c,0,low,high));
    const uint8_t refresh_low[8]={0x40,0,0x06,0x48,0xff,0x0f,0,0};
    const uint8_t refresh_high[8]={0x46,0,0x06,0x48,0,0,0,0};
    CHECK(!std::memcmp(low,refresh_low,8));CHECK(!std::memcmp(high,refresh_high,8));
    c=config(0xfff,true,2,0xff,0xff,true);
    CHECK(c.mask==0xfffffff && c.rate_id==2 && c.ht && c.sgi20 && c.tx_streams==2 && c.initial_rate==27);
    CHECK(rtw_8814au_build_ra_commands(&c,1,low,high));
    const uint8_t ht_low[8]={0x40,0,0xa2,0x40,0xff,0xff,0xff,0x0f};
    const uint8_t ht_high[8]={0x46,0,0xa2,0x40,0,0,0,0};
    CHECK(!std::memcmp(low,ht_low,8));CHECK(!std::memcmp(high,ht_high,8));
    CHECK(config(0xff0,true).rate_id==4);CHECK(config(0xfff,true,1).rate_id==3);
    CHECK(config(0xff0,true,1).rate_id==5);CHECK(config(0xff0).rate_id==7);CHECK(config(0xf).rate_id==8);
    CHECK(config(0xfff,true,1).mask==0xfffff);CHECK(!config(0xfff,true,2,0,0,true).ht);
    auto invalid=c;invalid.mask|=UINT64_C(1)<<32;CHECK(!rtw_8814au_build_ra_commands(&invalid,1,low,high));
    invalid=c;invalid.mask|=UINT64_C(1)<<28;CHECK(!rtw_8814au_build_ra_commands(&invalid,1,low,high));
    invalid=c;invalid.rate_id=0;CHECK(!rtw_8814au_build_ra_commands(&invalid,1,low,high));
    invalid=c;invalid.initial_rate=28;CHECK(!rtw_8814au_build_ra_commands(&invalid,1,low,high));
    invalid=c;invalid.ht=0;CHECK(!rtw_8814au_build_ra_commands(&invalid,1,low,high));
    invalid=c;invalid.tx_streams=3;CHECK(!rtw_8814au_build_ra_commands(&invalid,1,low,high));
    invalid=c;invalid.init_ra_level=4;CHECK(!rtw_8814au_build_ra_commands(&invalid,1,low,high));
    CHECK(!rtw_8814au_build_ra_commands(&c,2,low,high));
    CHECK(!rtw_8814au_build_ra_commands(nullptr,1,low,high));
    CHECK(!rtw_8814au_build_ra_commands(&c,1,nullptr,high));
    CHECK(!rtw_8814au_build_ra_commands(&c,1,low,nullptr));
    const uint8_t mcs[2]={0xff,0xff};
    for(unsigned chains: {0U,3U,4U}) CHECK(!rtw_8814au_build_rate_config(0xfff,mcs,chains,1,1,1,&c));
    CHECK(!rtw_8814au_build_rate_config(0,mcs,2,1,1,1,&c));
    CHECK(!rtw_8814au_build_rate_config(0x1000,mcs,2,1,1,1,&c));
    CHECK(!rtw_8814au_build_rate_config(0xfff,nullptr,2,1,1,1,&c));
    CHECK(!rtw_8814au_build_rate_config(0xfff,mcs,2,2,1,1,&c));
    CHECK(!rtw_8814au_build_rate_config(0xfff,mcs,2,1,2,1,&c));
    CHECK(!rtw_8814au_build_rate_config(0xfff,mcs,2,1,1,2,&c));
    CHECK(!rtw_8814au_build_rate_config(0xfff,mcs,2,1,1,1,nullptr));
    CHECK(rtw_8814au_make_media_status(1,low));
    const uint8_t media[8]={1,1,0,0,0,0,0,0};CHECK(!std::memcmp(low,media,8));
    CHECK(rtw_8814au_make_media_status(0,low));CHECK(low[0]==1 && low[1]==0);
    CHECK(!rtw_8814au_make_media_status(2,low));CHECK(!rtw_8814au_make_media_status(1,nullptr));
    CHECK(rtw_8814au_make_rssi_monitor(59,low));
    const uint8_t rssi[8]={0x42,0,0,59,0,0,0,0};CHECK(!std::memcmp(low,rssi,8));
    CHECK(rtw_8814au_make_rssi_monitor(100,low));CHECK(!rtw_8814au_make_rssi_monitor(101,low));
    CHECK(!rtw_8814au_make_rssi_monitor(59,nullptr));
}
static void test_tx() {
    auto c=config();auto o=options();uint8_t d[40];
    CHECK(rtw_8814au_make_tx_descriptor(100,&o,&c,d));CHECK(!std::memcmp(d,fw_legacy_tx,40));
    o={};o.qsel=18;o.sequence=0xfff;
    const uint8_t mgmt[40]={
        0x20,0,0x28,0x84, 0,0x12,0x08,0, 0,0,0,0, 0,0x05,0,0,
        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0x93,0,0, 0,0,0,0, 0,0xf0,0xff,0
    };
    CHECK(rtw_8814au_make_tx_descriptor(32,&o,nullptr,d));CHECK(!std::memcmp(d,mgmt,40));
    o=options();o.eapol=1;o.firmware_rate=0;o.rate=0;
    CHECK(rtw_8814au_make_tx_descriptor(40,&o,&c,d));
    CHECK(get32(d+12)==0x500 && get32(d+4)==0x80000 && get32(d+16)==0);
    o.firmware_rate=1;CHECK(!rtw_8814au_make_tx_descriptor(40,&o,&c,d));
    o=options();o.multicast=1;CHECK(!rtw_8814au_make_tx_descriptor(40,&o,&c,d));
    o.firmware_rate=0;o.rate=4;
    CHECK(rtw_8814au_make_tx_descriptor(40,&o,nullptr,d));
    CHECK((get32(d)&0x01000000)!=0 && (get32(d+12)&0x500)==0x500);
    o.qsel=18;o.eapol=1;o.rate=0;
    CHECK(rtw_8814au_make_tx_descriptor(40,&o,nullptr,d));
    CHECK((get32(d+4)&0x1f00U)==0x1200U && get32(d+16)==0 && get32(d+12)==0x500U);
    o=options();o.protection=1;
    CHECK(rtw_8814au_make_tx_descriptor(100,&o,&c,d));
    CHECK(get32(d+12)==0x1000 && get32(d+16)==0x0800000b && get32(d+20)==0x1000);
    for(unsigned mode: {2U,3U,255U}) {o.protection=uint8_t(mode);CHECK(!rtw_8814au_make_tx_descriptor(100,&o,&c,d));}
    o=options();o.firmware_rate=0;o.rate=3;o.short_preamble=1;
    CHECK(rtw_8814au_make_tx_descriptor(100,&o,&c,d));CHECK(get32(d+20)==0x10);
    o.rate=0;CHECK(!rtw_8814au_make_tx_descriptor(100,&o,&c,d));
    o.rate=4;CHECK(!rtw_8814au_make_tx_descriptor(100,&o,&c,d));
    c=config(0xfff,true,2,0xff,0xff,true);
    for(uint8_t rate=12;rate<28;++rate) {
        o=options();o.rate=rate;o.short_gi=1;
        CHECK(rtw_8814au_make_tx_descriptor(100,&o,&c,d));
        CHECK(get32(d+16)==rate && get32(d+20)==0x10 && get32(d+4)==0x20000);
        CHECK(!(get32(d+4)&0x00c00000) && !get32(d+8) && !get32(d+32));
    }
    for(uint8_t rate: {uint8_t(28),uint8_t(43),uint8_t(44),uint8_t(83),uint8_t(127)}) {
        o=options();o.rate=rate;CHECK(!rtw_8814au_make_tx_descriptor(100,&o,&c,d));
    }
    o=options();o.rate=12;CHECK(!rtw_8814au_make_tx_descriptor(100,&o,nullptr,d));
    c=config(0xfff,true,1);o.rate=20;CHECK(!rtw_8814au_make_tx_descriptor(100,&o,&c,d));
    o.rate=12;o.short_gi=1;CHECK(!rtw_8814au_make_tx_descriptor(100,&o,&c,d));
    o=options();CHECK(!rtw_8814au_make_tx_descriptor(100,&o,nullptr,d));
    c=config();o.qsel=8;CHECK(!rtw_8814au_make_tx_descriptor(100,&o,&c,d));
    o=options();o.qsel=18;CHECK(!rtw_8814au_make_tx_descriptor(100,&o,&c,d));
    o=options();o.sequence=4096;CHECK(!rtw_8814au_make_tx_descriptor(100,&o,&c,d));
    o=options();o.data=2;CHECK(!rtw_8814au_make_tx_descriptor(100,&o,&c,d));
    o=options();o.multicast=2;CHECK(!rtw_8814au_make_tx_descriptor(100,&o,&c,d));
    o=options();o.eapol=2;CHECK(!rtw_8814au_make_tx_descriptor(100,&o,&c,d));
    o=options();o.firmware_rate=2;CHECK(!rtw_8814au_make_tx_descriptor(100,&o,&c,d));
    o=options();o.short_gi=2;CHECK(!rtw_8814au_make_tx_descriptor(100,&o,&c,d));
    o=options();o.short_preamble=2;CHECK(!rtw_8814au_make_tx_descriptor(100,&o,&c,d));
    o=options();CHECK(!rtw_8814au_make_tx_descriptor(0,&o,&c,d));
    CHECK(!rtw_8814au_make_tx_descriptor(65536,&o,&c,d));
    CHECK(!rtw_8814au_make_tx_descriptor(100,nullptr,&c,d));
    CHECK(!rtw_8814au_make_tx_descriptor(100,&o,&c,nullptr));
    for(unsigned seq: {0U,1U,0xffU,0xfffU}) {
        o.sequence=uint16_t(seq);CHECK(rtw_8814au_make_tx_descriptor(100,&o,&c,d));
        uint16_t checksum=0;for(unsigned i=0;i<32;i+=2) checksum^=uint16_t(unsigned(d[i])|(unsigned(d[i+1])<<8));
        CHECK(!checksum);CHECK(get32(d+36)==seq<<12);
        CHECK(d[28]==0x41 && d[29]==0x04); /* DW9 is outside checksum. */
    }
}
static void test_tx_ampdu() {
    auto c=config(0xfff,true,2,255,255,true);auto o=options();uint8_t d[40];
    o.qos=o.ack_expected=o.ampdu=1;o.density=7;o.max_agg=8;o.qsel=5;o.rate=27;o.short_gi=1;
    // Literal pinned tx.h fields plus Linux tx.c packing: HT MCS15, SGI20,
    // TID5, density7, 8 PAIRS (16 MPDUs), software sequence0x123. No BK,
    // hardware sequence/cipher, USB batching, or forced per-MPDU protection.
    const uint8_t expected[40]={
        0x64,0,0x28,0x04, 0,0x05,0x02,0, 0,0x10,0x70,0,
        0,0,0x10,0, 0x1b,0,0,0, 0x10,0,0,0, 0,0,0,0,
        0x25,0x11,0,0, 0,0,0,0, 0,0x30,0x12,0
    };
    CHECK(rtw_8814au_make_tx_descriptor(100,&o,&c,d));CHECK(!std::memcmp(d,expected,40));
    CHECK(!rtw_8814au_make_tx_descriptor(24,&o,&c,d));
    for(unsigned density=0;density<8;++density)for(unsigned pairs=1;pairs<=16;++pairs) {
        o.density=uint8_t(density);o.max_agg=uint8_t(pairs);
        CHECK(rtw_8814au_make_tx_descriptor(100,&o,&c,d));
        CHECK(get32(d+8)==(RTW_TX_DESC_W2_AGG_EN|((density<<20)&RTW_TX_DESC_W2_AMPDU_DEN)));
        CHECK(get32(d+12)==((pairs<<17)&RTW_TX_DESC_W3_MAX_AGG_NUM));
        CHECK(!get32(d+32)&&get32(d+36)==0x123000U);
        uint16_t sum=0;for(unsigned i=0;i<32;i+=2)sum^=uint16_t(unsigned(d[i])|(unsigned(d[i+1])<<8));CHECK(!sum);
    }
    o.density=7;o.max_agg=8;
    for(unsigned bad=0;bad<18;++bad) {
        auto v=o;auto rates=c;
        switch(bad) {
        case 0:v.data=0;v.qsel=18;break;case 1:v.multicast=1;break;case 2:v.eapol=1;break;
        case 3:v.firmware_rate=0;break;case 4:v.qos=0;break;case 5:v.ack_expected=0;break;
        case 6:v.fragment=1;break;case 7:v.rate=11;v.short_gi=0;break;
        case 8:v.max_agg=0;break;case 9:v.max_agg=17;break;case 10:v.density=8;break;
        case 11:v.qos=2;break;case 12:v.ack_expected=2;break;case 13:v.fragment=2;break;
        case 14:v.ampdu=2;break;case 15:v.ampdu=0;break;
        case 16:rates=config();v.rate=11;v.short_gi=0;break;case 17:v.data=2;break;
        }
        std::memset(d,0xa5,sizeof(d));CHECK(!rtw_8814au_make_tx_descriptor(100,&v,&rates,d));
        for(auto byte:d)CHECK(byte==0xa5); // Reject without leaking a partial descriptor.
    }
}

/* Independent wire header: len28, AES4, drvinfo4*8, SHIFT2, PHYST1,
 * SWDEC1; CAMID3, PPDU2, rate54M11, TSF0x44332211. PHY is at26, not24. */
static const uint8_t rx_header[24]={
    0x1c,0,0x44,0x0e, 3,0,0,0, 0,0,0,0x40,
    11,0,0,0, 0,0,0,0, 0x11,0x22,0x33,0x44
};
/* Jaguar gains A64/B68/C68/D72, channel6, RF20. */
static const uint8_t ofdm_phy[32]={
    64,68,6,0, 0,0,0,0, 0,0xfb,7,0, 0,0xe0,0xd8,40,
    38,0,0,0xc4, 0xb0,36,34,68, 72,0xf0,0,0, 0,0,0,0
};
static std::vector<uint8_t> rx_transfer() {
    std::vector<uint8_t> b(88,0xa5);
    std::memcpy(b.data(),rx_header,24);std::memcpy(b.data()+26,ofdm_phy,32);
    std::memset(b.data()+58,0x5a,28);return b;
}
static void test_rx_and_phy() {
    auto b=rx_transfer();rtw_rx_iterator it{};rtw_8814au_rx_frame f{};
    rtw_rx_iterator_init(&it,b.data(),b.size());
    CHECK(rtw_8814au_rx_next(&it,&f)==RTW_PACKETS_OK);
    CHECK(f.packet.phy_info==b.data()+26 && f.packet.phy_info_length==32);
    CHECK(f.packet.mpdu==b.data()+58 && f.packet.mpdu_length==28 && it.cursor==88);
    CHECK(f.security_code==4 && f.sw_decrypt_required && !f.is_c2h && f.cam_id==3 && f.ppdu_count==2);
    CHECK(f.packet.rate_index==11 && f.packet.timestamp==0x44332211 && f.packet.phy_status_present);
    b[1]=0xc0;b[3]=0x06; /* Literal CRC/ICV bad, AES4, hardware-decrypted. */
    rtw_rx_iterator_init(&it,b.data(),b.size());
    CHECK(rtw_8814au_rx_next(&it,&f)==RTW_PACKETS_OK);
    CHECK(f.packet.crc_error && f.packet.icv_error && f.security_code==4 && !f.sw_decrypt_required);
    b[1]=0;b[3]=0x0e;
    rtw_rx_iterator_init(&it,b.data(),b.size());CHECK(rtw_8814au_rx_next(&it,&f)==RTW_PACKETS_OK);
    rtw_8814au_phy_status p{};
    CHECK(rtw_8814au_parse_phy(&f,3,&p));
    CHECK(p.path_dbm[0]==-46 && p.path_dbm[1]==-42 && p.path_dbm[2]==-42 && p.path_dbm[3]==-38);
    CHECK(p.active_mask==3 && p.measured_mask==15 && p.signal_dbm==-42 && p.channel==6 && !p.bandwidth_code);
    CHECK(p.snr_raw[0]==40 && p.snr_raw[1]==38 && p.snr_raw[2]==36 && p.snr_raw[3]==34);
    CHECK(p.evm_raw[0]==-16 && p.evm_raw[1]==-40 && p.evm_raw[2]==-60 && p.evm_raw[3]==-80);
    CHECK(p.cfo_tail_raw[0]==-5 && p.cfo_tail_raw[1]==7 && !p.cck);
    CHECK(p.rssi==59); /* primary phy.c integer linear average, NOT max(AB). */
    b[26]=106;b[27]=66;b[49]=88;b[50]=72;
    CHECK(rtw_8814au_parse_phy(&f,3,&p));CHECK(p.signal_dbm==-38);
    b[26]=64;b[27]=68;b[49]=68;
    /* Valid foreign MCS31 and VHT4SS9 must not poison the USB aggregate. */
    for(uint8_t rate: {uint8_t(12),uint8_t(27),uint8_t(43),uint8_t(44),uint8_t(83)}) {
        b[12]=rate;rtw_rx_iterator_init(&it,b.data(),b.size());
        CHECK(rtw_8814au_rx_next(&it,&f)==RTW_PACKETS_OK);CHECK(f.packet.rate_index==rate);
        CHECK(rtw_8814au_parse_phy(&f,7,&p));CHECK(p.evm_raw[0]==-32);
    }
    b[12]=3;
    for(const auto pair: {std::array<int,2>{7,-48},{5,-38},{3,-18},{2,-11}}) {
        b[31]=uint8_t((pair[0]<<5)|5);rtw_rx_iterator_init(&it,b.data(),b.size());
        CHECK(rtw_8814au_rx_next(&it,&f)==RTW_PACKETS_OK);
        CHECK(rtw_8814au_parse_phy(&f,3,&p));
        CHECK(p.cck && p.signal_dbm==pair[1] && p.path_dbm[0]==pair[1] && p.measured_mask==1);
    }
    b[31]=0;CHECK(!rtw_8814au_parse_phy(&f,3,&p));
    b=rx_transfer();rtw_rx_iterator_init(&it,b.data(),b.size());CHECK(rtw_8814au_rx_next(&it,&f)==RTW_PACKETS_OK);
    for(const auto pair: {std::array<unsigned,3>{1,0,1},{2,0,2},{2,9,1},{2,10,1},{2,1,0},{0,0,0}}) {
        b[29]=uint8_t((pair[0]<<6)|(pair[1]<<2));
        b[16]=uint8_t(((pair[2]+1)%3)<<4); // Deliberately conflicting descriptor BW.
        rtw_rx_iterator_init(&it,b.data(),b.size());
        CHECK(rtw_8814au_rx_next(&it,&f)==RTW_PACKETS_OK);
        CHECK(f.packet.bandwidth_code==pair[2]);
        CHECK(rtw_8814au_parse_phy(&f,3,&p));CHECK(p.bandwidth_code==pair[2]);
    }
    CHECK(!rtw_8814au_parse_phy(&f,0,&p));CHECK(!rtw_8814au_parse_phy(&f,16,&p));
    CHECK(!rtw_8814au_parse_phy(nullptr,3,&p));CHECK(!rtw_8814au_parse_phy(&f,3,nullptr));
    auto malformed=f;malformed.packet.phy_info_length=27;CHECK(!rtw_8814au_parse_phy(&malformed,3,&p));
    malformed=f;malformed.packet.phy_status_present=0;CHECK(!rtw_8814au_parse_phy(&malformed,3,&p));
    malformed=f;malformed.is_c2h=1;CHECK(!rtw_8814au_parse_phy(&malformed,3,&p));
    CHECK(rtw_8814au_rx_next(&it,&f)==RTW_PACKETS_DONE);
    /* First unit ends86 and aligns88; a following no-PHY beacon ends140. */
    b=rx_transfer();b.resize(140,0);b[88]=28;b[100]=4;
    rtw_rx_iterator_init(&it,b.data(),b.size());
    CHECK(rtw_8814au_rx_next(&it,&f)==RTW_PACKETS_OK);CHECK(it.cursor==88);
    CHECK(rtw_8814au_rx_next(&it,&f)==RTW_PACKETS_OK);CHECK(f.packet.mpdu==b.data()+112 && it.cursor==140);
    CHECK(rtw_8814au_rx_next(&it,&f)==RTW_PACKETS_DONE);
    for(size_t n=0;n<86;++n) {
        auto partial=rx_transfer();rtw_rx_iterator_init(&it,partial.data(),n);
        CHECK(rtw_8814au_rx_next(&it,&f)==(n?RTW_PACKETS_TRUNCATED:RTW_PACKETS_DONE));
    }
    b=rx_transfer();rtw_rx_iterator_init(&it,b.data(),86);CHECK(rtw_8814au_rx_next(&it,&f)==RTW_PACKETS_OK);
    // Complete wire payload/FCS ends at86; its final alignment pad is optional.
    for(size_t length:{size_t(86),size_t(87),size_t(88)}){
        rtw_rx_iterator_init(&it,b.data(),length);CHECK(rtw_8814au_rx_next(&it,&f)==RTW_PACKETS_OK);
        CHECK(it.cursor==length&&f.packet.mpdu_length==28);
    }
    for(size_t missing=1;missing<=4;++missing){
        rtw_rx_iterator_init(&it,b.data(),86-missing);CHECK(rtw_8814au_rx_next(&it,&f)==RTW_PACKETS_TRUNCATED);
        CHECK(it.cursor==0&&it.frames==0);
    }
    for(unsigned offset: {0U,12U,2U,3U}) {
        b=rx_transfer();
        if(offset==0) b[0]=4; /* ordinary FCS-only frame is not an MPDU */
        if(offset==12) b[12]=84;
        if(offset==2) b[2]=0x41; /* invalid info-size1 */
        if(offset==3) b[2]=0x40; /* PHYST1 but info-size0 */
        rtw_rx_iterator_init(&it,b.data(),b.size());CHECK(rtw_8814au_rx_next(&it,&f)==RTW_PACKETS_MALFORMED);
    }
    b.assign(32,0);b[0]=2;b[11]=0x10;b[24]=0x32;b[25]=5;
    rtw_rx_iterator_init(&it,b.data(),b.size());CHECK(rtw_8814au_rx_next(&it,&f)==RTW_PACKETS_OK);
    CHECK(f.is_c2h && f.packet.mpdu_length==2 && f.packet.mpdu==b.data()+24);
    b.assign(24,0);rtw_rx_iterator_init(&it,b.data(),b.size());CHECK(rtw_8814au_rx_next(&it,&f)==RTW_PACKETS_DONE);
    b[23]=1;rtw_rx_iterator_init(&it,b.data(),b.size());CHECK(rtw_8814au_rx_next(&it,&f)==RTW_PACKETS_MALFORMED);
    b=rx_transfer();rtw_rx_iterator_init(&it,b.data(),b.size());it.frames=4096;
    CHECK(rtw_8814au_rx_next(&it,&f)==RTW_PACKETS_TOO_MANY);
    CHECK(rtw_8814au_rx_next(nullptr,&f)==RTW_PACKETS_BAD_ARGUMENT);
    CHECK(rtw_8814au_rx_next(&it,nullptr)==RTW_PACKETS_BAD_ARGUMENT);
    rtw_rx_iterator_init(&it,nullptr,1);CHECK(rtw_8814au_rx_next(&it,&f)==RTW_PACKETS_BAD_ARGUMENT);
}
static void test_events() {
    auto c=config(0xfff,true,2,0xff,0xff,true);rtw_8814au_c2h_event e{};
    const uint8_t ra[8]={0x0c,0x7f,0x9b,0,0x55,0xa1,0x66,0x70};
    CHECK(rtw_8814au_parse_c2h(ra,8,&c,&e)==RTW_8814AU_C2H_OK);
    CHECK(e.kind==RTW_8814AU_EVENT_RA && e.id==12 && e.sequence==0x7f);
    CHECK(e.rate==27 && e.short_gi && !e.mac_id && e.status==0xa1 && e.ra_ratio==0x70);
    /* Repeated sequence is valid telemetry; no packet-ACK semantics. */
    CHECK(rtw_8814au_parse_c2h(ra,8,&c,&e)==RTW_8814AU_C2H_OK);
    for(size_t n=0;n<8;++n) CHECK(rtw_8814au_parse_c2h(ra,n,&c,&e)==RTW_8814AU_C2H_MALFORMED);
    uint8_t bigger[9];std::memcpy(bigger,ra,8);bigger[8]=0;
    CHECK(rtw_8814au_parse_c2h(bigger,9,&c,&e)==RTW_8814AU_C2H_MALFORMED);
    uint8_t bad[8];std::memcpy(bad,ra,8);bad[3]=1;
    CHECK(rtw_8814au_parse_c2h(bad,8,&c,&e)==RTW_8814AU_C2H_IGNORED);
    bad[3]=0;bad[2]=28;CHECK(rtw_8814au_parse_c2h(bad,8,&c,&e)==RTW_8814AU_C2H_IGNORED);
    bad[2]=0x8b;CHECK(rtw_8814au_parse_c2h(bad,8,&c,&e)==RTW_8814AU_C2H_IGNORED);
    c=config();CHECK(rtw_8814au_parse_c2h(ra,8,&c,&e)==RTW_8814AU_C2H_IGNORED);
    bad[2]=11;CHECK(rtw_8814au_parse_c2h(bad,8,&c,&e)==RTW_8814AU_C2H_OK);
    CHECK(!e.short_gi && e.rate==11);
    CHECK(rtw_8814au_parse_c2h(ra,8,nullptr,&e)==RTW_8814AU_C2H_IGNORED);
    const uint8_t rf_on[2]={0x32,4};
    CHECK(rtw_8814au_parse_c2h(rf_on,2,nullptr,&e)==RTW_8814AU_C2H_OK);
    CHECK(e.kind==RTW_8814AU_EVENT_RF_ON && e.sequence==4);
    const uint8_t ccx[10]={3,7,0,0,0,0,0,0,0,0};
    CHECK(rtw_8814au_parse_c2h(ccx,10,&c,&e)==RTW_8814AU_C2H_IGNORED && e.kind==RTW_8814AU_EVENT_NONE);
    for(unsigned id=0;id<256;++id) {
        uint8_t unknown[2]={uint8_t(id),0};
        if(id==0xc || id==0x32) continue;
        CHECK(rtw_8814au_parse_c2h(unknown,2,nullptr,&e)==RTW_8814AU_C2H_IGNORED);
    }
    std::vector<uint8_t> huge(258,0x77);
    CHECK(rtw_8814au_parse_c2h(huge.data(),258,nullptr,&e)==RTW_8814AU_C2H_MALFORMED);
    CHECK(rtw_8814au_parse_c2h(huge.data(),257,nullptr,&e)==RTW_8814AU_C2H_IGNORED);
    CHECK(rtw_8814au_parse_c2h(nullptr,2,&c,&e)==RTW_8814AU_C2H_MALFORMED);
    CHECK(rtw_8814au_parse_c2h(ra,8,&c,nullptr)==RTW_8814AU_C2H_MALFORMED);
}
int main() {
    test_rates_and_h2c();test_tx();test_tx_ampdu();test_rx_and_phy();test_events();
    std::printf("RTL8814AU packet/control codec: %u checks passed\n",checks);
    return 0;
}
#endif
