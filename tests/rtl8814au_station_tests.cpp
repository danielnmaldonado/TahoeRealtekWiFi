/* SPDX-License-Identifier: BSD-3-Clause */
#include "rtw/RTL8814AUStation.h"
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <vector>
struct Write {uint16_t address;unsigned width;uint32_t value;};
struct Bus {uint8_t bytes[65536]{};std::vector<Write>writes;unsigned calls=0,fail=0;};
static int rd(void *p,uint16_t a,unsigned w,uint32_t*v){auto&b=*static_cast<Bus*>(p);if(++b.calls==b.fail)return EIO;*v=0;for(unsigned i=0;i<w;i++)*v|=uint32_t(b.bytes[a+i])<<(8*i);return 0;}
static int wr(void *p,uint16_t a,unsigned w,uint32_t v){auto&b=*static_cast<Bus*>(p);if(++b.calls==b.fail)return EIO;b.writes.push_back({a,w,v});for(unsigned i=0;i<w;i++)b.bytes[a+i]=uint8_t(v>>(8*i));if(a==0x670)b.bytes[a+3]&=0x7f;return 0;}
static void delay(void*,uint32_t){}
static rtw_8814au context(Bus&b){rtw_8814au c{};rtw_8814au_bus_ops o{&b,rd,wr,nullptr,delay};rtw_8814au_init(&c,&o);c.board_valid=c.powered=c.fw_running=c.mac_initialized=c.phy_initialized=1;return c;}
static uint32_t value(Bus&b,uint16_t a,unsigned w){uint32_t v=0;for(unsigned i=0;i<w;i++)v|=uint32_t(b.bytes[a+i])<<(8*i);return v;}
static rtw_8814au_station_params params(){rtw_8814au_station_params p{};const uint8_t m[6]={2,1,2,3,4,5},ap[6]={2,6,7,8,9,10};std::memcpy(p.mac,m,6);std::memcpy(p.bssid,ap,6);p.aid=42;p.basic_rates=0x15f;p.short_slot=1;return p;}
int main(){
    Bus b;auto c=context(b);auto p=params();
    assert(rtw_8814au_station_initialize(&c,&p)==0);
    assert(value(b,0x680,2)==0);assert(value(b,0x670,4)==0x40000000U);
    assert(value(b,0x608,4)==0xf010400eU); // FCS/MIC/ICV/PHY+PKTCTL/HTC, broadcast/multicast/own
    assert(value(b,0x6a0,2)==0xffff);assert(value(b,0x6a2,2)==0);assert(value(b,0x6a4,2)==0);
    assert(std::memcmp(b.bytes+0x610,p.mac,6)==0);
    for(bool five:{false,true,false}){
        p.band_5g=five;p.basic_rates=five?0x150:0x15f;
        b.bytes[0x440]=0x5d;b.bytes[0x441]=0;b.writes.clear();
        assert(rtw_8814au_station_configure(&c,RTW_8814AU_STATION_AUTH,&p)==0);
        assert((value(b,0x440,4)&0xfffff)==p.basic_rates);
        size_t rate=b.writes.size(),resume=b.writes.size();
        for(size_t i=0;i<b.writes.size();i++){
            if(b.writes[i].address==0x440)rate=i;
            if(b.writes[i].address==0x522&&b.writes[i].value==15)resume=i;
        }
        assert(rate<resume);
    }
    assert(rtw_8814au_station_configure(&c,RTW_8814AU_STATION_SCAN,&p)==0);
    assert((value(b,0x608,4)&0xc0)==0);assert((value(b,0x100,4)&0x30000)==0);
    assert(value(b,0x522,1)==15);assert(value(b,0x550,1)&16);
    assert(rtw_8814au_station_configure(&c,RTW_8814AU_STATION_RUN,&p)==0);
    assert((value(b,0x608,4)&0xc0)==0xc0);assert((value(b,0x100,4)&0x30000)==0x20000);
    assert(std::memcmp(b.bytes+0x618,p.bssid,6)==0);assert(value(b,0x6a8,4)==42);
    assert(value(b,0x440,4)==0x15f);assert(value(b,0x6a4,2)==0xffff);assert(value(b,0x522,1)==0);
    assert((value(b,0x550,1)&0x18)==8);
    assert(b.writes.back().address==0x100); // Associated media is the final hardware operation.
    p.ht=1;assert(rtw_8814au_station_configure(&c,RTW_8814AU_STATION_RUN,&p)==0);
    assert(value(b,0x6a2,2)==0x100); // Only BlockAckReq reaches host reorder.
    assert(rtw_8814au_station_configure(&c,RTW_8814AU_STATION_INIT,nullptr)==0);
    assert(value(b,0x6a2,2)==0);
    c.phy_initialized=0;p.passive_scan=1;p.band_5g=1;p.basic_rates=0x150;
    assert(rtw_8814au_station_configure(&c,RTW_8814AU_STATION_SCAN,&p)==0);
    assert(value(b,0x522,1)==0x3f&&(value(b,0x608,4)&0xc0)==0&&value(b,0x6a2,2)==0);
    p.passive_scan=0;auto paused=b.writes.size();
    assert(rtw_8814au_station_configure(&c,RTW_8814AU_STATION_SCAN,&p)==ENXIO&&b.writes.size()==paused);
    c.phy_initialized=1;p=params();

    assert(rtw_8814au_station_configure(&c,RTW_8814AU_STATION_INIT,nullptr)==0);
    assert((value(b,0x100,4)&0x30000)==0);assert(value(b,0x6a4,2)==0);
    rtw_8814au_edca_ac edca[4]={{3,4,10,0},{7,4,10,0},{2,3,4,94},{2,2,3,47}};
    assert(rtw_8814au_station_edca(&c,1,edca)==0);assert(value(b,0x508,4)==0xa425);assert(value(b,0x504,4)==0x5e431c);
    assert(value(b,0x51b,1)==9);
    // 5GHz has16us SIFS,9us slots and no CCK basic rates.
    edca[3].txop=65535;auto before_edca=b.writes.size();
    assert(rtw_8814au_station_edca(&c,1,edca)==EINVAL&&b.writes.size()==before_edca);
    edca[3].txop=2047;b.bytes[0x503]=0xa8;
    assert(rtw_8814au_station_edca(&c,1,edca)==0);
    assert(((value(b,0x500,4)>>16)&0x7ff)==2047);
    assert((value(b,0x500,4)&0xf8000000U)==0xa8000000U);edca[3].txop=47;
    p.band_5g=1;p.short_slot=0;p.basic_rates=0x150;
    assert(rtw_8814au_station_configure(&c,RTW_8814AU_STATION_RUN,&p)==0);
    assert(value(b,0x51b,1)==9);
    assert(rtw_8814au_station_edca_band(&c,0,1,edca)==0);
    assert(value(b,0x508,4)==0xa42b); // 3*9+16=43us AIFS.
    auto blocked=b.writes.size();p.basic_rates=1;
    assert(rtw_8814au_station_configure(&c,RTW_8814AU_STATION_RUN,&p)==EINVAL);
    assert(b.writes.size()==blocked);
    p=params();
    const auto before=b.writes.size();edca[0].ecwmin=16;assert(rtw_8814au_station_edca(&c,1,edca)==EINVAL);assert(b.writes.size()==before);
    p.bssid[0]=1;assert(rtw_8814au_station_configure(&c,RTW_8814AU_STATION_RUN,&p)==EINVAL);assert(b.writes.size()==before);
    p=params();p.aid=2008;assert(rtw_8814au_station_configure(&c,RTW_8814AU_STATION_RUN,&p)==EINVAL);assert(b.writes.size()==before);
    p=params();c.phy_initialized=0;assert(rtw_8814au_station_configure(&c,RTW_8814AU_STATION_AUTH,&p)==ENXIO);assert(b.writes.size()==before);
    // Every failed I/O must propagate and cannot commit associated media.
    Bus reference;auto rc=context(reference);p=params();assert(!rtw_8814au_station_configure(&rc,RTW_8814AU_STATION_RUN,&p));
    for(unsigned failure=1;failure<=reference.calls;failure++){
        Bus bad;auto bc=context(bad);bad.fail=failure;assert(rtw_8814au_station_configure(&bc,RTW_8814AU_STATION_RUN,&p)==EIO);
        assert((value(bad,0x100,4)&0x30000)==0);
    }
    std::printf("8814AU station: register contracts and %u injected I/O failures passed\n",reference.calls);
}
