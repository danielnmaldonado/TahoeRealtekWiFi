/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/RTL8192DUDataPath.h"
#include "rtw/FCS.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
static void put(uint8_t* p,uint32_t n) {for(unsigned i=0;i<4;++i)p[i]=uint8_t(n>>(8*i));}
static uint32_t get(const uint8_t* p) {return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
static uint16_t xorsum(const uint8_t* p) {uint16_t sum=0;for(unsigned i=0;i<32;i+=2)sum^=uint16_t(unsigned(p[i])|(unsigned(p[i+1])<<8));return sum;}
static std::vector<uint8_t> rx(bool ht=false) {
    const size_t mpdu=32,info=32,shift=2;
    std::vector<uint8_t> p(24+shift+info+mpdu+4);
    put(p.data(),uint32_t(mpdu+4)|(4U<<16)|(2U<<24)|(1U<<26)|(1U<<27));
    put(p.data()+4,0xc000);put(p.data()+12,ht ? 12U|64U:4U);
    put(p.data()+20,0x12345678);
    p[24+shift]=0x11;p[24+shift+4]=0x80;
    for(size_t i=0;i<mpdu;++i)p[24+shift+info+i]=uint8_t(i*3+7);
    put(p.data()+p.size()-4,rtw_fcs32(p.data()+24+shift+info,mpdu));return p;
}
int main() {
    std::array<uint8_t,32> d{};
    rtw_92du_tx_options o{};o.rate=4;o.raid=5;o.qsel=0x12;o.rf_paths=2;o.sequence=0x123;
    assert(rtw_92du_make_tx_descriptor(30,&o,d.data()));
    // DU Linux base.h: 32-byte header, no dummy offset; SW crypto/sequence.
    assert(get(d.data())==0x8c20001e && get(d.data()+4)==0x00051240);
    assert(get(d.data()+12)==0x01230000 && (get(d.data()+16)&0x580)==0x500);
    assert(get(d.data()+20)==0x0001ff04 && !xorsum(d.data()));
    assert(!(get(d.data()+4)&0x00c00000) && !(get(d.data()+16)&0x80));
    // Non-QoS unicast data still uses BE and the negotiated firmware RA.
    o.qsel=0;o.firmware_rate=1;
    assert(rtw_92du_make_tx_descriptor(30,&o,d.data()));
    assert(!(get(d.data()+16)&0x540));o.firmware_rate=0;
    o.rate=27;o.qsel=0;o.raid=0;o.qos=1;o.ampdu=1;o.max_agg=20;o.density=4;o.short_gi=1;o.is_5ghz=1;
    assert(rtw_92du_make_tx_descriptor(1000,&o,d.data()));
    assert((get(d.data()+4)&0x60)==0x20 && ((get(d.data()+8)>>20)&7)==4);
    assert((get(d.data()+16)&0x40) && (get(d.data()+20)&0x7f)==0x5b);
    assert(((get(d.data()+24)>>11)&31)==20 && !xorsum(d.data()));
    const auto saved=d;
    o.rf_paths=1;assert(!rtw_92du_make_tx_descriptor(1000,&o,d.data()) && d==saved);
    o.rate=19;assert(rtw_92du_make_tx_descriptor(1000,&o,d.data()));
    o.short_gi=0;assert(!rtw_92du_make_tx_descriptor(1000,&o,d.data()));
    o.short_gi=1;o.rate=0;assert(!rtw_92du_make_tx_descriptor(1000,&o,d.data()));
    o.rate=12;o.qos=0;assert(!rtw_92du_make_tx_descriptor(1000,&o,d.data()));
    o.qos=1;o.multicast=1;assert(!rtw_92du_make_tx_descriptor(1000,&o,d.data()));
    {
        rtw_92du_tx_options p{};p.rate=12;p.qsel=0;p.qos=1;p.rf_paths=2;
        p.protection=1;p.rts_rate=0;
        assert(rtw_92du_make_tx_descriptor(100,&p,d.data()) && (get(d.data()+16)&0x381f)==0x3000 && !xorsum(d.data()));
        p.protection=2;p.rts_short=1;p.rts_rate=3;
        assert(rtw_92du_make_tx_descriptor(100,&p,d.data()) && (get(d.data()+16)&0x0400381f)==0x04002803 && !xorsum(d.data()));
        p.is_5ghz=1;assert(!rtw_92du_make_tx_descriptor(100,&p,d.data()));
        p.rts_rate=4;p.rts_short=0;
        assert(rtw_92du_make_tx_descriptor(100,&p,d.data()) && (get(d.data()+16)&31)==4);
    }
    for(bool ht:{false,true}) {
        auto p=rx(ht);rtw_92du_rx_frame f{};
        assert(rtw_92du_parse_rx(p.data(),p.size(),&f)==RTW_PACKETS_OK);
        assert(f.packet.mpdu_length==32 && f.packet.mpdu==p.data()+58);
        // DU PHY info follows descriptor SHIFT bytes (not CU's pointer).
        assert(f.packet.phy_info==p.data()+26 && f.packet.phy_info[0]==0x11);
        assert(f.packet.phy_info_length==32 && f.packet.timestamp==0x12345678);
        assert(f.ht==ht && f.ampdu && f.first_ampdu && f.fcs_valid);
        assert(f.sw_decrypt_required && !f.security_code);
        assert(f.packet.rate_index==(ht?12:4));
        p.back()^=1;assert(rtw_92du_parse_rx(p.data(),p.size(),&f)==RTW_PACKETS_OK && !f.fcs_valid);
        assert(rtw_92du_parse_rx(p.data(),p.size()-1,&f)==RTW_PACKETS_TRUNCATED);
    }
    {
        auto p=rx();rtw_92du_rx_frame f{};
        put(p.data()+12,0x4004); // 88E report-type bits have no DU meaning.
        assert(rtw_92du_parse_rx(p.data(),p.size(),&f)==RTW_PACKETS_OK && f.packet.mpdu_length==32);
        put(p.data()+12,0x3f);assert(rtw_92du_parse_rx(p.data(),p.size(),&f)==RTW_PACKETS_MALFORMED);
        p=rx();p.insert(p.end(),32,0x42);
        assert(rtw_92du_parse_rx(p.data(),p.size(),&f)==RTW_PACKETS_MALFORMED);
        p=rx();p.insert(p.end(),7,0);assert(rtw_92du_parse_rx(p.data(),p.size(),&f)==RTW_PACKETS_OK);
        assert(rtw_92du_parse_rx(nullptr,0,&f)==RTW_PACKETS_BAD_ARGUMENT);
    }
    for(uint16_t mps:{uint16_t(64),uint16_t(512)})for(size_t n=24;n<=4096;++n) {
        rtw_92du_tx_layout layout{};
        assert(rtw_92du_tx_transfer_layout(n,mps,&layout));
        assert(layout.mpdu_offset==32U+8U*layout.packet_offset && layout.total==n+layout.mpdu_offset);
        assert(layout.total%mps && layout.packet_offset==((n+32U)%mps==0U));
        rtw_92du_tx_options data{};data.rate=4;data.raid=5;data.qsel=0;data.rf_paths=2;
        data.packet_offset=layout.packet_offset;
        assert(rtw_92du_make_tx_descriptor(n,&data,d.data()));
        assert(((get(d.data()+4)>>26)&31)==layout.packet_offset && !xorsum(d.data()));
    }
    {
        auto p=rx();rtw_92du_rx_frame frame{};rtw_92du_rx_signal sig{};
        assert(rtw_92du_parse_rx(p.data(),p.size(),&frame)==RTW_PACKETS_OK);
        assert(rtw_92du_parse_signal(&frame,2,0,&sig) && sig.dbm==-42 && sig.pwdb==58);
        // DU OFDM PWDB uses -106; CU's -110 formula is not interchangeable.
        put(p.data()+12,0);p[31]=0xc2;
        assert(rtw_92du_parse_rx(p.data(),p.size(),&frame)==RTW_PACKETS_OK);
        assert(rtw_92du_parse_signal(&frame,2,0,&sig) && sig.dbm==-48 && sig.pwdb==58);
        p[31]=0x62; // CCK AGC and CFOSHO[0] share PHY+5.
        assert(rtw_92du_parse_signal(&frame,1,1,&sig) && sig.dbm==-50 && sig.pwdb==56);
        assert(!rtw_92du_parse_signal(&frame,0,0,&sig));
    }
    std::puts("RTL8192DU datapath: independent32B TX/24B RX, HT2T2R, SHIFT-before-PHY, FCS, no guessed aggregate stride passed");
}
