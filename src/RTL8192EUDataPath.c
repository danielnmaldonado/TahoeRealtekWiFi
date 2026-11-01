/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/RTL8192EUDataPath.h"
#include <string.h>
uint64_t rtw_92e_beacon_timeout_ms(uint16_t interval_tu) {
    uint64_t timeout=((uint64_t)interval_tu*1024U*10U+999U)/1000U;
    return timeout<2000U?2000U:timeout>10000U?10000U:timeout;
}
int rtw_92e_beacon_expired(uint64_t now_ms,uint64_t last_ms,uint16_t interval_tu) {
    return now_ms<last_ms || now_ms-last_ms>=rtw_92e_beacon_timeout_ms(interval_tu);
}
void rtw_transfer_reset(rtw_transfer_ledger* p) { if(p) memset(p,0,sizeof(*p)); }
int rtw_transfer_begin(rtw_transfer_ledger* p,unsigned n) {
    uint32_t bit;
    if(!p || n>=RTW_TRANSFER_SLOTS || p->stopping ||
       p->order_count>=RTW_TRANSFER_SLOTS) return 0;
    bit=(uint32_t)1U<<n;
    if(p->pending&bit) return 0;
    p->order[p->order_count++]=(uint8_t)n;
    p->pending|=bit; return 1;
}
int rtw_transfer_complete(rtw_transfer_ledger* p,unsigned n) {
    uint32_t bit;
    if(!p || n>=RTW_TRANSFER_SLOTS) return 0;
    bit=(uint32_t)1U<<n;
    if(!(p->pending&bit) || (p->completed&bit)) return 0;
    p->completed|=bit; return 1;
}
int rtw_transfer_take(rtw_transfer_ledger* p,unsigned n) {
    uint32_t bit;unsigned i;
    if(!p || n>=RTW_TRANSFER_SLOTS || p->order_count>RTW_TRANSFER_SLOTS) return 0;
    bit=(uint32_t)1U<<n;
    if(!(p->completed&bit)) return 0;
    for(i=0;i<p->order_count;i++) if(p->order[i]==n) break;
    if(i==p->order_count) return 0;
    memmove(p->order+i,p->order+i+1,(size_t)p->order_count-i-1U);
    p->order[--p->order_count]=0;
    p->completed&=~bit; p->pending&=~bit; return 1;
}
static unsigned oldest_slot(const rtw_transfer_ledger* p,uint32_t mask) {
    unsigned i,n;
    if(!p || p->order_count>RTW_TRANSFER_SLOTS) return RTW_TRANSFER_SLOTS;
    for(i=0;i<p->order_count;i++) {
        n=p->order[i];
        if(n<RTW_TRANSFER_SLOTS && (mask&((uint32_t)1U<<n))) return n;
    }
    return RTW_TRANSFER_SLOTS;
}
int rtw_transfer_oldest_ready(const rtw_transfer_ledger* p,uint32_t mask) {
    unsigned n=oldest_slot(p,mask);
    return n<RTW_TRANSFER_SLOTS && (p->completed&((uint32_t)1U<<n))!=0;
}
int rtw_transfer_take_oldest(rtw_transfer_ledger* p,uint32_t mask,unsigned* slot) {
    unsigned n=oldest_slot(p,mask);
    if(!slot || n>=RTW_TRANSFER_SLOTS || !rtw_transfer_take(p,n)) return 0;
    *slot=n;return 1;
}
void rtw_transfer_stop(rtw_transfer_ledger* p) {if(p) p->stopping=1;}
int rtw_transfer_restart(rtw_transfer_ledger* p) {
    if(!p || p->pending || p->completed || p->order_count) return 0;
    p->stopping=0; return 1;
}
int rtw_transfer_drained(const rtw_transfer_ledger* p) {return p && !p->pending && !p->completed && !p->order_count;}
/* Generation-2 fields cross-checked with OpenBSD r92creg.h and Linux
 * rtl8xxxu TXDESC40_SEQ_SHIFT=12 (DW9). No hardware crypto or HWSEQ. */
static void put32(uint8_t* p,uint32_t n) {
    p[0]=(uint8_t)n; p[1]=(uint8_t)(n>>8);p[2]=(uint8_t)(n>>16);p[3]=(uint8_t)(n>>24);
}
int rtw_92e_make_tx_descriptor(size_t n,int data,int multicast,int b_only,
    uint8_t rate,uint16_t seq,uint8_t* out) {
    uint16_t sum=0; unsigned i;
    if(!out || n<24U || n>4096U || rate>11U || seq>4095U ||
       (data!=0 && data!=1) || (multicast!=0 && multicast!=1) ||
       (b_only!=0 && b_only!=1) || (data && multicast)) return 0;
    memset(out,0,RTW_92E_TX_DESCRIPTOR_BYTES);
    put32(out,0x8c280000U|(uint32_t)n|(multicast?0x01000000U:0U));
    put32(out+4,data?((b_only?8U:6U)<<16):0x00081200U);
    put32(out+8,data?0x00010000U:0U); /* no aggregation */
    put32(out+12,0x00000100U); /* explicit legacy rate */
    put32(out+16,0x00001f00U|(data?((uint32_t)rate|0x08000000U):0U));
    put32(out+36,(uint32_t)seq<<12);
    for(i=0;i<32U;i+=2U) sum^=(uint16_t)((unsigned)out[i]|((unsigned)out[i+1]<<8));
    out[28]=(uint8_t)sum;out[29]=(uint8_t)(sum>>8);
    return 1;
}

