/* SPDX-License-Identifier: BSD-3-Clause */
/* RTL8812AU bootstrap, derived from Linux rtw88 rtw88xxa.c/rtw8812a_table.c
 * (GPL-2.0 OR BSD-3-Clause), Copyright(c)2024 Realtek Corporation.
 * See RTL8812AUBoot-Upstream.json and RTL8812AUBoot-LICENSE.txt.
 * No Linux execution context, dynamic allocation, RF or station code here. */
#include "rtw/RTL8812AUBoot.h"
#include "rtw/Efuse.h"
#include <string.h>
#ifdef KERNEL
#include <sys/errno.h>
#else
#include <errno.h>
#endif
static int overlaps(const void *a,size_t an,const void *b,size_t bn){
    uintptr_t x,y;if(!a||!b||!an||!bn)return 0;x=(uintptr_t)a;y=(uintptr_t)b;
    return x<=y?y-x<an:x-y<bn;
}
static uint16_t le16(const uint8_t *p){return (uint16_t)((unsigned)p[0]|((unsigned)p[1]<<8));}
static uint32_t le32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int bus_valid(const rtw_8812au_boot *c){return c&&c->bus.control_in&&c->bus.control_out&&c->waiter.wait_us;}
static int read_reg(rtw_8812au_boot *c,uint16_t reg,uint8_t width,uint32_t *out){
    if(c->io_error)return c->io_error;
    if(rtw_usb_rw_read(&c->bus,reg,width,out)!=RTW_USB_OK)c->io_error=EIO;
    return c->io_error;
}
static int write_reg(rtw_8812au_boot *c,uint16_t reg,uint8_t width,uint32_t value){
    if(c->io_error)return c->io_error;
    if(rtw_usb_write_register(&c->bus,reg,width,value)!=RTW_USB_OK)c->io_error=EIO;
    return c->io_error;
}
static int update(rtw_8812au_boot *c,uint16_t reg,uint8_t width,uint32_t mask,uint32_t value){
    uint32_t old=0;int e=read_reg(c,reg,width,&old);
    return e?e:write_reg(c,reg,width,(old&~mask)|(value&mask));
}
static int poll(rtw_8812au_boot *c,uint16_t reg,uint32_t mask,uint32_t expected,unsigned attempts,unsigned delay){
    unsigned i;uint32_t value=0;int e;
    for(i=0;i<attempts;i++){
        e=read_reg(c,reg,1,&value);if(e)return e;
        if((value&mask)==expected)return 0;
        if(i+1<attempts&&!c->waiter.wait_us(c->waiter.context,delay))return EIO;
    }
    return ETIMEDOUT;
}
void rtw_8812au_boot_init(rtw_8812au_boot *c,const rtw_usb_rw_transport *bus,const rtw_waiter *waiter){
    rtw_usb_rw_transport saved_bus={NULL,NULL,NULL};rtw_waiter saved_waiter={NULL,NULL};
    if(!c)return;
    if(bus)saved_bus=*bus;if(waiter)saved_waiter=*waiter;
    memset(c,0,sizeof(*c));c->bus=saved_bus;c->waiter=saved_waiter;
}
int rtw_8812au_parse_board(uint32_t cfg,const uint8_t map[512],const uint8_t hints[4],rtw_8812au_board *out){
    rtw_8812au_board b;unsigned i,nonzero=0,antenna=0,wmode=0;uint8_t rfe;
    if(out&&(overlaps(out,sizeof(*out),map,512)||overlaps(out,sizeof(*out),hints,4)))return EINVAL;
    if(out)memset(out,0,sizeof(*out));
    if(!map||!hints||!out||!cfg||cfg==UINT32_MAX||(cfg&(UINT32_C(1)<<23))||
       !(cfg&(UINT32_C(1)<<26))||le16(map)!=0x8129)return EINVAL;
    if(map[0xd7]&1)return EINVAL;
    for(i=0;i<6;i++)nonzero|=map[0xd7+i];
    if(!nonzero)return EINVAL;
    memset(&b,0,sizeof(b));b.sys_cfg=cfg;b.raw_cut=(uint8_t)((cfg>>12)&15);
    /* RTL8812A logical silicon cuts are one above SYS_CFG1's raw encoding. */
    b.cut=(uint8_t)(b.raw_cut+1);memcpy(b.mac,map+0xd7,6);
    b.eeprom_vendor=le16(map+0xd0);b.eeprom_product=le16(map+0xd2);
    b.channel_plan=map[0xb8];b.crystal_cap=map[0xb9]==0xff?0x20:map[0xb9];
    b.thermal_meter=map[0xba];b.thermal_valid=(uint8_t)(b.thermal_meter>0&&b.thermal_meter<63);
    b.pa_type=map[0xbc];b.lna_type_2g=map[0xbd];b.lna_type_5g=map[0xbf];
    b.ext_pa_2g=(uint8_t)((b.pa_type&0x30)==0x30);b.ext_pa_5g=(uint8_t)((b.pa_type&3)==3);
    b.ext_lna_2g=(uint8_t)((b.lna_type_2g&0x88)==0x88);b.ext_lna_5g=(uint8_t)((b.lna_type_5g&0x88)==0x88);
    if(b.ext_pa_2g)b.gpa_type=(uint8_t)(((b.lna_type_2g>>6)&1)*4+((b.lna_type_2g>>2)&1));
    if(b.ext_pa_5g)b.apa_type=(uint8_t)(((b.lna_type_5g>>6)&1)*4+((b.lna_type_5g>>2)&1));
    if(b.ext_lna_2g)b.glna_type=(uint8_t)(((b.lna_type_2g>>4)&3)*4+(b.lna_type_2g&3));
    if(b.ext_lna_5g)b.alna_type=(uint8_t)(((b.lna_type_5g>>4)&3)*4+(b.lna_type_5g&3));
    rfe=map[0xca];
    if(rfe==0xff)b.rfe_option=0;
    else if(rfe&0x80)b.rfe_option=(uint8_t)(b.ext_lna_5g?
        (b.ext_pa_5g?(b.ext_lna_2g&&b.ext_pa_2g?3:0):2):4);
    else {
        b.rfe_option=rfe&0x3f;
        if(b.rfe_option==4&&(b.ext_pa_2g||b.ext_pa_5g||b.ext_lna_2g||b.ext_lna_5g))b.rfe_option=0;
    }
    b.rf_board_option=map[0xc1];b.regd=b.rf_board_option&7;b.bt_setting=map[0xc3];
    b.btcoex=(uint8_t)((b.rf_board_option&0xe0)==0x20);b.share_ant=b.bt_setting&1;
    b.tx_bb_swing_2g=map[0xc6];b.tx_bb_swing_5g=map[0xc7];memcpy(b.country_code,map+0xcb,2);
    memcpy(b.txpwr_idx_table[0],map+0x10,42);memcpy(b.txpwr_idx_table[1],map+0x3a,42);
    /* Upstream probes1019,1018 and1021,1020 in exactly this order. */
    for(i=0;i<2;i++){uint8_t v=hints[1-i];antenna=(v>>5)&7;if(!antenna)antenna=(v>>1)&7;if(antenna)break;}
    for(i=0;i<2;i++){wmode=(hints[3-i]>>2)&3;if(wmode)break;}
    b.tx_streams=b.rx_streams=(uint8_t)(antenna==1?1:2);b.vht=(uint8_t)!(antenna==2&&wmode==2);
    *out=b;return 0;
}
static int otp_byte(rtw_8812au_boot *c,unsigned address,uint8_t *out){
    unsigned i;uint32_t v=0;int e;
    if(address>=1024)return EINVAL;
    e=write_reg(c,0x31,1,address&255);if(e)return e;
    e=update(c,0x32,1,3,address>>8);if(e)return e;
    e=update(c,0x33,1,0x80,0);if(e)return e;
    for(i=0;i<100;i++){
        e=read_reg(c,0x30,4,&v);if(e)return e;
        if(v&UINT32_C(0x80000000)){*out=(uint8_t)v;return 0;}
        if(i+1<100&&!c->waiter.wait_us(c->waiter.context,1))return EIO;
    }
    return ETIMEDOUT;
}
int rtw_8812au_identify_read_board(rtw_8812au_boot *c){
    uint8_t physical[512],logical[512],hints[4];uint32_t cfg=0;unsigned i;int e,closed;
    if(!bus_valid(c))return EINVAL;
    if(c->powered||c->fw_running)return EBUSY;
    c->identified=0;memset(&c->board,0,sizeof(c->board));
    e=read_reg(c,0xf0,4,&cfg);if(e)return e;
    if(!cfg||cfg==UINT32_MAX||(cfg&(UINT32_C(1)<<23))||!(cfg&(UINT32_C(1)<<26)))return EINVAL;
    e=write_reg(c,0xcf,1,0x69);
    if(!e)e=update(c,2,2,0x1000,0x1000);
    if(!e)e=update(c,8,2,0x22,0x22);
    for(i=0;!e&&i<512;i++)e=otp_byte(c,i,&physical[i]);
    for(i=0;!e&&i<4;i++)e=otp_byte(c,1018+i,&hints[i]);
    /* Always attempt gate closure, including an ambiguous failed USB write. */
    closed=rtw_usb_write_register(&c->bus,0xcf,1,0)==RTW_USB_OK;
    /* A failed closing transfer may leave the gate open. Retry once while
     * retaining the first failure and keeping identity/readiness invalid. */
    if(!closed)(void)rtw_usb_write_register(&c->bus,0xcf,1,0);
    if(!e&&!closed){c->io_error=EIO;e=EIO;}
    if(e)return e;
    if(rtw_efuse_decode(physical,sizeof(physical),logical,sizeof(logical),NULL)!=RTW_EFUSE_OK)return EBADMSG;
    e=rtw_8812au_parse_board(cfg,logical,hints,&c->board);
    if(!e)c->identified=1;
    return e;
}
/* USB-selected commands from card_enable_flow_8812a. All source commands
 * use RTW_PWR_CUT_ALL_MSK; PCI-only steps are intentionally not imported. */
