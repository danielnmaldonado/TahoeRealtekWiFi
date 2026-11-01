/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
 * Copyright(c)2018-2019 Realtek Corporation.
 * Checked B/C port of the pinned Linux rtw88 conditional PHY table loader and
 * direct/SIPI RF transports. Loading tables does not complete TRX,RFE,power
 * limits,IQK/DAC/DPK calibration or station initialization. */
#include "rtw/RTW88PHY.h"
#ifdef KERNEL
#include <sys/errno.h>
#else
#include <errno.h>
#endif
#include <string.h>
#define BIT(n) (UINT32_C(1)<<(n))
struct rtw88_phy_table {uint8_t kind,path,btg,rfk;const uint32_t*data;size_t words;};
#include "RTW88PHYTables.inc"
static int table_list(rtw_wifi_family f,const struct rtw88_phy_table**out,size_t*n){
 if(f==RTW_WIFI_RTL8821CU){*out=tables_8821c;*n=sizeof(tables_8821c)/sizeof(tables_8821c[0]);}
 else if(f==RTW_WIFI_RTL8822BU){*out=tables_8822b;*n=sizeof(tables_8822b)/sizeof(tables_8822b[0]);}
 else if(f==RTW_WIFI_RTL8822CU){*out=tables_8822c;*n=sizeof(tables_8822c)/sizeof(tables_8822c[0]);}
 else return EINVAL;return 0;
}
static int known_rfe(rtw_wifi_family f,uint8_t rfe){return f==RTW_WIFI_RTL8821CU?(rfe==0||rfe==2||rfe==4||rfe==6):f==RTW_WIFI_RTL8822BU?(rfe==2||rfe==3||rfe==5):f==RTW_WIFI_RTL8822CU&&rfe<=6;}
static int preflight(rtw88_phy_table_kind kind,uint8_t path,const uint32_t*p,size_t words){size_t i;int inside=0,pending=0,has_else=0;
 if(kind<RTW88_PHY_MAC8||kind>RTW88_PHY_RF20||path>1U||(!p&&words)||(words&1U)||words>262144U)return EINVAL;
 for(i=0;i<words;i+=2U){uint32_t a=p[i],v=p[i+1U];if(a&BIT(31)){unsigned branch=(a>>28U)&3U;if(branch==0){if(inside)return EINVAL;inside=1;pending=1;has_else=0;}else if(branch==1){if(!inside||has_else||pending)return EINVAL;pending=1;}else if(branch==2){if(!inside||has_else||pending)return EINVAL;has_else=1;}else{if(!inside||pending)return EINVAL;inside=0;}}else if(a&BIT(30)){if(!inside||!pending)return EINVAL;pending=0;}else{if(pending)return EINVAL;if(kind==RTW88_PHY_RF20){if((a>255U&&a!=4094U)||v>0xfffffU)return EINVAL;}else if(kind==RTW88_PHY_MAC8){if(a>65535U||v>255U)return EINVAL;}else if(a>65532U||((a&3U)&&!(kind==RTW88_PHY_BB32&&a>=249U&&a<=254U)))return EINVAL;}}
 return inside||pending?EINVAL:0;
}
static int matches(uint32_t positive,uint8_t cut,uint8_t package,uint8_t rfe){unsigned pc=(positive>>24U)&15U,pp=(positive>>12U)&15U,pi=(positive>>8U)&15U;return (!pc||pc==(cut?cut:15U))&&(!pp||pp==(package?package:15U))&&(!pi||pi==2U)&&(positive&255U)==rfe;}
int rtw88_phy_walk_table(uint8_t cut,uint8_t package,uint8_t rfe,rtw88_phy_table_kind kind,uint8_t path,const uint32_t*p,size_t words,rtw88_phy_emit emit,void*cookie){size_t i;uint32_t positive=0;int matched=1,skipped=0,e;if(cut>6U||package>15U||!emit)return EINVAL;if((e=preflight(kind,path,p,words)))return e;
 for(i=0;i<words;i+=2U){uint32_t a=p[i];if(a&BIT(31)){unsigned branch=(a>>28U)&3U;if(branch==3){matched=1;skipped=0;}else if(branch==2)matched=!skipped;else positive=a;}else if(a&BIT(30)){if(!skipped){matched=matches(positive,cut,package,rfe);skipped=matched;}else matched=0;}else if(matched){e=emit(cookie,kind,path,a,p[i+1U]);if(e)return e;}}return 0;
}
int rtw88_phy_visit_boot_tables(rtw_wifi_family f,uint8_t cut,uint8_t package,uint8_t rfe,rtw88_phy_emit emit,void*cookie){const struct rtw88_phy_table*t;size_t n,i;int e;if(cut>6U||package>15U||!emit||!known_rfe(f,rfe))return EINVAL;if((e=table_list(f,&t,&n)))return e;for(i=0;i<n;i++)if((e=preflight((rtw88_phy_table_kind)t[i].kind,t[i].path,t[i].data,t[i].words)))return e;
 for(i=0;i<n;i++){if(t[i].btg&&rfe!=2&&rfe!=4)continue;e=rtw88_phy_walk_table(cut,package,rfe,(rtw88_phy_table_kind)t[i].kind,t[i].path,t[i].data,t[i].words,emit,cookie);if(e)return e;}return 0;
}
static int error(rtw88_phy_context*c,int e){if(e<0)e=e==INT32_MIN?EIO:-e;if(e&&!c->boot->firmware.last_error)c->boot->firmware.last_error=e;return e;}
static int valid(const rtw88_phy_context*c){return c&&c->boot&&c->mac&&c->mac->device==&c->boot->firmware&&c->mac->mac_ready&&c->boot->firmware.identified&&c->boot->firmware.powered&&c->boot->firmware.firmware_running&&c->boot->board_valid&&c->boot->board.rf_paths>=1U&&c->boot->board.rf_paths<=2U&&(c->boot->firmware.family!=RTW_WIFI_RTL8821CU||c->boot->board.rf_paths==1U)&&c->boot->board.cut<=6U&&c->boot->board.package_type<=15U&&known_rfe(c->boot->firmware.family,c->boot->board.rfe_option)&&c->boot->firmware.bus.read&&c->boot->firmware.bus.write&&c->boot->firmware.bus.delay_us;}
static int rd(rtw88_phy_context*c,uint16_t a,unsigned w,uint32_t*v){rtw88_fw_context*d=&c->boot->firmware;*v=0;return error(c,d->bus.read(d->bus.cookie,a,w,v));}
static int wr(rtw88_phy_context*c,uint16_t a,unsigned w,uint32_t v){rtw88_fw_context*d=&c->boot->firmware;return error(c,d->bus.write(d->bus.cookie,a,w,v));}
static int update(rtw88_phy_context*c,uint16_t a,unsigned w,uint32_t mask,uint32_t value){uint32_t v;int e=rd(c,a,w,&v);return e?e:wr(c,a,w,(v&~mask)|(value&mask));}
static void delay(rtw88_phy_context*c,uint32_t us){rtw88_fw_context*d=&c->boot->firmware;d->bus.delay_us(d->bus.cookie,us);}
static unsigned shift(uint32_t mask){unsigned s=0;while(!(mask&1U)){mask>>=1U;++s;}return s;}
int rtw88_phy_read_rf(rtw88_phy_context*c,uint8_t path,uint8_t address,uint32_t mask,uint32_t*out){uint32_t v;uint16_t base;int e;if(!out)return EINVAL;*out=0;if(!valid(c)||path>=c->boot->board.rf_paths||!mask||(mask&~0xfffffU))return EINVAL;if(c->boot->firmware.last_error)return c->boot->firmware.last_error;base=(uint16_t)(c->boot->firmware.family==RTW_WIFI_RTL8822CU?(path?0x4c00:0x3c00):(path?0x2c00:0x2800));e=rd(c,(uint16_t)(base+(unsigned)address*4U),4,&v);if(e)return e;if(v==UINT32_MAX)return error(c,ENODEV);*out=(v&mask)>>shift(mask);return 0;}
int rtw88_phy_write_rf(rtw88_phy_context*c,uint8_t path,uint8_t address,uint32_t mask,uint32_t value){uint32_t old=0,data;uint16_t target;unsigned s;int e;if(!valid(c)||path>=c->boot->board.rf_paths||!mask||(mask&~0xfffffU))return EINVAL;if(c->boot->firmware.last_error)return c->boot->firmware.last_error;s=shift(mask);if(((uint64_t)value<<s)&~(uint64_t)mask)return EINVAL;
 if(c->boot->firmware.family==RTW_WIFI_RTL8822CU&&address!=0){target=(uint16_t)((path?0x4c00U:0x3c00U)+(unsigned)address*4U);e=update(c,target,4,mask,value<<s);if(!e)delay(c,1);return e;}
 if(mask!=0xfffffU){e=rtw88_phy_read_rf(c,path,address,0xfffffU,&old);if(e)return e;}data=(old&~mask)|((value<<s)&mask);target=(uint16_t)(c->boot->firmware.family==RTW_WIFI_RTL8822CU?(path?0x4108:0x1808):(path?0xe90:0xc90));e=wr(c,target,4,((uint32_t)address<<20U)|(data&0xfffffU));if(!e)delay(c,13);return e;
}
static int emit(void*p,rtw88_phy_table_kind kind,uint8_t path,uint32_t a,uint32_t v){rtw88_phy_context*c=p;int e;if(kind==RTW88_PHY_RF20){if(a==0xffeU){delay(c,50000);return 0;}if(a==0xfeU){delay(c,100);return 0;}e=rtw88_phy_write_rf(c,path,(uint8_t)a,0xfffffU,v);if(!e)delay(c,1);return e;}if(kind==RTW88_PHY_BB32&&a>=0xf9U&&a<=0xfeU){static const uint32_t waits[]={1,5,50,1000,5000,50000};delay(c,waits[a-0xf9U]);return 0;}return wr(c,(uint16_t)a,kind==RTW88_PHY_MAC8?1U:4U,v);}
#define TRY(x) do{if((e=(x))!=0)goto fail;}while(0)
int rtw88_phy_load_boot_tables(rtw88_phy_context*c,rtw88_boot_context*b,rtw88_mac_context*m){const struct rtw88_phy_table*t;size_t n,i;int e;uint32_t v,cap;rtw_wifi_family f;if(!c)return EINVAL;c->tables_loaded=0;c->radio_state=NULL;c->boot=b;c->mac=m;if(!valid(c))return ENXIO;f=b->firmware.family;if(b->firmware.last_error)return b->firmware.last_error;if(f==RTW_WIFI_RTL8822CU&&b->board.rf_paths!=2U)return ENODEV;TRY(table_list(f,&t,&n));for(i=0;i<n;i++)TRY(preflight((rtw88_phy_table_kind)t[i].kind,t[i].path,t[i].data,t[i].words));
 if(f==RTW_WIFI_RTL8821CU){TRY(rd(c,2,1,&v));v|=0x40U;TRY(wr(c,2,1,v));v|=3U;TRY(wr(c,2,1,v));TRY(wr(c,2,1,v&~3U));TRY(wr(c,2,1,v));TRY(wr(c,0x1f,1,7));delay(c,10);TRY(wr(c,0xef,1,7));delay(c,10);}else{TRY(update(c,2,1,3,3));TRY(update(c,0x1f,1,7,7));TRY(update(c,0xec,4,0x07000000,0x07000000));}
 if(f==RTW_WIFI_RTL8822CU){TRY(update(c,0xa70,4,0x3ff,0x3ff));TRY(update(c,0x180c,4,0x10000003,0x10000003));TRY(update(c,0x410c,4,0x10000003,0x10000003));TRY(update(c,0x1c3c,4,3,0));}else TRY(update(c,0x808,4,0x30000000,0));
 for(i=0;i<n;i++){if(t[i].btg&&b->board.rfe_option!=2&&b->board.rfe_option!=4)continue;if(t[i].kind==RTW88_PHY_RF20&&t[i].path>=b->board.rf_paths)continue;if(t[i].rfk){TRY(update(c,0x1e24,4,BIT(17),BIT(17)));TRY(update(c,0x1cd0,4,BIT(28),BIT(28)));TRY(update(c,0x1cd0,4,BIT(29),BIT(29)));TRY(update(c,0x1cd0,4,BIT(30),BIT(30)));TRY(update(c,0x1cd0,4,BIT(31),0));}TRY(rtw88_phy_walk_table(b->board.cut,b->board.package_type,b->board.rfe_option,(rtw88_phy_table_kind)t[i].kind,t[i].path,t[i].data,t[i].words,emit,c));}
 cap=b->board.crystal_cap;if(f==RTW_WIFI_RTL8822CU){cap&=127U;TRY(update(c,0x1040,4,0x00fffc00U,(cap|(cap<<7U))<<10U));TRY(update(c,0x180c,4,0x10000003,0x10000003));TRY(update(c,0x410c,4,0x10000003,0x10000003));TRY(update(c,0x1c3c,4,3,3));}else{cap&=63U;TRY(update(c,0x24,4,0x7e000000U,cap<<25U));TRY(update(c,0x28,4,0x7eU,cap<<1U));if(f==RTW_WIFI_RTL8821CU)TRY(update(c,0xa2c,4,BIT(18)|BIT(22),0));TRY(update(c,0x808,4,0x30000000,0x30000000));if(f==RTW_WIFI_RTL8821CU){TRY(rd(c,0xa24,4,c->channel_filter));TRY(rd(c,0xa28,4,c->channel_filter+1));TRY(rd(c,0xaac,4,c->channel_filter+2));}}
 c->tables_loaded=1;return 0;
fail:
 (void)update(c,2,1,3,0);(void)update(c,0x1f,1,7,0);(void)update(c,0xec,4,0x07000000,0);return error(c,e);
}
