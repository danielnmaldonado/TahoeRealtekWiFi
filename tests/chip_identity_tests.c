/* SPDX-License-Identifier: GPL-2.0-only. Host-only; no hardware access. */
#include "rtw/ChipIdentity.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdalign.h>
struct bus {uint32_t value;int error;unsigned reads,writes,delays,bulk;};
static rtw88_fw_bus *mutated_bus;static void *replacement_cookie;
static int read_reg(void*p,uint16_t a,unsigned w,uint32_t*v){struct bus*b=p;assert(a==0xfc&&w==1);++b->reads;*v=b->value;if(mutated_bus){mutated_bus->cookie=replacement_cookie;mutated_bus=NULL;}return b->error;}
static int other_read(void*p,uint16_t a,unsigned w,uint32_t*v){(void)p;(void)a;(void)w;(void)v;assert(0);return EIO;}
static int write_reg(void*p,uint16_t a,unsigned w,uint32_t v){struct bus*b=p;(void)a;(void)w;(void)v;++b->writes;assert(0);return EIO;}
static int bulk(void*p,uint8_t q,const uint8_t*b,size_t n){struct bus*s=p;(void)q;(void)b;(void)n;++s->bulk;assert(0);return EIO;}
static void delay(void*p,uint32_t n){struct bus*b=p;(void)n;++b->delays;assert(0);}
static rtw88_fw_bus operations(struct bus*b){return (rtw88_fw_bus){b,read_reg,write_reg,bulk,delay,NULL,NULL};}
int main(void){
 const rtw_usb_wifi_profile*origin=rtw_usb_wifi_lookup(0x13b1,0x0043,0,1,0);assert(origin&&!origin->primary_verified);
 rtw_chip_identity_descriptor d={0x13b1,0x0043,0x200,1,0,0xff,0xff,0xff,0};
 assert(rtw_chip_identity_candidate(origin,&d));
 assert(rtw_chip_identity_size()<=RTW_CHIP_IDENTITY_STORAGE_BYTES);
 unsigned positive=0;
 for(unsigned code=0;code<256;++code){alignas(max_align_t)uint8_t storage[RTW_CHIP_IDENTITY_STORAGE_BYTES]={0};struct bus b={code,0,0,0,0,0};rtw88_fw_bus ops=operations(&b);rtw_chip_identity*c=NULL;
  int e=rtw_chip_identity_init(storage,sizeof(storage),origin,&d,&ops,&c);
  assert(b.reads==1&&!b.writes&&!b.bulk&&!b.delays);
  if(code==0x0a||code==0x13){const rtw_usb_wifi_profile*p;assert(!e&&c);p=rtw_chip_identity_profile(c);assert(p&&p!=origin&&p->vendor==0x13b1&&p->product==0x43&&p->name==origin->name&&!p->primary_verified&&p->family==(code==0x0a?RTW_WIFI_RTL8822BU:RTW_WIFI_RTL8822CU));assert(rtw_chip_identity_valid(c,p,&ops));assert(rtw_usb_wifi_station_backend(p)==RTW_WIFI_UNKNOWN);assert(!rtw_wifi_radio_profile_capabilities(p).identity_verified);assert(!rtw_chip_identity_revalidate(c,&ops)&&b.reads==2);
   alignas(max_align_t)uint8_t clone[RTW_CHIP_IDENTITY_STORAGE_BYTES];memcpy(clone,storage,sizeof(clone));assert(!rtw_chip_identity_valid((rtw_chip_identity*)clone,p,&ops));
   struct bus foreign={code,0,0,0,0,0};rtw88_fw_bus other=operations(&foreign);assert(!rtw_chip_identity_valid(c,p,&other)&&!foreign.reads);other=ops;other.read=other_read;assert(!rtw_chip_identity_valid(c,p,&other));
   rtw_usb_wifi_profile copy=*p;assert(!rtw_chip_identity_valid(c,&copy,&ops));
   b.value=code==0x0a?0x13:0x0a;assert(rtw_chip_identity_revalidate(c,&ops)==ENODEV&&!rtw_chip_identity_profile(c));++positive;
  }else assert(e==ENODEV&&!c);
 }
 assert(positive==2);
 for(unsigned field=0;field<8;++field){rtw_chip_identity_descriptor bad=d;switch(field){case 0:bad.vendor^=1;break;case 1:bad.product^=1;break;case 2:bad.configuration=2;break;case 3:bad.interface_number=1;break;case 4:bad.interface_class=0;break;case 5:bad.interface_subclass=0;break;case 6:bad.interface_protocol=0;break;default:bad.vendor=0x0bda;bad.product=0x8192;break;}
  alignas(max_align_t)uint8_t storage[RTW_CHIP_IDENTITY_STORAGE_BYTES]={0};struct bus b={0x0a,0,0,0,0,0};rtw88_fw_bus ops=operations(&b);rtw_chip_identity*c=NULL;assert(!rtw_chip_identity_candidate(origin,&bad));assert(rtw_chip_identity_init(storage,sizeof(storage),origin,&bad,&ops,&c)!=0&&!c&&!b.reads&&!b.writes);
 }
 for(unsigned i=0;i<4;++i){alignas(max_align_t)uint8_t storage[RTW_CHIP_IDENTITY_STORAGE_BYTES]={0};struct bus b={0x0a,i&1?-EIO:EIO,0,0,0,0};rtw88_fw_bus ops=operations(&b);rtw_chip_identity*c=NULL;if(i<2){assert(rtw_chip_identity_init(storage,sizeof(storage),origin,&d,&ops,&c)==EIO&&!c);}else{b.error=0;assert(!rtw_chip_identity_init(storage,sizeof(storage),origin,&d,&ops,&c));b.error=i&1?-EIO:EIO;assert(rtw_chip_identity_revalidate(c,&ops)==EIO&&!rtw_chip_identity_profile(c));}assert(!b.writes);}
 for(unsigned stage=0;stage<2;++stage){alignas(max_align_t)uint8_t storage[RTW_CHIP_IDENTITY_STORAGE_BYTES]={0};struct bus b={0x0a,0,0,0,0,0},foreign={0x0a,0,0,0,0,0};rtw88_fw_bus ops=operations(&b);rtw_chip_identity*c=NULL;if(stage){assert(!rtw_chip_identity_init(storage,sizeof(storage),origin,&d,&ops,&c));}mutated_bus=&ops;replacement_cookie=&foreign;if(stage)assert(rtw_chip_identity_revalidate(c,&ops)==EINVAL&&!rtw_chip_identity_profile(c));else assert(rtw_chip_identity_init(storage,sizeof(storage),origin,&d,&ops,&c)==EINVAL&&!c);assert(!foreign.reads&&!b.writes);}
 puts("Chip identity: all256 codes, actual immutable conflict row, descriptor/cookie/clone/read-error/reread guards and unchanged metadata copy rejection passed");
}
