/* SPDX-License-Identifier: GPL-2.0-only
 * rtl8xxxu_fill_txdesc_v2/parse_rxdesc24, Linux pinned in RTL8723BU-Upstream.json.
 * Software encryption/sequence, HT20/1SS and no TX aggregation in this port. */
#include "rtw/RTL8723BUDataPath.h"
#ifdef KERNEL
#include <libkern/libkern.h>
#else
#include <string.h>
#endif
static uint32_t get32(const uint8_t*p){return(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static void put32(uint8_t*p,uint32_t value){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(value>>(8*i));}
int rtw_8723bu_make_tx_descriptor(size_t n,const rtw_8723bu_tx_options*o,uint8_t*out){
 uint8_t wire[40];uint16_t sum=0;uint32_t dw1,dw4;
 if(!o||!out||n<24||n>4096||o->sequence>4095||o->data>1||o->multicast>1||o->firmware_rate>1||o->short_preamble>1||o->rate>19||
    (o->data&&o->multicast)||(o->data?o->qsel>7:o->qsel!=0x12)||(!o->firmware_rate&&o->rate>11)||
    (o->firmware_rate&&(!o->data||(o->raid!=3&&o->raid!=5&&o->raid!=6&&o->raid!=7&&o->raid!=8)))||
    (o->raid>=6&&o->rate>=12)||o->raid>8)return 0;
 memset(wire,0,sizeof(wire));put32(wire,0x8c280000U|(uint32_t)n|(o->multicast?0x01000000U:0U));
 dw1=((uint32_t)o->raid<<16)|((uint32_t)o->qsel<<8)|(o->data?2U:0U);put32(wire+4,dw1);
 put32(wire+8,o->data?0x10000U:0U);put32(wire+12,o->firmware_rate?0U:0x100U);
 dw4=(uint32_t)o->rate|(o->data?0x08001f00U:0x001a0000U);put32(wire+16,dw4);
 put32(wire+20,o->short_preamble?0x10U:0U);put32(wire+36,(uint32_t)o->sequence<<12);
 for(unsigned i=0;i<32;i+=2)sum^=(uint16_t)((unsigned)wire[i]|((unsigned)wire[i+1]<<8));wire[28]=(uint8_t)sum;wire[29]=(uint8_t)(sum>>8);memcpy(out,wire,sizeof(wire));return 1;
}
rtw_packet_error rtw_8723bu_rx_next(rtw_rx_iterator*it,rtw_8192eu_rx_frame*out){
 uint32_t w0,w1,w2;const uint8_t*base;rtw_packet_error e;
 if(out)memset(out,0,sizeof(*out));if(!it||!out||(!it->buffer&&it->length)||it->cursor>it->length)return RTW_PACKETS_BAD_ARGUMENT;
 if(it->length-it->cursor<24)return rtw_rx_next(it,&out->payload);base=it->buffer+it->cursor;w0=get32(base);w1=get32(base+4);w2=get32(base+8);
 e=rtw_rx_next(it,&out->payload);if(e!=RTW_PACKETS_OK)return e;
 out->report_is_c2h=(uint8_t)((w2>>28)&1U);out->security_code=(uint8_t)((w0>>20)&7U);out->sw_decrypt_required=(uint8_t)((w0>>27)&1U);
 out->qos=(uint8_t)((w0>>23)&1U);out->sequence=(uint16_t)(w2&4095U);out->fragment_number=(uint8_t)((w2>>12)&15U);out->fragmented=(uint8_t)(((w1>>27)&1U)||out->fragment_number);
 if(!out->report_is_c2h){if(out->payload.mpdu_length<4)return RTW_PACKETS_MALFORMED;out->payload.mpdu_length-=4;}return RTW_PACKETS_OK;
}
