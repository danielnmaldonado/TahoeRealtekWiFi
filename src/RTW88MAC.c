/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
 * Copyright(c)2018-2019 Realtek Corporation.
 * Portable rtw88/mac.c,usb.c FIFO/DMA/H2C/MAC setup for USB8821C/8822B/8822C.
 * No RF register or PHY table is written by this module. */
#include "rtw/RTW88MAC.h"
#ifdef KERNEL
#include <sys/errno.h>
#else
#include <errno.h>
#endif
#include <string.h>
struct rtw88_mac_op{uint16_t address;uint8_t width;uint32_t mask,value;};
#include "RTW88MACChipTables.inc"
static int known(rtw_wifi_family f){return f==RTW_WIFI_RTL8821CU||f==RTW_WIFI_RTL8822BU||f==RTW_WIFI_RTL8822CU;}
int rtw88_mac_prepare(rtw_wifi_family f,unsigned pipes,rtw88_mac_plan*out){
 rtw88_mac_plan p;uint8_t rq[6];unsigned i,cur,reserved;
 if(!out||!known(f)||pipes<2U||pipes>4U)return EINVAL;
 memset(&p,0,sizeof(p));p.family=f;p.bulk_out_count=(uint8_t)pipes;
 p.tx_pages=(uint16_t)(f==RTW_WIFI_RTL8821CU?512:2048);p.rx_fifo_bytes=f==RTW_WIFI_RTL8821CU?16384U:24576U;
 p.driver_pages=(uint16_t)(f==RTW_WIFI_RTL8822CU?16:8);p.csi_pages=(uint16_t)(f==RTW_WIFI_RTL8822CU?50:0);
 reserved=(unsigned)p.driver_pages+44U+p.csi_pages;p.reserved_pages=(uint16_t)reserved;
 p.reserved_boundary=(uint16_t)((unsigned)p.tx_pages-reserved);
 cur=(unsigned)p.tx_pages-p.csi_pages-4U;p.firmware_tx_page=(uint16_t)cur;
 cur-=8U;p.h2c_page=(uint16_t)cur;cur-=8U;p.h2c_static_page=(uint16_t)cur;
 cur-=24U;p.h2c_info_page=(uint16_t)cur;
 p.high_pages=(uint16_t)(f==RTW_WIFI_RTL8821CU?16:64);p.low_pages=p.high_pages;
 p.normal_pages=(uint16_t)(pipes>=3U?p.high_pages:0U);
 p.extra_pages=(uint16_t)(pipes==4U?(f==RTW_WIFI_RTL8821CU?14:64):0);
 p.public_pages=(uint16_t)(p.reserved_boundary-p.high_pages-p.low_pages-p.normal_pages-p.extra_pages-1U);
 rq[0]=2;rq[1]=2;rq[2]=(uint8_t)(pipes==2U?2:1);rq[3]=(uint8_t)(pipes==2U?3:1);rq[4]=(uint8_t)(pipes==4U?0:3);rq[5]=3;
 for(i=0;i<6U;i++)p.queue_mapping|=(uint16_t)((unsigned)rq[i]<<(4U+2U*i));
 memset(p.queue_endpoint,255,sizeof(p.queue_endpoint));
 p.queue_endpoint[0]=(uint8_t)(3U-rq[2]);p.queue_endpoint[1]=(uint8_t)(3U-rq[3]);
 p.queue_endpoint[2]=p.queue_endpoint[1];p.queue_endpoint[3]=p.queue_endpoint[0];
 p.queue_endpoint[4]=(uint8_t)(3U-rq[1]);p.queue_endpoint[5]=p.queue_endpoint[4];
 p.queue_endpoint[6]=(uint8_t)(3U-rq[0]);p.queue_endpoint[7]=p.queue_endpoint[6];
 p.queue_endpoint[16]=0;p.queue_endpoint[17]=0;p.queue_endpoint[18]=(uint8_t)(3U-rq[4]);p.queue_endpoint[19]=0;
 *out=p;return 0;
}
int rtw88_mac_queue_endpoint(const rtw88_mac_plan*p,unsigned queue,uint8_t*out){
 if(!p||!out||!known(p->family)||p->bulk_out_count<2U||p->bulk_out_count>4U||queue>=20U||(queue>=8U&&queue<=15U)||p->queue_endpoint[queue]>=p->bulk_out_count)return EINVAL;
 *out=p->queue_endpoint[queue];return 0;
}
static int err(rtw88_fw_context*c,int e){if(e<0)e=e==INT32_MIN?EIO:-e;if(e&&!c->last_error)c->last_error=e;return e;}
static int rd(rtw88_fw_context*c,uint16_t a,unsigned w,uint32_t*v){*v=0;return err(c,c->bus.read(c->bus.cookie,a,w,v));}
static int wr(rtw88_fw_context*c,uint16_t a,unsigned w,uint32_t v){return err(c,c->bus.write(c->bus.cookie,a,w,v));}
static int update(rtw88_fw_context*c,uint16_t a,unsigned w,uint32_t mask,uint32_t value){uint32_t v;int e=rd(c,a,w,&v);return e?e:wr(c,a,w,(v&~mask)|(value&mask));}
static int poll_llt(rtw88_fw_context*c){unsigned i;uint32_t v;int e;for(i=0;i<1000U;i++){if((e=rd(c,0x208,1,&v)))return e;if(!(v&1U))return 0;if(i<999U)c->bus.delay_us(c->bus.cookie,10);}return err(c,ETIMEDOUT);}
#define TRY(x) do{if((e=(x))!=0)goto fail;}while(0)
int rtw88_mac_initialize(rtw88_mac_context*c,rtw88_fw_context*d,unsigned pipes,rtw88_usb_speed speed){
 rtw88_mac_plan p;const struct rtw88_mac_op*ops;size_t count,j;uint32_t head,tail,wp,rp;int e;
 if(!c)return EINVAL;c->mac_ready=0;
 if(!d||!d->bus.read||!d->bus.write||!d->bus.delay_us||speed<RTW88_USB_FULL_SPEED||speed>RTW88_USB_SUPER_SPEED)return EINVAL;
 if((e=rtw88_mac_prepare(d->family,pipes,&p)))return e;
 c->mac_ready=0;if(!d->identified||!d->powered||!d->firmware_running)return ENXIO;
 if(d->last_error)return d->last_error;
 c->device=d;c->fifo=p;
 /* TRX queue mapping,priority FIFO pages and LLT,from mac.c. */
 TRY(wr(d,0x10c,2,p.queue_mapping));TRY(wr(d,0x100,1,0));TRY(wr(d,0x100,1,0xff));
 TRY(wr(d,0x1330,4,0x80000000U));TRY(update(d,0x10c,1,1,1));
 TRY(wr(d,0x230,2,p.high_pages));TRY(wr(d,0x234,2,p.low_pages));TRY(wr(d,0x238,2,p.normal_pages));
 TRY(wr(d,0x23c,2,p.extra_pages));TRY(wr(d,0x240,2,p.public_pages));TRY(update(d,0x22c,4,0x80000000U,0x80000000U));
 TRY(wr(d,0x204,2,p.reserved_boundary));TRY(update(d,0x422,1,0x10,0x10));
 TRY(wr(d,0x424,2,p.reserved_boundary));TRY(wr(d,0x206,2,p.reserved_boundary));TRY(wr(d,0x456,2,p.reserved_boundary));
 TRY(wr(d,0x11c,4,p.rx_fifo_bytes-257U));TRY(update(d,0x208,1,0xf0,0x30));TRY(wr(d,0x20b,1,3));
 TRY(update(d,0x20d,1,2,2));TRY(update(d,0x208,1,1,1));TRY(poll_llt(d));TRY(wr(d,0x103,1,0));
 /* Reserved firmware H2C ring:24+8+8+4 pages,plus chip CSI allocation. */
 head=(uint32_t)p.h2c_page<<7U;tail=head+1024U;
 TRY(update(d,0x244,4,0x3ffff,head));TRY(update(d,0x24c,4,0x3ffff,head));TRY(update(d,0x248,4,0x3ffff,tail));
 TRY(update(d,0x254,1,3,1));TRY(update(d,0x254,1,4,4));TRY(update(d,0x20d,1,0x80,0x80));
 TRY(rd(d,0x10d4,4,&wp));TRY(rd(d,0x10d0,4,&rp));wp&=0x3ffff;rp&=0x3ffff;
 if(wp!=rp){e=err(d,EBUSY);goto fail;}
 if(d->family==RTW_WIFI_RTL8821CU){ops=mac_8821c;count=sizeof(mac_8821c)/sizeof(mac_8821c[0]);}
 else if(d->family==RTW_WIFI_RTL8822BU){ops=mac_8822b;count=sizeof(mac_8822b)/sizeof(mac_8822b[0]);}
 else{ops=mac_8822c;count=sizeof(mac_8822c)/sizeof(mac_8822c[0]);}
 for(j=0;j<count;j++){
  const struct rtw88_mac_op*op=ops+j;uint32_t full=op->width==4U?UINT32_MAX:op->width==2U?0xffffU:0xffU;
  if(op->mask==full)TRY(wr(d,op->address,op->width,op->value));else TRY(update(d,op->address,op->width,op->mask,op->value));
 }
 /* Driver-info and USB burst/aggregate setup,from mac.c and usb.c. */
 TRY(wr(d,0x60f,1,4));TRY(update(d,0x115,1,0x0f,0x0f));TRY(update(d,0x608,4,0x10000000,0x10000000));
 TRY(update(d,0x7d4,4,0x300,0));
 TRY(wr(d,0x290,1,speed==RTW88_USB_FULL_SPEED?0x2eU:speed==RTW88_USB_HIGH_SPEED?0x1eU:0x0eU));
 TRY(update(d,0x20c,2,0x200,0x200));TRY(update(d,0x10c,1,4,4));
 TRY(update(d,0x283,1,0x80,0));TRY(wr(d,0x280,2,0x2005));
 c->mac_ready=1;return 0;
fail:
 (void)wr(d,0x100,1,0);return e;
}
