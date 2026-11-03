/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
 * Copyright(c)2018-2019 Realtek Corporation.
 * Portable adaptation of rtw88/mac.c's3081 USB firmware flow, not a radio HAL.
 * All bus results are checked; polling and scratch writes are bounded.
 * Immutable references: RTW88USBDataPath-Upstream.json. */
#include "rtw/RTW88Firmware.h"
#include "rtw/RTW88USBDataPath.h"
#ifdef KERNEL
#include <sys/errno.h>
#else
#include <errno.h>
#endif
#include <string.h>
#define BIT(n) (UINT32_C(1)<<(n))
#define FWCTRL 0x80U
#define DMACTRL 0x1208U
static uint16_t le16(const uint8_t*b){return (uint16_t)((uint16_t)b[0]|((uint16_t)b[1]<<8U));}
static uint32_t le32(const uint8_t*b){return (uint32_t)b[0]|((uint32_t)b[1]<<8U)|((uint32_t)b[2]<<16U)|((uint32_t)b[3]<<24U);}
static int overlap(const void*a,size_t an,const void*b,size_t bn){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return an&&bn&&(x<=y?y-x<an:x-y<bn);}
static int family(rtw_wifi_family f){return f==RTW_WIFI_RTL8821CU||f==RTW_WIFI_RTL8822BU||f==RTW_WIFI_RTL8822CU;}
static int region(uint32_t a,size_t n,uint32_t base,uint32_t end){return n&&a>=base&&a<end&&n<=end-a;}
int rtw88_fw_inspect(rtw_wifi_family f,const uint8_t*b,size_t n,rtw88_fw_image*out){
 rtw88_fw_image im;size_t d,i,e;uint32_t imem_base=f==RTW_WIFI_RTL8822BU?0:0x30000U;
 if(!out||!b||n<64U||!family(f)||overlap(out,sizeof(*out),b,n))return EINVAL;
 if(le16(b)!=(f==RTW_WIFI_RTL8821CU?0x8821U:0x8822U))return EINVAL;
 d=le32(b+36);i=le32(b+48);e=(b[24]&0x10U)?le32(b+52):0U;
 if(!d||!i||d>0x10000U-8U||i>0x40000U-imem_base-8U||e>0x20000U-8U)return EINVAL;
 d+=8U;i+=8U;if(e)e+=8U;
 if(d>n-64U||i>n-64U-d||e!=n-64U-d-i)return EINVAL;
 memset(&im,0,sizeof(im));im.signature=le16(b);im.version=le16(b+4);im.subversion=b[6];
 im.dmem=(rtw88_fw_segment){64U,d,le32(b+32)&0x7fffffffU};
 im.imem=(rtw88_fw_segment){64U+d,i,le32(b+60)&0x7fffffffU};
 im.emem=(rtw88_fw_segment){64U+d+i,e,le32(b+56)&0x7fffffffU};
 if(im.imem.address!=imem_base||!region(im.dmem.address,d,0x200000U,0x210000U)||!region(im.imem.address,i,imem_base,0x40000U)||
    (e&&!region(im.emem.address,e,0x100000U,0x120000U)))return EINVAL;
 *out=im;return 0;
}
static int error(rtw88_fw_context*c,int e){if(e<0)e=e==INT32_MIN?EIO:-e;if(e&&!c->last_error)c->last_error=e;return e;}
static int rd(rtw88_fw_context*c,uint16_t a,unsigned w,uint32_t*v){*v=0;return error(c,c->bus.read(c->bus.cookie,a,w,v));}
static int wr(rtw88_fw_context*c,uint16_t a,unsigned w,uint32_t v){return error(c,c->bus.write(c->bus.cookie,a,w,v));}
static int update(rtw88_fw_context*c,uint16_t a,unsigned w,uint32_t mask,uint32_t v){uint32_t old;int e=rd(c,a,w,&old);return e?e:wr(c,a,w,(old&~mask)|(v&mask));}
static int poll(rtw88_fw_context*c,uint16_t a,unsigned w,uint32_t mask,uint32_t expected){
 unsigned j;uint32_t v;int e;for(j=0;j<1000U;j++){e=rd(c,a,w,&v);if(e)return e;if((v&mask)==expected)return 0;if(j<999U)c->bus.delay_us(c->bus.cookie,10);}
 return error(c,ETIMEDOUT);
}
static int cpu(rtw88_fw_context*c,int enable){int e;if(enable){e=update(c,0x1d,1,1,1);return e?e:update(c,3,1,4,4);}e=update(c,3,1,4,0);return e?e:update(c,0x1d,1,1,0);}
#define TRY(x) do {if((e=(x))!=0)goto fail;}while(0)
static int reserved_page(rtw88_fw_context*c,const uint8_t*data,size_t n,uint8_t*scratch){
 uint32_t bcn,cr,page;int e=0,cleanup=0;size_t padding=((n+48U)&511U)==0U?1U:0U;
 if((e=rd(c,0x550,1,&bcn))||(e=rd(c,0x101,1,&cr))||(e=rd(c,0x204,2,&page)))return e;
 if(rtw88_usb_make_control_descriptor(c->family,n+padding,RTW88_USB_CONTROL_RESERVED,scratch,48)!=RTW88_USB_TX_OK)return error(c,EINVAL);
 memcpy(scratch+48,data,n);if(padding)scratch[48+n]=0;
 TRY(wr(c,0x204,2,BIT(15)));TRY(wr(c,0x101,1,cr|1U));TRY(wr(c,0x550,1,(bcn&~8U)|16U));
 TRY(error(c,c->bus.bulk_write(c->bus.cookie,16,scratch,48U+n+padding)));
 TRY(poll(c,0x204,2,BIT(15),BIT(15)));
fail:
 cleanup=wr(c,0x204,2,(page&0x7fffU)|BIT(15));if(!e)e=cleanup;
 cleanup=wr(c,0x550,1,bcn);if(!e)e=cleanup;
 cleanup=wr(c,0x101,1,cr);if(!e)e=cleanup;
 return e;
}
static int segment(rtw88_fw_context*c,const uint8_t*b,const rtw88_fw_segment*s,uint8_t*scratch){
 size_t off=0,n;uint32_t v,mask,done;int e;if(!s->length)return 0;
 if((e=poll(c,DMACTRL,4,BIT(31),0))||
    (e=update(c,DMACTRL,4,BIT(25),BIT(25))))return e;
 while(off<s->length){
  n=s->length-off;if(n>4096U)n=4096U;
  if((e=reserved_page(c,b+s->offset+off,n,scratch))||
     (e=poll(c,DMACTRL,4,BIT(31),0))||
     (e=wr(c,0x1200,4,0x18780000U+48U))||
     (e=wr(c,0x1204,4,s->address+(uint32_t)off))||
     (e=wr(c,DMACTRL,4,BIT(29)|BIT(31)|(off?BIT(24):0U)|(uint32_t)n))||
     (e=poll(c,DMACTRL,4,BIT(31),0)))return e;
  off+=n;
 }
 if((e=rd(c,DMACTRL,4,&v)))return e;
 mask=s->address<0x200000U?BIT(3)|BIT(4):BIT(5)|BIT(6);
 done=s->address<0x200000U?BIT(3):BIT(5);
 if((e=update(c,FWCTRL,1,mask,(v&BIT(27))?done:mask)))return e;
 return v&BIT(27)?error(c,EBADMSG):0;
}
struct backup {uint16_t address;unsigned width;uint32_t value;};
int rtw88_fw_download(rtw88_fw_context*c,const uint8_t*b,size_t n,uint8_t*scratch,size_t scratch_size){
 rtw88_fw_image im;uint32_t coex=0,fw=0,v=0;unsigned saved=0,j;int e=0,coex_saved=0,mutated=0;
 struct backup backup[]={
 {0x10d,1,0},{0x100,1,0},{0x1330,4,BIT(31)},{0x230,2,0},{0x22c,4,0},{0x550,1,0},
 {3,1,0},{0x1d,1,0},{0x1082,1,0},{9,1,0},{0x101,1,0},{0x204,2,0}};
 if(!c||!b||!scratch||scratch_size<RTW88_FW_SCRATCH_BYTES||!family(c->family)||
    overlap(c,sizeof(*c),b,n)||overlap(scratch,RTW88_FW_SCRATCH_BYTES,b,n)||
    overlap(c,sizeof(*c),scratch,RTW88_FW_SCRATCH_BYTES)||!c->bus.read||!c->bus.write||!c->bus.bulk_write||
    !c->bus.delay_us||!c->bus.coex_read||!c->bus.coex_write)return EINVAL;
 if(!c->identified||!c->powered)return ENXIO;
 e=rtw88_fw_inspect(c->family,b,n,&im);if(e)return e;
 c->last_error=0;c->firmware_running=0;
 TRY(error(c,c->bus.coex_read(c->bus.cookie,0x38,&coex)));coex_saved=1;
 for(j=0;j<sizeof(backup)/sizeof(backup[0]);j++){
  if(j!=2U)TRY(rd(c,backup[j].address,backup[j].width,&backup[j].value));saved=j+1U;
 }
 mutated=1;TRY(cpu(c,0));
 TRY(wr(c,0x10d,1,3U<<6U));TRY(wr(c,0x100,1,5));TRY(wr(c,0x1330,4,BIT(31)));
 TRY(wr(c,0x230,2,0x200));TRY(wr(c,0x22c,4,backup[4].value|BIT(31)));
 TRY(wr(c,0x550,1,(backup[5].value&~8U)|16U));
 TRY(update(c,0x1082,1,1,0));TRY(update(c,9,1,0x40,0));
 TRY(update(c,0x1082,1,1,1));TRY(update(c,9,1,0x40,0x40));
 TRY(rd(c,FWCTRL,2,&v));TRY(wr(c,FWCTRL,2,(v&0x3800U)|1U));
 TRY(segment(c,b,&im.dmem,scratch));TRY(segment(c,b,&im.imem,scratch));TRY(segment(c,b,&im.emem,scratch));
 for(j=0;j<6U;j++)TRY(wr(c,backup[j].address,backup[j].width,j==4U?backup[j].value|BIT(31):backup[j].value));
 TRY(wr(c,0x210,4,BIT(2)));TRY(rd(c,FWCTRL,2,&fw));if((fw&0x50U)!=0x50U){e=error(c,EBADMSG);goto fail;}
 TRY(wr(c,FWCTRL,2,(fw|BIT(14))&~1U));TRY(cpu(c,1));
 TRY(error(c,c->bus.coex_write(c->bus.cookie,0x38,coex)));
 TRY(poll(c,FWCTRL,2,0xcfffU,0xc078U));
 c->firmware_running=1;c->h2c_box=0;return 0;
fail:
 if(mutated){
  for(j=0;j<saved;j++)(void)wr(c,backup[j].address,backup[j].width,backup[j].value);
  if(!c->bus.read(c->bus.cookie,FWCTRL,1,&v))(void)wr(c,FWCTRL,1,v&~1U);
 }
 if(coex_saved)(void)error(c,c->bus.coex_write(c->bus.cookie,0x38,coex));
 return e?e:c->last_error?c->last_error:EIO;
}

