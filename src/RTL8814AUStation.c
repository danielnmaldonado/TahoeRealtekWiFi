/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
 * Copyright(c) 2018-2019 Realtek Corporation.
 * Port0/RCR/EDCA adapted from pinned Linux rtw88 main.c, mac80211.c, reg.h,
 * sec.h (0e135910deec43ffe97eca0b6af1f8f847285b59). Station-only policy,
 * checked I/O and software crypto are local adaptations. */
#include "RTL8814AURegs.h"
#include "rtw/RTL8814AUStation.h"
static int valid_mac(const uint8_t *m) {
    unsigned any=0;for(unsigned i=0;i<6;i++)any|=m[i];return any&&!(m[0]&1);
}
static int ready(struct rtw_8814au *c) {
    if(!c)return EINVAL;if(c->io_error)return c->io_error;
    return c->board_valid&&c->powered&&c->fw_running&&c->mac_initialized?0:ENXIO;
}
static void address(struct rtw_8814au *c,uint16_t a,const uint8_t *m) {
    for(unsigned i=0;i<6;i++)W8(c,(uint16_t)(a+i),m[i]);
}
int rtw_8814au_station_initialize(struct rtw_8814au *c,const struct rtw_8814au_station_params *p) {
    int e=ready(c);if(e)return e;
    if(!p||!valid_mac(p->mac)||p->band_5g>1)return EINVAL;
    W16(c,0x680,0); /* Disable hardware encrypt/decrypt and default-key lookup. */
    W32(c,0x670,0xc0000000U);
    if((e=rtw_8814au_poll(c,0x670,4,0x80000000U,0,10,1000)))return e;
    address(c,0x610,p->mac);
    W32(c,0x620,0xffffffffU);W32(c,0x624,0xffffffffU);
    /* APP_FCS/MIC/ICV/PHYSTS, PKTCTL_DLEN, HTC_LOC_CTRL, AB/AM/APM.
     * Neither CRC/ICV failures nor arbitrary destination addresses admitted. */
    W32(c,0x608,0xf010400eU);
    W16(c,0x6a0,0xffff);W16(c,0x6a2,0);W16(c,0x6a4,0);
    U32(c,0x100,0x30000,0);U8(c,0x550,16,16);
    return c->io_error;
}
int rtw_8814au_station_configure(struct rtw_8814au *c,enum rtw_8814au_station_state state,
    const struct rtw_8814au_station_params *p) {
    int e=ready(c);if(e)return e;
    if(state<RTW_8814AU_STATION_INIT||state>RTW_8814AU_STATION_RUN)return EINVAL;
    if(state!=RTW_8814AU_STATION_INIT&&(!p||!valid_mac(p->mac)))return ENXIO;
    if(state!=RTW_8814AU_STATION_INIT&&!c->phy_initialized&&
       !(state==RTW_8814AU_STATION_SCAN&&p->passive_scan))return ENXIO;
    if(p&&(p->band_5g>1||p->short_slot>1||p->passive_scan>1||p->ht>1||(p->basic_rates&0xf000U)||(p->band_5g&&p->basic_rates&15U)))return EINVAL;
    if(state==RTW_8814AU_STATION_RUN&&(!valid_mac(p->bssid)||!p->aid||p->aid>2007||
       !p->basic_rates||(p->basic_rates&~0xfff)))return EINVAL;
    if(state!=RTW_8814AU_STATION_RUN) {
        U32(c,0x100,0x30000,0);W16(c,0x6a4,0);W16(c,0x6a2,0);
        U32(c,0x608,0xc0,0);U8(c,0x550,16,16);
        /* INIT pauses every queue; scan/auth retain the management queue. */
        if(p&&state!=RTW_8814AU_STATION_INIT)
            U32(c,0x440,0xfffff,p->basic_rates?p->basic_rates:p->band_5g?0x150:0x15f);
        W8(c,0x522,state==RTW_8814AU_STATION_INIT||(state==RTW_8814AU_STATION_SCAN&&p->passive_scan)?0x3f:15);
        if(p&&state!=RTW_8814AU_STATION_INIT){address(c,0x610,p->mac);
            if(state==RTW_8814AU_STATION_AUTH||state==RTW_8814AU_STATION_ASSOC)address(c,0x618,p->bssid);}
    } else {
        address(c,0x610,p->mac);address(c,0x618,p->bssid);
        U32(c,0x6a8,0x7ff,p->aid);U32(c,0x440,0xfffff,p->basic_rates);
        W8(c,0x51b,p->band_5g||p->short_slot?9:20);
        W16(c,0x6a0,0xffff);W16(c,0x6a2,p->ht?0x100:0);W16(c,0x6a4,0xffff);
        U32(c,0x608,0xc0,0xc0);U8(c,0x550,0x18,0x08);W8(c,0x522,0);
        U32(c,0x100,0x30000,0x20000);
    }
    return c->io_error;
}
int rtw_8814au_station_edca_band(struct rtw_8814au *c,int short_slot,int band_5g,
    const struct rtw_8814au_edca_ac p[4]) {
    int e=ready(c);if(e)return e;if(!p||(short_slot!=0&&short_slot!=1)||(band_5g!=0&&band_5g!=1))return EINVAL;
    unsigned slot=band_5g||short_slot?9:20,sifs=band_5g?16:10;
    for(unsigned i=0;i<4;i++)if(p[i].aifsn>15||p[i].ecwmin>15||p[i].ecwmax>15||
        p[i].ecwmin>p[i].ecwmax||p[i].txop>0x7ff||p[i].aifsn*slot+sifs>255)return EINVAL;
    static const uint16_t reg[4]={0x508,0x50c,0x504,0x500};
    W8(c,0x51b,slot);
    for(unsigned i=0;i<4;i++)U32(c,reg[i],0x07ffffffU,(uint32_t)p[i].txop<<16|
        (uint32_t)p[i].ecwmax<<12|(uint32_t)p[i].ecwmin<<8|(p[i].aifsn*slot+sifs));
    return c->io_error;
}

int rtw_8814au_station_edca(struct rtw_8814au*c,int short_slot,const struct rtw_8814au_edca_ac p[4]){
    return rtw_8814au_station_edca_band(c,short_slot,0,p);
}
