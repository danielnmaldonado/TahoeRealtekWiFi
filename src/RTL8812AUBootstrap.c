/* SPDX-License-Identifier: BSD-3-Clause */
/* Real RTL8812AU/RTL8821AU bootstrap from pinned Linuxrtw88. All readiness
 * state is typed AU state. See RTL8812AUBootstrap-Upstream.json and
 * RTL8812AUPHY-LICENSE.txt. No 8814AU context or 3081 downloader is used. */
#include "rtw/RTL8812AUBootstrap.h"
#include "rtw/Efuse.h"
#include "RTL8812AUCoreRegs.h"
#include <string.h>
#ifdef KERNEL
#include <sys/errno.h>
#else
#include <errno.h>
#endif
#include "RTL8812AUPower.inc"
static int valid(const rtw_r12au_boot*c){return c&&(c->chip==RTW_R12AU_CHIP_8812AU||c->chip==RTW_R12AU_CHIP_8821AU)&&c->bus.control_in&&c->bus.control_out&&c->waiter.wait_us;}
static int latch(rtw_r12au_boot*c,int e){if(e&&!c->io_error)c->io_error=e;return e;}
static int overlap(const void*a,size_t na,const void*b,size_t nb){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return a&&b&&na&&nb&&(x<=y?y-x<na:x-y<nb);}
static uint16_t le16(const uint8_t*p){return(uint16_t)((unsigned)p[0]|((unsigned)p[1]<<8));}
static uint32_t le32(const uint8_t*p){return(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
int rtw_r12au_read(rtw_r12au_boot*c,uint16_t a,uint8_t w,uint32_t*out){
 if(out)*out=0;if(!valid(c)||!out||(w!=1&&w!=2&&w!=4)||(uint32_t)a+w>65536)return EINVAL;if(c->io_error&&!c->cleanup)return c->io_error;
 return latch(c,rtw_usb_rw_read(&c->bus,a,w,out)==RTW_USB_OK?0:EIO);
}
int rtw_r12au_write(rtw_r12au_boot*c,uint16_t a,uint8_t w,uint32_t value){
 if(!valid(c)||(w!=1&&w!=2&&w!=4)||(uint32_t)a+w>65536)return EINVAL;if(c->io_error&&!c->cleanup)return c->io_error;
 return latch(c,rtw_usb_write_register(&c->bus,a,w,value)==RTW_USB_OK?0:EIO);
}
int rtw_r12au_update(rtw_r12au_boot*c,uint16_t a,uint8_t w,uint32_t mask,uint32_t value){uint32_t old=0;int e=rtw_r12au_read(c,a,w,&old);return e?e:rtw_r12au_write(c,a,w,(old&~mask)|(value&mask));}
int rtw_r12au_delay(rtw_r12au_boot*c,uint32_t us){if(!valid(c)||!us)return EINVAL;if(c->io_error&&!c->cleanup)return c->io_error;return latch(c,c->waiter.wait_us(c->waiter.context,us)?0:EIO);}
static int poll(rtw_r12au_boot*c,uint16_t a,uint8_t w,uint32_t mask,uint32_t bits,unsigned attempts,unsigned delay){
 uint32_t value=0;for(unsigned i=0;i<attempts;++i){int e=rtw_r12au_read(c,a,w,&value);if(e)return e;if((value&mask)==bits)return 0;if(i+1<attempts&&(e=rtw_r12au_delay(c,delay)))return e;}return latch(c,ETIMEDOUT);
}
static int power_flow(rtw_r12au_boot*c,const rtw_power_step*t,size_t n){
 int first=0;for(size_t i=0;i<n;++i){const rtw_power_step*s=t+i;int e;
  if(s->opcode==RTW_PWR_END)return first;
  if(s->opcode==RTW_PWR_DELAY_US)e=rtw_r12au_delay(c,s->address_or_delay);
  else if(s->opcode==RTW_PWR_POLL8)e=poll(c,s->address_or_delay,1,s->mask,s->value&s->mask,1000,1000);
  else e=rtw_r12au_update(c,s->address_or_delay,1,s->mask,s->value);
  if(e&&!first)first=e;if(e&&!c->cleanup)return e;
 }return first?first:EINVAL;
}
static int config_valid(const rtw_r12au_usb_config*u){
 if(!u||u->bulk_out_count<2||u->bulk_out_count>4||!(u->bulk_in&0x80)||!(u->bulk_in&15)||(u->bulk_in&0x70))return 0;
 unsigned packet=u->speed==RTW_R12AU_USB_FULL?64:u->speed==RTW_R12AU_USB_HIGH?512:u->speed==RTW_R12AU_USB_SUPER?1024:0;
 if(!packet||u->bulk_max_packet!=packet)return 0;
 for(unsigned i=0;i<u->bulk_out_count;++i){if(!u->bulk_out[i]||u->bulk_out[i]>15)return 0;for(unsigned j=0;j<i;++j)if(u->bulk_out[i]==u->bulk_out[j])return 0;}return 1;
}
int rtw_r12au_boot_init(rtw_r12au_boot*c,rtw_r12au_chip chip,const rtw_usb_rw_transport*bus,const rtw_waiter*wait,const rtw_r12au_usb_config*usb){
 if(!c||!bus||!bus->control_in||!bus->control_out||!wait||!wait->wait_us||!config_valid(usb)||(chip!=RTW_R12AU_CHIP_8812AU&&chip!=RTW_R12AU_CHIP_8821AU))return EINVAL;
 rtw_usb_rw_transport b=*bus;rtw_waiter w=*wait;rtw_r12au_usb_config u=*usb;memset(c,0,sizeof(*c));c->chip=chip;c->bus=b;c->waiter=w;c->usb=u;return 0;
}
int rtw_r12au_decode_board(rtw_r12au_chip chip,uint32_t cfg,const uint8_t map[512],const uint8_t hints[4],unsigned bt,rtw_r12au_board*out){
 rtw_r12au_board b;unsigned antenna=0,wmode=0;uint8_t cut=(uint8_t)((cfg>>12)&15),paths=1;
 if(!out||!map||overlap(out,sizeof(*out),map,512)||overlap(out,sizeof(*out),hints,4))return EINVAL;
 memset(out,0,sizeof(*out));if(!cfg||cfg==UINT32_MAX||(cfg&BIT_RTL_ID)||bt>1)return EINVAL;
 if(chip==RTW_R12AU_CHIP_8812AU){if(!hints||cut>=14)return EINVAL;++cut;paths=2;
  for(unsigned i=0;i<2;++i){unsigned v=hints[1-i];antenna=(v>>5)&7;if(!antenna)antenna=(v>>1)&7;if(antenna)break;}
  for(unsigned i=0;i<2;++i){wmode=(hints[3-i]>>2)&3;if(wmode)break;}if(antenna==1)paths=1;
 }else if(chip!=RTW_R12AU_CHIP_8821AU||hints)return EINVAL;
 memset(&b,0,sizeof(b));int e=rtw_r12au_phy_parse_board(chip,cut,paths,bt,map,&b.phy);if(e)return e;
 /* The primary uses bt_setting bit0 directly for shared antenna, including
  *0xff. This is a packed selector, not a missing calibration index. */
 unsigned mac=chip==RTW_R12AU_CHIP_8812AU?0xd7:0x107,vid=chip==RTW_R12AU_CHIP_8812AU?0xd0:0x100;
 memcpy(b.mac,map+mac,6);b.eeprom_vendor=le16(map+vid);b.eeprom_product=le16(map+vid+2);b.sys_cfg=cfg;
 b.channel_plan=map[0xb8];memcpy(b.country,map+0xcb,2);b.regd=map[0xc1]&7;b.bt_setting=map[0xc3];b.share_ant=map[0xc3]&1;
 b.vht=(uint8_t)!(chip==RTW_R12AU_CHIP_8812AU&&antenna==2&&wmode==2);*out=b;return 0;
}
static int otp_byte(rtw_r12au_boot*c,unsigned address,uint8_t*out){
 unsigned limit=c->chip==RTW_R12AU_CHIP_8812AU?1024:512;uint32_t v=0;int e;if(address>=limit)return EINVAL;
 if((e=rtw_r12au_write(c,0x31,1,address&255))||(e=rtw_r12au_update(c,0x32,1,3,address>>8))||(e=rtw_r12au_update(c,0x33,1,0x80,0)))return e;
 for(unsigned i=0;i<100;++i){if((e=rtw_r12au_read(c,0x30,4,&v)))return e;if(v&BIT(31)){*out=(uint8_t)v;return 0;}if(i+1<100&&(e=rtw_r12au_delay(c,1)))return e;}return latch(c,ETIMEDOUT);
}
int rtw_r12au_identify(rtw_r12au_boot*c){
 uint8_t raw[512],hints[4];uint32_t cfg=0,bt=0;int e,closed;
 if(!valid(c))return EINVAL;if(c->powered||c->firmware_running||c->mac_initialized)return EBUSY;if(c->io_error)return c->io_error;
 c->identified=0;memset(&c->board,0,sizeof(c->board));memset(c->logical_rom,255,sizeof(c->logical_rom));
 if((e=rtw_r12au_read(c,REG_SYS_CFG1,4,&cfg)))return e;if(!cfg||cfg==UINT32_MAX||(cfg&BIT_RTL_ID))return EINVAL;
 if(c->chip==RTW_R12AU_CHIP_8821AU&&(e=rtw_r12au_read(c,REG_WL_BT_PWR_CTRL,4,&bt)))return e;
 e=rtw_r12au_write(c,REG_EFUSE_ACCESS,1,EFUSE_ACCESS_ON);
 if(!e)e=rtw_r12au_update(c,REG_SYS_FUNC_EN,2,BIT_FEN_ELDR,BIT_FEN_ELDR);
 if(!e)e=rtw_r12au_update(c,REG_SYS_CLKR,2,BIT_LOADER_CLK_EN|BIT_ANA8M,BIT_LOADER_CLK_EN|BIT_ANA8M);
 for(unsigned i=0;!e&&i<512;++i)e=otp_byte(c,i,raw+i);
 if(c->chip==RTW_R12AU_CHIP_8812AU)for(unsigned i=0;!e&&i<4;++i)e=otp_byte(c,1018+i,hints+i);
 c->cleanup=1;closed=rtw_r12au_write(c,REG_EFUSE_ACCESS,1,EFUSE_ACCESS_OFF);
 /* A failed closing transfer may have left the OTP clock gate open. Make
  * one bounded best-effort close retry while preserving the original error. */
 if(closed)(void)rtw_r12au_write(c,REG_EFUSE_ACCESS,1,EFUSE_ACCESS_OFF);c->cleanup=0;if(!e)e=closed;if(e)return e;
 if(rtw_efuse_decode(raw,sizeof(raw),c->logical_rom,sizeof(c->logical_rom),NULL)!=RTW_EFUSE_OK)return EBADMSG;
 e=rtw_r12au_decode_board(c->chip,cfg,c->logical_rom,c->chip==RTW_R12AU_CHIP_8812AU?hints:NULL,(bt&BIT_BT_FUNC_EN)!=0,&c->board);
 if(!e)c->identified=1;return e;
}
static int reset_resident_mcu(rtw_r12au_boot*c,uint32_t status);
int rtw_r12au_power_on(rtw_r12au_boot*c){
 uint32_t status=0;int e;if(!valid(c)||!c->identified)return EINVAL;if(c->io_error)return c->io_error;if(c->powered)return 0;
 if((e=rtw_r12au_read(c,REG_MCUFW_CTRL,1,&status)))return e;
 if((e=rtw_r12au_write(c,REG_TXPAUSE,1,0x3f)))return e;
 if(c->chip==RTW_R12AU_CHIP_8812AU){if((e=rtw_r12au_write(c,REG_RF_CTRL,1,5))||(e=rtw_r12au_write(c,REG_RF_CTRL,1,7))||(e=rtw_r12au_write(c,REG_RF_B_CTRL,1,5))||(e=rtw_r12au_write(c,REG_RF_B_CTRL,1,7)))return e;
 }
 if((status&BIT_RAM_DL_SEL)&&(e=reset_resident_mcu(c,status)))return e;
 if(c->chip==RTW_R12AU_CHIP_8812AU)e=power_flow(c,au_8812a_enable,ARRAY_SIZE(au_8812a_enable));
 else e=power_flow(c,au_8821a_enable,ARRAY_SIZE(au_8821a_enable));
 if(!e)e=rtw_r12au_write(c,REG_CR,2,0);if(!e)e=rtw_r12au_write(c,REG_CR,2,0x63f);
 if(!e&&c->chip==RTW_R12AU_CHIP_8821AU&&(c->board.sys_cfg&BIT_LDO))e=rtw_r12au_update(c,REG_LDO_SWR_CTRL,1,BIT(6),BIT(6));
 if(!e)c->powered=1;return e;
}
int rtw_r12au_fw_inspect(rtw_r12au_chip chip,const uint8_t*data,size_t n,rtw_fw_image*out){
 if(!out)return EINVAL;if(data&&overlap(out,sizeof(*out),data,n))return EINVAL;memset(out,0,sizeof(*out));if(!data)return EINVAL;
 if(n<=32||n>32+32768)return EBADMSG;uint16_t sig=le16(data);unsigned expected=chip==RTW_R12AU_CHIP_8812AU?0x9500U:chip==RTW_R12AU_CHIP_8821AU?0x2100U:0;
 if(!expected||(sig&0xfff0U)!=expected||data[2]!=(chip==RTW_R12AU_CHIP_8812AU?0x10:0x13)||data[3]!=0||le16(data+12)!=n-32)return EBADMSG;
 out->signature=sig;out->category=data[2];out->function=data[3];out->version=le16(data+4);out->subversion=data[6];out->payload=data+32;out->payload_length=n-32;out->pages=(uint8_t)((n-32+4095)/4096);return 0;
}
static int reset_mcu(rtw_r12au_boot*c){
 uint32_t cpu=0;uint32_t bit=c->chip==RTW_R12AU_CHIP_8812AU?BIT(3):BIT(0);int e;
 if((e=rtw_r12au_update(c,REG_RSV_CTRL,1,BIT(1),0))||(e=rtw_r12au_update(c,REG_RSV_CTRL+1,1,bit,0))||(e=rtw_r12au_read(c,REG_SYS_FUNC_EN+1,1,&cpu))||(e=rtw_r12au_write(c,REG_SYS_FUNC_EN+1,1,cpu&~BIT(2)))||(e=rtw_r12au_update(c,REG_RSV_CTRL,1,BIT(1),0))||(e=rtw_r12au_update(c,REG_RSV_CTRL+1,1,bit,bit))||(e=rtw_r12au_write(c,REG_SYS_FUNC_EN+1,1,cpu|BIT(2))))return e;return 0;
}
/* Pinned Linux rtw88xxau_hw_reset: retire resident RAM firmware before the
 * cold power sequence. Caller has positively identified and exclusively owns
 * this typed AU; no ROM/RFE or firmware-ready admission is relaxed. Unlike
 * upstream's void callback, a failed MAC-off handshake stops this boot. */
static int reset_resident_mcu(rtw_r12au_boot*c,uint32_t status){
 int e;++c->warm_reset_attempts;c->warm_reset_mcu=status;c->warm_reset_error=0;c->warm_reset_stage=1;
 c->dma_prepared=c->firmware_running=c->mac_initialized=c->associated=0;c->h2c_box=0;
 if((e=reset_mcu(c))||(e=rtw_r12au_write(c,REG_MCUFW_CTRL,1,0)))goto fail;
 c->warm_reset_stage=2;
 if((e=rtw_r12au_update(c,REG_FPGA0_XCD_RF_PARA,4,BIT(6),BIT(6)))||
    (e=rtw_r12au_update(c,REG_SYS_FUNC_EN,1,BIT(0)|BIT(1),0))||
    (e=rtw_r12au_write(c,REG_RF_CTRL,1,0))||(e=rtw_r12au_write(c,REG_CR,2,0)))goto fail;
 c->warm_reset_stage=3;
 if((e=rtw_r12au_update(c,REG_APS_FSMCO+1,1,APS_FSMCO_MAC_OFF>>8,APS_FSMCO_MAC_OFF>>8))||
    (e=poll(c,REG_APS_FSMCO+1,1,APS_FSMCO_MAC_OFF>>8,0,5000,1)))goto fail;
 c->warm_reset_stage=4;
 if((e=rtw_r12au_update(c,REG_APS_FSMCO+1,1,APS_FSMCO_MAC_ENABLE>>8,APS_FSMCO_MAC_ENABLE>>8))||
    (e=rtw_r12au_update(c,REG_SYS_FUNC_EN+1,1,BIT(4)|BIT(7),0))||
    (e=rtw_r12au_update(c,REG_SYS_FUNC_EN+1,1,BIT(4)|BIT(7),BIT(4)|BIT(7))))goto fail;
 c->warm_reset_stage=5;return 0;
fail:
 c->warm_reset_error=e;return e;
}
/* Checksum is write-one-to-clear; unrelated MCU control writes must not
 * accidentally acknowledge a freshly completed checksum status. */
static int mcu_update(rtw_r12au_boot*c,uint8_t width,uint32_t mask,uint32_t bits){uint32_t old=0;int e=rtw_r12au_read(c,REG_MCUFW_CTRL,width,&old);return e?e:rtw_r12au_write(c,REG_MCUFW_CTRL,width,((old&~mask)|(bits&mask))&~BIT_FWDL_CHK_RPT);}
static int download_disable(rtw_r12au_boot*c){uint8_t cleanup=c->cleanup;c->cleanup=1;int e=mcu_update(c,1,BIT_MCUFWDL_EN,0);c->cleanup=cleanup;return e;}
int rtw_r12au_download_fw(rtw_r12au_boot*c,const uint8_t*image,size_t n,rtw_boot_report*report){
 rtw_fw_image fw;uint32_t status=0;int e;size_t done=0;
 if(report&&(overlap(report,sizeof(*report),image,n)||overlap(report,sizeof(*report),c,c?sizeof(*c):0)))return EINVAL;
 if(report)memset(report,0,sizeof(*report));if(!valid(c)||!report||!c->identified||!c->powered||!c->dma_prepared)return EINVAL;
 if(c->io_error)return c->io_error;if(c->firmware_running)return EBUSY;if((e=rtw_r12au_fw_inspect(c->chip,image,n,&fw)))return e;
 c->firmware_running=0;c->mac_initialized=0;c->h2c_box=0;report->version=fw.version;report->subversion=fw.subversion;
 if((e=rtw_r12au_read(c,REG_MCUFW_CTRL,1,&status)))return e;if(status&BIT_RAM_DL_SEL)return EBUSY;
 if((e=reset_mcu(c))||(e=mcu_update(c,1,BIT_MCUFWDL_EN,BIT_MCUFWDL_EN)))goto fail;
 if((e=poll(c,REG_MCUFW_CTRL,1,BIT_MCUFWDL_EN,BIT_MCUFWDL_EN,10,20000)))goto fail;
 if((e=rtw_r12au_update(c,REG_MCUFW_CTRL+2,1,BIT(3),0)))goto fail;
 /* Explicit W1C reset before writing a single image byte. */
 if((e=rtw_r12au_read(c,REG_MCUFW_CTRL,1,&status))||(e=rtw_r12au_write(c,REG_MCUFW_CTRL,1,status|BIT_FWDL_CHK_RPT))||(e=rtw_r12au_read(c,REG_MCUFW_CTRL,1,&status)))goto fail;
 if(status&BIT_FWDL_CHK_RPT){e=latch(c,EIO);goto fail;}
 while(done<fw.payload_length){unsigned page=(unsigned)(done/4096);size_t left=fw.payload_length-done;if(left>4096)left=4096;
  if((e=rtw_r12au_update(c,REG_MCUFW_CTRL+2,1,7,page)))goto fail;report->pages_written=(uint8_t)(page+1);
  size_t in_page=0;while(in_page<left){size_t rem=left-in_page;uint16_t count=(uint16_t)(rem>=196?196:rem>=8?8:1);
   if(rtw_usb_write_block(&c->bus,(uint16_t)(0x1000+in_page),fw.payload+done+in_page,count)!=RTW_USB_OK){e=latch(c,EIO);goto fail;}
   in_page+=count;report->bytes_written+=count;++report->vendor_writes;
  }done+=left;
 }
 if((e=poll(c,REG_MCUFW_CTRL,4,BIT_FWDL_CHK_RPT,BIT_FWDL_CHK_RPT,2000,10)))goto fail;report->checksum_seen=1;
 if((e=download_disable(c))||(e=mcu_update(c,4,BIT_MCUFWDL_RDY|BIT_WINTINI_RDY,BIT_MCUFWDL_RDY))||(e=reset_mcu(c)))goto fail;
 if((e=poll(c,REG_MCUFW_CTRL,4,FW_READY_LEGACY,FW_READY_LEGACY,10,20000)))goto fail;
 if((e=rtw_r12au_read(c,REG_MCUFW_CTRL,4,&status)))goto fail;report->last_mcu_fwdl=status;report->firmware_ready=1;
 c->firmware_running=1;c->firmware_version=fw.version;c->firmware_subversion=fw.subversion;return 0;
fail:
 (void)download_disable(c);c->firmware_running=0;c->mac_initialized=0;c->firmware_version=0;c->firmware_subversion=0;return e;
}
int rtw_r12au_h2c(rtw_r12au_boot*c,const uint8_t command[8]){
 int e;if(!valid(c)||!command||!c->identified||!c->powered||!c->firmware_running||c->h2c_box>3)return EINVAL;if(c->io_error)return c->io_error;
 uint8_t box=c->h2c_box;if((e=poll(c,REG_HMETFR,1,BIT(box),0,31,100)))return e;
 if((e=rtw_r12au_write(c,(uint16_t)(0x1f0+box*4),4,le32(command+4)))||(e=rtw_r12au_write(c,(uint16_t)(0x1d0+box*4),4,le32(command))))return e;c->h2c_box=(uint8_t)((box+1)&3);return 0;
}
int rtw_r12au_power_off_checked(rtw_r12au_boot*c){
 if(!valid(c)||!c->identified)return EINVAL;int original=c->io_error,first=0,e;uint32_t fsm=0,status=0;c->io_error=0;c->powered=0;c->dma_prepared=0;c->firmware_running=0;c->mac_initialized=0;c->associated=0;c->h2c_box=0;c->firmware_version=0;c->firmware_subversion=0;c->cleanup=1;
 #define TRY(expr) do{e=(expr);if(e&&!first)first=e;}while(0)
 TRY(rtw_r12au_write(c,REG_TXPAUSE,1,0xff));TRY(rtw_r12au_read(c,REG_APS_FSMCO,2,&fsm));
 TRY(rtw_r12au_update(c,REG_APS_FSMCO,2,APS_FSMCO_HW_POWERDOWN,0));
 if(!c->board.phy.btcoex)TRY(rtw_r12au_update(c,REG_GPIO_MUXCFG,2,BIT_EN_SIC,0));
 if(c->usb.speed==RTW_R12AU_USB_SUPER)TRY(rtw_r12au_update(c,REG_USB_MOD,1,BIT(4)|BIT(3),BIT(4)|BIT(3)));
 TRY(rtw_r12au_write(c,REG_HISR0,4,UINT32_MAX));TRY(rtw_r12au_write(c,REG_HISR1,4,UINT32_MAX));TRY(rtw_r12au_write(c,REG_HIMR0,4,0));TRY(rtw_r12au_write(c,REG_HIMR1,4,0));
 TRY(rtw_r12au_update(c,REG_TX_RPT_CTRL,1,BIT(1),0));TRY(rtw_r12au_write(c,REG_CR,1,0));
 if(c->chip==RTW_R12AU_CHIP_8812AU)TRY(power_flow(c,au_8812a_lps,ARRAY_SIZE(au_8812a_lps)));else TRY(power_flow(c,au_8821a_lps,ARRAY_SIZE(au_8821a_lps)));
 TRY(rtw_r12au_read(c,REG_MCUFW_CTRL,1,&status));if(status&BIT_RAM_DL_SEL)TRY(reset_mcu(c));
 TRY(rtw_r12au_update(c,REG_SYS_FUNC_EN+1,1,BIT(2),0));TRY(rtw_r12au_write(c,REG_MCUFW_CTRL,1,0));
 if(c->chip==RTW_R12AU_CHIP_8812AU)TRY(power_flow(c,au_8812a_disable,ARRAY_SIZE(au_8812a_disable)));else TRY(power_flow(c,au_8821a_disable,ARRAY_SIZE(au_8821a_disable)));
 if(fsm&APS_FSMCO_HW_POWERDOWN)TRY(rtw_r12au_update(c,REG_APS_FSMCO,2,APS_FSMCO_HW_POWERDOWN,APS_FSMCO_HW_POWERDOWN));
 #undef TRY
 c->cleanup=0;if(original)c->io_error=original;if(!c->cleanup_error)c->cleanup_error=first;return c->cleanup_error;
}
int rtw_r12au_power_off(rtw_r12au_boot*c){int original=c?c->io_error:0;int cleanup=rtw_r12au_power_off_checked(c);return original?original:cleanup;}

