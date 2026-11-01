/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
 * Copyright(c)2018-2019 Realtek Corporation.
 * Portable B/C USB boot and OTP adaptation of pinned Linux rtw88 main.c,
 * mac.c,efuse.c,util.c and each chip's power/read_efuse functions.
 * No RF/PHY-ready or station state is synthesized here. */
#include "rtw/RTW88Boot.h"
#ifdef KERNEL
#include <sys/errno.h>
#else
#include <errno.h>
#endif
#include <string.h>
#define BIT(n) (UINT32_C(1)<<(n))
struct rtw88_power_op {uint16_t address;uint8_t cut,command,mask,value;};
#include "RTW88BootPowerTables.inc"
static int supported(rtw_wifi_family f){return f==RTW_WIFI_RTL8821CU||f==RTW_WIFI_RTL8822BU||f==RTW_WIFI_RTL8822CU;}
static int overlap(const void*a,size_t an,const void*b,size_t bn){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return an&&bn&&(x<=y?y-x<an:x-y<bn);}
static int error(rtw88_boot_context*c,int e){if(e<0)e=e==INT32_MIN?EIO:-e;if(e&&!c->firmware.last_error)c->firmware.last_error=e;return e;}
static int catalog(const rtw_usb_wifi_profile*p){size_t i;for(i=0;i<rtw_usb_wifi_profile_count();i++)if(p==rtw_usb_wifi_profile_at(i))return p->primary_verified&&supported(p->family);return 0;}
static int valid(const rtw88_boot_context*c){return c&&c->firmware.bus.cookie==c&&c->identity&&(c->resolved_identity?(rtw_chip_identity_bound_to(c->resolved_identity,c)&&rtw_chip_identity_valid(c->resolved_identity,c->identity,&c->raw_bus)):catalog(c->identity))&&c->identity->family==c->firmware.family;}
static int bus_read(void*p,uint16_t a,unsigned w,uint32_t*v){rtw88_boot_context*c=p;if(!v)return EINVAL;*v=0;if(!c||(w!=1&&w!=2&&w!=4)||a>65536U-w||!c->raw_bus.read)return EINVAL;int e=error(c,c->raw_bus.read(c->raw_bus.cookie,a,w,v));if(e)*v=0;return e;}
static int bus_write(void*p,uint16_t a,unsigned w,uint32_t v){rtw88_boot_context*c=p;if(!c||(w!=1&&w!=2&&w!=4)||a>65536U-w||!c->raw_bus.write)return EINVAL;return error(c,c->raw_bus.write(c->raw_bus.cookie,a,w,v));}
static int bus_bulk(void*p,uint8_t q,const uint8_t*b,size_t n){rtw88_boot_context*c=p;return c->raw_bus.bulk_write?error(c,c->raw_bus.bulk_write(c->raw_bus.cookie,q,b,n)):error(c,ENOTSUP);}
static void bus_delay(void*p,uint32_t u){rtw88_boot_context*c=p;c->raw_bus.delay_us(c->raw_bus.cookie,u);}
static int rd(rtw88_boot_context*c,uint16_t a,unsigned w,uint32_t*v){return bus_read(c,a,w,v);}
static int wr(rtw88_boot_context*c,uint16_t a,unsigned w,uint32_t v){return bus_write(c,a,w,v);}
static int update(rtw88_boot_context*c,uint16_t a,unsigned w,uint32_t mask,uint32_t value){uint32_t v;int e=rd(c,a,w,&v);return e?e:wr(c,a,w,(v&~mask)|(value&mask));}
static int poll(rtw88_boot_context*c,uint16_t a,unsigned w,uint32_t mask,uint32_t value,unsigned delay,unsigned count){unsigned i;uint32_t v;int e;for(i=0;i<count;i++){if((e=rd(c,a,w,&v)))return e;if((v&mask)==(value&mask))return 0;if(i+1U<count)bus_delay(c,delay);}return error(c,ETIMEDOUT);}
static int coex_read(void*p,uint16_t a,uint32_t*v){rtw88_boot_context*c=p;int e;if(!v)return EINVAL;*v=0;if((e=poll(c,0x1700,4,BIT(29),BIT(29),10,1000))||(e=wr(c,0x1700,4,0x800f0000U|a)))return e;return rd(c,0x1708,4,v);}
static int coex_write(void*p,uint16_t a,uint32_t v){rtw88_boot_context*c=p;int e;if((e=poll(c,0x1700,4,BIT(29),BIT(29),10,1000))||(e=wr(c,0x1704,4,v)))return e;return wr(c,0x1700,4,0xc00f0000U|a);}
int rtw88_boot_geometry(rtw_wifi_family f,rtw88_efuse_geometry*out){rtw88_efuse_geometry g;if(!out||!supported(f))return EINVAL;g.physical_bytes=(uint16_t)(f==RTW_WIFI_RTL8822BU?1024:512);g.logical_bytes=(uint16_t)(f==RTW_WIFI_RTL8821CU?512:768);g.protected_bytes=(uint16_t)(f==RTW_WIFI_RTL8822CU?124:96);g.usb_mac_offset=(uint16_t)(f==RTW_WIFI_RTL8822CU?0x157:0x107);*out=g;return 0;}
static int init_context(rtw88_boot_context*c,const rtw_usb_wifi_profile*p,const rtw88_fw_bus*b,rtw_chip_identity*resolved){rtw88_fw_bus raw;if(!c)return EINVAL;if(b)raw=*b;else memset(&raw,0,sizeof(raw));memset(c,0,sizeof(*c));if(resolved?!rtw_chip_identity_valid(resolved,p,&raw):!catalog(p))return ENODEV;if(!raw.read||!raw.write||!raw.delay_us)return EINVAL;c->power_epoch=1;c->identity=p;c->resolved_identity=resolved;c->raw_bus=raw;c->firmware.family=p->family;c->firmware.bus=(rtw88_fw_bus){c,bus_read,bus_write,bus_bulk,bus_delay,coex_read,coex_write};return 0;}
int rtw88_boot_init(rtw88_boot_context*c,const rtw_usb_wifi_profile*p,const rtw88_fw_bus*b){return init_context(c,p,b,NULL);}
int rtw88_boot_init_resolved(rtw88_boot_context*c,rtw_chip_identity*resolved,const rtw88_fw_bus*b){int e;const rtw_usb_wifi_profile*p=rtw_chip_identity_profile(resolved);
 if(!c)return EINVAL;
 if(resolved&&overlap(c,sizeof(*c),resolved,rtw_chip_identity_size()))return EINVAL;
 if(!rtw_chip_identity_valid(resolved,p,b))return ENODEV;
 if((e=rtw_chip_identity_bind_context(resolved,c)))return e;
 return init_context(c,p,b,resolved);
}
static int resolved_error(rtw88_boot_context*c,int e){
 if(c->resolved_identity)rtw_chip_identity_forget(c->resolved_identity);
 c->firmware.identified=c->firmware.powered=c->firmware.firmware_running=c->board_valid=c->physical_valid=0;
 return error(c,e);
}
int rtw88_boot_revalidate_identity(rtw88_boot_context*c){int e;
 if(!valid(c))return EINVAL;
 if(c->firmware.last_error)return c->firmware.last_error;
 if(!c->resolved_identity)return 0;
 e=rtw_chip_identity_revalidate(c->resolved_identity,&c->raw_bus);
 return e?resolved_error(c,e):0;
}
int rtw88_boot_identify(rtw88_boot_context*c){
 uint32_t config,config2,power;int e;if(!valid(c))return EINVAL;
 ++c->power_epoch;c->firmware.identified=c->firmware.powered=c->firmware.firmware_running=c->board_valid=c->physical_valid=0;
 if(c->firmware.last_error)return c->firmware.last_error;
 if((e=rd(c,0xf0,4,&config))||(e=rd(c,0xfc,4,&config2))||(e=rd(c,4,4,&power)))return c->resolved_identity?resolved_error(c,e):e;
 if(config==UINT32_MAX||config2==UINT32_MAX||power==UINT32_MAX||(config&BIT(23))||((config>>12U)&15U)>6U)return c->resolved_identity?resolved_error(c,ENODEV):error(c,ENODEV);
 /* A runtime USB conflict is not resolved by the latest matching table.
  * The same actual chip code must still match this exact proof before power. */
 if(c->resolved_identity&&(config2&255U)!=rtw_chip_identity_code(c->resolved_identity))return resolved_error(c,ENODEV);
 memset(&c->board,0,sizeof(c->board));c->board.sys_cfg=config;c->board.sys_cfg2=config2;c->board.power_control=power;c->board.cut=(uint8_t)((config>>12U)&15U);c->board.vendor=(uint8_t)((config>>16U)&15U);c->board.mp_chip=1;c->board.rf_paths=(uint8_t)(c->firmware.family==RTW_WIFI_RTL8821CU?1:(config&BIT(27))?2:1);c->firmware.identified=1;return 0;
}
#define TRY(x) do{if((e=(x))!=0)goto fail;}while(0)
static int pre_system(rtw88_boot_context*c){int e;TRY(wr(c,0x1c,1,0));TRY(update(c,0x64,4,0x30000000,0x30000000));TRY(update(c,0x4c,4,0x06000000,0));TRY(update(c,0x40,4,4,4));TRY(update(c,2,1,3,0));TRY(update(c,0x1f,1,7,0));TRY(update(c,0xec,4,0x07000000,0));return 0;fail:return e;}
static int sequence(rtw88_boot_context*c,int on){const struct rtw88_power_op*ops;size_t n,i;unsigned cut=1U<<((unsigned)c->board.cut+1U);int e;
#define SELECT(s) do{ops=(s);n=sizeof(s)/sizeof((s)[0]);}while(0)
 if(c->firmware.family==RTW_WIFI_RTL8821CU){if(on)SELECT(power_on_8821c);else SELECT(power_off_8821c);}else if(c->firmware.family==RTW_WIFI_RTL8822BU){if(on)SELECT(power_on_8822b);else SELECT(power_off_8822b);}else{if(on)SELECT(power_on_8822c);else SELECT(power_off_8822c);}
#undef SELECT
 for(i=0;i<n;i++){const struct rtw88_power_op*p=ops+i;if(p->command==4)return 0;if(!(p->cut&cut))continue;switch(p->command){case 0:break;case 1:TRY(update(c,p->address,1,p->mask,p->value));break;case 2:TRY(poll(c,p->address,1,p->mask,p->value,50,20000));break;case 3:bus_delay(c,(uint32_t)p->address*(p->value?1000U:1U));break;default:return error(c,EINVAL);}}return error(c,EINVAL);fail:return e;
}
static int power_switch(rtw88_boot_context*c,int on){uint32_t rpwm,fw,cr,status;int current,e;TRY(rd(c,0xfe58,1,&rpwm));TRY(rd(c,0x80,2,&fw));if(fw==0xc078U)TRY(wr(c,0xfe58,1,(rpwm^0x80U)&0x80U));TRY(rd(c,0x100,1,&cr));TRY(rd(c,0xf5,1,&status));current=cr!=0xeaU&&!(status&1U);if(current==on)return EALREADY;TRY(sequence(c,on));if(on)TRY(update(c,0xf5,1,1,0));return 0;fail:return e;}
int rtw88_boot_power_on(rtw88_boot_context*c){uint32_t fw;int e;if(!valid(c))return EINVAL;++c->power_epoch;c->firmware.powered=c->firmware.firmware_running=c->board_valid=0;if(!c->firmware.identified)return ENXIO;if(c->firmware.last_error)return c->firmware.last_error;if((e=rtw88_boot_revalidate_identity(c)))return e;TRY(pre_system(c));e=power_switch(c,1);if(e==EALREADY){e=power_switch(c,0);if(e&&e!=EALREADY)goto fail;TRY(pre_system(c));e=power_switch(c,1);}if(e)goto fail;TRY(update(c,0x1080,4,BIT(16)|BIT(8),BIT(16)|BIT(8)));TRY(update(c,3,1,c->firmware.family==RTW_WIFI_RTL8822BU?0xdcU:0xd8U,c->firmware.family==RTW_WIFI_RTL8822BU?0xdcU:0xd8U));TRY(update(c,0x1103,1,15,12));TRY(rd(c,0x80,4,&fw));if(fw&BIT(20)){TRY(wr(c,0x80,4,fw&~BIT(20)));TRY(update(c,0x40,4,BIT(19),0));}c->firmware.powered=1;return 0;fail:(void)wr(c,0x100,1,0);return error(c,e);}
int rtw88_boot_stop(rtw88_boot_context*c){int previous,e;if(!valid(c))return EINVAL;if(c->resolved_identity){e=rtw_chip_identity_revalidate(c->resolved_identity,&c->raw_bus);if(e)return resolved_error(c,e);}++c->power_epoch;previous=c->firmware.last_error;c->firmware.last_error=0;e=power_switch(c,0);if(e==EALREADY)e=0;c->firmware.powered=c->firmware.firmware_running=c->board_valid=0;if(previous)c->firmware.last_error=previous;return previous?previous:e;}
int rtw88_boot_decode_efuse(rtw_wifi_family f,const uint8_t*physical,size_t pn,uint8_t*logical,size_t ln){rtw88_efuse_geometry g;size_t p=0,end;if(rtw88_boot_geometry(f,&g)||!physical||!logical||pn!=g.physical_bytes||ln!=g.logical_bytes||overlap(physical,pn,logical,ln))return EINVAL;end=pn-g.protected_bytes;memset(logical,255,ln);while(p<end){unsigned block,mask,word;uint8_t h=physical[p++];if(h==255)return 0;if((h&31U)==15U){uint8_t h2;if(p==end)return EINVAL;h2=physical[p++];if(h2==255)return 0;block=((h2&0xf0U)>>1U)|((h>>5U)&7U);mask=h2&15U;}else{block=h>>4U;mask=h&15U;}for(word=0;word<4;word++)if(!(mask&(1U<<word))){size_t l=(size_t)block*8U+word*2U;if(end-p<2U||l>=ln||ln-l<2U)return EINVAL;logical[l]=physical[p++];logical[l+1U]=physical[p++];}}return 0;}
static uint8_t erased_zero(uint8_t b){return b==255?0:b;}
int rtw88_boot_parse_board(rtw88_boot_context*c,const uint8_t*b,size_t n){rtw88_efuse_geometry g;rtw88_board board;unsigned i,any=0,allff=1;rtw_wifi_family f;if(!valid(c)||!b)return EINVAL;c->board_valid=c->physical_valid=0;if(!c->firmware.identified)return ENXIO;f=c->firmware.family;if(rtw88_boot_geometry(f,&g)||n!=g.logical_bytes)return EINVAL;memset(&board,0,sizeof(board));board.sys_cfg=c->board.sys_cfg;board.sys_cfg2=c->board.sys_cfg2;board.power_control=c->board.power_control;board.cut=c->board.cut;board.vendor=c->board.vendor;board.rf_paths=c->board.rf_paths;board.mp_chip=c->board.mp_chip;memcpy(board.logical,b,n);memcpy(board.mac,b+g.usb_mac_offset,6);for(i=0;i<6U;i++){any|=board.mac[i];if(board.mac[i]!=255)allff=0;}if(!any||allff||(board.mac[0]&1U))return EINVAL;board.rfe_option=f==RTW_WIFI_RTL8821CU?(uint8_t)(b[0xca]&31U):b[0xca];if((f==RTW_WIFI_RTL8821CU&&board.rfe_option!=0&&board.rfe_option!=2&&board.rfe_option!=4&&board.rfe_option!=6)||(f==RTW_WIFI_RTL8822BU&&board.rfe_option!=2&&board.rfe_option!=3&&board.rfe_option!=5)||(f==RTW_WIFI_RTL8822CU&&board.rfe_option>6))return ENODEV;
 board.package_type=(uint8_t)(f==RTW_WIFI_RTL8821CU&&((b[0xca]&32U)!=0));board.rfe_btg=(uint8_t)(f==RTW_WIFI_RTL8821CU&&(board.rfe_option==2||board.rfe_option==4));board.crystal_cap=erased_zero(f==RTW_WIFI_RTL8822CU?(uint8_t)(b[0xb9]&127U):b[0xb9]);board.channel_plan=b[0xb8]==255?0x7f:b[0xb8];board.country_code[0]=b[0xcb];board.country_code[1]=b[0xcc];board.rf_board_option=erased_zero(b[0xc1]);board.bt_setting=b[0xc3];board.share_ant=(board.bt_setting&1U)!=0;board.btcoex=(board.rf_board_option&0xe0U)==0x20U;board.regd=b[0xc1]&7U;board.usb_mode_switch=(uint8_t)(f!=RTW_WIFI_RTL8821CU&&(b[6]&128U));
 if(f!=RTW_WIFI_RTL8822CU){board.pa_type_2g=board.pa_type_5g=erased_zero(b[0xbc]);board.lna_type_2g=erased_zero(b[0xbd]);board.lna_type_5g=erased_zero(b[0xbf]);board.thermal_meter[0]=board.thermal_meter_k=b[0xba];}else{board.thermal_meter[0]=b[0xd0];board.thermal_meter[1]=b[0xd1];board.thermal_meter_k=(uint8_t)(((unsigned)b[0xd0]+b[0xd1])>>1U);board.power_track_type=(uint8_t)(b[0xc8]>>4U);}
 board.ext_pa_2g=(board.pa_type_2g&16U)!=0;board.ext_pa_5g=(board.pa_type_5g&1U)!=0;board.ext_lna_2g=(board.lna_type_2g&8U)!=0;board.ext_lna_5g=(board.lna_type_5g&8U)!=0;
 if(f==RTW_WIFI_RTL8821CU){board.tx_bb_swing_setting_2g=erased_zero(b[0xc6]);board.tx_bb_swing_setting_5g=erased_zero(b[0xc7]);}memcpy(board.txpwr_idx_table,b+0x10,sizeof(board.txpwr_idx_table));if(f==RTW_WIFI_RTL8821CU&&(board.rfe_option==2||board.rfe_option==4))memcpy(board.txpwr_idx_table[0],board.txpwr_idx_table[1],18);c->board=board;c->board_valid=1;return 0;}
