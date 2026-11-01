/* SPDX-License-Identifier: BSD-3-Clause */
/* Wire fields and Jaguar calibration derived from Linux rtw88, dual licensed
 * GPL-2.0 OR BSD-3-Clause, Copyright(c) 2018-2019,2024-2025 Realtek Corporation.
 * Pinned torvalds/linux f47d223627f80cb7a1eb14c585fe6cb35de5e984:
 * tx.h/tx.c, rx.h/rx.c, fw.h/fw.c, main.h/main.c, phy.c,
 * rtw8814a.c and rtw88xxa.h. No Linux execution context is imported. */
#include "rtw/RTL8814AUDataPath.h"
#include <string.h>

static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void put32(uint8_t *p,uint32_t v)
{
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}
static int boolean(int value) { return value==0 || value==1; }
static int zeroes(const uint8_t *p,size_t n)
{
    size_t i;for(i=0;i<n;++i)if(p[i])return 0;return 1;
}
static void checksum(uint8_t out[40])
{
    unsigned i;uint16_t sum=0;
    out[28]=out[29]=0;
    /* 8814A uses exactly16 words; DW8/DW9 are not included. */
    for(i=0;i<32U;i+=2U)sum^=(uint16_t)((unsigned)out[i]|((unsigned)out[i+1U]<<8));
    out[28]=(uint8_t)sum;out[29]=(uint8_t)(sum>>8);
}

int rtw_8814au_build_rate_config(uint16_t legacy,const uint8_t peer_mcs[2],
    unsigned chains,int ht,int local_sgi,int peer_sgi,rtw_8814au_rate_config *out)
{
    uint16_t mcs=0;unsigned i;
    if(!out)return 0;
    memset(out,0,sizeof(*out));
    if(!legacy || (legacy&0xf000U) || chains<1U || chains>2U ||
       !boolean(ht) || !boolean(local_sgi) || !boolean(peer_sgi) ||
       (ht && !peer_mcs))return 0;
    if(ht) {
        mcs=peer_mcs[0];
        if(chains==2U)mcs|=(uint16_t)((uint16_t)peer_mcs[1]<<8);
    }
    out->mask=(uint64_t)legacy|((uint64_t)mcs<<12);
    out->ht=(uint8_t)(mcs!=0);
    out->sgi20=(uint8_t)(mcs && local_sgi && peer_sgi);
    out->tx_streams=(uint8_t)((mcs&0xff00U)?2U:mcs?1U:0U);
    if(mcs)out->rate_id=(uint8_t)((legacy&0xfU)?
        (out->tx_streams==2U?2U:3U):(out->tx_streams==2U?4U:5U));
    else out->rate_id=(uint8_t)((legacy&0xff0U)?((legacy&0xfU)?6U:7U):8U);
    for(i=0;i<28U;++i)if(out->mask&(UINT64_C(1)<<i))out->initial_rate=(uint8_t)i;
    out->init_ra_level=1;
    return 1;
}
static int valid_config(const rtw_8814au_rate_config *c)
{
    rtw_8814au_rate_config v;uint8_t mcs[2];
    if(!c || !c->mask || (c->mask&~UINT64_C(0x0fffffff)) ||
       c->ht>1U || c->sgi20>1U || c->init_ra_level>3U ||
       (c->tx_streams>2U) || (!c->ht && (c->sgi20 || (c->mask&UINT64_C(0x0ffff000)))))return 0;
    mcs[0]=(uint8_t)(c->mask>>12);mcs[1]=(uint8_t)(c->mask>>20);
    if(!rtw_8814au_build_rate_config((uint16_t)(c->mask&0xfffU),mcs,
       c->tx_streams?c->tx_streams:1U,c->ht,c->sgi20,c->sgi20,&v))return 0;
    return c->mask==v.mask && c->rate_id==v.rate_id &&
        c->initial_rate==v.initial_rate && c->ht==v.ht &&
        c->sgi20==v.sgi20 && c->tx_streams==v.tx_streams;
}
int rtw_8814au_build_ra_commands(const rtw_8814au_rate_config *c,int reset,
    uint8_t low[8],uint8_t high[8])
{
    if(!low || !high || !boolean(reset) || !valid_config(c))return 0;
    memset(low,0,8);low[0]=0x40;
    low[2]=(uint8_t)(c->rate_id|((unsigned)c->init_ra_level<<5)|(c->sgi20?0x80U:0U));
    /* BW20, no LDPC/VHT, NO_UPDATE=!reset, disable power training. */
    low[3]=(uint8_t)(0x40U|(reset?0U:0x08U));
    put32(low+4,(uint32_t)c->mask);
    memcpy(high,low,8);high[0]=0x46;put32(high+4,(uint32_t)(c->mask>>32));
    return 1;
}
int rtw_8814au_make_media_status(int connected,uint8_t out[8])
{
    if(!out || !boolean(connected))return 0;
    memset(out,0,8);out[0]=1;out[1]=(uint8_t)connected;return 1;
}
int rtw_8814au_make_rssi_monitor(unsigned rssi,uint8_t out[8])
{
    if(!out || rssi>100U)return 0;
    memset(out,0,8);out[0]=0x42;out[3]=(uint8_t)rssi;return 1;
}

