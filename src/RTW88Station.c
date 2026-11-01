/* SPDX-License-Identifier: BSD-3-Clause
 * Port0/EDCA/software security: Linuxrtw88 main.c,mac80211.c,sec.c,reg.h;
 * H2C/rate layout: fw.c. Commit0e135910deec43ffe97eca0b6af1f8f847285b59.
 * Station-only checked-I/O adaptation. No AU device-context reuse. */
#include "rtw/RTW88Station.h"
#include "rtw/StationChannels.h"
#ifdef KERNEL
#include <sys/errno.h>
#else
#include <errno.h>
#endif
#include <string.h>
static int family(rtw_wifi_family f)
{
 return f==RTW_WIFI_RTL8821CU||f==RTW_WIFI_RTL8822BU||f==RTW_WIFI_RTL8822CU;
}
static int channel(unsigned ch)
{
 return (ch>=1&&ch<=11)||(ch<=255U&&rtw_station_5ghz_channel((uint8_t)ch));
}
static int valid_mac(const uint8_t *p)
{
 unsigned bits=0,i;if(!p)return 0;for(i=0;i<6;i++)bits|=p[i];return bits&&!(p[0]&1U);
}
static int overlap(const void*a,size_t an,const void*b,size_t bn)
{
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return an&&bn&&(x<=y?y-x<an:x-y<bn);
}
static int mac_ready(const rtw88_mac_context*m)
{
 const rtw88_fw_context*d;
 if(!m||!(d=m->device)||!family(d->family)||m->fifo.family!=d->family||
    !d->bus.read||!d->bus.write||!d->bus.delay_us)return EINVAL;
 if(d->last_error)return d->last_error;
 return m->mac_ready==1&&d->identified==1&&d->powered==1&&d->firmware_running==1?0:ENXIO;
}
static int latch(rtw88_station_context*c,int e)
{
 if(e<0)e=e==INT32_MIN?EIO:-e;
 if(e&&!c->mac->device->last_error)c->mac->device->last_error=e;
 return e;
}
static int read_reg(rtw88_station_context*c,uint16_t a,unsigned n,uint32_t*v)
{
 rtw88_fw_context*d=c->mac->device;*v=0;
 return d->last_error?d->last_error:latch(c,d->bus.read(d->bus.cookie,a,n,v));
}
static int write_reg(rtw88_station_context*c,uint16_t a,unsigned n,uint32_t v)
{
 rtw88_fw_context*d=c->mac->device;
 return d->last_error?d->last_error:latch(c,d->bus.write(d->bus.cookie,a,n,v));
}
static int update(rtw88_station_context*c,uint16_t a,unsigned n,uint32_t mask,uint32_t value)
{
 uint32_t old;int e=read_reg(c,a,n,&old);return e?e:write_reg(c,a,n,(old&~mask)|(value&mask));
}
static int fail(rtw88_station_context*c,int e)
{
 rtw88_fw_context*d=c->mac->device;uint32_t control=0;
 (void)latch(c,e);c->initialized=0;c->mac->mac_ready=0;
 /* Bypass the sticky error only for bounded best-effort quiescence. */
 (void)d->bus.write(d->bus.cookie,0x522,1,0x3f);
 if(!d->bus.read(d->bus.cookie,0x100,4,&control))
  (void)d->bus.write(d->bus.cookie,0x100,4,control&~0x30000U);
 return e;
}
static int address(rtw88_station_context*c,uint16_t a,const uint8_t *bytes)
{
 unsigned i;int e;for(i=0;i<6;i++)if((e=write_reg(c,(uint16_t)(a+i),1,bytes[i])))return e;return 0;
}
static int ready(rtw88_station_context*c)
{
 int e;if(!c||!c->radio_state)return EINVAL;e=mac_ready(c->mac);
 return e?e:c->initialized==1?0:ENXIO;
}
#define TRY(operation) do{int result_=(operation);if(result_)return fail(c,result_);}while(0)
int rtw88_station_initialize(rtw88_station_context*c,rtw88_mac_context*m,
 rtw88_station_radio_state radio,void*cookie,const uint8_t mac[6])
{
 int e;uint32_t command;unsigned i;rtw88_station_context value;
 if(!c||!radio||!valid_mac(mac))return EINVAL;
 if((e=mac_ready(m)))return e;
 if(overlap(c,sizeof(*c),m,sizeof(*m))||overlap(c,sizeof(*c),m->device,sizeof(*m->device))||
    overlap(c,sizeof(*c),mac,6))return EINVAL;
 memset(&value,0,sizeof(value));value.mac=m;value.radio_state=radio;value.radio_cookie=cookie;*c=value;
 TRY(write_reg(c,0x522,1,0x3f));TRY(write_reg(c,0x680,2,0));
 TRY(write_reg(c,0x670,4,0xc0000000U));
 for(i=0;i<1000;i++){
  TRY(read_reg(c,0x670,4,&command));if(!(command&0x80000000U))break;
  if(i<999)m->device->bus.delay_us(m->device->bus.cookie,10);
 }
 if(i==1000)return fail(c,latch(c,ETIMEDOUT));
 TRY(address(c,0x610,mac));TRY(write_reg(c,0x620,4,0xffffffffU));TRY(write_reg(c,0x624,4,0xffffffffU));
 TRY(write_reg(c,0x608,4,0xf010400eU));
 TRY(write_reg(c,0x6a0,2,0xffff));TRY(write_reg(c,0x6a2,2,0));TRY(write_reg(c,0x6a4,2,0));
 TRY(update(c,0x100,4,0x30000,0));TRY(update(c,0x550,1,16,16));
 c->initialized=1;return 0;
}
int rtw88_station_configure(rtw88_station_context*c,rtw88_station_state state,
 const rtw88_station_params*p)
{
 unsigned radio=0;int e=ready(c);if(e)return e;
 if(state<RTW88_STATION_INIT||state>RTW88_STATION_RUN)return EINVAL;
 if(state!=RTW88_STATION_INIT){
  if(!p||overlap(c,sizeof(*c),p,sizeof(*p))||!valid_mac(p->mac)||!channel(p->channel)||
     p->short_slot>1||p->ht>1||p->passive_scan>1||(p->basic_rates&0xf000U)||
     (p->channel>14&&(p->basic_rates&15U)))return EINVAL;
  if((state==RTW88_STATION_AUTH||state==RTW88_STATION_ASSOC||state==RTW88_STATION_RUN)&&!valid_mac(p->bssid))return EINVAL;
  if(state==RTW88_STATION_RUN&&(!p->aid||p->aid>2007||!p->basic_rates))return EINVAL;
  radio=c->radio_state(c->radio_cookie,p->channel);
  if(!radio||radio>2||(radio!=2&&!(state==RTW88_STATION_SCAN&&p->passive_scan)))return ENXIO;
 }
 if(state!=RTW88_STATION_RUN){
  TRY(update(c,0x100,4,0x30000,0));TRY(write_reg(c,0x6a4,2,0));TRY(write_reg(c,0x6a2,2,0));
  TRY(update(c,0x608,4,0xc0,0));TRY(update(c,0x550,1,16,16));
  /* Program response rates while management TX remains paused. Old-band
   * CCK response bits must never survive a move into5GHz AUTH. */
  if(state!=RTW88_STATION_INIT)TRY(update(c,0x440,4,0xfffff,p->basic_rates?p->basic_rates:p->channel>14?0x150:0x15f));
  TRY(write_reg(c,0x522,1,state==RTW88_STATION_INIT||(state==RTW88_STATION_SCAN&&p->passive_scan)?0x3f:15));
  if(state!=RTW88_STATION_INIT){TRY(address(c,0x610,p->mac));
   if(state==RTW88_STATION_AUTH||state==RTW88_STATION_ASSOC)TRY(address(c,0x618,p->bssid));}
 }else{
  TRY(address(c,0x610,p->mac));TRY(address(c,0x618,p->bssid));
  TRY(update(c,0x6a8,4,0x7ff,p->aid));TRY(update(c,0x440,4,0xfffff,p->basic_rates));
  TRY(write_reg(c,0x51b,1,p->channel>14||p->short_slot?9:20));
  TRY(write_reg(c,0x6a0,2,0xffff));TRY(write_reg(c,0x6a2,2,p->ht?0x100:0));TRY(write_reg(c,0x6a4,2,0xffff));
  TRY(update(c,0x608,4,0xc0,0xc0));TRY(update(c,0x550,1,0x18,8));TRY(write_reg(c,0x522,1,0));
  TRY(update(c,0x100,4,0x30000,0x20000));
 }
 c->state=state;c->channel=state==RTW88_STATION_INIT?0:p->channel;return 0;
}
int rtw88_station_set_edca(rtw88_station_context*c,int short_slot,const rtw88_station_edca ac[4])
{
 static const uint16_t reg[4]={0x508,0x50c,0x504,0x500};
 unsigned i,slot,sifs;int e=ready(c);if(e)return e;
 if(!ac||(short_slot!=0&&short_slot!=1)||overlap(c,sizeof(*c),ac,4*sizeof(*ac)))return EINVAL;
 if(!c->channel||c->radio_state(c->radio_cookie,c->channel)!=2)return ENXIO;
 slot=c->channel>14||short_slot?9:20;sifs=c->channel>14?16:10;
 for(i=0;i<4;i++)if(ac[i].aifsn>15||ac[i].ecwmin>15||ac[i].ecwmax>15||ac[i].ecwmin>ac[i].ecwmax||ac[i].txop>0x7ff||
    ac[i].aifsn*slot+sifs>255)return EINVAL;
 TRY(write_reg(c,0x51b,1,slot));
 for(i=0;i<4;i++)TRY(update(c,reg[i],4,0x07ffffffU,(uint32_t)ac[i].txop<<16|
    (uint32_t)ac[i].ecwmax<<12|(uint32_t)ac[i].ecwmin<<8|(ac[i].aifsn*slot+sifs)));
 return 0;
}
int rtw88_station_h2c(rtw88_station_context*c,const uint8_t command[8])
{
 int e=ready(c);if(e)return e;
 e=rtw88_fw_h2c(c->mac->device,command);return e&&e!=EINVAL?fail(c,e):e;
}
int rtw88_station_prepare_ht20(rtw_wifi_family f,uint8_t ch,unsigned streams,uint16_t legacy,
 const uint8_t mcs[2],int ht,int local_sgi,int peer_sgi,rtw88_station_rate*out)
{
 rtw88_station_rate value;uint8_t high[8];
 if(!out||!family(f)||!channel(ch)||streams<1||streams>(f==RTW_WIFI_RTL8821CU?1U:2U)||(ch>14&&(legacy&15U)))return EINVAL;
 memset(&value,0,sizeof(value));
 if(!rtw_8814au_build_rate_config(legacy,mcs,streams,ht,local_sgi,peer_sgi,&value.rates)||
    !rtw_8814au_build_ra_commands(&value.rates,1,value.command,high))return EINVAL;
 *out=value;return 0;
}