int rtw88_boot_read_board(rtw88_boot_context*c,uint8_t*scratch,size_t size){rtw88_efuse_geometry g;uint32_t bank=0,ldo=0,control=0;unsigned at,saved_bank=0,saved_ldo=0;int e=0,cleanup;if(!valid(c)||!scratch||size<RTW88_EFUSE_SCRATCH_BYTES||overlap(c,sizeof(*c),scratch,RTW88_EFUSE_SCRATCH_BYTES))return EINVAL;c->board_valid=0;if(!c->firmware.identified||!c->firmware.powered||!c->firmware.firmware_running)return ENXIO;if(c->firmware.last_error)return c->firmware.last_error;TRY(rtw88_boot_geometry(c->firmware.family,&g));TRY(rd(c,0x34,4,&bank));saved_bank=1;if(c->firmware.family==RTW_WIFI_RTL8822CU){TRY(rd(c,0x29,1,&ldo));saved_ldo=1;TRY(wr(c,0x29,1,ldo&~1U));TRY(wr(c,0x34,4,bank&~0x300U));}else TRY(wr(c,0x34,4,bank&~(0x80000000U|0x300U)));
 TRY(rd(c,0x30,4,&control));for(at=0;at<g.physical_bytes;at++){control=(control&~(0x80000000U|0x3ff00U|255U))|((uint32_t)at<<8U);TRY(wr(c,0x30,4,control));TRY(poll(c,0x30,4,BIT(31),BIT(31),10,1000));TRY(rd(c,0x30,4,&control));scratch[at]=(uint8_t)control;}
 TRY(rtw88_boot_decode_efuse(c->firmware.family,scratch,g.physical_bytes,c->board.logical,g.logical_bytes));TRY(rtw88_boot_parse_board(c,c->board.logical,g.logical_bytes));memcpy(c->physical,scratch,g.physical_bytes);c->physical_valid=1;
fail:
 if(saved_bank){cleanup=wr(c,0x34,4,bank);if(!e)e=cleanup;}if(saved_ldo){cleanup=wr(c,0x29,1,ldo);if(!e)e=cleanup;}if(e)c->board_valid=c->physical_valid=0;return error(c,e);
}