int rtw_8814au_make_tx_descriptor(size_t n,const rtw_8814au_tx_options *o,
    const rtw_8814au_rate_config *c,uint8_t out[40])
{
    uint32_t w0,w1,w2=0,w3,w4,w5=0;uint8_t raid;
    if(!o || !out || n<24U || n>4096U || o->sequence>4095U ||
       o->data>1U || o->multicast>1U || o->eapol>1U || o->firmware_rate>1U ||
       o->short_preamble>1U || o->short_gi>1U || o->protection>1U ||
       o->qos>1U || o->ack_expected>1U || o->fragment>1U || o->ampdu>1U ||
       o->density>7U || o->max_agg>16U ||
       (o->qos && (n<26U || !o->data)) ||
       o->rate>27U || (!o->data && o->qsel!=18U) ||
       (o->data && !o->multicast && o->qsel>7U) ||
       (o->data && o->multicast && o->qsel>7U && o->qsel!=18U) ||
       (o->eapol && !o->data) ||
       (o->protection && (!o->data || o->multicast)) ||
       (c && !valid_config(c)))return 0;
    if(o->firmware_rate && (!c || !o->data || o->multicast || o->eapol))return 0;
    if((o->firmware_rate || o->rate>=12U) &&
       (!c || !(c->mask&(UINT64_C(1)<<o->rate))))return 0;
    if(o->rate>=12U && (!o->data || o->multicast || o->eapol || !c->ht))return 0;
    if(o->short_gi && (o->rate<12U || !c || !c->sgi20 || o->short_preamble))return 0;
    if(o->short_preamble && (o->rate==0U || o->rate>3U))return 0;
    if(o->ampdu && (!o->firmware_rate || !c || !c->ht || o->rate<12U ||
       !o->qos || !o->ack_expected || o->fragment || o->multicast || o->eapol ||
       !o->max_agg))return 0;
    if(!o->ampdu && (o->density || o->max_agg))return 0;
    raid=(uint8_t)((o->firmware_rate || o->rate>=12U)?c->rate_id:(o->rate<=3U?8U:7U));
    /* No OWN/FIRST: bit31 is DISQSELSEQ on this descriptor generation. */
    w0=0x04280000U|(uint32_t)n|(o->multicast?0x01000000U:0U)|(!o->data?0x80000000U:0U);
    w1=((uint32_t)raid<<16)|((uint32_t)o->qsel<<8); /* MACID0, SEC_TYPE0. */
    w3=o->firmware_rate?0U:0x500U; /* explicit USE_RATE + DISDATAFB. */
    /* rtw88 tx.h DW2 AGG_EN[12]/AMPDU_DEN[22:20], DW3 MAX_AGG[21:17].
     * This is radio aggregation; the USB transfer still contains one MPDU. */
    if(o->ampdu){w2=0x1000U|((uint32_t)o->density<<20);w3|=(uint32_t)o->max_agg<<17;}
    w4=o->rate; /* 8814A old_datarate_fb_limit=false: never set0x1f. */
    if(o->protection) {w3|=0x1000U;w4|=0x08000000U;w5|=0x1000U;}
    if(o->short_preamble || o->short_gi)w5|=0x10U;
    memset(out,0,40);put32(out,w0);put32(out+4,w1);put32(out+8,w2);
    put32(out+12,w3);put32(out+16,w4);put32(out+20,w5);
    /* HWSEQ remains clear. This port supplies the complete MPDU sequence. */
    put32(out+36,(uint32_t)o->sequence<<12);checksum(out);return 1;
}

