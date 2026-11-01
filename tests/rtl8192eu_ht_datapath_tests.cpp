#include "rtw/RTL8192EUDataPath.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifndef RTW_92E_HT_TX_OPTIONS
struct rtw_92e_tx_options { uint8_t data, multicast, raid, rate, qsel, firmware_rate, ampdu, density, max_agg, short_preamble; uint16_t sequence; };
static int rtw_92e_make_tx_descriptor_ex(size_t n,const rtw_92e_tx_options*o,uint8_t*d) {
 return rtw_92e_make_tx_descriptor(n,o->data,o->multicast,o->raid==8,o->rate,o->sequence,d);
}
#endif
#ifndef RTW_92E_RX_BAR_FILTER
static int rtw_92e_rx_frame_allowed(const uint8_t*b,size_t n,int,const uint8_t*,const uint8_t*) { return b&&n>=24&&n<=4096; }
#endif
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static uint32_t word(const uint8_t*p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
static void primary_bits(uint8_t*p,unsigned first,unsigned count,uint32_t value){
 const uint32_t mask=((uint32_t(1)<<count)-1U)<<first;
 const uint32_t updated=(word(p)&~mask)|((value<<first)&mask);
 for(unsigned i=0;i<4;++i)p[i]=uint8_t(updated>>(8*i));
}
#define SET_BITS_TO_LE_4BYTE(p,first,count,value) primary_bits((p),(first),(count),(value))
#include "fixtures/rtl8192eu_txdesc_primary.h"
static void checksum(const std::array<uint8_t,40>&d){unsigned sum=0;for(unsigned i=0;i<32;i+=2)sum^=unsigned(d[i])|(unsigned(d[i+1])<<8);CHECK(sum==0);}
static void primary_phy_vectors(){
 // Literal PHY words follow BWMapping_92E / SCMapping_92E and the pinned
 // primary gen2 setters. Gen1 DW4 PHY fields, lost SGI, and missing primary
 // selection for 20-in-40 transmissions must all break this comparison.
 struct Vector{uint8_t packet40,radio40,above,sgi,preamble,rate,firmware,data,ampdu;uint32_t phy;};
 const Vector vectors[]={
  {0,0,0,0,0,27,1,1,0,0x00},{0,0,0,1,0,27,1,1,0,0x10},
  {1,1,0,0,0,27,1,1,0,0x20},{1,1,1,0,0,27,1,1,0,0x20},
  {1,1,0,1,0,27,1,1,1,0x30},{1,1,1,1,0,27,1,1,1,0x30},
  {0,1,0,0,0,4,0,1,0,0x02},{0,1,1,0,0,4,0,1,0,0x01},
  {0,1,0,0,0,4,0,0,0,0x02},{0,1,1,0,0,4,0,0,0,0x01},
  {0,0,0,0,1,1,0,1,0,0x10},{0,1,1,0,1,1,0,1,0,0x11},
  {0,1,0,1,0,27,1,1,0,0x12},{0,1,1,1,0,27,1,1,0,0x11}
 };
 for(const auto&v:vectors){
  rtw_92e_tx_options o{};o.data=v.data;o.raid=v.firmware?2:7;o.rate=v.rate;o.qsel=v.data?5:18;o.firmware_rate=v.firmware;
  o.bw40=v.packet40;o.channel_bw40=v.radio40;o.primary_above=v.above;o.short_gi=v.sgi;o.short_preamble=v.preamble;o.sequence=0xabc;
  o.ampdu=v.ampdu;o.density=v.ampdu?3:0;o.max_agg=v.ampdu?16:0;
  std::array<uint8_t,40> d{},primary{};
  SET_TX_DESC_PKT_SIZE_92E(primary.data(),1500);SET_TX_DESC_OFFSET_92E(primary.data(),40);
  SET_TX_DESC_LAST_SEG_92E(primary.data(),1);SET_TX_DESC_FIRST_SEG_92E(primary.data(),1);SET_TX_DESC_OWN_92E(primary.data(),1);
  SET_TX_DESC_RATE_ID_92E(primary.data(),o.raid);SET_TX_DESC_QUEUE_SEL_92E(primary.data(),o.qsel);
  SET_TX_DESC_AGG_ENABLE_92E(primary.data(),v.ampdu);SET_TX_DESC_BK_92E(primary.data(),v.data&&!v.ampdu);
  SET_TX_DESC_AMPDU_DENSITY_92E(primary.data(),o.density);SET_TX_DESC_USE_RATE_92E(primary.data(),!v.firmware);
  SET_TX_DESC_MAX_AGG_NUM_92E(primary.data(),o.max_agg);SET_TX_DESC_RTS_ENABLE_92E(primary.data(),v.ampdu);SET_TX_DESC_HW_RTS_ENABLE_92E(primary.data(),v.ampdu);
  SET_TX_DESC_TX_RATE_92E(primary.data(),v.rate);SET_TX_DESC_DATA_RATE_FB_LIMIT_92E(primary.data(),v.data?31:0);
  SET_TX_DESC_RTS_RATE_92E(primary.data(),v.data?8:0);SET_TX_DESC_RETRY_LIMIT_ENABLE_92E(primary.data(),!v.data);SET_TX_DESC_DATA_RETRY_LIMIT_92E(primary.data(),v.data?0:6);
  SET_TX_DESC_DATA_BW_92E(primary.data(),(v.phy>>5)&3);SET_TX_DESC_DATA_SC_92E(primary.data(),v.phy&15);SET_TX_DESC_DATA_SHORT_92E(primary.data(),(v.phy>>4)&1);SET_TX_DESC_SEQ_92E(primary.data(),0xabc);
  unsigned sum=0;for(unsigned i=0;i<32;i+=2)sum^=unsigned(primary[i])|(unsigned(primary[i+1])<<8);primary[28]=uint8_t(sum);primary[29]=uint8_t(sum>>8);
  CHECK(rtw_92e_make_tx_descriptor_ex(1500,&o,d.data()));CHECK(d==primary);CHECK(word(d.data()+20)==v.phy);checksum(d);
  const uint32_t radio=word(d.data()+20),retry=word(d.data()+16);
  for(unsigned protection=0;protection<3;++protection){CHECK(rtw_92e_tx_protection(protection,d.data()));CHECK(word(d.data()+20)==radio&&word(d.data()+16)==retry);checksum(d);}
  if(v.data){CHECK(rtw_92e_tx_report_request(1,d.data()));CHECK(word(d.data()+20)==radio&&word(d.data()+16)==retry);checksum(d);}
 }
 for(unsigned bad=0;bad<7;++bad){rtw_92e_tx_options o{};o.data=1;o.qsel=5;o.raid=2;o.firmware_rate=1;o.rate=27;
  switch(bad){case 0:o.channel_bw40=2;break;case 1:o.short_gi=2;break;case 2:o.short_gi=1;o.firmware_rate=0;break;case 3:o.short_gi=1;o.rate=4;break;case 4:o.short_gi=o.short_preamble=1;break;case 5:o.short_preamble=1;break;case 6:o.short_preamble=1;o.rate=0;break;}
  std::array<uint8_t,40>d;d.fill(0xa5);CHECK(!rtw_92e_make_tx_descriptor_ex(100,&o,d.data()));CHECK(d[0]==0xa5);
 }
}
int main(){
 primary_phy_vectors();
 std::array<uint8_t,40>d{};rtw_92e_tx_options o{};
 o.data=1;o.raid=2;o.rate=0;o.qsel=6;o.firmware_rate=1;o.sequence=4095;
 CHECK(rtw_92e_make_tx_descriptor_ex(1500,&o,d.data()));
 CHECK(word(d.data()+4)==0x00020600);CHECK((word(d.data()+12)&0x100)==0);
 CHECK(word(d.data()+8)==0x10000);CHECK(word(d.data()+16)==0x08001f00);
 CHECK(word(d.data()+32)==0);CHECK(word(d.data()+36)==0x00fff000);checksum(d);
 o.bw40=1;o.primary_above=0;
 CHECK(rtw_92e_make_tx_descriptor_ex(1500,&o,d.data()));
 // The pinned Realtek gen2 setters put DATA_BW in DW5, independently of
 // DW4's retry limit and RTS rate. Full-width HT40 needs no subchannel.
 std::array<uint8_t,40> primary{};SET_TX_DESC_DATA_BW_92E(primary.data(),1);
 CHECK(word(d.data()+20)==word(primary.data()+20));
 CHECK(word(d.data()+16)==0x08001f00U);checksum(d);
 o.primary_above=1;
 CHECK(rtw_92e_make_tx_descriptor_ex(1500,&o,d.data()));
 CHECK(word(d.data()+20)==0x20U);CHECK(word(d.data()+16)==0x08001f00U);checksum(d);
 o.bw40=0;o.primary_above=0;
 for(unsigned tid=0;tid<8;++tid){o.qsel=uint8_t(tid);o.sequence=uint16_t(tid*17);
  CHECK(rtw_92e_make_tx_descriptor_ex(128,&o,d.data()));CHECK(((word(d.data()+4)>>8)&31)==tid);
  CHECK((word(d.data()+4)&0xc00000)==0);CHECK(word(d.data()+36)==uint32_t(o.sequence)<<12);checksum(d);}
 o.qsel=5;o.ampdu=1;o.density=7;o.max_agg=16;
 CHECK(rtw_92e_make_tx_descriptor_ex(1500,&o,d.data()));
 CHECK(word(d.data()+8)==0x701000);CHECK(((word(d.data()+12)>>17)&31)==16);
 CHECK((word(d.data()+12)&0x3000)==0x3000);checksum(d);
 for(unsigned n=1;n<=31;++n){o.max_agg=uint8_t(n);o.density=uint8_t(n%8);
  CHECK(rtw_92e_make_tx_descriptor_ex(200,&o,d.data()));CHECK(((word(d.data()+12)>>17)&31)==n);
  CHECK(((word(d.data()+8)>>20)&7)==n%8);checksum(d);}
 o.ampdu=0;o.density=0;o.max_agg=0;o.firmware_rate=0;o.rate=4;o.raid=7;o.qsel=0;
 CHECK(rtw_92e_make_tx_descriptor_ex(100,&o,d.data()));CHECK(word(d.data()+12)==0x100);CHECK((word(d.data()+16)&127)==4);checksum(d);
 o.data=0;o.qsel=0x12;o.rate=4;o.raid=7;
 CHECK(rtw_92e_make_tx_descriptor_ex(50,&o,d.data()));CHECK(word(d.data()+12)==0x100);CHECK(word(d.data()+4)==0x00071200);
 CHECK((word(d.data()+16)&0x1ff007f)==(0x1a0000|4));checksum(d);
 for(unsigned mode=0;mode<10;++mode){auto bad=o;
  switch(mode){case 0:bad.rate=28;break;case 1:bad.qsel=8;break;case 2:bad.raid=9;break;case 3:bad.firmware_rate=1;break;
   case 4:bad.ampdu=1;bad.max_agg=1;break;case 5:bad.sequence=4096;break;case 6:bad.short_preamble=2;break;
   case 7:bad.data=1;bad.multicast=1;break;case 8:bad.data=1;bad.qsel=0;bad.firmware_rate=1;bad.ampdu=1;bad.max_agg=0;break;
   case 9:bad.data=1;bad.qsel=0;bad.firmware_rate=1;bad.ampdu=1;bad.max_agg=16;bad.density=8;break;}
  d.fill(0xa5);CHECK(!rtw_92e_make_tx_descriptor_ex(100,&bad,d.data()));CHECK(d[0]==0xa5);}
 CHECK(!rtw_92e_make_tx_descriptor_ex(0,&o,d.data()));CHECK(!rtw_92e_make_tx_descriptor_ex(4097,&o,d.data()));
 CHECK(!rtw_92e_make_tx_descriptor_ex(100,nullptr,d.data()));CHECK(!rtw_92e_make_tx_descriptor_ex(100,&o,nullptr));
 // A BAR is20 bytes, has no normal24-byte seq/addr3 fields and is
 // accepted only from the current AP after HT authorization.
 std::array<uint8_t,24>b{};uint8_t local[6]={2,0,0,0,0,1},peer[6]={2,0,0,0,0,2};
 b[0]=0x84;std::memcpy(b.data()+4,local,6);std::memcpy(b.data()+10,peer,6);
 CHECK(rtw_92e_rx_frame_allowed(b.data(),20,1,local,peer));
 for(unsigned n=0;n<20;n++)CHECK(!rtw_92e_rx_frame_allowed(b.data(),n,1,local,peer));
 CHECK(!rtw_92e_rx_frame_allowed(b.data(),20,0,local,peer));
 CHECK(!rtw_92e_rx_frame_allowed(b.data(),20,1,nullptr,peer));CHECK(!rtw_92e_rx_frame_allowed(b.data(),20,1,local,nullptr));
 for(unsigned off:{4U,10U}){b[off]^=1;CHECK(!rtw_92e_rx_frame_allowed(b.data(),20,1,local,peer));b[off]^=1;}
 for(uint8_t fc:{uint8_t(0x94),uint8_t(0xd4),uint8_t(0x85)}){b[0]=fc;CHECK(!rtw_92e_rx_frame_allowed(b.data(),24,1,local,peer));}b[0]=0x84;
 for(uint8_t fc:{uint8_t(1),uint8_t(2),uint8_t(3),uint8_t(0x40)}){b[1]=fc;CHECK(!rtw_92e_rx_frame_allowed(b.data(),20,1,local,peer));}b[1]=0;
 b[0]=0x08;CHECK(!rtw_92e_rx_frame_allowed(b.data(),23,0,nullptr,nullptr));CHECK(rtw_92e_rx_frame_allowed(b.data(),24,0,nullptr,nullptr));
 CHECK(!rtw_92e_rx_frame_allowed(nullptr,24,0,nullptr,nullptr));CHECK(!rtw_92e_rx_frame_allowed(b.data(),4097,0,nullptr,nullptr));
 std::printf("HT gen2 descriptors: %u checks passed\n",checks);
}
