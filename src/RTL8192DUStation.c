/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright(c) 2009-2012, 2024 Realtek Corporation.
 * Adapted from pinned rtlwifi rtl8192du/{hw,trx,dm}.c and
 * rtl8192d/{hw,fw}_common.c at 0e135910deec43ffe97eca0b6af1f8f847285b59.
 * See RTL8192DUBoot-Upstream.json for exact primary source hashes. */
#include "rtw/StationChannels.h"
#include "rtw/RTL8192DUStation.h"
#include "rtw/RTL8192DUUSB.h"
#include <string.h>
#define BIT(n) (1U<<(n))
#include "RTL8192DURegisters.inc"
static int valid(const rtw_92du_mac *m) {
    return m && m->boot && m->boot->board.profile && m->out_pipes>=1U &&
        m->out_pipes<=3U && m->boot->bus.control_in && m->boot->bus.control_out &&
        m->boot->waiter.wait_us && m->boot->max_polls && m->boot->max_polls<=1000U;
}
static void fail(rtw_92du_mac *m,rtw_92du_result result) {
    if(m->boot->last_error==RTW_92DU_OK)m->boot->last_error=result;
    m->rate_ready=m->joined=0;
}
static uint32_t read_reg(rtw_92du_mac *m,uint16_t a,uint8_t width) {
    uint32_t value=0;
    if(m->boot->last_error!=RTW_92DU_OK)return 0;
    m->boot->last_register=a;
    if(rtw_usb_rw_read(&m->boot->bus,a,width,&value)!=RTW_USB_OK)fail(m,RTW_92DU_IO_ERROR);
    return value;
}
static void write_reg(rtw_92du_mac *m,uint16_t a,uint8_t width,uint32_t value) {
    if(m->boot->last_error!=RTW_92DU_OK)return;
    m->boot->last_register=a;
    if(rtw_usb_write_register(&m->boot->bus,a,width,value)!=RTW_USB_OK)fail(m,RTW_92DU_IO_ERROR);
}
static void update(rtw_92du_mac *m,uint16_t a,uint8_t width,uint32_t clear,uint32_t set) {
    const uint32_t old=read_reg(m,a,width);write_reg(m,a,width,(old&~clear)|set);
}
static void wait_us(rtw_92du_mac *m,uint32_t n) {
    if(m->boot->last_error==RTW_92DU_OK && !m->boot->waiter.wait_us(m->boot->waiter.context,n))
        fail(m,RTW_92DU_WAIT_ERROR);
}
static int poll_clear(rtw_92du_mac *m,uint16_t a,uint8_t width,uint32_t mask,uint32_t delay) {
    unsigned i,limit=m->boot->max_polls;
    if(a==REG_HMETFR && limit>100U)limit=100U;
    if(a==REG_LLT_INIT && limit>POLLING_LLT_THRESHOLD)limit=POLLING_LLT_THRESHOLD;
    for(i=0;i<limit && m->boot->last_error==RTW_92DU_OK;++i) {
        const uint32_t value=read_reg(m,a,width);
        if(m->boot->last_error!=RTW_92DU_OK)return 0;
        if(!(value&mask))return 1;
        if(delay)wait_us(m,delay);
    }
    fail(m,RTW_92DU_POLL_TIMEOUT);return 0;
}
static rtw_92du_result finish(rtw_92du_mac *m) {
    if(m->boot->last_error!=RTW_92DU_OK) {
        m->rate_ready=m->joined=0;
        if(!m->cleanup_done) {
            m->cleanup_done=1;
            (void)rtw_usb_write_register(&m->boot->bus,REG_TXPAUSE,1U,0xffU);
        }
    }
    return m->boot->last_error;
}
rtw_92du_result rtw_92du_mac_health(rtw_92du_mac *m) {
    uint32_t global,local=0;
    if(!valid(m))return RTW_92DU_BAD_ARGUMENT;
    if(!m->started || !m->boot->power_ready || !m->boot->firmware_ready)
        fail(m,RTW_92DU_WRONG_STATE);
    global=read_reg(m,REG_MCUFWDL,1);
    /* DU uses per-MAC FW-ready bytes, not the AU WINTINI_RDY mask and
     * not MCUFWDL+1, whose bit7 is the independent MAC0_ON flag. */
    if(m->boot->last_error==RTW_92DU_OK && (global&(MCUFWDL_EN|MCUFWDL_RDY))!=MCUFWDL_RDY)
        fail(m,RTW_92DU_IO_ERROR);
    if(m->boot->last_error==RTW_92DU_OK)
        local=read_reg(m,m->boot->board.mac_index?FW_MAC1_READY:FW_MAC0_READY,1);
    if(m->boot->last_error==RTW_92DU_OK && !(local&BIT(0)))fail(m,RTW_92DU_IO_ERROR);
    if(m->boot->last_error!=RTW_92DU_OK)m->boot->firmware_ready=0;
    return finish(m);
}
static rtw_92du_result initialize_mac(rtw_92du_mac *out,rtw_92du_boot *boot,const rtw_backend_usb_plan *usb,const rtw_usb_endpoints *endpoints) {
    rtw_92du_mac next;rtw_92du_boot checked;rtw_92du_boot_options options={1,1,0};
    if(!out || !boot || !usb)return RTW_92DU_BAD_ARGUMENT;
    if(rtw_92du_validate_usb_plan(usb)!=RTW_92DU_OK || boot->board.profile!=usb->profile)
        return RTW_92DU_IDENTITY_UNVERIFIED;
    if(endpoints->interface_number!=boot->board.mac_index || endpoints->alternate_setting ||
       endpoints->num_bulk_in!=1U || endpoints->num_bulk_out<1U || endpoints->num_bulk_out>3U)
        return RTW_92DU_UNSUPPORTED_CHIP;
    options.max_polls=boot->max_polls;
    if(rtw_92du_boot_init(&checked,&boot->board,&boot->bus,&boot->waiter,&options)!=RTW_92DU_OK)
        return RTW_92DU_IDENTITY_UNVERIFIED;
    if(boot->last_error!=RTW_92DU_OK)return boot->last_error;
    memset(&next,0,sizeof(next));next.boot=boot;next.out_pipes=endpoints->num_bulk_out;
    memcpy(next.station_address,boot->board.mac,6);
    *out=next;return RTW_92DU_OK;
}
rtw_92du_result rtw_92du_mac_init(rtw_92du_mac *out,rtw_92du_boot *boot,const rtw_backend_usb_plan *usb) {
    if(!out || !boot || !usb)return RTW_92DU_BAD_ARGUMENT;
    if(boot->board.mac_index)return RTW_92DU_IDENTITY_UNVERIFIED;
    return initialize_mac(out,boot,usb,&usb->endpoints);
}
rtw_92du_result rtw_92du_mac_init_for_interface(rtw_92du_mac *out,rtw_92du_boot *boot,
    const rtw_backend_usb_plan *usb,const uint8_t *cfg,size_t length) {
    rtw_usb_endpoints endpoints;rtw_92du_result result;
    if(!out || !boot)return RTW_92DU_BAD_ARGUMENT;
    result=rtw_92du_usb_interface_endpoints(usb,&boot->board,cfg,length,&endpoints);
    if(result!=RTW_92DU_OK)return result;
    return initialize_mac(out,boot,usb,&endpoints);
}
rtw_92du_result rtw_92du_mac_set_address(rtw_92du_mac *m,const uint8_t address[6]) {
    unsigned i;uint8_t combined=0;
    if(!valid(m) || !address)return RTW_92DU_BAD_ARGUMENT;
    for(i=0;i<6U;++i)combined|=address[i];
    if(!combined || (address[0]&1U))return RTW_92DU_BAD_ARGUMENT;
    if(!m->started || m->state>RTW_92DU_ST_SCAN || m->joined || m->rate_ready ||
       rtw_92du_phy_tx_active(m->boot))return RTW_92DU_WRONG_STATE;
    if(m->boot->last_error!=RTW_92DU_OK)return m->boot->last_error;
    for(i=0;i<6U;++i)write_reg(m,(uint16_t)(REG_MACID+i),1,address[i]);
    if(m->boot->last_error==RTW_92DU_OK)memcpy(m->station_address,address,6);
    return finish(m);
}
rtw_92du_result rtw_92du_mac_prepare(rtw_92du_mac *m) {
    unsigned i,boundary,maxpage;
    if(!valid(m) || !m->boot->power_ready || m->boot->firmware_ready || m->prepared)return RTW_92DU_WRONG_STATE;
    if(m->boot->last_error!=RTW_92DU_OK)return finish(m);
    boundary=m->boot->board.mode==RTW_92DU_SINGLE_MAC?249U:123U;
    maxpage=m->boot->board.mode==RTW_92DU_SINGLE_MAC?255U:127U;
    write_reg(m,REG_TXPAUSE,1,0xff);
    for(i=0;i<=maxpage && m->boot->last_error==RTW_92DU_OK;++i) {
        const unsigned next=i==boundary-1U?255U:i==maxpage?boundary:i+1U;
        write_reg(m,REG_LLT_INIT,4,BIT(30)|(i<<8U)|next);
        (void)poll_clear(m,REG_LLT_INIT,4,3U<<30U,0);
    }
    m->prepared=(uint8_t)(m->boot->last_error==RTW_92DU_OK);return finish(m);
}
static uint32_t receive_config(void) {
    return RCR_APM|RCR_AM|RCR_AB|RCR_ADF|RCR_AMF|RCR_HTC_LOC_CTRL|
        RCR_APP_ICV|RCR_APP_MIC|RCR_APP_PHYST_RXFF|RCR_APPFCS;
}
static void stop_beacon(rtw_92du_mac *m) {
    update(m,REG_FWHW_TXQ_CTRL+2U,1,BIT(6),0);
    write_reg(m,REG_BCN_MAX_ERR,1,0xff);
    write_reg(m,REG_TBTT_PROHIBIT+1U,1,0x64);
    update(m,REG_TBTT_PROHIBIT+2U,1,BIT(0),0);
}
rtw_92du_result rtw_92du_mac_start(rtw_92du_mac *m) {
    unsigned i,boundary,pub,remaining,unit,hq,lq,nq,be,bk,vi,priority;
    rtw_92du_result result;
    if(!valid(m) || !m->prepared || m->started || !m->boot->power_ready || !m->boot->firmware_ready || !m->boot->phy)
        return RTW_92DU_WRONG_STATE;
    if(m->boot->last_error!=RTW_92DU_OK)return finish(m);
    result=rtw_92du_phy_load_mac_table(m->boot);
    if(result!=RTW_92DU_OK){fail(m,result);return finish(m);}
    boundary=m->boot->board.mode==RTW_92DU_SINGLE_MAC?249U:123U;
    pub=m->boot->board.mode==RTW_92DU_SINGLE_MAC?0x89U:0x5aU;
    remaining=(m->boot->board.mode==RTW_92DU_SINGLE_MAC?0xf8U:0x7aU)-pub;
    unit=remaining/m->out_pipes;hq=unit+remaining%m->out_pipes;
    lq=m->out_pipes==3U?unit:0U;nq=m->out_pipes>=2U?unit:0U;
    if(m->boot->board.mode==RTW_92DU_DUAL_MAC && m->out_pipes==3U){hq=0x10;lq=0x10;nq=0;}
    write_reg(m,REG_RQPN_NPQ,1,nq);write_reg(m,REG_RQPN,4,BIT(31)|hq|(lq<<8U)|(pub<<16U));
    write_reg(m,REG_TXPKTBUF_BCNQ_BDNY,1,boundary);write_reg(m,REG_TXPKTBUF_MGQ_BDNY,1,boundary);
    write_reg(m,REG_TXPKTBUF_WMAC_LBK_BF_HD,1,boundary);write_reg(m,REG_TRXFF_BNDY,1,boundary);
    write_reg(m,REG_TDECTRL+1U,1,boundary);
    be=bk=m->out_pipes==1U?QUEUE_HIGH:m->out_pipes==2U?QUEUE_NORMAL:QUEUE_LOW;
    vi=m->out_pipes==3U?QUEUE_NORMAL:QUEUE_HIGH;
    priority=(be<<8U)|(bk<<10U)|(vi<<6U)|(QUEUE_HIGH<<4U)|(QUEUE_HIGH<<12U)|(QUEUE_HIGH<<14U);
    update(m,REG_TRXDMA_CTRL,2,0xfff0U,priority);
    write_reg(m,REG_PBP,1,0x11);write_reg(m,REG_RX_DRVINFO_SZ,1,4);
    write_reg(m,REG_HISR,4,0xffffffffU);write_reg(m,REG_HIMR,4,0xffffffffU);
    update(m,MSR,1,3U,0);write_reg(m,REG_RCR,4,receive_config());
    write_reg(m,REG_MAR,4,0xffffffffU);write_reg(m,REG_MAR+4U,4,0xffffffffU);
    update(m,REG_RRSR,4,0xfffffU,m->boot->board.initial_band==RTW_WIFI_BAND_5GHZ?0xffff0U:0xffff1U);
    write_reg(m,REG_SPEC_SIFS,2,0x1010);write_reg(m,REG_RL,2,0x3030);
    update(m,REG_RD_CTRL,2,0,DIS_EDCA_CNT_DWN);
    write_reg(m,REG_SIFS_CTX,2,0x0a0a);write_reg(m,REG_SIFS_TRX,2,0x1010);
    write_reg(m,REG_PROT_MODE_CTRL,2,0x0204);write_reg(m,REG_BAR_MODE_CTRL,4,0x014004);
    write_reg(m,REG_EDCA_BE_PARAM,4,0x005ea42b);write_reg(m,REG_EDCA_BK_PARAM,4,0x0000a44f);
    write_reg(m,REG_EDCA_VI_PARAM,4,0x005ea324);write_reg(m,REG_EDCA_VO_PARAM,4,0x002fa226);
    write_reg(m,REG_PIFS,1,0x1c);write_reg(m,REG_AGGR_BREAK_TIME,1,0x16);
    write_reg(m,REG_NAV_PROT_LEN,2,0x40);write_reg(m,REG_BCNDMATIM,1,2);write_reg(m,REG_ATIMWND,1,2);
    write_reg(m,REG_DARFRC,4,0);write_reg(m,REG_DARFRC+4U,4,0x10080404);
    write_reg(m,REG_RARFRC,4,0x04030201);write_reg(m,REG_RARFRC+4U,4,0x08070605);
    update(m,REG_FWHW_TXQ_CTRL,1,0,EN_AMPDU_RTY_NEW);write_reg(m,REG_ACKTO,1,0x40);
    write_reg(m,REG_BWOPMODE,1,BW_OPMODE_20MHZ);
    write_reg(m,REG_AMPDU_MIN_SPACE,1,(unsigned)(m->boot->board.rf_paths==2U?MAX_MSS_DENSITY_2T:MAX_MSS_DENSITY_1T)<<3U);
    write_reg(m,REG_BCN_CTRL,2,0x1010);write_reg(m,REG_TBTT_PROHIBIT,2,0x3c02);
    write_reg(m,REG_DRVERLYINT,1,5);write_reg(m,REG_BCNDMATIM,1,3);write_reg(m,REG_BCNTCFG,2,0x660f);
    write_reg(m,REG_AGGLEN_LMT,4,m->boot->board.mode==RTW_92DU_SINGLE_MAC?0x88728841U:0x66525541U);
    write_reg(m,REG_BCN_MAX_ERR,1,0xff);write_reg(m,REG_PKT_VO_VI_LIFE_TIME,2,0x400);
    write_reg(m,REG_PKT_BE_BK_LIFE_TIME,2,0x400);
    write_reg(m,REG_LEDCFG0,2,0x8282);write_reg(m,REG_LEDCFG2,1,0x82);
    update(m,REG_TXDMA_OFFSET_CHK,4,0,DROP_DATA_EN);
    for(i=0;i<4U;++i)write_reg(m,(uint16_t)(REG_ARFR0+4U*i),4,0x1f8ffff0);
    if(m->boot->board.mode==RTW_92DU_SINGLE_MAC && m->out_pipes==2U)write_reg(m,REG_FAST_EDCA_CTRL,4,0x03066666);
    else write_reg(m,REG_FAST_EDCA_CTRL,2,m->boot->board.mode==RTW_92DU_SINGLE_MAC?0x8888U:0x5555U);
    update(m,0x605,1,0,0xf0);write_reg(m,REG_RXTSF_OFFSET_CCK,1,0x30);
    write_reg(m,REG_RXTSF_OFFSET_OFDM,1,0x30);write_reg(m,0x606,1,0x30);
    write_reg(m,REG_DIS_TXREQ_CLR,1,0x10);update(m,REG_RD_CTRL,2,0,BIT(12));
    write_reg(m,REG_TXPAUSE,1,0xff);update(m,0xfe56,1,0,3);write_reg(m,REG_EARLY_MODE_CONTROL,1,0);
    /* Mandatory differences for the existing net80211 software CCMP and
     * software sequence path. No CAM or implicit hardware crypto survives. */
    write_reg(m,REG_SECCFG,1,0);write_reg(m,REG_CAMCMD,4,BIT(31)|BIT(30));
    (void)poll_clear(m,REG_CAMCMD,4,BIT(31),1);
    write_reg(m,REG_HWSEQ_CTRL,1,0);write_reg(m,REG_RXFLTMAP0,2,0xffff);
    write_reg(m,REG_RXFLTMAP1,2,0);write_reg(m,REG_RXFLTMAP2,2,0);
    for(i=0;i<6U;++i)write_reg(m,(uint16_t)(REG_MACID+i),1,m->boot->board.mac[i]);
    write_reg(m,REG_BCN_CTRL,2,0x1818);stop_beacon(m);
    update(m,REG_FWHW_TXQ_CTRL,4,0,BIT(12));
    m->started=(uint8_t)(m->boot->last_error==RTW_92DU_OK);return finish(m);
}
rtw_92du_result rtw_92du_h2c(rtw_92du_mac *m,uint8_t id,const uint8_t *data,uint8_t length) {
    uint8_t main[4]={0,0,0,0};unsigned i,offset=0;uint16_t box,ext;
    if(!valid(m) || !data || !length || length>5U || id>0x7fU)return RTW_92DU_BAD_ARGUMENT;
    if(!m->started || !m->boot->firmware_ready || m->next_h2c>3U)return RTW_92DU_WRONG_STATE;
    if(m->boot->last_error!=RTW_92DU_OK)return finish(m);
    if(!poll_clear(m,REG_HMETFR,1,BIT(m->next_h2c),10))return finish(m);
    box=(uint16_t)(REG_HMEBOX_0+4U*m->next_h2c);ext=(uint16_t)(REG_HMEBOX_EXT_0+2U*m->next_h2c);
    main[0]=id;
    if(length>3U) {
        main[0]|=0x80U;write_reg(m,ext,1,data[0]);write_reg(m,(uint16_t)(ext+1U),1,data[1]);offset=2;
    }
    for(i=offset;i<length;++i)main[1U+i-offset]=data[i];
    /* Exact DU source write order; extension first, then mailbox bytes0..3. */
    for(i=0;i<4U;++i)write_reg(m,(uint16_t)(box+i),1,main[i]);
    if(m->boot->last_error==RTW_92DU_OK)m->next_h2c=(uint8_t)((m->next_h2c+1U)&3U);
    return finish(m);
}
static int channel_valid(const rtw_92du_boot *b,uint8_t channel) {
    const int band5=rtw_station_5ghz_channel(channel);
    if(!band5 && (channel<1U || channel>11U))return 0;
    return b->board.mode!=RTW_92DU_DUAL_MAC || band5==(b->board.mac_index==0U);
}
rtw_92du_result rtw_92du_build_rate_config(const rtw_92du_boot *boot,uint8_t channel,
    const rtw_92du_peer_rates *peer,rtw_92du_rate_config *out) {
    rtw_92du_rate_config next;uint32_t mask,mcs;unsigned i;
    if(!boot || !peer || !out || !channel_valid(boot,channel) || peer->legacy>0xfffU ||
        !peer->legacy || peer->ht>1U || peer->sgi20>1U || peer->smps>3U || peer->smps==2U ||
        (peer->sgi20 && !peer->ht) || (!peer->ht && peer->mcs) ||
        (boot->board.rf_paths!=1U && boot->board.rf_paths!=2U))return RTW_92DU_BAD_ARGUMENT;
    memset(&next,0,sizeof(next));next.channel=channel;mask=peer->legacy;
    if(channel>14U)mask&=0xff0U;
    if(!mask)return RTW_92DU_BAD_ARGUMENT;
    mcs=peer->ht?peer->mcs:0U;
    if(boot->board.rf_paths==1U || peer->smps!=3U)mcs&=0xffU;
    if(boot->board.mode==RTW_92DU_DUAL_MAC && !peer->sgi20)mcs&=~BIT(7);
    next.ht=(uint8_t)(mcs!=0U);next.sgi20=(uint8_t)(next.ht && peer->sgi20);
    next.mask=mask|(mcs<<12U);
    next.raid=next.ht?(uint8_t)((mask&15U)?((mask&0xff0U)?0:2):1):(uint8_t)((mask&0xff0U)?((mask&15U)?4:5):6);
    for(i=0;i<28U;++i)if(next.mask&BIT(i))next.initial_rate=(uint8_t)i;
    *out=next;return RTW_92DU_OK;
}
rtw_92du_result rtw_92du_mac_set_rates(rtw_92du_mac *m,const rtw_92du_rate_config *r) {
    uint8_t data[5];uint32_t packed;unsigned i,highest=0,raid;
    if(!valid(m) || !r || !r->mask || (r->mask&0xf0000000U) || r->raid>6U ||
        r->initial_rate>27U || !(r->mask&BIT(r->initial_rate)) || r->ht>1U || r->sgi20>1U ||
        (r->sgi20 && !r->ht) || !!(r->mask&0x0ffff000U)!=!!r->ht ||
        !channel_valid(m->boot,r->channel) || (r->channel>14U && (r->mask&15U)) ||
        (m->boot->board.rf_paths==1U && (r->mask&0x0ff00000U)) ||
        (m->boot->board.mode==RTW_92DU_DUAL_MAC && !r->sgi20 && (r->mask&BIT(19))))return RTW_92DU_BAD_ARGUMENT;
    for(i=0;i<28U;++i)if(r->mask&BIT(i))highest=i;
    raid=r->ht?((r->mask&15U)?((r->mask&0xff0U)?0U:2U):1U):((r->mask&0xff0U)?((r->mask&15U)?4U:5U):6U);
    if(!(r->mask&0xfffU) || r->initial_rate!=highest || r->raid!=raid)return RTW_92DU_BAD_ARGUMENT;
    if(!m->started || !rtw_92du_phy_radio_ready(m->boot) || r->channel!=rtw_92du_phy_channel(m->boot))
        return RTW_92DU_WRONG_STATE;
    packed=r->mask|((uint32_t)r->raid<<28U);
    for(i=0;i<4U;++i)data[i]=(uint8_t)(packed>>(8U*i));
    data[4]=(uint8_t)(0x80U|(r->sgi20?BIT(5):0U));m->rate_ready=0;
    if(rtw_92du_h2c(m,6,data,5)!=RTW_92DU_OK)return finish(m);
    m->rates=*r;m->rate_ready=1;return RTW_92DU_OK;
}
static int mac_address_valid(const uint8_t *b) {
    unsigned i,any=0;if(b[0]&1U)return 0;
    for(i=0;i<6U;++i)any|=b[i];return any!=0U;
}
rtw_92du_result rtw_92du_mac_set_state(rtw_92du_mac *m,rtw_92du_station_state state,const rtw_92du_bss *bss) {
    unsigned i,highest=0;uint8_t status;uint32_t filter=receive_config();
    if(!valid(m) || state<RTW_92DU_ST_INIT || state>RTW_92DU_ST_RUN)return RTW_92DU_BAD_ARGUMENT;
    if(state==RTW_92DU_ST_INIT)m->rate_ready=0;
    if(!m->started)return RTW_92DU_WRONG_STATE;
    if(m->boot->last_error!=RTW_92DU_OK)return finish(m);
    if(state>=RTW_92DU_ST_AUTH) {
        if(!bss || !mac_address_valid(bss->bssid) || !channel_valid(m->boot,bss->channel) ||
            bss->short_slot>1U || !bss->basic_rates || bss->basic_rates>0xfffU ||
            (bss->channel>14U && (bss->basic_rates&15U)) || !bss->beacon_interval ||
            (state==RTW_92DU_ST_RUN && (!bss->aid || bss->aid>2007U)))return RTW_92DU_BAD_ARGUMENT;
        if(!rtw_92du_phy_radio_ready(m->boot) || bss->channel!=rtw_92du_phy_channel(m->boot))return RTW_92DU_WRONG_STATE;
        if(state==RTW_92DU_ST_RUN && (!m->rate_ready || m->rates.channel!=bss->channel ||
            (m->state!=RTW_92DU_ST_AUTH && m->state!=RTW_92DU_ST_ASSOC && m->state!=RTW_92DU_ST_RUN) ||
            memcmp(m->bssid,bss->bssid,6U)))return RTW_92DU_WRONG_STATE;
    }
    if(state!=RTW_92DU_ST_RUN)m->rate_ready=0;
    /* The DU's ACF bit12 remains low: otherwise RXFLTMAP1 is bypassed.
     * Clear control admission before disconnect/H2C can fail. The vendor
     * AP-only 0x0400 setting admits PS-POLL (subtype10), not BAR (8). */
    write_reg(m,REG_RXFLTMAP1,2,0);
    if(state==RTW_92DU_ST_INIT && m->boot->phy)(void)rtw_92du_phy_pause(m->boot);
    if(m->joined && state!=RTW_92DU_ST_RUN) {status=0;(void)rtw_92du_h2c(m,2,&status,1);m->joined=0;}
    update(m,MSR,1,3U,0);write_reg(m,REG_RXFLTMAP2,2,0);
    if(state>=RTW_92DU_ST_AUTH) {
        for(i=0;i<6U;++i)write_reg(m,(uint16_t)(REG_BSSID+i),1,bss->bssid[i]);
        write_reg(m,REG_RRSR,2,bss->basic_rates);
        for(i=0;i<12U;++i)if(bss->basic_rates&BIT(i))highest=i;
        if(m->boot->firmware_version>14U)write_reg(m,REG_INIRTS_RATE_SEL,1,highest);
        write_reg(m,REG_SLOT,1,bss->short_slot?9U:20U);
    }
    if(state==RTW_92DU_ST_RUN) {
        write_reg(m,REG_BCN_PSR_RPT,2,0xc000U|bss->aid);
        write_reg(m,REG_BCN_INTERVAL,2,bss->beacon_interval);write_reg(m,REG_ATIMWND,2,2);
        write_reg(m,REG_BCNTCFG,2,0x660f);write_reg(m,REG_RXTSF_OFFSET_CCK,1,0x20);
        write_reg(m,REG_RXTSF_OFFSET_OFDM,1,bss->channel>14U?0x30U:0x20U);
        update(m,REG_BCN_CTRL,1,DIS_TSF_UDT,EN_BCN_FUNCTION);
        filter|=RCR_CBSSID_BCN|RCR_CBSSID_DATA;
        write_reg(m,REG_RCR,4,filter);write_reg(m,REG_RXFLTMAP2,2,0xffff);
        status=1;(void)rtw_92du_h2c(m,2,&status,1);
        update(m,MSR,1,3U,2U);
        write_reg(m,REG_RXFLTMAP1,2,m->rates.ht?0x0100U:0U);
        m->joined=(uint8_t)(m->boot->last_error==RTW_92DU_OK);
    } else {
        write_reg(m,REG_RCR,4,filter);update(m,REG_BCN_CTRL,1,0,DIS_TSF_UDT|EN_BCN_FUNCTION);
    }
    if(m->boot->last_error==RTW_92DU_OK) {
        m->state=state;
        if(state==RTW_92DU_ST_INIT || state==RTW_92DU_ST_SCAN)memset(m->bssid,0,sizeof(m->bssid));
        else memcpy(m->bssid,bss->bssid,6U);
    }
    return finish(m);
}
rtw_92du_result rtw_92du_mac_report_rssi(rtw_92du_mac *m,uint8_t pwdb) {
    const uint8_t data[3]={0,32,pwdb};
    if(!valid(m) || pwdb>100U)return RTW_92DU_BAD_ARGUMENT;
    if(!m->joined || !m->rate_ready)return RTW_92DU_WRONG_STATE;
    return rtw_92du_h2c(m,5,data,3);
}
rtw_92du_result rtw_92du_mac_edca(rtw_92du_mac *m,uint8_t channel,uint8_t short_slot,const rtw_92du_edca_ac *ac) {
    static const uint16_t registers[4]={REG_EDCA_BE_PARAM,REG_EDCA_BK_PARAM,REG_EDCA_VI_PARAM,REG_EDCA_VO_PARAM};
    uint32_t values[4];unsigned i,slot,sifs,aifs;
    if(!valid(m) || !ac || !channel_valid(m->boot,channel) || short_slot>1U)return RTW_92DU_BAD_ARGUMENT;
    if(!m->started)return RTW_92DU_WRONG_STATE;
    slot=channel>14U || short_slot?9U:20U;sifs=channel>14U?16U:10U;
    for(i=0;i<4U;++i) {
        if(!ac[i].aifsn || ac[i].aifsn>15U || ac[i].ecwmin>15U || ac[i].ecwmax>15U || ac[i].ecwmin>ac[i].ecwmax)
            return RTW_92DU_BAD_ARGUMENT;
        aifs=sifs+ac[i].aifsn*slot;if(aifs>255U)return RTW_92DU_BAD_ARGUMENT;
        values[i]=aifs|((uint32_t)ac[i].ecwmin<<8U)|((uint32_t)ac[i].ecwmax<<12U)|((uint32_t)ac[i].txop<<16U);
    }
    write_reg(m,REG_SLOT,1,slot);
    for(i=0;i<4U;++i)write_reg(m,registers[i],4,values[i]);
    write_reg(m,REG_ACMHWCTRL,1,0);return finish(m);
}
/* Teardown is separate from normal poisoned command dispatch. Each cleanup
 * register operation gets one bounded attempt, retaining the first error.
 * Failed reads never supply values to a read/modify/write or ownership test. */