/* Linuxfw.c rtw_fw_send_h2c_command_register; shared by RFK/coex/station. */
static int h2c(rtw88_fw_context*c,const uint8_t command[8],int advisory)
{
 unsigned i;uint32_t busy;int e;
 if(!c||!command||!family(c->family)||!c->bus.read||!c->bus.write||!c->bus.delay_us||
    c->h2c_box>3||overlap(c,sizeof(*c),command,8))return EINVAL;
 if(c->last_error)return c->last_error;
 if(c->identified!=1||c->powered!=1||c->firmware_running!=1)return ENXIO;
 for(i=0;i<=30;i++){
  if((e=rd(c,0x1cc,1,&busy)))return e;
  if(!(busy&(1U<<c->h2c_box)))break;
  if(i<30)c->bus.delay_us(c->bus.cookie,100);
 }
 if(i>30)return advisory?EBUSY:error(c,ETIMEDOUT);
 if((e=wr(c,(uint16_t)(0x1f0+4*c->h2c_box),4,le32(command+4))))return e;
 if((e=wr(c,(uint16_t)(0x1d0+4*c->h2c_box),4,le32(command))))return e;
 c->h2c_box=(uint8_t)((c->h2c_box+1U)&3U);return 0;
}
int rtw88_fw_h2c(rtw88_fw_context*c,const uint8_t command[8]){return h2c(c,command,0);}
int rtw88_fw_h2c_rssi_monitor(rtw88_fw_context*c,const uint8_t command[8])
{
 if(!command||command[0]!=0x42||command[1]||command[2]||command[3]>100||command[4]||command[5]||command[6]||command[7])return EINVAL;
 return h2c(c,command,1);
}