/* rtw8814a_query_phy_status overrides RX DW4 with Jaguar PHY metadata. */
static uint8_t phy_bandwidth(uint32_t word)
{
    unsigned mode=word>>30,sub=(word>>26)&15U;
    if(mode==1U&&!sub)return 1;
    if(mode==2U){if(!sub)return 2;if(sub==9U||sub==10U)return 1;}
    return 0;
}

rtw_packet_error rtw_8814au_rx_next(rtw_rx_iterator *it,rtw_8814au_rx_frame *out)
{
    const uint8_t *base;size_t remain,n,info,shift,used,padded;uint32_t w0,w2,w3;
    rtw_8814au_rx_frame f;
    if(out)memset(out,0,sizeof(*out));
    if(!it || !out || (!it->buffer && it->length) || it->cursor>it->length)return RTW_PACKETS_BAD_ARGUMENT;
    if(it->cursor==it->length)return RTW_PACKETS_DONE;
    base=it->buffer+it->cursor;remain=it->length-it->cursor;
    if(remain<24U) {
        if(remain<=7U && zeroes(base,remain)){it->cursor=it->length;return RTW_PACKETS_DONE;}
        return RTW_PACKETS_TRUNCATED;
    }
    if(it->frames>=RTW_MAX_RX_FRAMES)return RTW_PACKETS_TOO_MANY;
    w0=get32(base);w2=get32(base+8);w3=get32(base+12);
    n=w0&0x3fffU;
    if(!n) {
        if(!zeroes(base,remain))return RTW_PACKETS_MALFORMED;
        it->cursor=it->length;return RTW_PACKETS_DONE;
    }
    info=(size_t)((w0>>16)&15U)*8U;shift=(w0>>24)&3U;
    if((info && info!=32U) || ((w0&0x04000000U) && !info) ||
       n>RTW_8814AU_MAX_RX_MPDU || (w3&127U)>=84U)return RTW_PACKETS_MALFORMED;
    if(w2&0x10000000U) {
        if(n<2U || n>RTW_8814AU_C2H_MAX_BYTES)return RTW_PACKETS_MALFORMED;
    } else if(n<=4U)return RTW_PACKETS_MALFORMED;
    if(info>remain-24U || shift>remain-24U-info || n>remain-24U-info-shift)return RTW_PACKETS_TRUNCATED;
    used=24U+info+shift+n;padded=(used+7U)&~(size_t)7U;
    if(padded>remain && used!=remain)return RTW_PACKETS_TRUNCATED;
    memset(&f,0,sizeof(f));
    f.packet.phy_info=base+24U+shift;f.packet.phy_info_length=info;
    f.packet.mpdu=f.packet.phy_info+info;f.packet.mpdu_length=n;
    f.packet.crc_error=(uint8_t)((w0>>14)&1U);f.packet.icv_error=(uint8_t)((w0>>15)&1U);
    f.packet.phy_status_present=(uint8_t)((w0>>26)&1U);
    f.packet.rate_index=(uint8_t)(w3&127U);f.packet.bandwidth_code=(uint8_t)((get32(base+16)>>4)&3U);
    f.packet.timestamp=get32(base+20);f.is_c2h=(uint8_t)((w2>>28)&1U);
    if(!f.is_c2h&&f.packet.phy_status_present)
        f.packet.bandwidth_code=f.packet.rate_index<=3U?0:phy_bandwidth(get32(f.packet.phy_info));
    f.security_code=(uint8_t)((w0>>20)&7U);f.sw_decrypt_required=(uint8_t)((w0>>27)&1U);
    f.cam_id=(uint8_t)(get32(base+4)&127U);f.ppdu_count=(uint8_t)((w2>>29)&3U);
    it->cursor=padded>remain?it->length:it->cursor+padded;++it->frames;*out=f;
    return RTW_PACKETS_OK;
}

