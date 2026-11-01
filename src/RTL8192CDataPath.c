/* SPDX-License-Identifier: GPL-2.0-only */
/* Wire fields cross-checked against OpenBSD src 3ff365285878f448ba65325e29c8d1e185af3e6d:
 * sys/dev/ic/r92creg.h, sys/dev/usb/if_urtwn.c; and Linux f47d223627f80cb7a1eb14c585fe6cb35de5e984:
 * drivers/net/wireless/realtek/rtl8xxxu/{core.c,rtl8xxxu.h}.
 * No bit in a USB completion is treated as an RF acknowledgement. */
#include "rtw/RTL8192CDataPath.h"
#include <string.h>

static uint32_t read32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) |
        ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
static void write32(uint8_t *p,uint32_t w) {
    p[0]=(uint8_t)w;p[1]=(uint8_t)(w>>8);p[2]=(uint8_t)(w>>16);p[3]=(uint8_t)(w>>24);
}
static void checksum(uint8_t *d) {
    uint16_t sum=0;unsigned i;
    d[28]=d[29]=0;
    for(i=0;i<32U;i+=2U)sum^=(uint16_t)((unsigned)d[i]|((unsigned)d[i+1U]<<8));
    d[28]=(uint8_t)sum;d[29]=(uint8_t)(sum>>8);
}
int rtw_92c_make_tx_descriptor(size_t n,const rtw_92e_tx_options *o,uint8_t *d) {
    uint32_t dw4;
    if(!d || !o || n<24U || n>4096U || o->data>1U || o->multicast>1U ||
       o->short_preamble>1U || o->firmware_rate>1U || o->rate>11U ||
       o->sequence>4095U || o->raid<4U || o->raid>6U ||
       o->ampdu || o->density || o->max_agg || o->bw40 || o->channel_bw40 ||
       o->primary_above || o->short_gi || (o->data && o->multicast) ||
       (o->data ? o->qsel>7U : o->qsel!=0x12U) ||
       (o->firmware_rate && !o->data))return 0;
    memset(d,0,32U);
    /* Software sequence in DW3 bits27:16. Both HW sequence controls stay off. */
    write32(d,0x8c200000U|(uint32_t)n|(o->multicast?0x01000000U:0U));
    write32(d+4,((uint32_t)o->raid<<16)|((uint32_t)o->qsel<<8)|(o->data?0x40U:0U));
    write32(d+12,(uint32_t)o->sequence<<16);
    dw4=o->firmware_rate?0U:0x100U;
    if(o->data && o->raid!=6U)dw4|=8U; /* RTS OFDM24, or CCK1 for 11b. */
    if(o->short_preamble)dw4|=0x01000000U;
    write32(d+16,dw4);
    write32(d+20,(uint32_t)o->rate|(o->data?0x0001ff00U:0x001a0000U));
    checksum(d);return 1;
}
int rtw_92c_tx_protection(unsigned mode,uint8_t *d) {
    uint32_t dw4;
    if(!d || mode>2U)return 0;
    dw4=read32(d+16)&~0x3800U;
    if(mode)dw4|=mode==1U?0x3000U:0x2800U;
    write32(d+16,dw4);checksum(d);return 1;
}
int rtw_92c_tx_report_request(int is_88e,int requested,uint8_t *d) {
    uint32_t dw1,dw2;int aggbk;
    if(!d || (is_88e!=0 && is_88e!=1) || (requested!=0 && requested!=1))return 0;
    dw1=read32(d+4);dw2=read32(d+8);
    /* CU firmware RA and 88E host RA have different completion policies. */
    if(is_88e && !(read32(d+16)&0x100U))return 0;
    if(requested && (!is_88e || (d[3]&1U) || (dw1&0x3fU) ||
       ((dw1>>8)&0x1fU)>7U))return 0;
    aggbk=((dw1&0x40U)!=0U)||((dw2&0x10000U)!=0U);
    dw1&=~0x40U;dw2&=~0x90000U;
    if(aggbk) { if(is_88e)dw2|=0x10000U;else dw1|=0x40U; }
    if(requested)dw2|=0x80000U; /* 88E CCX/SPE report, not an ACK itself. */
    write32(d+4,dw1);write32(d+8,dw2);checksum(d);return 1;
}
static int zeroes(const uint8_t *p,size_t n) {
    size_t i;for(i=0;i<n;i++)if(p[i])return 0;return 1;
}
rtw_packet_error rtw_92c_rx_next(rtw_rx_iterator *it,int is_88e,rtw_92c_rx_frame *out) {
    const uint8_t *b;uint32_t dw0,dw3;size_t remain,n,info,shift,used,padded,next;
    rtw_92c_rx_frame f;
    if(out)memset(out,0,sizeof(*out));
    if(!it || !out || (is_88e!=0 && is_88e!=1) ||
       (!it->buffer && it->length) || it->cursor>it->length)return RTW_PACKETS_BAD_ARGUMENT;
    if(it->cursor==it->length)return RTW_PACKETS_DONE;
    b=it->buffer+it->cursor;remain=it->length-it->cursor;
    if(remain<24U) {
        if(it->frames && zeroes(b,remain)){it->cursor=it->length;return RTW_PACKETS_DONE;}
        return RTW_PACKETS_TRUNCATED;
    }
    if(it->frames>=RTW_MAX_RX_FRAMES)return RTW_PACKETS_TOO_MANY;
    dw0=read32(b);dw3=read32(b+12);n=dw0&0x3fffU;
    if(!n) {
        if(!zeroes(b,remain))return RTW_PACKETS_MALFORMED;
        it->cursor=it->length;return RTW_PACKETS_DONE;
    }
    info=((dw0>>16)&15U)*8U;shift=(dw0>>24)&3U;
    if(info>remain-24U || shift>remain-24U-info || n>remain-24U-info-shift)
        return RTW_PACKETS_TRUNCATED;
    memset(&f,0,sizeof(f));
    f.report_type=is_88e?(uint8_t)((dw3>>14)&3U):0U;
    if(f.report_type && (info || shift))return RTW_PACKETS_MALFORMED;
    if(!f.report_type && !(dw3&0x40U) && (dw3&0x3fU)>11U)return RTW_PACKETS_MALFORMED;
    used=24U+info+shift+n;padded=(used+127U)&~(size_t)127U;
    if(padded>remain) {
        if(used!=remain)return RTW_PACKETS_TRUNCATED;
        next=it->length;
    } else next=it->cursor+padded;
    f.packet.mpdu=b+24U+info+shift;f.packet.mpdu_length=n;
    f.packet.phy_info=b+24U;f.packet.phy_info_length=info;
    f.packet.crc_error=(uint8_t)((dw0>>14)&1U);f.packet.icv_error=(uint8_t)((dw0>>15)&1U);
    f.packet.phy_status_present=(uint8_t)((dw0>>26)&1U);
    f.packet.rate_index=(uint8_t)(dw3&0x3fU);f.packet.bandwidth_code=(uint8_t)((dw3>>9)&1U);
    f.packet.timestamp=read32(b+20);
    f.security_code=(uint8_t)((dw0>>20)&7U);f.sw_decrypt_required=(uint8_t)((dw0>>27)&1U);
    f.ht=(uint8_t)((dw3>>6)&1U);
    it->cursor=next;it->frames++;*out=f;return RTW_PACKETS_OK;
}
int rtw_88e_parse_tx_report(const uint8_t *b,size_t n,rtw_88e_tx_report *out) {
    rtw_88e_tx_report r;
    if(out)memset(out,0,sizeof(*out));
    if(!b || !out || n!=8U || (b[1]&0x3fU) || (b[1]&0x80U) ||
       b[5]>11U || (b[6]>>4)>7U || ((b[1]&0x40U) && (b[2]&0xc0U)))return 0;
    memset(&r,0,sizeof(r));
    r.packet_ok=(uint8_t)((b[1]>>6)&1U);r.report.retries=b[2]&0x3fU;
    r.report.lifetime_expired=(uint8_t)((b[2]>>6)&1U);r.report.retry_over=(uint8_t)((b[2]>>7)&1U);
    r.report.queue_time=(uint16_t)((unsigned)b[3]|((unsigned)b[4]<<8));r.report.final_rate=b[5];
    *out=r;return 1;
}