int rtw88_boot_read_features(rtw88_boot_context*c){
 uint8_t bytes[13];uint32_t id,v=0;unsigned i,nss,ant,bw;int e;
 if(!valid(c))return EINVAL;++c->power_epoch;c->board.features_valid=0;
 if(!c->firmware.powered||!c->firmware.firmware_running||!c->board_valid)return ENXIO;
 if((e=rd(c,0x1a0,1,&id)))return e;if(id!=0x19)return error(c,EBUSY);
 for(i=0;i<13;i++){if((e=rd(c,(uint16_t)(0x1a2+i),1,&v)))return e;bytes[i]=(uint8_t)v;}
 if((e=wr(c,0x1a0,1,0)))return e;
 v=(uint32_t)bytes[4]|((uint32_t)bytes[5]<<8U)|((uint32_t)bytes[6]<<16U)|((uint32_t)bytes[7]<<24U);
 nss=(v>>19U)&3U;ant=(v>>21U)&7U;bw=(v>>16U)&7U;
 /* Physical RF registers remain addressable for8822C DACK on both paths.
  * Actual TRX,DPK and stream metadata use the source-confirmed antenna cap. */
 c->board.active_rf_paths=(uint8_t)(ant==1?1:c->board.rf_paths);
 if(!nss||nss>c->board.active_rf_paths)nss=c->board.active_rf_paths;
 c->board.tx_streams=(uint8_t)nss;c->board.feature_bw=(uint8_t)(!bw||bw==7?80:bw==6?40:20);c->board.feature_protocol=(uint8_t)((v>>26U)&3U);c->board.features_valid=1;return 0;
}
