/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright(c) 2009-2012, 2024 Realtek Corporation.
 * DU-only wire fields from pinned Linux rtl8192d/trx_common.h/.c,
 * rtl8192du/trx.h/.c, rtlwifi/base.h, base.c and usb.c.
 * See RTL8192DUBoot-Upstream.json. No CU parser/stride alias is used. */
#include "rtw/RTL8192DUDataPath.h"
#include "rtw/FCS.h"
#include <string.h>
static uint32_t get32(const uint8_t *b) {
    return (uint32_t)b[0]|((uint32_t)b[1]<<8U)|((uint32_t)b[2]<<16U)|((uint32_t)b[3]<<24U);
}
static void put32(uint8_t *b,uint32_t value) {
    unsigned i;for(i=0;i<4U;++i)b[i]=(uint8_t)(value>>(8U*i));
}
static void checksum(uint8_t *b) {
    uint16_t sum=0;unsigned i;
    b[28]=b[29]=0;
    for(i=0;i<32U;i+=2U)sum^=(uint16_t)((unsigned)b[i]|((unsigned)b[i+1U]<<8U));
    b[28]=(uint8_t)sum;b[29]=(uint8_t)(sum>>8U);
}
int rtw_92du_make_tx_descriptor(size_t length,const rtw_92du_tx_options *o,uint8_t *out) {
    uint8_t next[32];uint32_t dw4,dw5;
    if(!out || !o || length<24U || length>4096U || o->rate>27U ||
        o->raid>15U || o->sequence>4095U || o->firmware_rate>1U ||
        o->multicast>1U || o->qos>1U || o->ampdu>1U ||
        o->short_preamble>1U || o->short_gi>1U || o->is_5ghz>1U ||
        (o->rf_paths!=1U && o->rf_paths!=2U) || o->packet_offset>1U || o->protection>2U || o->rts_short>1U ||
        (o->protection && (o->rts_rate>11U || (o->is_5ghz && o->rts_rate<4U))) ||
        (o->rts_short && (!o->protection || o->rts_rate<1U || o->rts_rate>3U)) ||
        (o->rf_paths==1U && o->rate>19U) ||
        (o->is_5ghz && o->rate<4U) ||
        (o->short_preamble && (o->rate<1U || o->rate>3U)) ||
        (o->short_gi && o->rate<12U) ||
        (o->rf_paths==1U && o->rate==19U && !o->short_gi) ||
        (o->qos ? o->qsel>7U:(o->qsel!=0U && o->qsel!=0x12U)) ||
        (o->firmware_rate && (o->multicast || o->qsel==0x12U)) ||
        (o->ampdu ? (!o->qos || o->multicast || o->rate<12U ||
            o->density>7U || !o->max_agg || o->max_agg>20U):
            (o->density || o->max_agg))) return 0;
    memset(next,0,sizeof(next));
    put32(next,0x8c200000U|(uint32_t)length|(o->multicast ? 0x01000000U:0U));
    put32(next+4,((uint32_t)o->raid<<16U)|((uint32_t)o->qsel<<8U)|
        (o->ampdu ? 0x20U:0x40U)|((uint32_t)o->packet_offset<<26U));
    put32(next+8,(uint32_t)o->density<<20U);
    put32(next+12,(uint32_t)o->sequence<<16U);
    dw4=(o->qos ? 0x40U:0U)|(o->firmware_rate ? 0U:0x500U);
    /* No CCK RTS on 5GHz; all protection enables remain off. */
    if(o->rate>=4U || o->is_5ghz)dw4|=8U;
    if(o->protection) {
        dw4=(dw4&~31U)|o->rts_rate|(o->protection==1U?0x3000U:0x2800U);
        if(o->rts_short)dw4|=0x04000000U;
    }
    dw5=o->rate|((o->short_gi || o->short_preamble) ? 0x40U:0U)|0x1ff00U;
    put32(next+16,dw4);put32(next+20,dw5);
    put32(next+24,(uint32_t)o->max_agg<<11U);
    checksum(next);memcpy(out,next,sizeof(next));return 1;
}
rtw_packet_error rtw_92du_parse_rx(const uint8_t *b,size_t length,rtw_92du_rx_frame *out) {
    rtw_92du_rx_frame next;
    uint32_t dw0,dw1,dw3;
    size_t n,info,shift,used,i;
    uintptr_t x,y;
    if(!b || !out || length>65536U) return RTW_PACKETS_BAD_ARGUMENT;
    x=(uintptr_t)b;y=(uintptr_t)out;
    if(x<=y ? y-x<length:x-y<sizeof(*out)) return RTW_PACKETS_BAD_ARGUMENT;
    memset(out,0,sizeof(*out));
    if(length<24U) return RTW_PACKETS_TRUNCATED;
    dw0=get32(b);dw1=get32(b+4U);dw3=get32(b+12U);
    n=dw0&0x3fffU;info=((dw0>>16U)&15U)*8U;shift=(dw0>>24U)&3U;
    if(n<14U) return RTW_PACKETS_MALFORMED;
    if(info>length-24U || shift>length-24U-info ||
        n>length-24U-info-shift) return RTW_PACKETS_TRUNCATED;
    memset(&next,0,sizeof(next));
    next.ht=(uint8_t)((dw3>>6U)&1U);
    next.security_code=(uint8_t)((dw0>>20U)&7U);
    next.packet.rate_index=(uint8_t)(dw3&63U);
    next.packet.phy_status_present=(uint8_t)((dw0>>26U)&1U);
    if(next.security_code>5U ||
        (next.ht ? (next.packet.rate_index<12U || next.packet.rate_index>27U):
                   next.packet.rate_index>11U) ||
        (next.packet.phy_status_present && info<28U)) return RTW_PACKETS_MALFORMED;
    used=24U+shift+info+n;
    for(i=used;i<length;++i)if(b[i])return RTW_PACKETS_MALFORMED;
    next.packet.phy_info=b+24U+shift;next.packet.phy_info_length=info;
    next.packet.mpdu=b+24U+shift+info;next.packet.mpdu_length=n-4U;
    next.packet.timestamp=get32(b+20U);
    next.packet.bandwidth_code=(uint8_t)((dw3>>9U)&1U);
    next.packet.crc_error=(uint8_t)((dw0>>14U)&1U);
    next.packet.icv_error=(uint8_t)((dw0>>15U)&1U);
    next.sw_decrypt_required=(uint8_t)((dw0>>27U)&1U);
    next.short_preamble=(uint8_t)((dw3>>8U)&1U);
    next.ampdu=(uint8_t)((dw1>>14U)&1U);
    next.first_ampdu=(uint8_t)(next.ampdu && (dw1&0x8000U));
    next.fcs_valid=(uint8_t)rtw_fcs_valid(next.packet.mpdu,n);
    *out=next;return RTW_PACKETS_OK;
}

