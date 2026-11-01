/* SPDX-License-Identifier: BSD-3-Clause
 * B/C runtime coexistence. The whole device, policy, firmware mailbox and
 * these timer/state callbacks share the controller's serialized workloop.
 */
bool RTWUSBController::coex88Ready() const
{
    if(!owns88Authority(true))return false;
    if(!r88Coex_)return !r88Core_.board.btcoex;
    return rtw88_coex_ready(r88Coex_,&r88PHY_)!=0;
}
rtw88_coex_input RTWUSBController::coex88Input(ieee80211_state target,uint8_t channel) const
{
    const auto&ic=core_.sc_ic;const auto*n=ic.ic_bss;
    rtw88_coex_input input{};input.now_ms=nowMS();
    input.primary=input.center=channel?channel:r88Channel_?r88Channel_:1;
    input.width_mhz=20;input.connected=target==IEEE80211_S_RUN;
    input.scanning=target==IEEE80211_S_SCAN;input.rssi_percent=r88RSSIValid_?r88RSSI_:0;
    input.beacon_interval=n?n->ni_intval:0;
    input.scan_ap_count=(uint16_t)(ic.ic_nnodes<0?0:ic.ic_nnodes>65535?65535:ic.ic_nnodes);
    input.tx_bytes=r88CoexTXBytes_;input.rx_bytes=r88CoexRXBytes_;
    input.tx_packets=r88CoexTXPackets_;input.rx_packets=r88CoexRXPackets_;
    input.rx_rate=r88CoexRXRate_;input.tx_rate=r88CoexTXRate_;return input;
}
int RTWUSBController::coex88Start()
{
    r88CoexScanning_=r88CoexOneSS_=r88SMPSPending_=r88SMPSWaitingUSB_=false;
    r88CoexTXBytes_=r88CoexRXBytes_=r88CoexTXPackets_=r88CoexRXPackets_=0;
    r88CoexRXRate_=r88CoexTXRate_=2;r88CoexWatchdogAt_=0;
    core_.sc_ic.ic_htcaps=(core_.sc_ic.ic_htcaps&~(uint32_t)IEEE80211_HTCAP_SMPS_MASK)|
        (IEEE80211_HTCAP_SMPS_DIS<<IEEE80211_HTCAP_SMPS_SHIFT);
    if(!r88Core_.board.btcoex)return 0;
    if(!r88Storage_||!r88CoexTimer_)return ENXIO;
    auto*storage=static_cast<uint8_t*>(r88Storage_)+R88CoexOffset;
    int error=rtw88_coex_init(storage,RTW88_COEX_SCRATCH_BYTES,&r88PHY_,coex88Mimo,this,&r88Coex_);
    auto input=coex88Input(IEEE80211_S_INIT);
    if(!error)error=rtw88_coex_start(r88Coex_,&input);
    if(!error){r88CoexWatchdogAt_=input.now_ms+2000;coex88Schedule();}
    return error;
}
int RTWUSBController::coex88Notify(rtw88_coex_event event,ieee80211_state target,uint8_t channel)
{
    if(!r88Coex_)return r88Core_.board.btcoex?ENXIO:0;
    auto input=coex88Input(target,channel);
    int error=rtw88_coex_notify(r88Coex_,event,&input);
    if(!error){if(event==RTW88_CX_SCAN_START)r88CoexScanning_=true;
        if(event==RTW88_CX_SCAN_FINISH)r88CoexScanning_=false;coex88Schedule();}
    return error;
}
void RTWUSBController::coex88Stop()
{
    if(r88CoexTimer_)r88CoexTimer_->cancelTimeout();
    if(r88Coex_){if(!ioError(this)&&r88Core_.firmware.powered)(void)rtw88_coex_stop(r88Coex_);
        else rtw88_coex_forget(r88Coex_);}
    r88Coex_=nullptr;r88CoexScanning_=r88CoexOneSS_=r88SMPSPending_=r88SMPSWaitingUSB_=false;
    r88CoexWatchdogAt_=r88SMPSDeadline_=0;
}
void RTWUSBController::coex88Schedule()
{
    if(!r88CoexTimer_||!r88Coex_||!running_||stopping_||!present_||sleeping_)return;
    if(core_.core_error){r88CoexTimer_->setTimeoutMS(1);return;}
    uint64_t due=rtw88_coex_next_deadline(r88Coex_);
    if(r88CoexWatchdogAt_&&(!due||r88CoexWatchdogAt_<due))due=r88CoexWatchdogAt_;
    if(r88SMPSPending_&&r88SMPSDeadline_&&(!due||r88SMPSDeadline_<due))due=r88SMPSDeadline_;
    if(!due)return;uint64_t now=nowMS(),wait=due>now?due-now:1;
    r88CoexTimer_->setTimeoutMS((UInt32)(wait>2000?2000:wait));
}
int RTWUSBController::coex88Tick()
{
    if(!r88Coex_||!running_||stopping_||ioError(this))return ENXIO;
    if(core_.core_error)return core_.core_error;
    const uint64_t now=nowMS();
    if(r88SMPSPending_&&r88SMPSDeadline_&&now>=r88SMPSDeadline_)return ETIMEDOUT;
    int error=rtw88_coex_poll(r88Coex_,now);
    if(!error&&now>=r88CoexWatchdogAt_){auto input=coex88Input(core_.sc_ic.ic_state);
        error=rtw88_coex_watchdog(r88Coex_,&input);
        if(!error){r88CoexTXBytes_=r88CoexRXBytes_=r88CoexTXPackets_=r88CoexRXPackets_=0;
            r88CoexWatchdogAt_=now+2000;}}
    if(!error)coex88Schedule();return error;
}
void RTWUSBController::coex88TimerAction(OSObject*object,IOTimerEventSource*)
{
    auto*self=static_cast<RTWUSBController*>(object);
    if(!self->running_||self->stopping_||!self->r88Coex_)return;
    /* Even an idle poll checks MCU health over EP0. Keep that request out
     * of the interval reserved for clearing and draining the RX endpoint. */
    if(self->rxRecovery_.pending){
        if(self->r88CoexTimer_)self->r88CoexTimer_->setTimeoutMS(100);
        return;
    }
    int error=self->coex88Tick();
    if(error){self->core_.core_error=error;self->quiesce(false);}
}
int RTWUSBController::prepare88Rate(rtw88_station_rate*out) const
{
    const auto&ic=core_.sc_ic;const auto*n=ic.ic_bss;
    if(!out||!n)return EINVAL;uint8_t mcs[2]{};
    const bool ht=(n->ni_flags&IEEE80211_NODE_HT)!=0;unsigned streams=r88Core_.board.tx_streams;
    if(ht){mcs[0]=n->ni_rxmcs[0];mcs[1]=n->ni_rxmcs[1];
        if(r88CoexOneSS_||((n->ni_htcaps&IEEE80211_HTCAP_SMPS_MASK)>>IEEE80211_HTCAP_SMPS_SHIFT)!=IEEE80211_HTCAP_SMPS_DIS){mcs[1]=0;streams=1;}}
    return rtw88_station_prepare_ht20(family_,r88Channel_,streams,station88Rates(false),mcs,ht,
        (ic.ic_htcaps&IEEE80211_HTCAP_SGI20)!=0,(n->ni_htcaps&IEEE80211_HTCAP_SGI20)!=0,out);
}
int RTWUSBController::send88SMPS(bool enabled)
{
    auto&ic=core_.sc_ic;auto*n=ic.ic_bss;
    if(ic.ic_state!=IEEE80211_S_RUN||!n||!(n->ni_flags&IEEE80211_NODE_HT))return 0;
    uint64_t now=nowMS();bool found=false;
    for(unsigned i=0;i<64;++i){r88SMPSNextTag_=(uint8_t)((r88SMPSNextTag_+4U)&0xfcU);
        if(now>=r88SMPSTagRetire_[r88SMPSNextTag_>>2]){found=true;break;}}
    if(!found)return EAGAIN;
    mbuf_t packet=nullptr;if(mbuf_gethdr(MBUF_DONTWAIT,MT_DATA,&packet)||!packet)return ENOBUFS;
    mbuf_setlen(packet,27);mbuf_pkthdr_setlen(packet,27);
    auto*bytes=static_cast<uint8_t*>(mbuf_data(packet));bzero(bytes,27);
    auto*header=reinterpret_cast<ieee80211_frame*>(bytes);
    header->i_fc[0]=0xd0;memcpy(header->i_addr1,n->ni_bssid,6);
    memcpy(header->i_addr2,ic.ic_myaddr,6);memcpy(header->i_addr3,n->ni_bssid,6);
    const uint16_t sequence=n->ni_txseq&0xfffU;n->ni_txseq=(uint16_t)((sequence+1U)&0xfffU);
    header->i_seq[0]=(uint8_t)(sequence<<4);header->i_seq[1]=(uint8_t)(sequence>>4);
    // HT category7,SMPS action1; static1 or disabled0. Host sequence stays independent of CCX.
    bytes[24]=7;bytes[25]=1;bytes[26]=enabled?1:0;
    r88SMPSPending_=r88SMPSWaitingUSB_=true;r88SMPSTag_=r88SMPSNextTag_;
    r88SMPSDesired_=enabled?1:0;r88SMPSSequence_=sequence;r88SMPSAID_=n->ni_associd;
    memcpy(r88SMPSBSSID_,n->ni_bssid,6);r88SMPSDeadline_=now+1000;
    r88SMPSTagRetire_[r88SMPSTag_>>2]=now+1500;
    ieee80211_ref_node(n);int error=send88Frame(packet,n);
    if(error){ieee80211_release_node(&ic,n);r88SMPSPending_=r88SMPSWaitingUSB_=false;r88SMPSDeadline_=0;}
    coex88Schedule();return error;
}
int RTWUSBController::coex88Mimo(void*cookie,int enabled)
{
    auto*self=owner(cookie);if(!self||(enabled!=0&&enabled!=1)||self->family_!=RTW_WIFI_RTL8822CU||ioError(cookie))return EINVAL;
    if(self->r88CoexOneSS_==(enabled!=0))return 0;
    uint32_t pause=0;int error=r88Read(self,0x522,1,&pause);
    if(!error)error=r88Write(self,0x522,1,255);
    if(!error)error=rtw88_phy_radio_set_txrx_1ss(&self->r88PHY_,enabled);
    if(!error){self->r88CoexOneSS_=enabled!=0;self->r88RX_.rf_paths=(uint8_t)(enabled?1:self->r88Core_.board.active_rf_paths);
        if(self->r88Media_&&self->core_.sc_ic.ic_state==IEEE80211_S_RUN){rtw88_station_rate rate{};
            error=self->prepare88Rate(&rate);if(!error)error=rtw88_station_h2c(&self->r88Station_,rate.command);
            if(!error){self->r88Rates_=rate;error=self->send88SMPS(enabled!=0);}}else{
            auto&caps=self->core_.sc_ic.ic_htcaps;caps&=~(uint32_t)IEEE80211_HTCAP_SMPS_MASK;
            if(!enabled)caps|=IEEE80211_HTCAP_SMPS_DIS<<IEEE80211_HTCAP_SMPS_SHIFT;}}
    if(!error)error=r88Write(self,0x522,1,pause);
    if(error)self->core_.core_error=error;return error;
}
bool RTWUSBController::handle88SMPSReport(const uint8_t*bytes,size_t length)
{
    uint8_t token=0;int acked=0;int parsed=rtw88_coex_parse_tx_status(bytes,length,&token,&acked);
    if(parsed==ENOMSG)return false;if(parsed)return true;
    auto&ic=core_.sc_ic;auto*n=ic.ic_bss;
    if(!r88SMPSPending_||r88SMPSWaitingUSB_||token!=r88SMPSTag_||!n||
       ic.ic_state!=IEEE80211_S_RUN||n->ni_associd!=r88SMPSAID_||memcmp(n->ni_bssid,r88SMPSBSSID_,6)||
       !coex88Ready()||nowMS()>=r88SMPSDeadline_)return true;
    r88SMPSPending_=false;r88SMPSDeadline_=0;
    // USB completion never establishes AP acceptance. Hardware retries precede CCX.
    if(!acked)core_.core_error=ENOLINK;
    else{ic.ic_htcaps&=~(uint32_t)IEEE80211_HTCAP_SMPS_MASK;
        if(!r88SMPSDesired_)ic.ic_htcaps|=IEEE80211_HTCAP_SMPS_DIS<<IEEE80211_HTCAP_SMPS_SHIFT;}
    coex88Schedule();return true;
}
