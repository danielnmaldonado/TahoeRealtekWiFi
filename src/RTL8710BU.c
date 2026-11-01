/* SPDX-License-Identifier: GPL-2.0-only */
/* Native, family-typed Linux rtl8xxxu8710BU adapter. The generated algorithms
 * retain their primary register order; host transport and error policy live
 * here. This context is never an EU/AU/8814 hardware alias. */
#include "rtw/RTL8710BU.h"
#include "rtw/Efuse.h"
#include <string.h>
#include <stdbool.h>
#ifdef KERNEL
#include <sys/errno.h>
#else
#include <errno.h>
#endif
typedef uint8_t u8;typedef int8_t s8;typedef uint16_t u16;typedef uint32_t u32;typedef int32_t s32;
#define BIT(n) (UINT32_C(1)<<(n))
#define GENMASK(h,l) ((UINT32_MAX<<(l))&(UINT32_MAX>>(31U-(h))))
#define ARRAY_SIZE(a) ((int)(sizeof(a)/sizeof((a)[0])))
#define IS_ALIGNED(v,n) (((v)&((n)-1U))==0)
#define __ffs(v) ((unsigned)__builtin_ctz((u32)(v)))
#define EFUSE_UNDEFINED 0xff
#define MAX_TOLERANCE 5
#include "RTL8710BURegisters.inc"
#include "RTL8710BUConstants.inc"
enum rtl8xxxu_rfpath {RF_A=0,RF_B=1};
struct rtl8xxxu_reg8val {u16 reg;u8 val;};struct rtl8xxxu_reg32val{u16 reg;u32 val;};struct rtl8xxxu_rfregval{u8 reg;u32 val;};
struct rtl8xxxu_rfregs{u16 hssiparm1,hssiparm2,lssiparm,hspiread,lssiread,rf_sw_ctrl;};
struct rtl8xxxu_hw_feature{u8 max_bw;};
struct rtl8xxxu_cfo_tracking{u8 crystal_cap;bool atc_status,adjust;};
struct rtl8xxxu_firmware_header{u8 bytes[32];u8 data[];};
struct bu_channel{u8 hw_value;};struct bu_config{struct{struct bu_channel*chan;u8 width;int8_t secondary;}chandef;};
struct ieee80211_hw{struct rtw_r10bu*priv;struct bu_config conf;};
struct bu_idx {s8 a,b;};
struct rtw_r10bu{
 rtw_usb_rw_transport bus;rtw_waiter waiter;rtw_r10bu_usb usb;rtw_r10bu_board board;rtw_r10bu_status status;
 u8 packed[512],logical[512],h2c_box;bool cleanup,cal_timeout;int error;
 struct ieee80211_hw hw;struct bu_channel channel;
 const struct rtl8xxxu_firmware_header*fw_data;int fw_size;
 unsigned rf_paths,tx_paths,rx_paths;u8 default_crystal_cap,package_type;bool no_pape,cck_new_agc,pi_enabled;
 unsigned ep_tx_count,ep_tx_high_queue,ep_tx_low_queue,ep_tx_normal_queue;u8 out_ep[3],pipe_out[32];
 u32 adda_backup[16],mac_backup[4],bb_backup[9],bb_recovery_backup[9],regrcr;
 u8 measured_tx[3],measured_rx[3],tx_iqk_mask,rx_iqk_mask;
 u8 cck_tx_power_index_A[6],cck_tx_power_index_B[6],ht40_1s_tx_power_index_A[5],ht40_1s_tx_power_index_B[5];
 struct bu_idx ht20_tx_power_diff[4],ht40_tx_power_diff[4],ofdm_tx_power_diff[4];
 struct rtl8xxxu_cfo_tracking cfo_tracking;struct rtl8xxxu_hw_feature hw_feature;
 u8 thermal_reference,thermal_previous;bool thermal_pending;
};
static u32 u32_get_bits(u32 value,u32 mask){return(value&mask)>>__ffs(mask);}
static void u32p_replace_bits(u32*p,u32 value,u32 mask){*p=(*p&~mask)|((value<<__ffs(mask))&mask);}
static void u8p_replace_bits(u8*p,u8 value,u32 mask){*p=(u8)(((u32)*p&~mask)|(((u32)value<<__ffs(mask))&mask));}
static int conf_is_ht40(const struct bu_config*c){return c->chandef.width==40;}
static int conf_is_ht40_plus(const struct bu_config*c){return conf_is_ht40(c)&&c->chandef.secondary>0;}
static int conf_is_ht40_minus(const struct bu_config*c){return conf_is_ht40(c)&&c->chandef.secondary<0;}
static void delay(struct rtw_r10bu*c,u32 us){if(c&&(!c->error||c->cleanup)&&us&&!c->waiter.wait_us(c->waiter.context,us))c->error=EIO;}
#define udelay(us) delay(priv,(u32)(us))
#define mdelay(ms) delay(priv,(u32)(ms)*1000U)
#define msleep(ms) mdelay(ms)
#define usleep_range(a,b) delay(priv,(u32)(a))
static u16 le16(const u8*b){return(u16)((u16)b[0]|((u16)b[1]<<8));}
static int scalar(struct rtw_r10bu*c,u16 reg,u8 width,u32*value,int write){
 if(!c||!value||(width!=1&&width!=2&&width!=4))return EINVAL;if(c->error&&!c->cleanup)return c->error;
 if((unsigned)reg+width>65536U)return EINVAL;
 u16 physical=reg<=0xffU?(u16)(reg|0x8000U):reg;
 rtw_usb_setup setup={(u8)(write?0x40:0xc0),5,physical,0,width,1000};u8 bytes[4]={0};u16 actual=0;rtw_usb_error result;
 if(write){for(unsigned i=0;i<width;++i)bytes[i]=(u8)(*value>>(8*i));result=c->bus.control_out(c->bus.context,&setup,bytes,&actual);}
 else result=c->bus.control_in(c->bus.context,&setup,bytes,&actual);
 if(result!=RTW_USB_OK||actual!=width){if(!c->error)c->error=EIO;return EIO;}
 if(!write){*value=0;for(unsigned i=0;i<width;++i)*value|=(u32)bytes[i]<<(8*i);}return 0;
}
int rtw_r10bu_read(rtw_r10bu*c,u16 reg,u8 width,u32*v){if(v)*v=0;return scalar(c,reg,width,v,0);}
int rtw_r10bu_write(rtw_r10bu*c,u16 reg,u8 width,u32 v){return scalar(c,reg,width,&v,1);}
int rtw_r10bu_update(rtw_r10bu*c,u16 reg,u8 width,u32 mask,u32 v){u32 old=0;int e=rtw_r10bu_read(c,reg,width,&old);return e?e:rtw_r10bu_write(c,reg,width,(old&~mask)|(v&mask));}
static u8 rtl8xxxu_read8(struct rtw_r10bu*c,u16 a){u32 v=0;(void)rtw_r10bu_read(c,a,1,&v);return(u8)v;}
static u16 rtl8xxxu_read16(struct rtw_r10bu*c,u16 a){u32 v=0;(void)rtw_r10bu_read(c,a,2,&v);return(u16)v;}
static u32 rtl8xxxu_read32(struct rtw_r10bu*c,u16 a){u32 v=0;(void)rtw_r10bu_read(c,a,4,&v);return v;}
static int rtl8xxxu_write8(struct rtw_r10bu*c,u16 a,u8 v){return rtw_r10bu_write(c,a,1,v)?-EIO:1;}
static int rtl8xxxu_write16(struct rtw_r10bu*c,u16 a,u16 v){return rtw_r10bu_write(c,a,2,v)?-EIO:2;}
static int rtl8xxxu_write32(struct rtw_r10bu*c,u16 a,u32 v){return rtw_r10bu_write(c,a,4,v)?-EIO:4;}
static int rtl8xxxu_writeN(struct rtw_r10bu*c,u16 reg,const u8*b,size_t n){
 if(!c||!b||n>4096||(size_t)reg+n>65536)return-EINVAL;if(c->error&&!c->cleanup)return-c->error;
 for(size_t offset=0;offset<n;){u16 count=(u16)((n-offset)>4?4:n-offset),actual=0;rtw_usb_setup setup={0x40,5,(u16)(reg+offset),0,count,1000};
 if(c->bus.control_out(c->bus.context,&setup,b+offset,&actual)!=RTW_USB_OK||actual!=count){if(!c->error)c->error=EIO;return-EIO;}offset+=count;}
 return(int)n;
}
/* Kfree consumes immutable physical OTP captured while the read gate was
 * open during identify. It never accesses the closed gate while tuning. */