int rtw_92e_make_tx_descriptor_ex(size_t n,const rtw_92e_tx_options* o,uint8_t* out) {
    uint16_t sum=0;unsigned i;uint32_t dw2=0,dw3=0,dw4=0,dw5=0;
    if(!o || !out || n<24U || n>4096U || o->rate>27U || o->sequence>4095U ||
       o->data>1U || o->multicast>1U || o->firmware_rate>1U || o->ampdu>1U ||
       o->short_preamble>1U || o->bw40>1U || o->primary_above>1U ||
       o->channel_bw40>1U || o->short_gi>1U ||
       o->raid>8U || o->density>7U || o->max_agg>31U ||
       (o->data && o->multicast) || (o->data?o->qsel>7U:o->qsel!=0x12U) ||
       (!o->firmware_rate && o->rate>11U) ||
       (o->firmware_rate && (!o->data || o->raid<2U || o->raid>5U)) ||
       (o->ampdu && (!o->firmware_rate || !o->max_agg)) ||
       (o->bw40 && (!o->firmware_rate || !o->data)) ||
       (o->short_gi && (!o->firmware_rate || o->rate<12U || o->short_preamble)) ||
       (o->short_preamble && (o->rate<1U || o->rate>3U)) ||
       (!o->bw40 && !o->channel_bw40 && o->primary_above)) return 0;
    if(o->data)dw2=o->ampdu?(0x1000U|((uint32_t)o->density<<20)):0x10000U;
    dw3=o->firmware_rate?0U:0x100U;
    if(o->ampdu)dw3|=0x3000U|((uint32_t)o->max_agg<<17);
    dw4=(uint32_t)o->rate|(o->data?0x08001f00U:0x001a0000U);
    /* Pinned Realtek rtl8192e_xmit.h DATA_BW/DATA_SC/DATA_SHORT setters:
     * the gen1 DW4 bandwidth bits alias gen2 RTS rate and retry fields.
     * SCMapping_92E uses no subchannel for full HT40 and upper=1/lower=2
     * for 20MHz packets on a 40MHz radio. */
    if(o->bw40)dw5=0x20U;
    else if(o->channel_bw40)dw5=o->primary_above?1U:2U;
    if(o->short_preamble || o->short_gi)dw5|=0x10U;
    memset(out,0,RTW_92E_TX_DESCRIPTOR_BYTES);
    put32(out,0x8c280000U|(uint32_t)n|(o->multicast?0x01000000U:0U));
    put32(out+4,((uint32_t)o->raid<<16)|((uint32_t)o->qsel<<8));
    put32(out+8,dw2);put32(out+12,dw3);put32(out+16,dw4);
    put32(out+20,dw5);
    put32(out+36,(uint32_t)o->sequence<<12);
    for(i=0;i<32U;i+=2U)sum^=(uint16_t)((unsigned)out[i]|((unsigned)out[i+1]<<8));
    out[28]=(uint8_t)sum;out[29]=(uint8_t)(sum>>8);return 1;
}

int rtw_92e_rx_frame_allowed(const uint8_t* b,size_t n,int allow_bar,
    const uint8_t* local,const uint8_t* peer) {
    if(!b || n<2U || n>4096U || (b[0]&3U))return 0;
    if((b[0]&0x0cU)!=4U)return n>=24U;
    return (b[0]&0xfcU)==0x84U && n>=20U && allow_bar==1 &&
        !(b[1]&0x43U) && local && peer && !memcmp(b+4,local,6) && !memcmp(b+10,peer,6);
}

int rtw_92e_tx_protection(unsigned mode,uint8_t* d) {
    uint32_t word;uint16_t sum=0;unsigned i;
    if(!d || mode>2U) return 0;
    word=(uint32_t)d[12]|((uint32_t)d[13]<<8)|((uint32_t)d[14]<<16)|((uint32_t)d[15]<<24);
    word&=~0x00003800U;
    /* Gen2 Linux/Realtek select CTS-to-self without the HW RTS engine. */
    if(mode) word|=mode==1U?0x00003000U:0x00000800U;
    put32(d+12,word);
    d[28]=d[29]=0;
    for(i=0;i<32U;i+=2U) sum^=(uint16_t)((unsigned)d[i]|((unsigned)d[i+1]<<8));
    d[28]=(uint8_t)sum;d[29]=(uint8_t)(sum>>8);return 1;
}

int rtw_92e_parse_tx_report(const uint8_t* b,size_t n,rtw_92e_tx_report* r) {
    if(r) memset(r,0,sizeof(*r));
    /* Vendor rtl8192e_cmd.h / OpenBSD r92e_c2h_tx_rpt. The two
     * reserved payload bytes are not interpreted as a software tag. */
    if(!b || !r || n!=10U || b[0]!=3U || (b[2]&0x20U)!=0U || (b[2]&0x1fU)>7U ||
       b[3]!=0U || b[7]>27U) return 0;
    r->event_sequence=b[1];r->retries=b[4]&0x3fU;r->final_rate=b[7];
    r->lifetime_expired=(b[2]>>6)&1U;r->retry_over=(b[2]>>7)&1U;
    r->queue_time=(uint16_t)((uint16_t)b[5]|((uint16_t)b[6]<<8));
    return 1;
}
int rtw_92e_tx_report_request(int requested,uint8_t* d) {
    uint16_t sum=0;unsigned i;
    if(!d || (requested!=0 && requested!=1)) return 0;
    if(requested && ((d[3]&1U)!=0U || (d[4]&0x7fU)!=0U ||
                     (d[5]&0x1fU)>7U)) return 0;
    d[10]=(uint8_t)((d[10]&~0x08U)|(requested?0x08U:0U));
    d[28]=d[29]=0;
    for(i=0;i<32U;i+=2U) sum^=(uint16_t)((unsigned)d[i]|((unsigned)d[i+1]<<8));
    d[28]=(uint8_t)sum;d[29]=(uint8_t)(sum>>8);return 1;
}
