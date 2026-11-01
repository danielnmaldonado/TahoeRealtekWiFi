/* SPDX-License-Identifier: BSD-3-Clause */
/* A managed station follows an observed AP. It never performs AP radar/CAC. */
bool RTWUSBController::dfsTXBlocked() const {return dfsStation_.blocked!=0||dfsTXHold_;}
void RTWUSBController::resetDFSStation(bool releaseTX) {
    if(releaseTX)dfsTXHold_=false;
    dfsAppliedReduction_=0;dfsAppliedLimitMBM_=INT32_MAX;
    rtw_dfs_station_reset(&dfsStation_);dfsTunedChannel_=dfsBoundChannel_=0;dfsBound_=false;
    memset(dfsBoundBSSID_,0,sizeof(dfsBoundBSSID_));
}
int RTWUSBController::candidateDFSPower(uint8_t channel,int32_t*limit,uint8_t*constraint) const {
    const auto&ic=core_.sc_ic;
    if(!limit||!constraint||!ic.ic_bss||!rtw_dfs_channel(channel)||ic.ic_des_esslen<=0||ic.ic_des_esslen>32)return EACCES;
    const int16_t factory=factoryDFSChannelLimitMBM(channel);
    if(factory==RTW_DFS_REGULATORY_UNAVAILABLE)return EACCES;
    int32_t maximum=factory<2000?factory:2000;
    const int ap=rtw_dfs_station_power_limit(&dfsStation_,ic.ic_bss->ni_bssid,ic.ic_des_essid,static_cast<size_t>(ic.ic_des_esslen),channel,nowMS());
    const int reduction=rtw_dfs_station_power_reduction(&dfsStation_,ic.ic_bss->ni_bssid,ic.ic_des_essid,static_cast<size_t>(ic.ic_des_esslen),channel,nowMS());
    if(ap==RTW_DFS_AP_POWER_INVALID||reduction<0)return EACCES;
    if(ap!=RTW_DFS_AP_POWER_UNSPECIFIED&&(int32_t)ap*100<maximum)maximum=(int32_t)ap*100;
    /* 802.11h constrains the local maximum. It is not a second mandatory
     * subtraction from a rate already below that maximum. */
    *limit=maximum-(int32_t)reduction*100;*constraint=(uint8_t)reduction;return 0;
}
int RTWUSBController::prepareDFSPower(unsigned channel) {
    uint8_t reduction=0;int32_t limit=INT32_MAX;
    if(channel<=255&&rtw_dfs_channel((uint8_t)channel)){
        const int candidate=candidateDFSPower((uint8_t)channel,&limit,&reduction);if(candidate)return candidate;
    }
    int error=0;
    if(family_==RTW_WIFI_RTL8814AU){
        int32_t dbm=RTW_8814AU_POWER_LIMIT_DISABLED;
        if(limit!=INT32_MAX){dbm=limit/100;if(limit<0&&limit%100)--dbm;if(dbm< -32||dbm>31)return ERANGE;}
        error=rtw_8814au_phy_set_power_limit_dbm(&acCore_,(int8_t)dbm);
        if(!error)error=rtw_8814au_phy_set_power_constraint(&acCore_,0);
    }else if(is8812Family()){
        error=rtw_r12au_phy_set_power_limit_mbm(auPHY_,limit);
        if(!error)error=rtw_r12au_phy_set_power_reduction(auPHY_,0);
    }else if(is88Family()){
        error=rtw88_phy_radio_set_power_limit_mbm(&r88PHY_,limit);
        if(!error)error=rtw88_phy_radio_set_power_reduction(&r88PHY_,0);
    }else if(family_==RTW_WIFI_RTL8192DU){
        error=set8192DUPowerLimitMBM(limit);
        if(!error)error=set8192DUPowerReduction(0);
    }
    if(!error){dfsAppliedReduction_=(uint8_t)reduction;dfsAppliedLimitMBM_=limit;}
    return error;
}
bool RTWUSBController::dfsStateAllowed(ieee80211_state state,unsigned channel) const {
    if(state==IEEE80211_S_INIT)return true;
    if(dfsTXBlocked())return false;
    if(channel>255||!rtw_dfs_channel((uint8_t)channel)||
       (state!=IEEE80211_S_AUTH&&state!=IEEE80211_S_ASSOC&&state!=IEEE80211_S_RUN))return true;
    const auto&ic=core_.sc_ic;const auto*n=ic.ic_bss;
    if(!(ic.ic_channels[channel].ic_flags&IEEE80211_CHAN_5GHZ)||!permitsDFSChannelByCountry((uint8_t)channel))return false;
    if(!n||!joinRequested_||ic.ic_des_esslen<=0||ic.ic_des_esslen>32||n->ni_esslen!=ic.ic_des_esslen||memcmp(n->ni_essid,ic.ic_des_essid,static_cast<size_t>(ic.ic_des_esslen)))return false;
    if(state==IEEE80211_S_AUTH)return rtw_dfs_station_authorize(&dfsStation_,n->ni_bssid,ic.ic_des_essid,static_cast<size_t>(ic.ic_des_esslen),(uint8_t)channel,nowMS())!=0;
    return dfsBound_&&dfsBoundChannel_==channel&&!memcmp(dfsBoundBSSID_,n->ni_bssid,6);
}
void RTWUSBController::dfsStateChanged(ieee80211_state state,unsigned channel,int error) {
    if(error){dfsTunedChannel_=0;dfsBound_=false;return;}
    if(state==IEEE80211_S_INIT){dfsTunedChannel_=0;dfsBound_=false;return;}
    if(state==IEEE80211_S_SCAN||state==IEEE80211_S_AUTH)dfsTunedChannel_=(uint8_t)channel;
    if(state==IEEE80211_S_SCAN)dfsBound_=false;
    if(state==IEEE80211_S_AUTH){dfsBound_=rtw_dfs_channel((uint8_t)channel)!=0;dfsBoundChannel_=(uint8_t)channel;if(dfsBound_&&core_.sc_ic.ic_bss)memcpy(dfsBoundBSSID_,core_.sc_ic.ic_bss->ni_bssid,6);}
}
bool RTWUSBController::dfsFrameAllowed(mbuf_t packet,ieee80211_node*node) const {
    if(dfsTXBlocked())return false;
    const auto&ic=core_.sc_ic;const unsigned channel=ic.ic_bss&&ic.ic_bss->ni_chan&&ic.ic_bss->ni_chan!=IEEE80211_CHAN_ANYC?
        ieee80211_chan2ieee(const_cast<ieee80211com*>(&ic),ic.ic_bss->ni_chan):dfsTunedChannel_;
    if(!rtw_dfs_channel((uint8_t)channel))return true;
    if(ic.ic_des_esslen<=0||ic.ic_des_esslen>32)return false;
    if(!packet||!node||!ic.ic_bss||node!=ic.ic_bss||ic.ic_state==IEEE80211_S_SCAN)return false;
    ieee80211_frame header{};
    if(mbuf_pkthdr_len(packet)<sizeof(header)||mbuf_copydata(packet,0,sizeof(header),&header))return false;
    if(header.i_fc[0]==(IEEE80211_FC0_TYPE_MGT|IEEE80211_FC0_SUBTYPE_PROBE_REQ))return false;
    if(memcmp(header.i_addr1,ic.ic_bss->ni_bssid,6)&&!(header.i_addr1[0]&1))return false;
    /* Before RUN only directed AUTH/ASSOC management to the observed AP. */
    if(ic.ic_state!=IEEE80211_S_RUN){
        const uint8_t subtype=header.i_fc[0];
        if((subtype!=0xb0&&subtype!=0x00&&subtype!=0x20)||memcmp(header.i_addr1,ic.ic_bss->ni_bssid,6)||memcmp(header.i_addr3,ic.ic_bss->ni_bssid,6))return false;
        return rtw_dfs_station_authorize(&dfsStation_,node->ni_bssid,ic.ic_des_essid,static_cast<size_t>(ic.ic_des_esslen),(uint8_t)channel,nowMS())!=0;
    }
    return dfsBound_&&dfsBoundChannel_==channel&&!memcmp(dfsBoundBSSID_,node->ni_bssid,6);
}
int RTWUSBController::pauseDFSStationTX() {
    /* Linux REG_TXPAUSE covers every TX queue. Use each family's typed bus,
     * never cast between chip contexts or stop merely host Ethernet output. */
    if(family_==RTW_WIFI_RTL8814AU)return rtw_8814au_write(&acCore_,0x522,1,0xff);
    if(is8812Family())return rtw_r12au_write(&auCore_,0x522,1,0xff);
    if(is88Family())return r88Write(this,0x522,1,0xff);
    if(family_==RTW_WIFI_RTL8192DU)return rtw_92du_phy_pause(&duCore_)==RTW_92DU_OK?0:EIO;
    return writeRegister(0x522,1,0xff);
}
void RTWUSBController::observeDFSFrame(const uint8_t*frame,size_t length,unsigned physicalChannel) {
    auto&ic=core_.sc_ic;
    if(!attached_||!running_||stopping_||!present_||sleeping_||usbError_!=kIOReturnSuccess||core_.core_error||!joinRequested_||!dfsTunedChannel_||ic.ic_des_esslen<=0||ic.ic_des_esslen>32||physicalChannel>255)return;
    const auto*peer=ic.ic_bss;
    const unsigned selected=peer&&peer->ni_chan&&peer->ni_chan!=IEEE80211_CHAN_ANYC?ieee80211_chan2ieee(&ic,peer->ni_chan):0;
    const bool phase=ic.ic_state==IEEE80211_S_AUTH||ic.ic_state==IEEE80211_S_ASSOC||ic.ic_state==IEEE80211_S_RUN;
    const bool bound=phase&&peer&&selected==dfsTunedChannel_&&dfsBound_&&dfsBoundChannel_==dfsTunedChannel_&&!memcmp(dfsBoundBSSID_,peer->ni_bssid,6);
    /* RUN on a non-DFS channel also honors its AP's CSA. On DFS, early
     * control frames need the same BSSID/channel binding established at AUTH. */
    const uint8_t*associated=bound||(peer&&selected==dfsTunedChannel_&&ic.ic_state==IEEE80211_S_RUN&&!rtw_dfs_channel(dfsTunedChannel_))?peer->ni_bssid:nullptr;
    const int observed=rtw_dfs_station_observe(&dfsStation_,frame,length,dfsTunedChannel_,physicalChannel<=255?(uint8_t)physicalChannel:0,
       ic.ic_des_essid,static_cast<size_t>(ic.ic_des_esslen),ic.ic_myaddr,associated,nowMS());
    bool changedPower=false;
    if(observed==RTW_DFS_OBSERVED&&associated&&rtw_dfs_channel(dfsTunedChannel_)){
        int32_t limit=0;uint8_t reduction=0;
        changedPower=candidateDFSPower(dfsTunedChannel_,&limit,&reduction)!=0||limit<dfsAppliedLimitMBM_;
    }
    if(observed==RTW_DFS_SWITCH||changedPower)stopDFSStation();
}
void RTWUSBController::stopDFSStation() {
    auto&ic=core_.sc_ic;dfsStation_.blocked=dfsStation_.pending=1;dfsTXHold_=true;
    dfsBound_=false;
    if(ic.ic_bss)ic.ic_bss->ni_port_valid=0;
    if(auto*q=getOutputQueue()){q->stop();q->flush();}
    ifq_flush(&ic.ic_if.if_snd);
    ieee80211_set_link_state(&ic,LINK_STATE_DOWN);super::setLinkStatus(kIONetworkLinkValid);
    if(scanTimer_)scanTimer_->cancelTimeout();if(calibTimer_)calibTimer_->cancelTimeout();
    const int error=pauseDFSStationTX();if(error&&!core_.core_error)core_.core_error=error;
    /* RX still owns node references. Defer key/node/USB teardown to the
     * serialized watchdog instead of recursively freeing them here. */
    if(watchdogTimer_)watchdogTimer_->setTimeoutMS(1);
}
void RTWUSBController::advanceDFSRecovery() {
    if(!dfsStation_.pending){
        if(dfsTXHold_&&(!present_||sleeping_||usbError_!=kIOReturnSuccess||core_.core_error))quiesce(false);
        return;
    }
    const bool restart=attached_&&running_&&enabled_&&joinRequested_&&present_&&!stopping_&&!sleeping_&&usbError_==kIOReturnSuccess&&!core_.core_error;
    quiesce(false);
    if(restart)(void)enableStation();
}
