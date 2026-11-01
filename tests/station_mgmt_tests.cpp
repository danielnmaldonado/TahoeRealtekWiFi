/* SPDX-License-Identifier: GPL-2.0-only. No hardware needed. */
#include "rtw/StationMgmt.h"
#include "rtw/FCS.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static unsigned checks=0;
#define CHECK(e) do{++checks;if(!(e)){std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#e);std::abort();}}while(0)
namespace {
constexpr uint8_t sta[6]={0x02,0x11,0x22,0x33,0x44,0x55};
constexpr uint8_t ap[6]={0x02,0x66,0x77,0x88,0x99,0xaa};
constexpr uint8_t ssid[4]={'T','e','s','t'};
constexpr uint8_t rates[4]={0x82,0x84,0x8b,0x96};
void le16(uint8_t* p,uint16_t val){p[0]=uint8_t(val);p[1]=uint8_t(val>>8U);}
uint16_t load16(const uint8_t* p){return uint16_t(p[0]) | uint16_t(uint16_t(p[1])<<8U);}
void response(uint8_t* b,uint8_t type,uint16_t code,uint16_t extra){
    std::memset(b,0,64);
    b[0]=type;
    std::memcpy(b+4,sta,6);std::memcpy(b+10,ap,6);
    std::memcpy(b+16,ap,6);
    if(type==0xb0){le16(b+24,0);le16(b+26,2);le16(b+28,code);}
    else if(type==0x10){le16(b+24,0x0001);le16(b+26,code);le16(b+28,extra);
        b[30]=1;b[31]=1;b[32]=0x82;}
    else le16(b+24,code);
}
void fcs(){
    static constexpr uint8_t test[9]={'1','2','3','4','5','6','7','8','9'};
    CHECK(rtw_fcs32(test,sizeof(test))==0xcbf43926U);
    CHECK(rtw_fcs32(nullptr,0)==0U);
    std::array<uint8_t,13> p{};
    std::memcpy(p.data(),test,sizeof(test));
    le16(p.data()+9,0x3926);le16(p.data()+11,0xcbf4);
    CHECK(rtw_fcs_valid(p.data(),p.size()));
    CHECK(!rtw_fcs_valid(p.data(),3));
    CHECK(!rtw_fcs_valid(nullptr,0));
    p[5]^=0x1;
    CHECK(!rtw_fcs_valid(p.data(),p.size()));
}
void basic(){
    rtw_sta_session s{};std::array<uint8_t,160> bytes{};
    size_t length=111;
    CHECK(rtw_sta_configure_open(&s,sta,ap,ssid,4,6,0,0)==RTW_STA_OK);
    CHECK(s.state==RTW_STA_CONFIGURED && s.ssid_length==4);
    CHECK(rtw_sta_configure_open(&s,s.station,s.bssid,s.ssid,s.ssid_length,6,0,0)==RTW_STA_OK);
    CHECK(s.state==RTW_STA_CONFIGURED && !std::memcmp(s.ssid,ssid,4));
    CHECK(rtw_sta_build_assoc_request(&s,7,1,rates,4,bytes.data(),bytes.size(),&length)==RTW_STA_BAD_STATE);
    CHECK(length==0);
    CHECK(rtw_sta_build_auth_request(&s,17,bytes.data(),15,&length)==RTW_STA_BUFFER_TOO_SMALL);
    CHECK(s.state==RTW_STA_CONFIGURED && length==0);
    CHECK(rtw_sta_build_auth_request(&s,17,bytes.data(),bytes.size(),&length)==RTW_STA_OK);
    CHECK(length==30 && bytes[0]==0xb0 && load16(bytes.data()+26)==1);
    CHECK(load16(bytes.data()+22)==(17U<<4U));
    CHECK(!std::memcmp(bytes.data()+4,ap,6) && !std::memcmp(bytes.data()+10,sta,6));
    CHECK(s.state==RTW_STA_AUTH_PENDING);
    std::array<uint8_t,64> rx{};
    response(rx.data(),0xb0,0,0);
    rx[10]^=1U;
    CHECK(rtw_sta_accept_auth_response(&s,rx.data(),30)==RTW_STA_WRONG_PEER);
    CHECK(s.state==RTW_STA_AUTH_PENDING);
    rx[10]^=1U;
    rx[26]=3;
    CHECK(rtw_sta_accept_auth_response(&s,rx.data(),30)==RTW_STA_MALFORMED_RESPONSE);
    CHECK(s.state==RTW_STA_AUTH_PENDING);
    rx[26]=2;
    CHECK(rtw_sta_accept_auth_response(&s,rx.data(),29)==RTW_STA_MALFORMED_RESPONSE);
    rx[1]=0x40;
    CHECK(rtw_sta_accept_auth_response(&s,rx.data(),30)==RTW_STA_MALFORMED_RESPONSE);
    rx[1]=0;
    CHECK(rtw_sta_accept_auth_response(&s,rx.data(),30)==RTW_STA_OK);
    CHECK(s.state==RTW_STA_AUTHENTICATED);
    const uint8_t badRates[2]={0x82,0x00};
    CHECK(rtw_sta_build_assoc_request(&s,0,1,badRates,2,bytes.data(),160,&length)==RTW_STA_BAD_ARGUMENT);
    CHECK(s.state==RTW_STA_AUTHENTICATED);
    CHECK(rtw_sta_build_assoc_request(&s,18,10,rates,4,bytes.data(),25,&length)==RTW_STA_BUFFER_TOO_SMALL);
    CHECK(s.state==RTW_STA_AUTHENTICATED);
    CHECK(rtw_sta_build_assoc_request(&s,18,10,rates,4,bytes.data(),bytes.size(),&length)==RTW_STA_OK);
    CHECK(length==40 && bytes[0]==0 && load16(bytes.data()+24)==1 && load16(bytes.data()+26)==10);
    CHECK(bytes[28]==0 && bytes[29]==4 && !std::memcmp(bytes.data()+30,ssid,4));
    CHECK(bytes[34]==1 && bytes[35]==4 && !std::memcmp(bytes.data()+36,rates,4));
    CHECK(s.state==RTW_STA_ASSOC_PENDING && s.aid==0);
    response(rx.data(),0x10,0,0xc001);
    rx[22]=0x01;
    CHECK(rtw_sta_accept_assoc_response(&s,rx.data(),33)==RTW_STA_MALFORMED_RESPONSE);
    rx[22]=0;
    rx[28]=0;
    CHECK(rtw_sta_accept_assoc_response(&s,rx.data(),33)==RTW_STA_MALFORMED_RESPONSE);
    rx[28]=1;
    CHECK(rtw_sta_accept_assoc_response(&s,rx.data(),30)==RTW_STA_MALFORMED_RESPONSE);
    rx[31]=2; // truncated Supported Rates IE
    CHECK(rtw_sta_accept_assoc_response(&s,rx.data(),33)==RTW_STA_MALFORMED_RESPONSE);
    rx[31]=1;
    rx[33]=1;rx[34]=1;rx[35]=0x82;
    CHECK(rtw_sta_accept_assoc_response(&s,rx.data(),36)==RTW_STA_MALFORMED_RESPONSE); // duplicate rates IE
    CHECK(rtw_sta_accept_assoc_response(&s,rx.data(),33)==RTW_STA_OK);
    CHECK(s.state==RTW_STA_ASSOCIATED && s.aid==1);
    CHECK(rtw_sta_accept_auth_response(&s,rx.data(),30)==RTW_STA_BAD_STATE);
    response(rx.data(),0xc0,7,0);
    CHECK(rtw_sta_accept_disconnect(&s,rx.data(),26)==RTW_STA_OK);
    CHECK(s.state==RTW_STA_DISCONNECTED && !s.aid && s.status==7);
    CHECK(rtw_sta_build_auth_request(&s,0,bytes.data(),160,&length)==RTW_STA_BAD_STATE);
    rtw_sta_reset(&s);
    CHECK(s.state==RTW_STA_EMPTY);
}
void rejection(){
    rtw_sta_session s{};std::array<uint8_t,64> rx{};
    uint8_t addr[6];std::memcpy(addr,ap,6);addr[0]=0xff;
    CHECK(rtw_sta_configure_open(&s,sta,addr,ssid,4,6,0,0)==RTW_STA_BAD_ARGUMENT);
    CHECK(s.state==RTW_STA_EMPTY);
    CHECK(rtw_sta_configure_open(&s,sta,ap,ssid,4,6,1,0)==RTW_STA_UNSUPPORTED_AP);
    CHECK(rtw_sta_configure_open(&s,sta,ap,ssid,4,11,0,0)==RTW_STA_UNSUPPORTED_AP);
    CHECK(rtw_sta_configure_open(&s,sta,ap,ssid,4,6,0,1)==RTW_STA_UNSUPPORTED_AP);
    uint8_t hidden[4]={};
    CHECK(rtw_sta_configure_open(&s,sta,ap,hidden,4,6,0,0)==RTW_STA_BAD_ARGUMENT);
    CHECK(rtw_sta_configure_open(&s,sta,ap,ssid,4,6,0,0)==RTW_STA_OK);
    std::array<uint8_t,80> tx{};size_t n=0;
    CHECK(rtw_sta_build_auth_request(&s,4096,tx.data(),80,&n)==RTW_STA_BAD_ARGUMENT);
    CHECK(s.state==RTW_STA_CONFIGURED);
    CHECK(rtw_sta_build_auth_request(&s,0,tx.data(),80,&n)==RTW_STA_OK);
    response(rx.data(),0xb0,13,0);
    CHECK(rtw_sta_accept_auth_response(&s,rx.data(),30)==RTW_STA_AP_REJECTED);
    CHECK(s.status==13 && s.state==RTW_STA_REJECTED);
    CHECK(rtw_sta_configure_open(&s,sta,ap,ssid,4,6,0,0)==RTW_STA_OK);
    CHECK(rtw_sta_build_auth_request(&s,0,tx.data(),80,&n)==RTW_STA_OK);
    response(rx.data(),0xb0,0,0);
    CHECK(rtw_sta_accept_auth_response(&s,rx.data(),30)==RTW_STA_OK);
    CHECK(rtw_sta_build_assoc_request(&s,0,1,rates,4,tx.data(),80,&n)==RTW_STA_OK);
    response(rx.data(),0x10,17,0);
    CHECK(rtw_sta_accept_assoc_response(&s,rx.data(),33)==RTW_STA_AP_REJECTED);
    CHECK(s.status==17 && s.state==RTW_STA_REJECTED);
}
void fuzz(){
    uint32_t seed=0xbac2da13U;
    std::array<uint8_t,160> raw{};
    for(unsigned i=0;i<15000;++i){
        for(auto& b:raw){seed^=seed<<13U;seed^=seed>>17U;seed^=seed<<5U;b=uint8_t(seed);}
        rtw_sta_session s{};size_t length=0;
        std::array<uint8_t,80> tx{};
        CHECK(rtw_sta_configure_open(&s,sta,ap,ssid,4,6,0,0)==RTW_STA_OK);
        CHECK(rtw_sta_build_auth_request(&s,0,tx.data(),tx.size(),&length)==RTW_STA_OK);
        (void)rtw_sta_accept_auth_response(&s,raw.data(),i%raw.size());
        CHECK(s.state==RTW_STA_AUTH_PENDING || s.state==RTW_STA_REJECTED || s.state==RTW_STA_AUTHENTICATED);
        if(s.state==RTW_STA_AUTHENTICATED){
            CHECK(rtw_sta_build_assoc_request(&s,0,1,rates,4,tx.data(),tx.size(),&length)==RTW_STA_OK);
            (void)rtw_sta_accept_assoc_response(&s,raw.data(),i%raw.size());
        }
    }
}
}
int main(){fcs();basic();rejection();fuzz();std::printf("rtw-station-tests: %u assertions, no radio, no network traffic\n",checks);}
