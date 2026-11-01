// SPDX-License-Identifier: GPL-2.0-only
bool RTWUSBController::usesFirmwareRate() const {
    const auto&ic=core_.sc_ic;
    return family_==RTW_WIFI_RTL8192EU&&running_&&!stopping_&&ic.ic_state==IEEE80211_S_RUN&&core_.fw_station_connected&&
        core_.rate_config.ht&&ic.ic_bss&&(ic.ic_bss->ni_flags&IEEE80211_NODE_HT);
}
int RTWUSBController::ampduRXStart(ieee80211com*ic,ieee80211_node*ni,uint8_t tid) {
    if(!ic||!ni||tid>=16)return EINVAL;
    auto*core=static_cast<rtwn_softc*>(ic->ic_softc);auto*s=owner(core->sc_ops.cookie);
    const auto&ba=ni->ni_rx_ba[tid];
    return (s->usesFirmwareRate()||(s->uses8814FirmwareRate()&&s->acRateConfig_.ht)||(s->uses88FirmwareRate()&&s->r88Rates_.rates.ht)||s->uses8812HighThroughput()||s->uses8192FUHighThroughput()||s->uses8710BUHighThroughput()||(s->uses8723FirmwareRate()&&(s->r23Rates_.mask&0xff000U)))&&s->authorized()&&ni==ic->ic_bss&&ba.ba_buf&&
        ba.ba_winsize>=1&&ba.ba_winsize<=IEEE80211_BA_MAX_WINSZ?0:EINVAL;
}
int RTWUSBController::ampduTXStart(ieee80211com*ic,ieee80211_node*ni,uint8_t tid) {
    if(!ic||!ni||tid>=16)return EINVAL;
    auto*core=static_cast<rtwn_softc*>(ic->ic_softc);auto*s=owner(core->sc_ops.cookie);
    if(s->is8710BUFamily())return ampdu8710BUTXStart(ic,ni,tid);
    if(s->is8192FUFamily())return ampdu8192FUTXStart(ic,ni,tid);
    if(s->is8812Family())return ampdu8812TXStart(ic,ni,tid);
    const auto&ba=ni->ni_tx_ba[tid];
    if(s->family_==RTW_WIFI_RTL8814AU){
        if(tid>=8||!s->uses8814FirmwareRate()||!s->acRateConfig_.ht||!(ni->ni_flags&IEEE80211_NODE_HT)||
           !s->authorized()||ni!=ic->ic_bss||!s->present_||s->usbError_!=kIOReturnSuccess||
           !s->acCore_.phy_initialized||s->acCore_.io_error||(ic->ic_tid_noack&(1U<<tid))||
           ba.ba_winsize<1||ba.ba_winsize>IEEE80211_BA_MAX_WINSZ)return EINVAL;
        /* Pinned Linux rtw8814a_set_ampdu_factor: without the peer byte cap,
         * some 11n APs cannot handle 8814A's aggregate and TX becomes slow.
         * HT factor is0..3 here. A failed write refuses BA; no cached enable
         * survives stop/reset, since each descriptor reads the live host BA. */
        const unsigned factor=ni->ni_ampdu_param&IEEE80211_AMPDU_PARAM_LE;
        return rtw_8814au_write(&s->acCore_,0x458,4,(8192U<<factor)-1U);
    }
    if(!s->usesFirmwareRate()||!s->authorized()||ni!=ic->ic_bss||(ic->ic_tid_noack&(1U<<tid))||ba.ba_winsize<1||
       ba.ba_winsize>IEEE80211_BA_MAX_WINSZ)return EINVAL;
    return rtwn_r92e_ampdu_params(core,ni->ni_ampdu_param&IEEE80211_AMPDU_PARAM_LE,
        (ni->ni_ampdu_param&IEEE80211_AMPDU_PARAM_SS)>>2);
}
void RTWUSBController::ampduRXStop(ieee80211com*,ieee80211_node*,uint8_t) {
    /* net80211 owns the receive window and its timers. No hardware BA table. */
}
void RTWUSBController::ampduTXStop(ieee80211com*,ieee80211_node*,uint8_t) {
    /* Each descriptor checks its TID's live agreement. Keep other TIDs active. */
}
