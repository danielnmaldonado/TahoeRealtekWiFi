/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright(c)2016-2019 Realtek Corporation. All rights reserved.
 * MaLd0n bounded byte-access adaptation of the pinned B14 HALMAC descriptor,
 * firmware header and OTP algorithms. Original definitions/notices are in
 * rtl8814b/halmac; immutable provenance: RTW8814BHAL-Upstream.json.
 */
#include "rtw/RTW8814BHAL.h"
#ifdef KERNEL
#include <sys/errno.h>
#else
#include <errno.h>
#endif
#include <string.h>
static uint16_t le16(const uint8_t*b){return (uint16_t)((uint16_t)b[0]|((uint16_t)b[1]<<8));}
static uint32_t le32(const uint8_t*b){return (uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);}
static void put32(uint8_t*b,uint32_t v){unsigned i;for(i=0;i<4;i++)b[i]=(uint8_t)(v>>(i*8));}
static int overlap(const void*a,size_t an,const void*b,size_t bn){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return an&&bn&&(x<=y?y-x<an:x-y<bn);}
int rtwb14_control_descriptor(unsigned kind,size_t n,uint8_t*out,size_t cap){
 uint8_t b[48]={0};unsigned i;uint16_t checksum=0;
 if(kind>1||!n||n>31744U||!out||cap<48)return EINVAL;
 /* Transfer format: body16, IE0..IE3, channel19=beacon,20=H2C. QSEL
  * beacon0x10/H2C0x13 and offset48 match the actual generation1 headers. */
 put32(b,(uint32_t)n|(48U<<16)|(4U<<24));
 put32(b+4,(kind?0x13U:0x10U)<<8);
 put32(b+12,(kind?20U:19U)<<11);
 for(i=0;i<4;i++)put32(b+16+i*8,(i<<24)|(i==3?UINT32_C(0x80000000):0U));
 for(i=0;i<48;i+=2)checksum^=le16(b+i);
 b[8]=(uint8_t)checksum;b[9]=(uint8_t)(checksum>>8);memcpy(out,b,48);return 0;
}
int rtwb14_descriptor_check(const uint8_t*b,size_t n){
 size_t count,len,j;uint32_t h;uint16_t sum=0;unsigned seen=0;
 if(!b||n<16)return EINVAL;h=le32(b);count=(h>>24)&31U;len=16+count*8;
 if(count>4||len>n||((h>>16)&255U)<len||((h>>16)&255U)>n)return EINVAL;
 if((!count)!=!!(h>>31))return EINVAL;
 for(j=0;j<count;j++){uint32_t ie=le32(b+16+j*8);unsigned id=(ie>>24)&15U;if(id>3||(seen&(1U<<id))||!!(ie>>31)!=(j+1==count))return EINVAL;seen|=1U<<id;}
 for(j=0;j<len;j+=2)sum^=le16(b+j);
 return sum?EINVAL:0;
}
int rtwb14_rx_inspect(const uint8_t*b,size_t n,rtwb14_rx_info*out){
 rtwb14_rx_info r;uint32_t w0,w2,w3;size_t info,shift;
 if(!b||!out||n<24||overlap(out,sizeof(*out),b,n))return EINVAL;
 w0=le32(b);w2=le32(b+8);w3=le32(b+12);info=((w0>>16)&15U)*8U;shift=(w0>>24)&3U;
 memset(&r,0,sizeof(r));r.payload_length=w0&0x3fffU;r.payload_offset=24+info+shift;
 if(!r.payload_length||r.payload_offset>n||r.payload_length>n-r.payload_offset)return EINVAL;
 r.record_length=r.payload_offset+r.payload_length;r.crc_error=(uint8_t)((w0>>14)&1);r.icv_error=(uint8_t)((w0>>15)&1);r.phy_status=(uint8_t)((w0>>26)&1);r.software_decrypted=(uint8_t)((w0>>27)&1);r.c2h=(uint8_t)((w2>>28)&1);r.rate=(uint8_t)(w3&127U);*out=r;return 0;
}
int rtwb14_firmware_inspect(const uint8_t*b,size_t n,uint16_t abi,rtwb14_fw_image*out){
 rtwb14_fw_image im;size_t d,i,e;uint32_t da,ia,ea;
 if(!b||!out||n<64||overlap(out,sizeof(*out),b,n)||(abi!=11&&abi!=15))return EINVAL;
 if(le16(b)!=0x8814U||le32(b+28)!=abi)return EINVAL;
 d=le32(b+36);i=le32(b+48);e=(b[24]&16U)?le32(b+52):0;
 da=le32(b+32)&0x7fffffffU;ia=le32(b+60)&0x7fffffffU;ea=le32(b+56)&0x7fffffffU;
 if(!d||!i||d>0x10000U-8U||i>0x20000U-8U||e>0x20000U-8U||da!=0x200000U||ia!=0x30000U||(e&&ea!=0x100000U))return EINVAL;
 d+=8;i+=8;if(e)e+=8;
 if(d>n-64||i>n-64-d||e!=n-64-d-i)return EINVAL;
 memset(&im,0,sizeof(im));im.signature=le16(b);im.version=le16(b+4);im.subversion=b[6];im.subindex=b[7];im.h2c_version=abi;
 im.dmem=(rtwb14_fw_segment){64,d,da};im.imem=(rtwb14_fw_segment){64+d,i,ia};im.emem=(rtwb14_fw_segment){64+d+i,e,ea};*out=im;return 0;
}
static int decode_otp(const uint8_t*phy,uint8_t*log){
 size_t p=0,limit=1024U-96U;unsigned block,en,k;uint8_t h,x;
 while(p<limit){h=phy[p++];if(h==255)return 0;
  if((h&31U)==15U){if(p>=limit)return EINVAL;x=phy[p++];if(x==255)return 0;block=((x&240U)>>1)|((h>>5)&7U);en=x&15U;}else{block=h>>4;en=h&15U;}
  for(k=0;k<4;k++)if(!(en&(1U<<k))){size_t off=(size_t)block*8+k*2;if(p>limit-2||off>1022)return EINVAL;if(log){log[off]=phy[p];log[off+1]=phy[p+1];}p+=2;}
 }
 return EINVAL;
}
int rtwb14_decode_otp(const uint8_t*phy,size_t n,uint8_t*log,size_t cap){int e;
 if(!phy||n!=1024||!log||cap<1024||overlap(phy,n,log,1024))return EINVAL;
 if((e=decode_otp(phy,NULL)))return e;
 memset(log,0xff,1024);return decode_otp(phy,log);
}
int rtwb14_control_packet(unsigned kind,const uint8_t*payload,size_t n,uint16_t mps,uint8_t*out,size_t cap,size_t*written){
 size_t pad,total;int e;
 if(!payload||!out||!written||!n||n>31744U||(mps!=512&&mps!=1024))return EINVAL;
 pad=((n+48U)%mps)==0U;total=48U+n+pad;
 if(cap<total||overlap(payload,n,out,total)||overlap(written,sizeof(*written),out,total)||overlap(written,sizeof(*written),payload,n))return EINVAL;
 if((e=rtwb14_control_descriptor(kind,n+pad,out,cap)))return e;
 memcpy(out+48,payload,n);if(pad)out[48+n]=0;*written=total;return 0;
}
