/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/RTL8192EUHT.h"
#include <string.h>

/* Wire fields follow the pinned Linux rtl8xxxu h2c_cmd.b_macid_cfg and
 * rtl8xxxu_gen2_update_rate_mask. RAID values are rtl8xxxu_ratr_idx. */
int rtw_92e_build_rate_config_width(uint16_t legacy,const uint8_t peer_mcs[2],
    unsigned tx_chains,int ht,int local_sgi20,int peer_sgi20,
    int ht40,int local_sgi40,int peer_sgi40,rtw_92e_rate_config *out)
{
    uint16_t mcs=0;
    unsigned i;
    if(!out) return 0;
    memset(out,0,sizeof(*out));
    if(!legacy || (legacy&~0x0fffU) || tx_chains<1U || tx_chains>2U ||
       (ht!=0 && ht!=1) || (local_sgi20!=0 && local_sgi20!=1) ||
       (peer_sgi20!=0 && peer_sgi20!=1) ||
       (ht40!=0 && ht40!=1) ||
       (local_sgi40!=0 && local_sgi40!=1) ||
       (peer_sgi40!=0 && peer_sgi40!=1) || (ht && !peer_mcs)) return 0;
    if(ht && peer_mcs[0]) {
        mcs=peer_mcs[0];
        if(tx_chains==2U) mcs|=(uint16_t)((uint16_t)peer_mcs[1]<<8);
    }
    out->rate_mask=legacy|((uint32_t)mcs<<12);
    out->ht=mcs!=0;
    out->bw40=(uint8_t)(out->ht && ht40);
    out->sgi20=(uint8_t)(out->ht && !out->bw40 && local_sgi20 && peer_sgi20);
    out->sgi40=(uint8_t)(out->bw40 && local_sgi40 && peer_sgi40);
    out->tx_streams=(uint8_t)(mcs&0xff00U?2U:mcs?1U:0U);
    if(mcs) out->raid=(uint8_t)((legacy&0x0fU)?(out->tx_streams==2U?2U:3U):
                                                      (out->tx_streams==2U?4U:5U));
    else out->raid=(uint8_t)((legacy&0x0ff0U)?((legacy&0x0fU)?6U:7U):8U);
    for(i=0;i<28U;i++) if(out->rate_mask&((uint32_t)1U<<i)) out->initial_rate=(uint8_t)i;
    return 1;
}

/* Compatibility entrypoint: existing gen2 callers remain HT20 by default. */
int rtw_92e_build_rate_config(uint16_t legacy,const uint8_t peer_mcs[2],
    unsigned tx_chains,int ht,int local_sgi20,int peer_sgi20,
    rtw_92e_rate_config *out)
{
    return rtw_92e_build_rate_config_width(legacy,peer_mcs,tx_chains,ht,
        local_sgi20,peer_sgi20,0,0,0,out);
}

int rtw_92e_make_macid_config(const rtw_92e_rate_config *c,uint8_t macid,uint8_t out[7])
{
    rtw_92e_rate_config verified;
    uint8_t mcs[2];
    if(!c || !out || macid>127U || !c->rate_mask ||
       (c->rate_mask&0xf0000000U) || c->raid>8U || c->sgi20>1U ||
       c->sgi40>1U || c->bw40>1U || c->ht>1U ||
       (!c->ht && (c->sgi20 || c->sgi40 || c->bw40 ||
                   (c->rate_mask&0x0ffff000U))) ||
       (c->bw40 && c->sgi20) || (!c->bw40 && c->sgi40)) return 0;
    mcs[0]=(uint8_t)(c->rate_mask>>12);mcs[1]=(uint8_t)(c->rate_mask>>20);
    if(!rtw_92e_build_rate_config_width((uint16_t)(c->rate_mask&0xfffU),mcs,
       c->tx_streams?c->tx_streams:1U,c->ht,c->sgi20,c->sgi20,
       c->bw40,c->sgi40,c->sgi40,&verified) ||
       c->rate_mask!=verified.rate_mask || c->raid!=verified.raid ||
       c->ht!=verified.ht || c->sgi20!=verified.sgi20 ||
       c->bw40!=verified.bw40 || c->sgi40!=verified.sgi40 ||
       c->tx_streams!=verified.tx_streams || c->initial_rate!=verified.initial_rate) return 0;
    out[0]=macid;
    out[1]=(uint8_t)(c->raid|((c->bw40?c->sgi40:c->sgi20)?0x80U:0U));
    out[2]=c->bw40?1U:0U; /* Linux gen2: BW20=0, BW40=1. */
    out[3]=(uint8_t)c->rate_mask;out[4]=(uint8_t)(c->rate_mask>>8);
    out[5]=(uint8_t)(c->rate_mask>>16);out[6]=(uint8_t)(c->rate_mask>>24);
    return 1;
}

int rtw_92e_make_media_status(int connect,uint8_t macid,uint8_t out[3])
{
    if(!out || macid>127U || (connect!=0 && connect!=1)) return 0;
    out[0]=(uint8_t)(0x20U|(unsigned)connect);out[1]=macid;out[2]=0;
    return 1;
}