static int stop_read(rtw_92du_mac *m,uint16_t a,uint8_t width,uint32_t *value) {
    m->boot->last_register=a;*value=0;
    if(rtw_usb_rw_read(&m->boot->bus,a,width,value)==RTW_USB_OK)return 1;
    fail(m,RTW_92DU_IO_ERROR);return 0;
}
static void stop_write(rtw_92du_mac *m,uint16_t a,uint8_t width,uint32_t value) {
    m->boot->last_register=a;
    if(rtw_usb_write_register(&m->boot->bus,a,width,value)!=RTW_USB_OK)fail(m,RTW_92DU_IO_ERROR);
}
static void stop_update(rtw_92du_mac *m,uint16_t a,uint8_t width,uint32_t clear,uint32_t set) {
    uint32_t value;
    if(stop_read(m,a,width,&value))stop_write(m,a,width,(value&~clear)|set);
}
static int stop_wait(rtw_92du_mac *m,uint32_t us) {
    if(m->boot->waiter.wait_us(m->boot->waiter.context,us))return 1;
    fail(m,RTW_92DU_WAIT_ERROR);return 0;
}
rtw_92du_result rtw_92du_mac_stop_checked(rtw_92du_mac *m) {
    if(!valid(m))return RTW_92DU_BAD_ARGUMENT;
    const rtw_92du_result original=m->boot->last_error;
    m->boot->last_error=RTW_92DU_OK;
    const rtw_92du_result cleanup=rtw_92du_mac_stop(m);
    if(original!=RTW_92DU_OK)m->boot->last_error=original;
    if(m->cleanup_error==RTW_92DU_OK)m->cleanup_error=cleanup;
    return m->cleanup_error;
}
rtw_92du_result rtw_92du_mac_stop(rtw_92du_mac *m) {
    unsigned i;uint32_t value;int phy_stopped=0;
    const uint16_t own=m && m->boot && m->boot->board.mac_index?REG_MAC1:REG_MAC0;
    const uint16_t other=own==REG_MAC0?REG_MAC1:REG_MAC0;
    const uint32_t own_on=own==REG_MAC0?MAC0_ON:MAC1_ON;
    const uint32_t other_on=other==REG_MAC0?MAC0_ON:MAC1_ON;
    if(!valid(m))return RTW_92DU_BAD_ARGUMENT;
    m->rate_ready=m->joined=0;
    if(m->boot->power_ready) {
        stop_write(m,REG_TXPAUSE,1,0xff);
        if(!stop_read(m,other,1,&value))goto finished;
        if(value&other_on){fail(m,RTW_92DU_MCU_BUSY);goto finished;}
        if(m->boot->phy){(void)rtw_92du_phy_stop(m->boot);phy_stopped=1;}
        stop_update(m,REG_GPIO_MUXCFG,2,BIT(12),0);stop_write(m,REG_TXPAUSE,1,0xff);(void)stop_wait(m,500);
        stop_write(m,REG_CR,1,0);
        stop_update(m,RFPGA0_ANALOGPARAMETER4,4,0x00f00000U,0x00f00000U);
        stop_write(m,REG_APSD_CTRL,1,0x40);stop_write(m,REG_SYS_FUNC_EN,1,FEN_USBD|FEN_USBA|FEN_BB_GLB_RSTN);
        if(m->boot->board.mode==RTW_92DU_SINGLE_MAC || m->boot->board.mac_index) {
            stop_update(m,RFPGA0_XCD_RFPARAMETER,4,0,BIT(31));
            stop_write(m,REG_SYS_FUNC_EN,1,FEN_USBD|FEN_USBA);
        }
        stop_update(m,own,1,own_on,0);
        if(!stop_read(m,other,1,&value))goto finished;
        if(value&other_on){fail(m,RTW_92DU_MCU_BUSY);goto finished;}
        if(m->boot->board.mode==RTW_92DU_DUAL_MAC)stop_update(m,REG_POWER_OFF_IN_PROCESS,1,0,BIT(7));
        stop_write(m,REG_LDOA15_CTRL,1,4);stop_write(m,REG_RF_CTRL,1,0);
        if(stop_read(m,REG_MCUFWDL,1,&value) && (value&BIT(1))) {
            stop_write(m,REG_FSIMR,1,0);stop_write(m,REG_FWIMR,1,0x20);stop_write(m,REG_FTIMR,1,0);
            stop_write(m,REG_MCUFWDL,1,0);stop_write(m,REG_HMETFR+3U,1,0x20);
            for(i=0;i<100U;++i) {
                if(!stop_read(m,REG_SYS_FUNC_EN,2,&value) || !(value&FEN_CPUEN))break;
                if(!stop_wait(m,50))break;
            }
            if(i==100U) {stop_write(m,REG_FWIMR,1,0);stop_write(m,REG_SYS_FUNC_EN+1U,1,0x50);(void)stop_wait(m,10000);}
        }
        stop_write(m,REG_SYS_FUNC_EN+1U,1,0x54);stop_write(m,REG_MCUFWDL,1,0);
        stop_write(m,REG_GPIO_PIN_CTRL+2U,2,0);
        if(stop_read(m,REG_GPIO_PIN_CTRL,4,&value))
            stop_write(m,REG_GPIO_PIN_CTRL,4,(value&~0x00ffff00U)|((value&255U)<<8U)|0x00ff0000U);
        stop_write(m,REG_MAC_PINMUX_CFG,1,0);
        if(stop_read(m,REG_GPIO_IO_SEL,2,&value))
            stop_write(m,REG_GPIO_IO_SEL,2,(value&~0x07f0U)|((value&15U)<<4U)|0x0780U);
        stop_write(m,REG_LEDCFG0,2,0x8888);stop_write(m,REG_LEDCFG2,1,0x88);
        stop_write(m,REG_SPS0_CTRL,1,0x23);stop_write(m,REG_APS_FSMCO,2,APDM_HOST|AFSM_HSUS|PFM_ALDN);
        stop_write(m,REG_RSV_CTRL,1,0x0e);
        if(m->boot->board.mode==RTW_92DU_DUAL_MAC)stop_update(m,REG_POWER_OFF_IN_PROCESS,1,BIT(7),0);
    }
finished:
    if(m->boot->phy && !phy_stopped){
        /* Invalidate PHY authority without RF sleep/global writes when the
         * sibling is active, unreadable, or our power is already gone. */
        const rtw_92du_result error=m->boot->last_error;
        m->boot->last_error=error==RTW_92DU_OK?RTW_92DU_WRONG_STATE:error;
        (void)rtw_92du_phy_stop(m->boot);m->boot->last_error=error;
    }
    m->prepared=m->started=0;m->boot->power_ready=m->boot->firmware_ready=0;
    m->cleanup_done=1;
    m->state=RTW_92DU_ST_INIT;return finish(m);
}
