// SPDX-License-Identifier: GPL-2.0-only
#include "rtw/RTL8192EUDataPath.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifndef RTW_92E_RX_ADMISSION_DIAGNOSTICS
int main(){std::fputs("FAIL: RX admission rejection reasons are unavailable\n",stderr);return 1;}
#else
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
// Independent snapshot of the previous admission policy: diagnostics must not
// admit a rejected frame or remove a previously accepted frame.
static bool previous_policy(const uint8_t*b,size_t n,int bar,const uint8_t*local,const uint8_t*peer){
    if(!b||n<2||n>4096||(b[0]&3))return false;
    if((b[0]&0x0c)!=4)return n>=24;
    return (b[0]&0xfc)==0x84&&n>=20&&bar==1&&!(b[1]&0x43)&&local&&peer&&
        !std::memcmp(b+4,local,6)&&!std::memcmp(b+10,peer,6);
}
int main(){
    std::array<uint8_t,4097> b{};
    const uint8_t local[6]={2,0,0,0,0,1},peer[6]={2,0,0,0,0,2};
    auto reason=[&](size_t n,int bar=1){return rtw_92e_rx_frame_reason(b.data(),n,bar,local,peer);};
    CHECK(rtw_92e_rx_frame_reason(nullptr,24,1,local,peer)==RTW_RX_REJECT_LENGTH);
    CHECK(reason(0)==RTW_RX_REJECT_LENGTH);CHECK(reason(1)==RTW_RX_REJECT_LENGTH);
    CHECK(reason(4097)==RTW_RX_REJECT_LENGTH);
    b[0]=0x08;CHECK(reason(23)==RTW_RX_REJECT_HEADER);CHECK(reason(24)==RTW_RX_ADMIT);
    b[0]=0x09;CHECK(reason(24)==RTW_RX_REJECT_VERSION);
    b[0]=0xd4;CHECK(reason(10)==RTW_RX_REJECT_CONTROL);
    b[0]=0x94;CHECK(reason(28)==RTW_RX_REJECT_CONTROL);
    b[0]=0x84;std::memcpy(b.data()+4,local,6);std::memcpy(b.data()+10,peer,6);
    CHECK(reason(19)==RTW_RX_REJECT_HEADER);CHECK(reason(20)==RTW_RX_ADMIT);
    CHECK(reason(20,0)==RTW_RX_REJECT_BAR_AUTH);CHECK(reason(20,2)==RTW_RX_REJECT_BAR_AUTH);
    CHECK(rtw_92e_rx_frame_reason(b.data(),20,1,nullptr,peer)==RTW_RX_REJECT_BAR_CONTEXT);
    CHECK(rtw_92e_rx_frame_reason(b.data(),20,1,local,nullptr)==RTW_RX_REJECT_BAR_CONTEXT);
    for(uint8_t flag:{uint8_t(1),uint8_t(2),uint8_t(0x40)}){b[1]=flag;CHECK(reason(20)==RTW_RX_REJECT_BAR_FLAGS);}
    b[1]=0;b[4]^=1;CHECK(reason(20)==RTW_RX_REJECT_BAR_RECEIVER);b[4]^=1;
    b[10]^=1;CHECK(reason(20)==RTW_RX_REJECT_BAR_TRANSMITTER);b[10]^=1;
    const size_t lengths[]={0,1,2,9,10,15,16,19,20,23,24,25,4096,4097};
    for(unsigned fc0=0;fc0<256;++fc0)for(unsigned fc1=0;fc1<256;++fc1)
        for(size_t n:lengths)for(int bar:{-1,0,1,2}){
            b[0]=(uint8_t)fc0;b[1]=(uint8_t)fc1;
            const bool expected=previous_policy(b.data(),n,bar,local,peer);
            auto why=reason(n,bar);
            CHECK((why==RTW_RX_ADMIT)==expected);
            CHECK((rtw_92e_rx_frame_allowed(b.data(),n,bar,local,peer)!=0)==expected);
            CHECK(why>=RTW_RX_ADMIT&&why<RTW_RX_REJECT_REASON_COUNT);
        }
    std::printf("RX admission: %u checks, previous acceptance policy unchanged\n",checks);
}
#endif