/* Literal rtw88/phy.c inverse dB table (including its historical entries).
 * Lower three rows have3 fractional bits. Preserve upstream integer rounding. */
static const uint32_t db_table[12][8]={
 {10,13,16,20,25,32,40,50},
 {64,80,101,128,160,201,256,318},
 {401,505,635,800,1007,1268,1596,2010},
 {316,398,501,631,794,1000,1259,1585},
 {1995,2512,3162,3981,5012,6310,7943,10000},
 {12589,15849,19953,25119,31623,39811,50119,63098},
 {79433,100000,125893,158489,199526,251189,316228,398107},
 {501187,630957,794328,1000000,1258925,1584893,1995262,2511886},
 {3162278,3981072,5011872,6309573,7943282,1000000,12589254,15848932},
 {19952623,25118864,31622777,39810717,50118723,63095734,79432823,100000000},
 {125892541,158489319,199526232,251188643,316227766,398107171,501187234,630957345},
 {794328235,1000000000,1258925412,1584893192,1995262315,2511886432U,3162277660U,3981071706U}
};
static uint8_t power_db(int power)
{
    if(power<=-100 || power>=20)return 0;
    return (uint8_t)(power>=0?100:100+power);
}
static uint64_t db_linear(unsigned power)
{
    unsigned i,j;uint64_t v;
    if(power>96U)power=96U;else if(power<1U)return 1;
    i=(power-1U)>>3;j=(power-1U)-(i<<3);v=db_table[i][j];
    return i>2U?v<<3:v;
}
static uint8_t linear_db(uint64_t linear)
{
    unsigned i,j;
    for(i=0;i<12U;++i)for(j=0;j<8U;++j) {
        if((i<=2U && (linear<<3)<=db_table[i][j]) || (i>2U && linear<=db_table[i][j]))goto found;
    }
    return 96;
found:
    if(!j && !i)return 1;
    if(!j) {
        if((uint64_t)db_table[i][0]-linear>linear-(uint64_t)db_table[i-1U][7]){--i;j=7;}
    } else if((uint64_t)db_table[i][j]-linear>linear-(uint64_t)db_table[i][j-1U])--j;
    return (uint8_t)((i<<3)+j+1U);
}
static uint8_t rf_rssi(const int8_t *powers,unsigned count)
{
    unsigned i;uint64_t sum=0;
    for(i=0;i<count;++i)sum+=db_linear(power_db(powers[i]));
    sum=(sum+4U)>>3;
    if(count==2U)sum>>=1;
    else if(count==3U)sum=(sum+(sum<<1)+(sum<<3))>>5;
    else if(count==4U)sum>>=2;
    return linear_db(sum);
}
static int8_t signed_byte(uint8_t b)
{
    return (int8_t)(b<128U?(int)b:(int)b-256);
}
int rtw_8814au_parse_phy(const rtw_8814au_rx_frame *f,uint8_t active,
    rtw_8814au_phy_status *out)
{
    const uint8_t *p;rtw_8814au_phy_status s;uint32_t w0;unsigned i,j;
    uint8_t gain[4],sorted[4],lna,vga;int cck_power;
    if(out)memset(out,0,sizeof(*out));
    if(!f || !out || !active || (active&0xf0U) || f->is_c2h ||
       !f->packet.phy_status_present || !f->packet.phy_info ||
       f->packet.phy_info_length!=32U || f->packet.rate_index>=84U)return 0;
    p=f->packet.phy_info;w0=get32(p);memset(&s,0,sizeof(s));
    s.active_mask=active;s.channel=(uint16_t)((w0>>16)&0x3ffU);
    if(f->packet.rate_index<=3U) {
        lna=(uint8_t)((p[5]>>5)&7U);vga=(uint8_t)(p[5]&31U);
        switch(lna) {
            case 7:cck_power=-38-2*(int)vga;break;
            case 5:cck_power=-28-2*(int)vga;break;
            case 3:cck_power=-8-2*(int)vga;break;
            case 2:cck_power=-1-2*(int)vga;break;
            default:return 0;
        }
        s.cck=1;s.measured_mask=1;s.path_dbm[0]=(int8_t)cck_power;
        s.signal_dbm=s.path_dbm[0];s.rssi=rf_rssi(s.path_dbm,1);s.path_rssi[0]=s.rssi;
    } else {
        gain[0]=(uint8_t)(w0&127U);gain[1]=(uint8_t)((w0>>8)&127U);
        gain[2]=(uint8_t)(p[23]&127U);gain[3]=(uint8_t)(p[24]&127U);
        s.snr_raw[0]=signed_byte(p[15]);s.snr_raw[1]=signed_byte(p[16]);
        s.snr_raw[2]=signed_byte(p[21]);s.snr_raw[3]=signed_byte(p[22]);
        s.evm_raw[0]=signed_byte(p[f->packet.rate_index<=11U?25:13]);
        s.evm_raw[1]=signed_byte(p[14]);s.evm_raw[2]=signed_byte(p[19]);s.evm_raw[3]=signed_byte(p[20]);
        s.cfo_tail_raw[0]=signed_byte(p[9]);s.cfo_tail_raw[1]=signed_byte(p[10]);
        for(i=0;i<4U;++i) {
            s.path_dbm[i]=(int8_t)((int)gain[i]-110);sorted[i]=gain[i];
            s.path_rssi[i]=rf_rssi(&s.path_dbm[i],1);
        }
        for(i=1;i<4U;++i)for(j=i;j>0U && sorted[j]<sorted[j-1U];--j) {
            uint8_t tmp=sorted[j];sorted[j]=sorted[j-1U];sorted[j-1U]=tmp;
        }
        s.signal_dbm=(int8_t)((int)sorted[1]-110);s.rssi=rf_rssi(s.path_dbm,4);s.measured_mask=15;
        s.bandwidth_code=phy_bandwidth(w0);
    }
    *out=s;return 1;
}

