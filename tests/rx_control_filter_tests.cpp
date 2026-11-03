// SPDX-License-Identifier: GPL-2.0-only
#include "rtw/RTL8192EUDataPath.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifndef RTW_92E_RX_BA_CONSUME
int main(){std::fputs("FAIL: strict BA consume helper unavailable\n",stderr);return 1;}
#else
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static const uint8_t local[6]={2,0,0,0,0,1},peer[6]={2,0,0,0,0,2};
// Complete local metadata buffers only: opaque bitmap, no RF or USB harness.
static std::array<uint8_t,153> ordinary(bool compressed,unsigned tid=0,unsigned ack=0,unsigned seq=0){
    std::array<uint8_t,153>b{};b.fill(0x5a);b[0]=0x94;b[1]=b[2]=b[3]=0;
    std::memcpy(b.data()+4,local,6);std::memcpy(b.data()+10,peer,6);
    const unsigned control=(tid<<12)|(compressed?4U:0U)|ack;
    b[16]=static_cast<uint8_t>(control);b[17]=static_cast<uint8_t>(control>>8);
    b[18]=static_cast<uint8_t>(seq<<4);b[19]=static_cast<uint8_t>(seq>>4);return b;
}
static bool accepts(const std::array<uint8_t,153>&b,size_t n,int authorized=1,const uint8_t*l=local,const uint8_t*p=peer){
    const auto before=b;const int value=rtw_92e_rx_ba_consumable(b.data(),n,authorized,l,p);
    CHECK(value==0||value==1);CHECK(b==before);return value!=0;
}
int main(){
    for(bool compressed:{false,true}){
        const size_t length=compressed?28U:148U;
        for(unsigned tid=0;tid<16;++tid)for(unsigned ack:{0U,1U})for(unsigned seq:{0U,1U,4095U}){
            const auto b=ordinary(compressed,tid,ack,seq);CHECK(accepts(b,length));
            // Existing admission semantics are deliberately preserved.
            CHECK(rtw_92e_rx_frame_reason(b.data(),length,1,local,peer)==RTW_RX_REJECT_CONTROL);
        }
        auto b=ordinary(compressed);
        for(size_t n=0;n<=b.size();++n)CHECK(accepts(b,n)==(n==length));
        CHECK(!accepts(b,static_cast<size_t>(-1)));
        for(int authorized:{-1,0,2})CHECK(!accepts(b,length,authorized));
        CHECK(!accepts(b,length,1,nullptr,peer));CHECK(!accepts(b,length,1,local,nullptr));
        CHECK(!rtw_92e_rx_ba_consumable(nullptr,length,1,local,peer));
        for(unsigned fc0=0;fc0<256;++fc0){b[0]=static_cast<uint8_t>(fc0);CHECK(accepts(b,length)==(fc0==0x94));}b[0]=0x94;
        for(unsigned fc1=0;fc1<256;++fc1){b[1]=static_cast<uint8_t>(fc1);CHECK(accepts(b,length)==(fc1==0));}b[1]=0;
        for(unsigned bit:{1U,3U,4U,5U,6U,7U,8U,9U,10U,11U}){
            const unsigned control=(compressed?4U:0U)|(1U<<bit);
            b[16]=static_cast<uint8_t>(control);b[17]=static_cast<uint8_t>(control>>8);CHECK(!accepts(b,length));
        }
        b=ordinary(!compressed);CHECK(!accepts(b,length));b=ordinary(compressed);
        for(unsigned fragment=1;fragment<16;++fragment){b[18]=static_cast<uint8_t>(fragment);CHECK(!accepts(b,length));}b[18]=0;
        for(unsigned offset:{4U,10U})for(unsigned byte=0;byte<6;++byte){b[offset+byte]^=1;CHECK(!accepts(b,length));b[offset+byte]^=1;}
        b[20]^=0xff;b[length-1]^=0xff;CHECK(accepts(b,length)); // Bitmap contents are not TX ACK input.
    }
    std::printf("Strict BA consume helper: %u local metadata/boundary checks passed\n",checks);
}
#endif