#include "RTL8710BUTables.inc"
#include "RTL8710BUAlgorithms.inc"
size_t rtw_r10bu_size(void){return sizeof(struct rtw_r10bu);}
int rtw_r10bu_init(void*memory,size_t bytes,const rtw_usb_rw_transport*bus,const rtw_waiter*wait,const rtw_r10bu_usb*usb,rtw_r10bu**out){
 if(out)*out=NULL;if(!memory||bytes<sizeof(struct rtw_r10bu)||!bus||!bus->control_in||!bus->control_out||!wait||!wait->wait_us||!usb||!out||(uintptr_t)memory%_Alignof(struct rtw_r10bu))return EINVAL;
 if(!usb->bulk_out_count||usb->bulk_out_count>3||usb->bulk_in<0x81||usb->bulk_in>0x8f||(usb->max_packet!=64&&usb->max_packet!=512))return EINVAL;
 for(unsigned i=0;i<usb->bulk_out_count;++i){if(!usb->bulk_out[i]||usb->bulk_out[i]>15)return EINVAL;for(unsigned j=0;j<i;++j)if(usb->bulk_out[j]==usb->bulk_out[i])return EINVAL;}
 struct rtw_r10bu*c=memory;memset(c,0,sizeof(*c));c->bus=*bus;c->waiter=*wait;c->usb=*usb;c->cfo_tracking.crystal_cap=255;c->rf_paths=c->rx_paths=c->tx_paths=1;c->hw.priv=c;c->hw.conf.chandef.chan=&c->channel;c->ep_tx_count=usb->bulk_out_count;c->ep_tx_high_queue=1;c->ep_tx_normal_queue=usb->bulk_out_count>=2;c->ep_tx_low_queue=usb->bulk_out_count>=3;memcpy(c->out_ep,usb->bulk_out,sizeof(c->out_ep));*out=c;return 0;
}
static int valid_package(u8 package){return package==PACKAGE_QFN48M_U||package==PACKAGE_QFN48M_S;}
int rtw_r10bu_decode_rom(const u8*rom,size_t bytes,rtw_r10bu_board*out){
 if(out)memset(out,0,sizeof(*out));if(!rom||bytes!=512||!out||le16(rom)!=0x8195)return EINVAL;
 rtw_r10bu_board b;memset(&b,0,sizeof(b));memcpy(b.mac,rom+0x11a,6);unsigned any=0,all=255;
 for(unsigned i=0;i<6;++i){any|=b.mac[i];all&=b.mac[i];}if(!any||all==255||(b.mac[0]&1))return EINVAL;
 if(rom[0xc8]==255||rom[0xc9]==255)return ENOTSUP;
 b.crystal=rom[0xc9]&63;b.thermal=rom[0xca]&63;b.channel_plan=rom[0xc8];b.vid=le16(rom+0x1c0);b.pid=le16(rom+0x1c2);
 memcpy(b.txpower,rom+0x20,12);*out=b;return 0;
}
int rtw_r10bu_validate_firmware(const u8*b,size_t bytes,rtw_r10bu_firmware*out){
 if(out)memset(out,0,sizeof(*out));if(!b||bytes<=32||bytes>32+32768||(le16(b)!=0x10b1&&le16(b)!=0x10b2)||b[2]!=0x10||b[3]||le16(b+12)!=bytes-32)return EINVAL;
 if(out){out->version=le16(b+4);out->subversion=b[6];out->payload_bytes=le16(b+12);}return 0;
}
static int physical_otp(struct rtw_r10bu*c,u16 address,u8*out){
 if(!c||!out||address>=512)return EINVAL;
 rtl8xxxu_write32(c,REG_USB_HOST_INDIRECT_ADDR_8710B,address);
 rtl8xxxu_write32(c,REG_EFUSE_INDIRECT_CTRL_8710B,EFUSE_READ_OFFSET);
 if(c->error)return c->error;
 // The source performs one settle read before its bounded completion loop.
 (void)rtl8xxxu_read32(c,REG_EFUSE_INDIRECT_CTRL_8710B);
 for(unsigned i=0;i<RTL8XXXU_MAX_REG_POLL;++i){u32 v=rtl8xxxu_read32(c,REG_EFUSE_INDIRECT_CTRL_8710B);if(c->error)return c->error;
  if(!(v&BIT(31))){*out=(u8)rtl8xxxu_read32(c,REG_USB_HOST_INDIRECT_DATA_8710B);return c->error;}}
 return c->error=ETIMEDOUT;
}
int rtw_r10bu_identify(rtw_r10bu*c){
 if(!c)return EINVAL;if(c->status.powered||c->status.firmware)return EBUSY;if(c->error)return c->error;
 c->status.identified=0;memset(&c->board,0,sizeof(c->board));
 u32 cfg=rtl8710b_read_syson_reg(c,REG_SYS_SYSTEM_CFG0_8710B);if(c->error)return c->error;
 if(cfg&BIT(16))return ENOTSUP;u8 vendor=(u8)((cfg>>6)&3U);if(vendor!=RTW_R10BU_SMIC&&vendor!=RTW_R10BU_UMC)return ENOTSUP;
 u32 cfg2=rtl8710b_read_syson_reg(c,REG_SYS_SYSTEM_CFG2_8710B);if(c->error)return c->error;
 u32 eprom=rtl8710b_read_syson_reg(c,REG_SYS_EEPROM_CTRL0_8710B);if(c->error)return c->error;
 for(u16 i=0;i<512;++i){int e=physical_otp(c,i,&c->packed[i]);if(e)return e;}
 size_t consumed=0;if(rtw_efuse_decode(c->packed,512,c->logical,512,&consumed)!=RTW_EFUSE_OK)return EINVAL;
 int e=rtw_r10bu_decode_rom(c->logical,512,&c->board);if(e)return e;
 u8 package=c->packed[EEPROM_PACKAGE_TYPE_8710B];
 if(package==255)package=vendor==RTW_R10BU_SMIC?PACKAGE_QFN48M_S:PACKAGE_QFN48M_U;
 if(!valid_package(package))return ENOTSUP;
 c->board.cut=(u8)(cfg&15U);c->board.vendor=vendor;c->board.rom_revision=(u8)(cfg2&15U);c->board.package=package;
 c->board.boot_eeprom=(u8)((eprom&EEPROM_BOOT)!=0);c->board.has_eeprom=(u8)((eprom&EEPROM_ENABLE)!=0);
 c->package_type=package;c->default_crystal_cap=c->board.crystal;c->thermal_reference=c->thermal_previous=c->board.thermal;c->status.identified=1;return 0;
}
int rtw_r10bu_board_info(const rtw_r10bu*c,rtw_r10bu_board*out){if(!c||!out)return EINVAL;*out=c->board;return c->status.identified?0:ENXIO;}
int rtw_r10bu_status_read(const rtw_r10bu*c,rtw_r10bu_status*out){if(!c||!out)return EINVAL;*out=c->status;out->error=c->error;out->tx_iqk_mask=c->tx_iqk_mask;out->rx_iqk_mask=c->rx_iqk_mask;return 0;}
static int nibble(u8 value){value&=15;return value&8?(int)value-16:value;}
static int operating_valid(const rtw_r10bu_board*b,const rtw_r10bu_operating*op){
 if(!b||!op||!valid_package(b->package)||(b->vendor!=RTW_R10BU_SMIC&&b->vendor!=RTW_R10BU_UMC)||op->primary<1||op->primary>11||op->ht>1)return 0;
 if(op->width_mhz==20)return op->center==op->primary;
 if(!op->ht||op->width_mhz!=40)return 0;
 return (op->center==op->primary+2&&op->primary+4<=11)||(op->primary>=5&&op->center==op->primary-2);
}
int rtw_r10bu_channel_supported(const rtw_r10bu_board*b,const rtw_r10bu_operating*op){
 if(!operating_valid(b,op))return 0;int group,cck;rtl8188f_channel_to_group(op->primary,&group,&cck);const u8*p=b->txpower;
 if(p[cck]>63||p[6+group]>63||p[11]==255)return 0;
 int base=p[6+group],ofdm=base+nibble(p[11]),mcs=base+(op->width_mhz==40?0:nibble(p[11]>>4));
 return ofdm>=0&&ofdm<=63&&mcs>=0&&mcs<=63;
}
static void populate_power(struct rtw_r10bu*c){const u8*p=c->board.txpower;
 memcpy(c->cck_tx_power_index_A,p,6);memcpy(c->ht40_1s_tx_power_index_A,p+6,5);
 c->ofdm_tx_power_diff[0].a=(s8)nibble(p[11]);c->ht20_tx_power_diff[0].a=(s8)nibble(p[11]>>4);
}
static int calibrated(struct rtw_r10bu*c){return!c->error&&!c->cal_timeout&&c->tx_iqk_mask==1&&c->rx_iqk_mask==1;}
static int pause_failed(struct rtw_r10bu*c,int error){c->status.calibrated=c->status.tx_enabled=0;c->cleanup=true;(void)rtw_r10bu_write(c,REG_TXPAUSE,1,255);c->cleanup=false;if(!c->error)c->error=error;return error;}
static int host_security(struct rtw_r10bu*c){
 int e=rtw_r10bu_write(c,REG_HWSEQ_CTRL,1,0);if(!e)e=rtw_r10bu_write(c,REG_SECURITY_CFG,1,0);
 for(unsigned i=0;!e&&i<100;++i){u32 value=0;e=rtw_r10bu_read(c,REG_CAM_CMD,4,&value);if(e)return e;
  if(!(value&CAM_CMD_POLLING))return 0;}
 return e?e:ETIMEDOUT;
}
int rtw_r10bu_boot(rtw_r10bu*c,const u8*fw,size_t bytes,u8 firmware_vendor){
 if(!c||!c->status.identified)return ENXIO;if(c->status.powered||c->status.firmware)return EBUSY;
 if(firmware_vendor!=c->board.vendor||(firmware_vendor!=RTW_R10BU_SMIC&&firmware_vendor!=RTW_R10BU_UMC))return EINVAL;
 int e=rtw_r10bu_validate_firmware(fw,bytes,NULL);if(e)return e;
 if(le16(fw)!=(firmware_vendor==RTW_R10BU_SMIC?0x10b1:0x10b2))return EINVAL;
 rtw_r10bu_operating initial={1,1,20,0};if(!rtw_r10bu_channel_supported(&c->board,&initial))return ENOTSUP;if(c->error)return c->error;
 populate_power(c);c->fw_data=(const struct rtl8xxxu_firmware_header*)fw;c->fw_size=(int)(bytes-32);c->h2c_box=0;c->cal_timeout=false;c->tx_iqk_mask=c->rx_iqk_mask=0;
 /* Even an interrupted power transition needs the exact 8710BU rollback. */
 c->status.powered=1;int result=rtl8xxxu_init_device(&c->hw);e=c->error?c->error:result<0?-result:0;
 if(!e&&(!calibrated(c)))e=ETIMEDOUT;
 if(!e)e=rtw_r10bu_write(c,REG_TXPAUSE,1,255);for(unsigned i=0;!e&&i<6;++i)e=rtw_r10bu_write(c,(u16)(REG_MACID+i),1,c->board.mac[i]);if(!e)e=rtw_r10bu_update(c,REG_CR,4,0x30000,0);if(!e)e=rtw_r10bu_write(c,REG_SECURITY_CFG,1,0);if(!e)e=rtw_r10bu_write(c,REG_RXFLTMAP1,2,0);if(!e)e=rtw_r10bu_write(c,REG_RXFLTMAP2,2,0);
 if(!e)e=host_security(c);if(e){(void)rtw_r10bu_stop(c);return e;}
 c->status.firmware=c->status.mac=c->status.phy=c->status.calibrated=1;c->status.tx_enabled=0;c->status.max_bw=40;c->status.primary=c->status.center=1;c->status.width_mhz=20;e=rtw_r10bu_set_operating(c,&initial);if(!e){u32 live=0;e=rtw_r10bu_read(c,REG_8051FW_CTRL_V1_8710B,1,&live);if(!e&&(live&0xc7)!=0xc6)e=ENXIO;}if(e){(void)rtw_r10bu_stop(c);return e;}return 0;
}
int rtw_r10bu_set_operating(rtw_r10bu*c,const rtw_r10bu_operating*op){
 if(!c||!c->status.phy||!c->status.firmware)return ENXIO;if(!rtw_r10bu_channel_supported(&c->board,op)||op->width_mhz>c->status.max_bw)return ENOTSUP;if(c->error)return c->error;
 c->status.calibrated=c->status.tx_enabled=0;int e=rtw_r10bu_write(c,REG_TXPAUSE,1,255);if(e)return pause_failed(c,e);
 c->channel.hw_value=op->primary;c->hw.conf.chandef.width=op->width_mhz;c->hw.conf.chandef.secondary=op->center>op->primary?1:op->center<op->primary?-1:0;
 rtl8710bu_config_channel(&c->hw);rtl8188f_set_tx_power(c,op->primary,op->width_mhz==40);c->cal_timeout=false;c->tx_iqk_mask=c->rx_iqk_mask=0;rtl8188f_phy_lc_calibrate(c);rtl8710bu_phy_iq_calibrate(c);
 e=c->error?c->error:calibrated(c)?0:ETIMEDOUT;if(e)return pause_failed(c,e);
 c->status.primary=op->primary;c->status.center=op->center;c->status.width_mhz=op->width_mhz;c->status.calibrated=1;return 0;
}
int rtw_r10bu_resume(rtw_r10bu*c){if(!c||!c->status.firmware||!c->status.mac||!c->status.calibrated||!calibrated(c))return ENXIO;rtl8710b_enable_rf(c);if(c->error)return pause_failed(c,c->error);c->status.tx_enabled=1;return 0;}
int rtw_r10bu_pause(rtw_r10bu*c){if(!c)return EINVAL;c->status.tx_enabled=0;int e=rtw_r10bu_write(c,REG_TXPAUSE,1,255);return e?pause_failed(c,e):0;}
int rtw_r10bu_stop(rtw_r10bu*c){
 if(!c)return EINVAL;int original=c->error;c->cleanup=true;(void)rtw_r10bu_write(c,REG_TXPAUSE,1,255);if(c->status.powered){rtl8710b_disable_rf(c);rtl8710bu_power_off(c);}(void)rtw_r10bu_write(c,REG_TXPAUSE,1,255);c->cleanup=false;
 u8 identified=c->status.identified;memset(&c->status,0,sizeof(c->status));c->status.identified=identified;c->tx_iqk_mask=c->rx_iqk_mask=0;c->h2c_box=0;c->fw_data=NULL;c->fw_size=0;c->thermal_pending=false;return original?original:c->error;
}
static int ready(struct rtw_r10bu*c){
 if(!c||!c->status.firmware||!c->status.mac||!c->status.phy)return ENXIO;if(c->error)return c->error;
 u32 state=0;int e=rtw_r10bu_read(c,REG_8051FW_CTRL_V1_8710B,1,&state);return e?e:(state&0xc7)==0xc6?0:ENXIO;
}
int rtw_r10bu_tx_endpoint(const rtw_r10bu*c,u8 queue,unsigned*out){
 if(!c||!out||queue>=32)return EINVAL;u8 q=queue;if(q<8)q=q==1||q==2?TXDESC_QUEUE_BK:q==4||q==5?TXDESC_QUEUE_VI:q>=6?TXDESC_QUEUE_VO:TXDESC_QUEUE_BE;
 u8 ep=c->pipe_out[q];for(unsigned i=0;i<c->usb.bulk_out_count;++i)if(c->usb.bulk_out[i]==ep){*out=i;return 0;}return EINVAL;
}
int rtw_r10bu_h2c(rtw_r10bu*c,const u8 command[8]){
 if(!command)return EINVAL;int e=ready(c);if(e)return e;u32 status=0;unsigned box=c->h2c_box;
 for(unsigned i=0;i<31;++i){e=rtw_r10bu_read(c,REG_HMTFR,1,&status);if(e)return e;if(!(status&BIT(box)))break;if(i==30)return ETIMEDOUT;delay(c,100);}
 u32 ext=(u32)command[4]|((u32)command[5]<<8)|((u32)command[6]<<16)|((u32)command[7]<<24),main=(u32)command[0]|((u32)command[1]<<8)|((u32)command[2]<<16)|((u32)command[3]<<24);
 e=rtw_r10bu_write(c,0x1f0+4*box,4,ext);if(!e)e=rtw_r10bu_write(c,0x1d0+4*box,4,main);if(!e)c->h2c_box=(u8)((box+1)&3);return e;
}
int rtw_r10bu_rate_build(u16 legacy,const u8 mcs[2],int ht,unsigned width,int local_sgi,int peer_sgi,rtw_r10bu_rate*out){
 if(out)memset(out,0,sizeof(*out));if(!out||!legacy||(legacy&~0xfffU)||(ht!=0&&ht!=1)||(width!=20&&width!=40)||(!ht&&width!=20)||(local_sgi!=0&&local_sgi!=1)||(peer_sgi!=0&&peer_sgi!=1)||!mcs)return 0;
 rtw_r10bu_rate r;memset(&r,0,sizeof(r));r.mask=legacy;r.width_mhz=(u8)width;r.ht=(u8)ht;
 if(ht){u32 high=(u32)mcs[0];if(!high||!mcs[0])return 0;r.mask|=high<<12;int two=0;r.rate_id=(u8)(legacy&15?(width==40?(two?0:1):(two?2:3)):(two?4:5));r.sgi=(u8)(local_sgi&&peer_sgi);}
 else r.rate_id=(u8)((legacy&0xff0)?(legacy&15?6:7):8);
 for(unsigned i=0;i<28;++i)if(r.mask&BIT(i))r.initial=(u8)i;*out=r;return 1;
}
int rtw_r10bu_rate_command(const rtw_r10bu_rate*r,int reset,u8 out[8]){
 if(out)memset(out,0,8);if(!r||!out||(reset!=0&&reset!=1))return 0;u8 mcs[2]={(u8)(r->mask>>12),(u8)(r->mask>>20)};rtw_r10bu_rate checked;
 if(!rtw_r10bu_rate_build((u16)(r->mask&0xfff),mcs,r->ht,r->width_mhz,r->sgi,r->sgi,&checked)||checked.mask!=r->mask||checked.rate_id!=r->rate_id||checked.initial!=r->initial)return 0;
 out[0]=0x40;out[2]=(u8)(r->rate_id|(r->sgi?0x80:0));out[3]=(u8)((reset?0:8)|(r->width_mhz==40?1:0));for(unsigned i=0;i<4;++i)out[i+4]=(u8)(r->mask>>(8*i));return 1;
}
int rtw_r10bu_station(rtw_r10bu*c,int associated,const u8 bssid[6],u16 aid,u16 interval,u16 basic,int short_slot,int ht){
 int e=ready(c);if(e)return e;if((associated!=0&&associated!=1)||(short_slot!=0&&short_slot!=1)||(ht!=0&&ht!=1)||!basic||(basic&~0xfffU)||(associated&&(!bssid||!aid||aid>2007||!interval)))return EINVAL;
 e=rtw_r10bu_pause(c);if(!e)e=rtw_r10bu_update(c,REG_CR,4,0x30000,associated?0x20000:0);
 if(!e)e=rtw_r10bu_update(c,REG_RESPONSE_RATE_SET,4,0xfffff,basic);if(!e)e=rtw_r10bu_write(c,REG_SLOT,1,short_slot?9:20);
 if(!e)e=rtw_r10bu_write(c,REG_RXFLTMAP1,2,associated&&ht?0x100:0);if(!e)e=rtw_r10bu_write(c,REG_RXFLTMAP2,2,associated?0xffff:0);
 if(associated){for(unsigned i=0;!e&&i<6;++i)e=rtw_r10bu_write(c,REG_BSSID+i,1,bssid[i]);if(!e)e=rtw_r10bu_write(c,REG_BCN_PSR_RPT,2,(aid&0x3fff)|0xc000);if(!e)e=rtw_r10bu_write(c,REG_BCN_INTERVAL,2,interval);if(!e)e=rtw_r10bu_update(c,REG_DUAL_TSF_RST,1,1,1);if(!e)e=rtw_r10bu_update(c,REG_BEACON_CTRL,1,BEACON_DISABLE_TSF_UPDATE,0);}
 else if(!e)e=rtw_r10bu_update(c,REG_BEACON_CTRL,1,BEACON_DISABLE_TSF_UPDATE,BEACON_DISABLE_TSF_UPDATE);
 if(!e)e=rtw_r10bu_update(c,REG_RCR,4,RCR_CHECK_BSSID_MATCH|RCR_CHECK_BSSID_BEACON,associated?(RCR_CHECK_BSSID_MATCH|RCR_CHECK_BSSID_BEACON):0);
 return e?pause_failed(c,e):0;
}
int rtw_r10bu_edca_update(rtw_r10bu*c,int short_slot,const rtw_r10bu_edca ac[4]){
 int e=ready(c);if(e)return e;if(!ac||(short_slot!=0&&short_slot!=1))return EINVAL;unsigned slot=short_slot?9:20;
 for(unsigned i=0;i<4;++i)if(ac[i].aifsn>15||ac[i].ecwmin>15||ac[i].ecwmax>15||ac[i].ecwmin>ac[i].ecwmax||10+slot*ac[i].aifsn>255)return EINVAL;
 static const u16 addresses[4]={REG_EDCA_BE_PARAM,REG_EDCA_BK_PARAM,REG_EDCA_VI_PARAM,REG_EDCA_VO_PARAM};
 /* RTL8710B SetHwReg8710B and rtl8xxxu_conf_tx replace the full DWORD
  * with the u16 TXOP at bit16. Do not retain old upper TXOP bits or borrow
  * the newer rtw88 eleven-bit field. See LEGACY-RX-EDCA-REVIEW.md. */
 e=rtw_r10bu_write(c,REG_SLOT,1,slot);for(unsigned i=0;!e&&i<4;++i){u32 word=((u32)ac[i].txop<<16)|((u32)ac[i].ecwmax<<12)|((u32)ac[i].ecwmin<<8)|(10+slot*ac[i].aifsn);e=rtw_r10bu_write(c,addresses[i],4,word);}return e;
}
int rtw_r10bu_tick(rtw_r10bu*c){
 int e=ready(c);if(e)return c?pause_failed(c,e):e;if(!c->status.calibrated||!calibrated(c))return ENXIO;
 /* The source8710BU enables this real six-bit thermal meter. RFK below uses
  * only measured samples; no thermal correction coefficient is fabricated. */
 if(!c->thermal_pending){u32 meter=rtl8xxxu_read_rfreg(c,RF_A,RF6052_REG_T_METER_8723B);rtl8xxxu_write_rfreg(c,RF_A,RF6052_REG_T_METER_8723B,meter|0x30000);if(c->error)return pause_failed(c,c->error);c->thermal_pending=true;return 0;}
 u32 value=rtl8xxxu_read_rfreg(c,RF_A,RF6052_REG_T_METER_8723B);c->thermal_pending=false;if(c->error)return pause_failed(c,c->error);u8 thermal=(u8)((value>>10)&63);if(!thermal||thermal==63)return EAGAIN;c->status.thermal=thermal;++c->status.thermal_samples;return 0;
}
int rtw_r10bu_decode_phy(const rtw_r10bu*c,u8 rate,const u8*phy,size_t bytes,int*out){
 if(out)*out=0;if(!c||!out||!phy||bytes!=32||rate>=20||!c->status.phy)return 0;int value;
 if(phy[0]==0&&rate<4){if(c->cck_new_agc)value=(int)phy[1]-110;else{unsigned lna=((phy[14]>>7)<<3)|(phy[13]>>5),vga=phy[13]&31;switch(lna){case 7:value=-52-2*(int)vga;break;case 6:value=-42-2*(int)vga;break;case 5:value=-36-2*(int)vga;break;case 3:value=-12-2*(int)vga;break;case 2:value=0-2*(int)vga;break;default:return 0;}}}
 else if((phy[0]==1||phy[0]==2)&&rate>=4)value=(int)phy[1]-110;
 else return 0;if(value<-110||value>-1)return 0;*out=value;return 1;
}
int rtw_r10bu_make_tx(const rtw_r10bu_tx_options*o,const rtw_r10bu_rate*r,u8 out[40]){
 if(out)memset(out,0,40);if(!o||!out)return 0;u8 command[8];if(o->firmware_rate&&(!rtw_r10bu_rate_command(r,1,command)||o->rate!=r->initial||!(r->mask&BIT(o->rate))||o->bandwidth_mhz!=r->width_mhz||o->short_gi!=r->sgi))return 0;
 if(!rtw_r10bu_tx_descriptor(o,out))return 0;if(o->firmware_rate){out[6]=(u8)((out[6]&0xe0)|r->rate_id);out[28]=out[29]=0;u16 sum=0;for(unsigned i=0;i<32;i+=2)sum^=(u16)((unsigned)out[i]|((unsigned)out[i+1]<<8));out[28]=(u8)sum;out[29]=(u8)(sum>>8);}return rtw_r10bu_tx_descriptor_valid(out,o->rf_chains);
}

int rtw_r10bu_invalidate(rtw_r10bu*c){if(!c)return EINVAL;c->status.calibrated=c->status.tx_enabled=0;c->tx_iqk_mask=c->rx_iqk_mask=0;c->cleanup=true;int e=rtw_r10bu_write(c,REG_TXPAUSE,1,255);if(e)(void)rtw_r10bu_write(c,REG_TXPAUSE,1,255);c->cleanup=false;return e;}
void rtw_r10bu_forget(rtw_r10bu*c){if(!c)return;memset(&c->status,0,sizeof(c->status));c->tx_iqk_mask=c->rx_iqk_mask=0;c->error=ENXIO;c->fw_data=NULL;c->fw_size=0;}