rtw_8814au_c2h_result rtw_8814au_parse_c2h(const uint8_t *b,size_t n,
    const rtw_8814au_rate_config *c,rtw_8814au_c2h_event *out)
{
    rtw_8814au_c2h_event e;uint8_t rate,sgi;
    if(out)memset(out,0,sizeof(*out));
    if(!b || !out || n<2U || n>RTW_8814AU_C2H_MAX_BYTES)return RTW_8814AU_C2H_MALFORMED;
    memset(&e,0,sizeof(e));e.id=b[0];e.sequence=b[1];
    if(b[0]==0x0cU) {
        if(n!=8U)return RTW_8814AU_C2H_MALFORMED;
        rate=b[2]&127U;sgi=b[2]>>7;
        if(b[3] || rate>=28U || !valid_config(c) || !(c->mask&(UINT64_C(1)<<rate)) ||
           (sgi && (rate<12U || !c->sgi20)))return RTW_8814AU_C2H_IGNORED;
        e.kind=RTW_8814AU_EVENT_RA;e.rate=rate;e.short_gi=sgi;
        e.mac_id=b[3];e.status=b[5];e.ra_ratio=b[7];
    } else if(b[0]==0x32U)e.kind=RTW_8814AU_EVENT_RF_ON;
    else return RTW_8814AU_C2H_IGNORED;
    *out=e;return RTW_8814AU_C2H_OK;
}