int rtw_92du_tx_transfer_layout(size_t length,uint16_t mps,rtw_92du_tx_layout *out) {
    rtw_92du_tx_layout next;
    if(!out || length<24U || length>4096U || (mps!=64U && mps!=512U))return 0;
    next.packet_offset=(uint8_t)((length+32U)%mps==0U);
    next.mpdu_offset=32U+8U*next.packet_offset;next.total=length+next.mpdu_offset;
    *out=next;return 1;
}
static unsigned power_percentage(int power) {
    if(power<=-100 || power>=20)return 0U;
    return power>=0 ? 100U:(unsigned)(100+power);
}
int rtw_92du_parse_signal(const rtw_92du_rx_frame *frame,uint8_t paths,uint8_t high,rtw_92du_rx_signal *out) {
    rtw_92du_rx_signal next;const uint8_t *phy;int power;unsigned percentage,i;
    static const int bases[4]={16,-12,-26,-46};
    if(!frame || !out || (paths!=1U && paths!=2U) || high>1U ||
        !frame->packet.phy_status_present || !frame->packet.phy_info ||
        frame->packet.phy_info_length<28U || frame->packet.rate_index>27U)return 0;
    phy=frame->packet.phy_info;memset(&next,0,sizeof(next));
    if(frame->packet.rate_index<4U) {
        const unsigned report=high ? ((unsigned)phy[5]>>5U)&3U:(unsigned)phy[5]>>6U;
        const unsigned loss=high ? ((unsigned)phy[5]&31U)*2U:(unsigned)phy[5]&0x3eU;
        power=bases[report]-(int)loss;percentage=power_percentage(power)+6U;
        if(percentage>100U)percentage=100U;
        if(percentage>34U && percentage<=42U)percentage-=2U;
        else if(percentage>26U && percentage<=34U)percentage-=6U;
        else if(percentage>14U && percentage<=26U)percentage-=8U;
        else if(percentage>4U && percentage<=14U)percentage-=4U;
        for(i=0;i<paths;++i)next.path_pwdb[i]=(uint8_t)percentage;
    } else {
        power=(int)(((unsigned)phy[4]>>1U)&127U)-106;
        percentage=power_percentage(power);
        for(i=0;i<paths;++i)next.path_pwdb[i]=(uint8_t)power_percentage((int)(phy[i]&63U)*2-110);
    }
    next.dbm=(int16_t)power;next.pwdb=(uint8_t)percentage;*out=next;return 1;
}
