/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/RTL8723BUDataPath.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL%u: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static uint32_t get32(const uint8_t*p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
static void put32(uint8_t*p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=uint8_t(v>>(i*8));}
int main(){
 std::array<uint8_t,40> d{};rtw_8723bu_tx_options o{1,0,3,19,0,1,0,0xabc};
 CHECK(rtw_8723bu_make_tx_descriptor(1500,&o,d.data()));
 CHECK(get32(d.data())==0x8c2805dc&&get32(d.data()+4)==0x00030002&&get32(d.data()+8)==0x00010000);
 CHECK(get32(d.data()+12)==0&&get32(d.data()+16)==0x08001f13&&get32(d.data()+36)==0x00abc000);
 uint16_t checksum=0;for(unsigned i=0;i<32;i+=2)checksum^=uint16_t(d[i]|(unsigned(d[i+1])<<8));CHECK(checksum==0);
 for(unsigned raid:{3U,5U,6U,7U,8U}){o.raid=uint8_t(raid);o.rate=uint8_t(raid>=6?(raid==8?3:11):19);CHECK(rtw_8723bu_make_tx_descriptor(24,&o,d.data()));CHECK((get32(d.data()+4)&127U)==2);}
 o.rate=20;auto saved=d;CHECK(!rtw_8723bu_make_tx_descriptor(24,&o,d.data())&&d==saved);o.rate=19;o.raid=2;CHECK(!rtw_8723bu_make_tx_descriptor(24,&o,d.data()));
 o={0,1,8,0,0x12,0,0,1};CHECK(rtw_8723bu_make_tx_descriptor(24,&o,d.data()));CHECK((get32(d.data()+4)&127U)==0&&get32(d.data()+12)==0x100);
 std::array<uint8_t,64> packet{};put32(packet.data(),28);put32(packet.data()+12,19);packet[24]=8;rtw_rx_iterator it{};rtw_8192eu_rx_frame out{};
 rtw_rx_iterator_init(&it,packet.data(),56);CHECK(rtw_8723bu_rx_next(&it,&out)==RTW_PACKETS_OK&&out.payload.mpdu_length==24&&!out.report_is_c2h);
 put32(packet.data(),9);put32(packet.data()+8,1U<<28);packet[24]=9;rtw_rx_iterator_init(&it,packet.data(),40);CHECK(rtw_8723bu_rx_next(&it,&out)==RTW_PACKETS_OK&&out.payload.mpdu_length==9&&out.report_is_c2h);
 put32(packet.data(),3);put32(packet.data()+8,0);rtw_rx_iterator_init(&it,packet.data(),32);CHECK(rtw_8723bu_rx_next(&it,&out)==RTW_PACKETS_MALFORMED);
 std::printf("8723BU datapath: %u wire/MACID2/FCS/CCMP boundary checks passed\n",checks);
}
