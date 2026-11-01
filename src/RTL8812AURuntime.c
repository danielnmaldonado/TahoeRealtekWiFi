/* SPDX-License-Identifier: BSD-3-Clause */
/* Pinned Linuxrtw88 fw.c/fw.h,rtw88xxa.c and chip-specific PHY gain rules.
 * Copyright(c)2018-2019,2024 Realtek Corporation. Retain PHY license/manifest. */
#include "rtw/RTL8812AURuntime.h"
#include "RTL8812AUCoreRegs.h"
#include <string.h>
#ifdef KERNEL
#include <sys/errno.h>
#else
#include <errno.h>
#endif
static uint32_t le32(const uint8_t*p){return(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
int rtw_r12au_build_rate_config(uint16_t legacy,const uint8_t mcs[2],uint16_t vht_map,unsigned physical,rtw_r12au_mode mode,unsigned width,int band,int local_sgi,int peer_sgi,rtw_r12au_rate_config*out){
 if(out)memset(out,0,sizeof(*out));if(!out||!legacy||(legacy&~0xfffU)||physical<1||physical>2||mode<RTW_R12AU_MODE_LEGACY||mode>RTW_R12AU_MODE_VHT||(band!=0&&band!=1)||(local_sgi!=0&&local_sgi!=1)||(peer_sgi!=0&&peer_sgi!=1)||(band&&(legacy&15))||(width!=20&&width!=40&&width!=80)||(mode==RTW_R12AU_MODE_LEGACY&&width!=20)||(mode==RTW_R12AU_MODE_HT&&width==80)||(mode==RTW_R12AU_MODE_VHT&&!band))return 0;
 rtw_r12au_rate_config value;memset(&value,0,sizeof(value));value.mask=legacy;value.bandwidth_mhz=(uint8_t)width;value.streams=1;
 if(mode==RTW_R12AU_MODE_HT){
  if(!mcs)return 0;uint32_t ht=mcs[0];if(physical==2)ht|=(uint32_t)mcs[1]<<8;if(!ht)return 0;
  value.mask|=ht<<12;value.ht=1;value.streams=(uint8_t)((ht&0xff00)?2:1);
  value.rate_id=(uint8_t)((legacy&15)?(width==40?(value.streams==2?0:1):(value.streams==2?2:3)):(value.streams==2?4:5));
 }else if(mode==RTW_R12AU_MODE_VHT){
  uint32_t vht=0;if((vht_map&3)==3)return 0;
  for(unsigned stream=0;stream<physical;++stream){unsigned map=((unsigned)vht_map>>(2*stream))&3;if(map==3)break;unsigned top=map==0?7:map==1?8:9;if(width==20&&top==9)top=8;vht|=((UINT32_C(1)<<(top+1))-1)<<(10*stream);value.streams=(uint8_t)(stream+1);}
  if(!vht)return 0;value.mask|=vht<<12;value.ht=value.vht=1;value.rate_id=(uint8_t)(value.streams==2?9:10);
 }else value.rate_id=(uint8_t)((legacy&0xff0)?((legacy&15)?6:7):8);
 value.short_gi=(uint8_t)(value.ht&&local_sgi&&peer_sgi);
 for(unsigned bit=0;bit<32;++bit)if(value.mask&BIT(bit))value.initial_rate=(uint8_t)(value.vht&&bit>=12?bit+32:bit);
 *out=value;return 1;
}
int rtw_r12au_build_legacy_rate(uint16_t mask,int band,rtw_r12au_legacy_rate*out){return rtw_r12au_build_rate_config(mask,NULL,0xffff,1,RTW_R12AU_MODE_LEGACY,20,band,0,0,out);}
static int valid_rate(const rtw_r12au_rate_config*r){
 if(!r||!r->mask||r->ht>1||r->vht>r->ht||r->short_gi>1||r->streams<1||r->streams>2)return 0;
 rtw_r12au_rate_config value;uint8_t mcs[2]={(uint8_t)(r->mask>>12),(uint8_t)(r->mask>>20)};uint16_t vht=0xffff;
 if(r->vht){for(unsigned ss=0;ss<r->streams;++ss){unsigned chunk=(r->mask>>(12+10*ss))&0x3ffU;unsigned map=chunk==0xff?0:chunk==0x1ff?1:chunk==0x3ff?2:3;if(map==3)return 0;vht=(uint16_t)((vht&~(3U<<(2*ss)))|(map<<(2*ss)));}}
 else if(r->mask&~(r->ht?UINT32_C(0x0fffffff):UINT32_C(0xfff)))return 0;
 if(!rtw_r12au_build_rate_config((uint16_t)(r->mask&0xfff),mcs,vht,r->streams,r->vht?RTW_R12AU_MODE_VHT:r->ht?RTW_R12AU_MODE_HT:RTW_R12AU_MODE_LEGACY,r->bandwidth_mhz,!(r->mask&15),r->short_gi,r->short_gi,&value))return 0;
 return value.mask==r->mask&&value.rate_id==r->rate_id&&value.initial_rate==r->initial_rate&&value.ht==r->ht&&value.vht==r->vht&&value.streams==r->streams&&value.short_gi==r->short_gi;
}
static int selected(const rtw_r12au_rate_config*r,uint8_t rate){
 unsigned bit=rate;if(rate>=44){if(!r->vht||rate>=44+10*r->streams)return 0;bit=rate-32;}
 else if(rate>=12&&(!r->ht||r->vht||rate>=12+8*r->streams))return 0;
 return bit<32&&(r->mask&BIT(bit))!=0;
}
int rtw_r12au_rate_selected(const rtw_r12au_rate_config*r,uint8_t rate){return valid_rate(r)&&selected(r,rate);}
int rtw_r12au_make_rate_command(const rtw_r12au_legacy_rate*r,int reset,uint8_t out[8]){
 if(out)memset(out,0,8);if(!out||!valid_rate(r)||(reset!=0&&reset!=1))return 0;
 out[0]=0x40;out[2]=(uint8_t)(r->rate_id|(reset?0x20U:0)|(r->short_gi?0x80U:0));out[3]=(uint8_t)(0x40U|(reset?0:8)|(r->vht?0x10U:0)|(r->bandwidth_mhz==80?2U:r->bandwidth_mhz==40?1U:0));
 for(unsigned i=0;i<4;++i)out[4+i]=(uint8_t)(r->mask>>(8*i));return 1;
}
int rtw_r12au_make_tx(const rtw_r12au_tx_options*o,const rtw_r12au_rate_config*r,uint8_t out[40]){
 if(out)memset(out,0,40);if(!o||!out||(o->firmware_rate&&(!valid_rate(r)||o->rate!=r->initial_rate||!selected(r,o->rate)||o->rf_chains<r->streams||o->bandwidth_mhz!=r->bandwidth_mhz||o->short_gi!=r->short_gi)))return 0;
 if(!rtw_r12au_tx_descriptor(o,out))return 0;
 if(o->firmware_rate){out[6]=(uint8_t)((out[6]&0xe0U)|r->rate_id);out[28]=out[29]=0;uint16_t sum=0;for(unsigned i=0;i<32;i+=2)sum^=(uint16_t)((unsigned)out[i]|((unsigned)out[i+1]<<8));out[28]=(uint8_t)sum;out[29]=(uint8_t)(sum>>8);}
 return rtw_r12au_tx_descriptor_valid(out,o->rf_chains);
}
int rtw_r12au_make_legacy_tx(const rtw_r12au_tx_options*o,const rtw_r12au_legacy_rate*r,uint8_t out[40]){
 if(out)memset(out,0,40);if(!o||o->rate>=12||o->bandwidth_mhz!=20||o->short_gi)return 0;return rtw_r12au_make_tx(o,r,out);
}
int rtw_r12au_make_media_command(int connected,uint8_t out[8]){if(out)memset(out,0,8);if(!out||(connected!=0&&connected!=1))return 0;out[0]=1;out[1]=(uint8_t)connected;return 1;}
int rtw_r12au_make_rssi_command(unsigned rssi,uint8_t out[8]){if(out)memset(out,0,8);if(!out||rssi>100)return 0;out[0]=0x42;out[3]=(uint8_t)rssi;return 1;}
int rtw_r12au_parse_rate_report(const uint8_t*b,size_t n,const rtw_r12au_legacy_rate*r,uint8_t*out){
 if(out)*out=0;if(!b||n<2||!out)return-1;if(b[0]!=0x0c)return 0;
 if(n<6||n>256||!valid_rate(r)||b[3]!=0||((b[2]&0x80)&&!r->short_gi)||!selected(r,(uint8_t)(b[2]&0x7f)))return-1;*out=(uint8_t)(b[2]&0x7f);return 1;
}
static int ready(rtw_r12au_boot*c){if(!c||!c->identified||!c->powered||!c->firmware_running||!c->mac_initialized)return ENXIO;return c->io_error;}
int rtw_r12au_station_rates(rtw_r12au_boot*c,uint16_t basic,int short_slot,int band){
 int e=ready(c);if(e)return e;if(!basic||(basic&~0xfffU)||(short_slot!=0&&short_slot!=1)||(band!=0&&band!=1)||(band&&(basic&15)))return EINVAL;
 e=rtw_r12au_update(c,REG_RRSR,4,0xfffff,basic);if(!e)e=rtw_r12au_write(c,REG_SLOT,1,band||short_slot?9:20);return e;
}
int rtw_r12au_station_edca(rtw_r12au_boot*c,int short_slot,int band,const rtw_r12au_edca_ac a[4]){
 int e=ready(c);if(e)return e;if(!a||(short_slot!=0&&short_slot!=1)||(band!=0&&band!=1))return EINVAL;
 unsigned slot=band||short_slot?9:20,sifs=band?16:10;
 for(unsigned i=0;i<4;++i)if(a[i].aifsn>15||a[i].ecwmin>15||a[i].ecwmax>15||a[i].ecwmin>a[i].ecwmax||a[i].txop>0x7ff||a[i].aifsn*slot+sifs>255)return EINVAL;
 static const uint16_t regs[4]={REG_EDCA_BE_PARAM,REG_EDCA_BK_PARAM,REG_EDCA_VI_PARAM,REG_EDCA_VO_PARAM};
 if((e=rtw_r12au_write(c,REG_SLOT,1,slot)))return e;for(unsigned i=0;i<4;++i){uint32_t value=((uint32_t)a[i].txop<<16)|((uint32_t)a[i].ecwmax<<12)|((uint32_t)a[i].ecwmin<<8)|(a[i].aifsn*slot+sifs);if((e=rtw_r12au_update(c,regs[i],4,0x07ffffff,value)))return e;}return 0;
}
int rtw_r12au_decode_phy_sample(rtw_r12au_chip chip,unsigned paths,int high,uint8_t rate,const uint8_t*phy,size_t n,rtw_r12au_phy_sample*out){
 if(out)memset(out,0,sizeof(*out));if(!out||!phy||n!=32||!rtw_r12au_rate_valid(rate,paths)||(high!=0&&high!=1)||
 (chip!=RTW_R12AU_CHIP_8812AU&&chip!=RTW_R12AU_CHIP_8821AU)||paths<1||paths>(chip==RTW_R12AU_CHIP_8812AU?2U:1U))return 0;
 int power;uint32_t w0=le32(phy),w1=le32(phy+4);
 if(rate<4){unsigned lna=(w1>>13)&7,vga=(w1>>8)&31;
  if(chip==RTW_R12AU_CHIP_8821AU){static const int gain[6]={15,-1,-17,0,-30,-38};if(lna>5||lna==3)return 0;power=gain[lna]-2*(int)vga;}
  else switch(lna){case 7:power=vga<=27?-94+2*(27-(int)vga):-94;break;case 6:power=-42+2*(2-(int)vga);break;case 5:power=-36+2*(7-(int)vga);break;case 4:power=-30+2*(7-(int)vga);break;case 3:power=-18+2*(7-(int)vga);break;case 2:power=2*(5-(int)vga);break;case 1:power=14-2*(int)vga;break;default:power=20-2*(int)vga;break;}
 }else{power=(int)(w0&127)-110;if(paths==2){int second=(int)((w0>>8)&127)-110;if(second>power)power=second;}}
 if(power<-127)return 0;unsigned percent=power>=0?100U:power<=-100?0U:(unsigned)(100+power);
 if(rate<4&&chip==RTW_R12AU_CHIP_8812AU&&!high){if(percent>=80)percent=((percent-80)<<1)+((percent-80)>>1)+80;else if(percent>=20&&percent<=78)percent+=3;}
 /* Valid CCK gain combinations can saturate above0dBm. Preserve their
  * primary100% RSSI while clipping to the shared cache's highest valid
  * negative dBm; zero remains the missing-measurement sentinel. */
 if(percent>100)percent=100;out->signal_dbm=power>=0?-1:power;out->rssi_percent=(uint8_t)percent;return 1;
}