static const rtw_power_step power_flow[]={
    {0x12,RTW_PWR_WRITE8,1,1},{0x14,RTW_PWR_WRITE8,0x80,0},
    {0x15,RTW_PWR_WRITE8,1,0},{0x23,RTW_PWR_WRITE8,0x10,0},
    {0x46,RTW_PWR_WRITE8,0xff,0},{0x43,RTW_PWR_WRITE8,0xff,0},
    {5,RTW_PWR_WRITE8,8,0},{3,RTW_PWR_WRITE8,4,4},
    {0x24,RTW_PWR_WRITE8,2,2},{0x28,RTW_PWR_WRITE8,8,8},
    {5,RTW_PWR_WRITE8,4,0},{6,RTW_PWR_POLL8,2,2},
    {5,RTW_PWR_WRITE8,8,0},{5,RTW_PWR_WRITE8,1,1},
    {5,RTW_PWR_POLL8,1,0},{0x24,RTW_PWR_WRITE8,2,0},
    {0x28,RTW_PWR_WRITE8,8,0},{0,RTW_PWR_END,0,0}
};
int rtw_8812au_power_on(rtw_8812au_boot *c){
    uint32_t status=0;size_t executed=0;rtw_power_error p;int e;
    if(!bus_valid(c)||!c->identified)return EINVAL;
    if(c->io_error)return c->io_error;
    if(c->powered)return 0;
    e=read_reg(c,0x80,1,&status);if(e)return e;
    if(status&0x80)return EBUSY;
    /* Quiesce outgoing queues before any cold power transition. */
    e=write_reg(c,0x522,1,0x3f);if(e)return e;
    p=rtw_power_execute(&c->bus,power_flow,sizeof(power_flow)/sizeof(power_flow[0]),1000,&c->waiter,&executed);
    if(p!=RTW_POWER_OK){if(p==RTW_POWER_POLL_TIMEOUT)return ETIMEDOUT;c->io_error=EIO;return EIO;}
    e=write_reg(c,0x100,2,0);if(!e)e=write_reg(c,0x100,2,0x63f);
    if(!e)c->powered=1;
    return e;
}
int rtw_8812au_download_firmware(rtw_8812au_boot *c,const uint8_t *image,size_t size,rtw_boot_report *report){
    rtw_fw_image parsed;rtw_boot_options options={400,5000};rtw_boot_error e;
    if(report)memset(report,0,sizeof(*report));
    if(!bus_valid(c)||!c->identified||!c->powered||!report||
       rtw_fw_inspect_8812au(image,size,&parsed)!=RTW_FW_OK)return EINVAL;
    c->fw_running=0;c->h2c_box=0;
    if(c->io_error)return c->io_error;
    e=rtw_fw_boot_8812au(&c->bus,&c->waiter,image,size,&options,report);
    if(e!=RTW_BOOT_OK){if(e==RTW_BOOT_CHECKSUM_TIMEOUT||e==RTW_BOOT_INIT_TIMEOUT)return ETIMEDOUT;c->io_error=EIO;return EIO;}
    c->fw_running=1;return 0;
}
int rtw_8812au_h2c_mailbox(rtw_8812au_boot *c,const uint8_t command[8]){
    int e;uint8_t box;
    if(!bus_valid(c)||!c->identified||!c->powered||!c->fw_running||!command||c->h2c_box>3)return EINVAL;
    if(c->io_error)return c->io_error;
    box=c->h2c_box;e=poll(c,0x1cc,1U<<box,0,31,100);if(e)return e;
    e=write_reg(c,(uint16_t)(0x1f0+4*box),4,le32(command+4));
    if(!e)e=write_reg(c,(uint16_t)(0x1d0+4*box),4,le32(command));
    if(!e)c->h2c_box=(uint8_t)((box+1)&3);
    return e;
}
