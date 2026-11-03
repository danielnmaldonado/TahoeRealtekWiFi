#include "rtw/StationChannels.h"
/* SPDX-License-Identifier: BSD-3-Clause */
/* Native RTL8812AU/RTL8821AU station adapter. Actual family-typed bootstrap,
 * LLT/MAC/8051 and PHY/RFK precede management transmission. The imported
 * net80211 stack owns scanning/authentication, WPA2 software CCMP and port
 * authorization. HT/VHT capabilities follow actual ROM operating-power profiles.
 * No rtwn hardware implementation or RTL8814AU context is used here. */
#include "RTL8812AUFirmware.inc"
#include "RTL8821AUFirmware.inc"
static uint16_t rtw8812RateMask(const ieee80211_rateset&rates,bool basicOnly)
{
    static const uint8_t values[12]={2,4,11,22,12,18,24,36,48,72,96,108};
    if(rates.rs_nrates>sizeof(rates.rs_rates))return 0;
    uint16_t mask=0;
    for(unsigned i=0;i<rates.rs_nrates;++i){
        if(basicOnly&&!(rates.rs_rates[i]&IEEE80211_RATE_BASIC))continue;
        uint8_t rate=rates.rs_rates[i]&IEEE80211_RATE_VAL;
        for(unsigned j=0;j<12;++j)if(rate==values[j])mask|=(uint16_t)(1U<<j);
    }
    return mask;
}
static bool rtw8812SamePHYIdentity(const rtw_r12au_phy_board&a,const rtw_r12au_phy_board&b)
{
    return a.chip==b.chip&&a.cut==b.cut&&a.rfe==b.rfe&&a.crystal==b.crystal&&a.thermal==b.thermal&&a.rf_paths==b.rf_paths&&a.package_type==b.package_type&&a.swing_2g==b.swing_2g&&a.swing_5g==b.swing_5g&&a.ext_pa_2g==b.ext_pa_2g&&a.ext_pa_5g==b.ext_pa_5g&&a.ext_lna_2g==b.ext_lna_2g&&a.ext_lna_5g==b.ext_lna_5g&&a.gpa_type==b.gpa_type&&a.apa_type==b.apa_type&&a.glna_type==b.glna_type&&a.alna_type==b.alna_type&&a.btcoex==b.btcoex&&!memcmp(a.txpwr_idx,b.txpwr_idx,sizeof(a.txpwr_idx));
}
void RTWUSBController::remember8812Authority()
{
    auAuthorityBound_=false;
    if(!auCore_.identified||!is8812Family()||auCore_.chip!=(family_==RTW_WIFI_RTL8812AU?RTW_R12AU_CHIP_8812AU:RTW_R12AU_CHIP_8821AU))return;
    auAuthority_=auCore_;auAuthorityFamily_=family_;auAuthorityProvider_=usb_;auAuthorityRX_=rxPipe_;auAuthorityEndpoints_=endpoints_;
    for(unsigned i=0;i<4;++i){auAuthorityTX_[i]=txPipes_[i];auAuthorityMPS_[i]=txMPS_[i];}auAuthorityBound_=true;
}
bool RTWUSBController::owns8812Authority() const
{
    if(!auAuthorityBound_||!auCore_.identified||!usbOpened_||!is8812Family()||family_!=auAuthorityFamily_||auCore_.chip!=auAuthority_.chip||usb_!=auAuthorityProvider_||rxPipe_!=auAuthorityRX_)return false;
    for(unsigned i=0;i<4;++i)if(txPipes_[i]!=auAuthorityTX_[i]||txMPS_[i]!=auAuthorityMPS_[i])return false;
    const auto&a=auAuthority_;const auto&b=auCore_;const auto&x=a.board;const auto&y=b.board;const auto&e=auAuthorityEndpoints_;const auto&f=endpoints_;
    return a.bus.context==b.bus.context&&a.bus.control_in==b.bus.control_in&&a.bus.control_out==b.bus.control_out&&a.waiter.context==b.waiter.context&&a.waiter.wait_us==b.waiter.wait_us&&
        rtw8812SamePHYIdentity(x.phy,y.phy)&&!memcmp(x.mac,y.mac,sizeof(x.mac))&&!memcmp(x.country,y.country,sizeof(x.country))&&x.channel_plan==y.channel_plan&&x.regd==y.regd&&x.bt_setting==y.bt_setting&&x.share_ant==y.share_ant&&x.vht==y.vht&&x.eeprom_vendor==y.eeprom_vendor&&x.eeprom_product==y.eeprom_product&&x.sys_cfg==y.sys_cfg&&
        !memcmp(a.logical_rom,b.logical_rom,sizeof(a.logical_rom))&&a.usb.speed==b.usb.speed&&a.usb.bulk_max_packet==b.usb.bulk_max_packet&&a.usb.bulk_in==b.usb.bulk_in&&a.usb.bulk_out_count==b.usb.bulk_out_count&&!memcmp(a.usb.bulk_out,b.usb.bulk_out,sizeof(a.usb.bulk_out))&&
        e.interface_number==f.interface_number&&e.alternate_setting==f.alternate_setting&&e.num_bulk_in==f.num_bulk_in&&e.num_bulk_out==f.num_bulk_out&&e.num_endpoints==f.num_endpoints&&!memcmp(e.bulk_in,f.bulk_in,sizeof(e.bulk_in))&&!memcmp(e.bulk_out,f.bulk_out,sizeof(e.bulk_out));
}
bool RTWUSBController::is8812Family() const
{
    return family_==RTW_WIFI_RTL8812AU||family_==RTW_WIFI_RTL8821AU;
}
bool RTWUSBController::supports8812Band(bool five) const
{
    if(!is8812Family()||!auCore_.identified)return false;
    if(!(rtw_usb_wifi_profile_allowed_bands(profile_)&(five?2U:1U)))return false;
    static const uint8_t channels5[]={36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165};
    if(five){for(uint8_t ch:channels5)if(rtw_r12au_phy_channel_supported(&auCore_.board.phy,ch,20))return true;}
    else for(uint8_t ch=1;ch<=11;++ch)if(rtw_r12au_phy_channel_supported(&auCore_.board.phy,ch,20))return true;
    return false;
}
rtw_usb_error RTWUSBController::auControlIn(void*p,const rtw_usb_setup*s,uint8_t*b,uint16_t*actual)
{
    if(actual)*actual=0;
    if(!p||!s||!b||!actual||s->bm_request_type!=0xc0||s->b_request!=5||s->w_index||
       (s->w_length!=1&&s->w_length!=2&&s->w_length!=4))return RTW_USB_INVALID_ARGUMENT;
    int e=owner(p)->vendorRequest(true,s->w_value,b,s->w_length);
    if(e)return RTW_USB_IO_ERROR;*actual=s->w_length;return RTW_USB_OK;
}
rtw_usb_error RTWUSBController::auControlOut(void*p,const rtw_usb_setup*s,const uint8_t*b,uint16_t*actual)
{
    if(actual)*actual=0;
    if(!p||!s||!b||!actual||s->bm_request_type!=0x40||s->b_request!=5||s->w_index||!s->w_length||s->w_length>196)return RTW_USB_INVALID_ARGUMENT;
    /* writeRegion copies immutable firmware bytes into bounded writable USB
     * request storage. Scalar register and196-byte firmware transfers use the
     * same verified endpoint0 transport, not a reserved-page bulk path. */
    int e=writeRegion(p,s->w_value,b,s->w_length);
    if(e)return RTW_USB_IO_ERROR;*actual=s->w_length;return RTW_USB_OK;
}
int RTWUSBController::auWait(void*p,uint32_t us)
{
    if(!p||ioError(p)||!us)return 0;delayUS(p,us);return !ioError(p);
}
int RTWUSBController::initialize8812Core()
{
    auAuthorityBound_=false;
    if(!is8812Family()||!rxPipe_||endpoints_.num_bulk_in!=1||
       endpoints_.num_bulk_out<2||endpoints_.num_bulk_out>4||ioError(this))return ENXIO;
    rtw_r12au_usb_config config{};
    const auto speed=rxPipe_->getSpeed();
    if(speed>=kUSBHostConnectionSpeedSuper)config.speed=RTW_R12AU_USB_SUPER;
    else if(speed==kUSBHostConnectionSpeedHigh)config.speed=RTW_R12AU_USB_HIGH;
    else if(speed==kUSBHostConnectionSpeedFull)config.speed=RTW_R12AU_USB_FULL;
    else return ENOTSUP;
    config.bulk_in=endpoints_.bulk_in[0];config.bulk_out_count=endpoints_.num_bulk_out;
    config.bulk_max_packet=txMPS_[0];
    for(unsigned i=0;i<config.bulk_out_count;++i){if(!txPipes_[i]||txMPS_[i]!=config.bulk_max_packet)return EINVAL;config.bulk_out[i]=endpoints_.bulk_out[i];}
    const rtw_usb_rw_transport bus{this,auControlIn,auControlOut};const rtw_waiter wait{this,auWait};
    int e=rtw_r12au_boot_init(&auCore_,family_==RTW_WIFI_RTL8812AU?RTW_R12AU_CHIP_8812AU:RTW_R12AU_CHIP_8821AU,&bus,&wait,&config);
    if(!e)e=rtw_r12au_identify(&auCore_);if(!e&&!validateFamilyRecoveryBoard())e=ENODEV;if(!e)remember8812Authority();return e;
}
void RTWUSBController::refresh8812Modes()
{
    auto&ic=core_.sc_ic;ic.ic_modecaps&=~(1U<<IEEE80211_MODE_11AX);ic.ic_flags&=~(uint32_t)IEEE80211_F_HEON;
    bool ht=auHTSupported_&&(ic.ic_curmode==IEEE80211_MODE_AUTO||ic.ic_curmode==IEEE80211_MODE_11N||ic.ic_curmode==IEEE80211_MODE_11AC);
    bool vht=ht&&auVHTSupported_&&!(ic.ic_userflags&IEEE80211_F_NOVHT)&&(ic.ic_curmode==IEEE80211_MODE_AUTO||ic.ic_curmode==IEEE80211_MODE_11AC);
    ic.ic_flags=(ic.ic_flags&~(uint32_t)(IEEE80211_F_HTON|IEEE80211_F_VHTON))|(ht?IEEE80211_F_HTON:0U)|(vht?IEEE80211_F_VHTON:0U);
}
uint32_t RTWUSBController::channel8812Modes(uint8_t channel) const
{
    if(!permitsDFSChannelByCountry(channel)||!(rtw_usb_wifi_profile_allowed_bands(profile_)&(channel>14?2U:1U)))return 0;
    rtw_r12au_power_plan power{};rtw_r12au_operating op{channel,channel,20,RTW_R12AU_MODE_HT};uint32_t flags=0;
    if(rtw_r12au_phy_power_plan(&auCore_.board.phy,&op,&power))return 0;flags|=IEEE80211_CHAN_HT20;
    if(rtw_station_is_dfs_channel(channel))return flags; // initial DFS station contract: HT20
    op.width_mhz=40;op.center=(uint8_t)(channel+2);if(!rtw_r12au_phy_power_plan(&auCore_.board.phy,&op,&power))flags|=IEEE80211_CHAN_HT40U;
    if(channel>2){op.center=(uint8_t)(channel-2);if(!rtw_r12au_phy_power_plan(&auCore_.board.phy,&op,&power))flags|=IEEE80211_CHAN_HT40D;}
    if(channel<=14||!auCore_.board.vht)return flags;op={channel,channel,20,RTW_R12AU_MODE_VHT};
    if(rtw_r12au_phy_power_plan(&auCore_.board.phy,&op,&power))return flags;flags|=IEEE80211_CHAN_VHT20;
    op.width_mhz=40;op.center=(uint8_t)(channel+2);if(!rtw_r12au_phy_power_plan(&auCore_.board.phy,&op,&power))flags|=IEEE80211_CHAN_VHT40U;
    if(channel>2){op.center=(uint8_t)(channel-2);if(!rtw_r12au_phy_power_plan(&auCore_.board.phy,&op,&power))flags|=IEEE80211_CHAN_VHT40D;}
    op.width_mhz=80;op.center=(uint8_t)(channel<100?42:155);if(!rtw_r12au_phy_power_plan(&auCore_.board.phy,&op,&power))flags|=IEEE80211_CHAN_VHT80;
    return flags;
}
int RTWUSBController::peer8812Operating(rtw_r12au_operating*out) const
{
    if(!out)return EINVAL;const auto*n=core_.sc_ic.ic_bss;if(!n||!n->ni_chan||n->ni_chan==IEEE80211_CHAN_ANYC)return ENXIO;
    unsigned primary=ieee80211_chan2ieee(const_cast<ieee80211com*>(&core_.sc_ic),n->ni_chan);if(primary>255)return EINVAL;
    rtw_r12au_operating op{(uint8_t)primary,(uint8_t)primary,20,RTW_R12AU_MODE_LEGACY};
    if(auVHTSupported_&&(n->ni_flags&IEEE80211_NODE_VHT))op.mode=RTW_R12AU_MODE_VHT;
    else if(auHTSupported_&&(n->ni_flags&IEEE80211_NODE_HT))op.mode=RTW_R12AU_MODE_HT;
    /* The imported VHT negotiation can retain the AP's80MHz width even on a
     * channel whose local registration is HT20-only. Keep the radio and RA
     * at the admitted20MHz definition for DFS. */
    if(rtw_station_is_dfs_channel((uint8_t)primary)){
        if(op.mode==RTW_R12AU_MODE_VHT)op.mode=RTW_R12AU_MODE_HT;
    }else if(op.mode!=RTW_R12AU_MODE_LEGACY){
        if(n->ni_chw==IEEE80211_CHAN_WIDTH_40){op.width_mhz=40;unsigned sco=n->ni_htop0&IEEE80211_HTOP0_SCO_MASK;if(sco==IEEE80211_HTOP0_SCO_SCA)op.center=(uint8_t)(primary+2);else if(sco==IEEE80211_HTOP0_SCO_SCB&&primary>2)op.center=(uint8_t)(primary-2);else return EINVAL;}
        else if(n->ni_chw==IEEE80211_CHAN_WIDTH_80&&op.mode==RTW_R12AU_MODE_VHT){op.width_mhz=80;op.center=n->ni_vht_chan1;}
        else if(n->ni_chw!=IEEE80211_CHAN_WIDTH_20&&n->ni_chw!=IEEE80211_CHAN_WIDTH_20_NOHT)return ENOTSUP;
    }
    rtw_r12au_power_plan power{};int e=rtw_r12au_phy_power_plan(&auCore_.board.phy,&op,&power);if(e)return e;*out=op;return 0;
}
int RTWUSBController::set8812Operating(const rtw_r12au_operating&op)
{
    if(!(rtw_usb_wifi_profile_allowed_bands(profile_)&(op.primary>14?2U:1U)))return ENOTSUP;
    if(!auPHY_||!owns8812Authority())return ENXIO;const bool bandChanged=auOperating_.primary&&((auOperating_.primary>14)!=(op.primary>14));int e=rtw_r12au_phy_set_operating(auPHY_,&op);if(!e)e=rtw_r12au_phy_calibrate(auPHY_);
    if(!e){auOperating_=op;auChannel_=op.primary;if(bandChanged)e=notify8812Coex(RTW_R21CX_SWITCH_BAND);}if(e){auChannel_=0;auOperating_={};core_.core_error=e;}return e;
}
bool RTWUSBController::uses8812HighThroughput() const {return uses8812FirmwareRate()&&auRateConfig_.ht;}
bool RTWUSBController::accepts8812ReceiveRate(const rtw_rx_packet&p) const
{
    if(!owns8812Authority())return false;if(p.rate_index<12)return true;if(!uses8812HighThroughput()||p.bandwidth_code>2)return false;
    unsigned width=p.bandwidth_code==2?80:p.bandwidth_code==1?40:20;if(width>auOperating_.width_mhz)return false;
    if(p.rate_index<44)return width<=40&&p.rate_index<12+8*auCore_.board.phy.rf_paths;
    return auOperating_.mode==RTW_R12AU_MODE_VHT&&p.rate_index<44+10*auCore_.board.phy.rf_paths&&!(width==20&&(p.rate_index==53||p.rate_index==63));
}
int RTWUSBController::ampdu8812TXStart(ieee80211com*ic,ieee80211_node*n,uint8_t tid)
{
    if(!ic||!n||tid>=8)return EINVAL;auto*core=static_cast<rtwn_softc*>(ic->ic_softc);auto*s=owner(core->sc_ops.cookie);const auto&ba=n->ni_tx_ba[tid];
    if(!s->uses8812HighThroughput()||!s->authorized()||n!=ic->ic_bss||(ic->ic_tid_noack&(1U<<tid))||ba.ba_winsize<1||ba.ba_winsize>IEEE80211_BA_MAX_WINSZ||!(ic->ic_flags&IEEE80211_F_RSNON)||!(n->ni_rsnprotos&IEEE80211_PROTO_RSN))return EINVAL;
    /* Source AU MAC initialization already programmed aggregation registers.
     * Each descriptor carries the peer's factor/density and checks live BA;
     * this callback never fabricates an agreement or programs92E registers. */
    return 0;
}
rtw_r21au_coex_input RTWUSBController::input8812Coex() const
{
    const auto&ic=core_.sc_ic;const auto*n=ic.ic_bss;rtw_r21au_coex_input i{};
    i.now_ms=nowMS();i.primary=auOperating_.primary;i.center=auOperating_.center;i.width_mhz=auOperating_.width_mhz;
    i.connected=auMediaConnected_&&ic.ic_state==IEEE80211_S_RUN;i.scanning=ic.ic_state==IEEE80211_S_SCAN;
    i.beacon_interval=n?n->ni_intval:0;i.scan_ap_count=(uint16_t)(ic.ic_nnodes<0?0:ic.ic_nnodes>65535?65535:ic.ic_nnodes);
    i.rssi_percent=auRSSIValid_?auRSSI_:0;i.rx_rate=auCoexRxRate_;i.tx_rate=auCoexTxRate_;
    i.tx_bytes=auCoexTxBytes_;i.rx_bytes=auCoexRxBytes_;i.tx_packets=auCoexTxFrames_;i.rx_packets=auCoexRxFrames_;return i;
}
int RTWUSBController::notify8812Coex(rtw_r21au_coex_event event)
{
    if(!owns8812Authority())return require8812Hardware();if(!auCoex_)return 0;if(ioError(this)){rtw_r21au_coex_forget(auCoex_);return 0;}rtw_r21au_coex_status s{};rtw_r21au_coex_get_status(auCoex_,&s);if(!s.active)return s.error;
    auto input=input8812Coex();if(event==RTW_R21CX_SCAN_START)input.scanning=1;
    else if(event!=RTW_R21CX_SWITCH_BAND)input.scanning=0;
    if(event==RTW_R21CX_MEDIA_DISCONNECT)input.connected=0;
    return rtw_r21au_coex_notify(auCoex_,event,&input);
}
int RTWUSBController::attach8812()
{
    if(auPHYStorage_||auPHY_||running_)return EBUSY;
    int e=initialize8812Core();if(e)return e;
    if(!supports8812Band(false)&&!supports8812Band(true))return ENOTSUP;
    auPHYSize_=rtw_r12au_phy_size();auPHYStorage_=IOMalloc(auPHYSize_);
    if(!auPHYStorage_)return ENOMEM;bzero(auPHYStorage_,auPHYSize_);
    if(auCore_.board.phy.btcoex){auCoexSize_=rtw_r21au_coex_size();auCoexStorage_=IOMalloc(auCoexSize_);if(!auCoexStorage_){release8812Storage();return ENOMEM;}bzero(auCoexStorage_,auCoexSize_);}

    core_.sc_pdev=&device_;core_.ntxchains=auCore_.board.phy.rf_paths;core_.nrxchains=auCore_.board.phy.rf_paths;
    core_.avg_pwdb=-1;task_set(&core_.init_task,initTask8812,this,"rtw8812_init");
    auto&ic=core_.sc_ic;auto&ifp=ic.ic_if;
    memcpy(ic.ic_myaddr,auCore_.board.mac,IEEE80211_ADDR_LEN);
    ic.ic_phytype=IEEE80211_T_OFDM;ic.ic_opmode=IEEE80211_M_STA;ic.ic_state=IEEE80211_S_INIT;
    ic.ic_caps=IEEE80211_C_SHPREAMBLE|IEEE80211_C_SHSLOT|IEEE80211_C_RSN|IEEE80211_C_SCANALLBAND;
    ic.ic_htcaps=0;ic.ic_tx_mcs_set=0;memset(ic.ic_sup_mcs,0,sizeof(ic.ic_sup_mcs));
    ic.ic_flags&=~(uint32_t)IEEE80211_F_QOS;
    ic.ic_sup_rates[IEEE80211_MODE_11A]=ieee80211_std_rateset_11a;
    ic.ic_sup_rates[IEEE80211_MODE_11B]=ieee80211_std_rateset_11b;
    ic.ic_sup_rates[IEEE80211_MODE_11G]=ieee80211_std_rateset_11g;
    uint8_t initial=0;auHTSupported_=auVHTSupported_=false;
    for(uint8_t ch=1;ch<=11;++ch)if(supports8812Band(false)&&rtw_r12au_phy_channel_supported(&auCore_.board.phy,ch,20)){
        ic.ic_channels[ch].ic_freq=(uint16_t)ieee80211_ieee2mhz(ch,IEEE80211_CHAN_2GHZ);
        ic.ic_channels[ch].ic_flags=IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM|IEEE80211_CHAN_DYN|IEEE80211_CHAN_2GHZ;
        uint32_t modes=channel8812Modes(ch);ic.ic_channels[ch].ic_flags|=modes;auHTSupported_|=(modes&IEEE80211_CHAN_HT20)!=0;auVHTSupported_|=(modes&IEEE80211_CHAN_VHT20)!=0;
        if(!initial)initial=ch;
    }
    static const uint8_t passive5g[]={36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165};
    for(uint8_t ch:passive5g)if(permitsDFSChannelByCountry(ch)&&supports8812Band(true)&&rtw_r12au_phy_channel_supported(&auCore_.board.phy,ch,20)){
        ic.ic_channels[ch].ic_freq=(uint16_t)ieee80211_ieee2mhz(ch,IEEE80211_CHAN_5GHZ);
        ic.ic_channels[ch].ic_flags=IEEE80211_CHAN_5GHZ|IEEE80211_CHAN_OFDM|IEEE80211_CHAN_PASSIVE;
        uint32_t modes=channel8812Modes(ch);ic.ic_channels[ch].ic_flags|=modes;auHTSupported_|=(modes&IEEE80211_CHAN_HT20)!=0;auVHTSupported_|=(modes&IEEE80211_CHAN_VHT20)!=0;
        if(!initial)initial=ch;
    }
    if(!initial)return ENOTSUP;ic.ic_ibss_chan=&ic.ic_channels[initial];
    if(auHTSupported_){ic.ic_caps|=IEEE80211_C_QOS|IEEE80211_C_TX_AMPDU;ic.ic_flags|=IEEE80211_F_QOS;ic.ic_htcaps=IEEE80211_HTCAP_CBW20_40|IEEE80211_HTCAP_SGI20|IEEE80211_HTCAP_SGI40|(IEEE80211_HTCAP_SMPS_DIS<<IEEE80211_HTCAP_SMPS_SHIFT);ic.ic_ampdu_params=IEEE80211_AMPDU_PARAM_LE|IEEE80211_AMPDU_PARAM_SS_16;ic.ic_sup_mcs[0]=255;if(auCore_.board.phy.rf_paths==2)ic.ic_sup_mcs[1]=255;ic.ic_tx_mcs_set=IEEE80211_TX_MCS_SET_DEFINED;}
    if(auVHTSupported_){ic.ic_vhtcaps=IEEE80211_VHTCAP_SHORT_GI_80|(3U<<IEEE80211_VHTCAP_MAX_A_MPDU_LENGTH_EXPONENT_SHIFT);uint16_t map=auCore_.board.phy.rf_paths==2?0xfffa:0xfffe;ic.ic_vht_tx_mcs_map=ic.ic_vht_rx_mcs_map=htole16(map);}

    ifp.if_softc=&core_;ifp.if_flags=IFF_BROADCAST|IFF_SIMPLEX|IFF_MULTICAST;
    ifp.if_ioctl=ioctl8812;ifp.if_start=rtwn_start;ifp.if_watchdog=watchdog8812;
    memcpy(ifp.if_xname,device_.dv_xname,IFNAMSIZ);if_attach(&ifp);ieee80211_ifattach(&ifp,this);
    ic.ic_updateslot=update8812EDCA;ic.ic_updateedca=update8812EDCA;
    core_.sc_newstate=ic.ic_newstate;ic.ic_newstate=stateTransition;
    /* The imported stack infers AX from any AC channel. AU has no HE PHY. */
    refresh8812Modes();ieee80211_media_init(&ifp);refresh8812Modes();
    setProperty("RTWAUChip",(unsigned long long)auCore_.chip,8);
    setProperty("RTWAUCut",(unsigned long long)auCore_.board.phy.cut,8);
    setProperty("RTWAURFE",(unsigned long long)auCore_.board.phy.rfe,8);
    setProperty("RTWAURFPaths",(unsigned long long)auCore_.board.phy.rf_paths,8);
    /* Expose the actual factory-power preflight without publishing MAC/OTP
     * identity. These reference-channel results do not bypass per-channel
     * admission or claim that a whole band is calibrated. */
    {rtw_r12au_power_plan plan{};rtw_r12au_operating op{1,1,20,RTW_R12AU_MODE_LEGACY};
     setProperty("RTWAU2GHzChannel1PowerResult",(unsigned long long)(uint32_t)rtw_r12au_phy_power_plan(&auCore_.board.phy,&op,&plan),32);
     op={36,36,20,RTW_R12AU_MODE_LEGACY};
     setProperty("RTWAU5GHzChannel36PowerResult",(unsigned long long)(uint32_t)rtw_r12au_phy_power_plan(&auCore_.board.phy,&op,&plan),32);}
    setProperty("RTWAUFactoryPower",auCore_.board.phy.txpwr_idx,sizeof(auCore_.board.phy.txpwr_idx));
    setProperty("RTWAUBootstrapStage",(unsigned long long)0,32);
    setProperty("RTWAUBootstrapStageName","not-started");
    setProperty("RTWAUBootstrapResult",(unsigned long long)0,32);
    setProperty("RTWAUBootstrapFirstError",(unsigned long long)0,32);
    setProperty("RTWAUBootstrapFailures",(unsigned long long)0,64);
    setProperty("RTWAUWarmResetAttempts",(unsigned long long)0,32);
    setProperty("RTWAUWarmResetStage",(unsigned long long)0,32);
    setProperty("RTWAUWarmResetStageName","none");
    setProperty("RTWAUWarmResetMCU",(unsigned long long)0,32);
    setProperty("RTWAUWarmResetResult",(unsigned long long)0,32);
    return 0;
}
int RTWUSBController::start8812()
{
    unsigned bootStage=1;const char*bootName="admission";
    auto report=[&](unsigned stage,const char*name,int error){
        bootStage=stage;bootName=name;
        if(error){++auBootstrapFailures_;if(!auBootstrapFirstError_)auBootstrapFirstError_=(uint32_t)error;}
        setProperty("RTWAUBootstrapStage",(unsigned long long)stage,32);
        setProperty("RTWAUBootstrapStageName",name);
        setProperty("RTWAUBootstrapResult",(unsigned long long)(uint32_t)error,32);
        setProperty("RTWAUBootstrapFirstError",(unsigned long long)auBootstrapFirstError_,32);
        setProperty("RTWAUBootstrapFailures",(unsigned long long)auBootstrapFailures_,64);
        /* Keep the last warm-reset result across a later cold re-identify. */
        if(auCore_.warm_reset_attempts){
            static const char*const names[]={"none","mcu","bb-rf","mac-off","mac-on","complete"};
            setProperty("RTWAUWarmResetAttempts",(unsigned long long)auCore_.warm_reset_attempts,32);
            setProperty("RTWAUWarmResetStage",(unsigned long long)auCore_.warm_reset_stage,32);
            setProperty("RTWAUWarmResetStageName",auCore_.warm_reset_stage<6?names[auCore_.warm_reset_stage]:"unknown");
            setProperty("RTWAUWarmResetMCU",(unsigned long long)auCore_.warm_reset_mcu,32);
            setProperty("RTWAUWarmResetResult",(unsigned long long)(uint32_t)auCore_.warm_reset_error,32);
        }
    };
    report(1,"admission",0);
    const int lifecycle=beginFamilyStart();if(lifecycle){report(bootStage,bootName,lifecycle);return lifecycle;}

    auto&ic=core_.sc_ic;auto&ifp=ic.ic_if;
    if(!is8812Family()||!owns8812Authority()||ic.ic_opmode!=IEEE80211_M_STA||!ic.ic_bss||!auPHYStorage_||ioError(this)){report(bootStage,bootName,ENXIO);return ENXIO;}
    if(auCore_.powered||auCore_.firmware_running||auCore_.mac_initialized){report(bootStage,bootName,EBUSY);return EBUSY;}
    refresh8812Modes();auCoex_=nullptr;auCoexTxBytes_=auCoexRxBytes_=0;auCoexTxFrames_=auCoexRxFrames_=0;auCoexRxRate_=auCoexTxRate_=2;auMediaConnected_=auRateReady_=auRSSIValid_=false;auRateConfig_={};auChannel_=auRSSI_=0;auOperating_={};auPHY_=nullptr;radioReady_=false;core_.core_error=0;
    int e=0;if(auCore_.io_error||!auCore_.identified){e=initialize8812Core();if(e)goto fail;}
    report(2,"transfers",0);e=allocateTransfers();if(e)goto fail;
    {
        const uint8_t*firmware=auCore_.chip==RTW_R12AU_CHIP_8812AU?RTW8812AUFirmware:RTW8821AUFirmware;
        const size_t size=auCore_.chip==RTW_R12AU_CHIP_8812AU?sizeof(RTW8812AUFirmware):sizeof(RTW8821AUFirmware);
        rtw_fw_image header{};rtw_boot_report firmwareReport{};
        report(3,"firmware-image",0);e=rtw_r12au_fw_inspect(auCore_.chip,firmware,size,&header);if(e)goto fail;
        report(4,"power-on",0);e=rtw_r12au_power_on(&auCore_);if(e)goto fail;
        report(5,"dma-llt",0);e=rtw_r12au_dma_prepare(&auCore_);if(e)goto fail;
        report(6,"firmware-download",0);e=rtw_r12au_download_fw(&auCore_,firmware,size,&firmwareReport);if(e)goto fail;
    }
    report(7,"mac",0);e=rtw_r12au_mac_init(&auCore_);if(e)goto fail;
    report(8,"phy-init",0);e=rtw_r12au_phy_init(auPHYStorage_,auPHYSize_,&auCore_.board.phy,&auCore_.bus,&auCore_.waiter,&auPHY_);if(e)goto fail;
    report(9,"phy-tables",0);e=rtw_r12au_phy_load_tables(auPHY_);if(e)goto fail;
    report(10,"cck-report",0);{uint32_t value=0;e=rtw_r12au_read(&auCore_,0x804,4,&value);if(e)goto fail;auCckHighPower_=(value&(1U<<16))!=0;}
    report(11,"channel-rfk",0);ic.ic_bss->ni_chan=ic.ic_ibss_chan;e=set8812Channel((uint8_t)ieee80211_chan2ieee(&ic,ic.ic_ibss_chan));if(e)goto fail;
    report(12,"rx-start",0);familyRecovery_.diag.stage=rtw::StationRecovery::RX;e=busInitialize(this);if(e)goto fail;
    familyRecovery_.diag.stage=rtw::StationRecovery::Start;
    if(auCore_.board.phy.btcoex){
        report(13,"coex-init",0);
        if(!auCoexStorage_){e=ENOMEM;goto fail;}
        e=rtw_r21au_coex_init(auCoexStorage_,auCoexSize_,&auCore_,&auCoex_);if(e)goto fail;
        report(14,"coex-start-bind",0);
        {auto input=input8812Coex();e=rtw_r21au_coex_start(auCoex_,&input);}if(e)goto fail;
        e=rtw_r21au_coex_bind_phy(auCoex_,auPHY_);if(e)goto fail;
    }

    ifq_clr_oactive(&ifp.if_snd);ifp.if_flags|=IFF_RUNNING;
    report(15,"scan",0);e=ic.ic_newstate(&ic,IEEE80211_S_SCAN,-1);if(e)goto fail;
    radioReady_=true;if(auCoex_)nextCalibration(this);finishFamilyStart();report(16,"ready",0);return 0;
fail:
    report(bootStage,bootName,e);failFamilyStart(e);stop8812();if(!familyRecovery_.transient(e))core_.core_error=e;return e;
}
void RTWUSBController::stop8812()
{
    familyRecovery_.inStop=true;
    auto&ifp=core_.sc_ic.ic_if;radioReady_=false;core_.sc_tx_timer=0;ifp.if_timer=0;
    bool owned=owns8812Authority();if(auCoex_){rtw_r21au_coex_status status{};rtw_r21au_coex_get_status(auCoex_,&status);owned=owned&&status.owner_valid!=0;}
    ifp.if_flags&=~IFF_RUNNING;ifq_clr_oactive(&ifp.if_snd);
    if(core_.sc_newstate)(void)core_.sc_newstate(&core_.sc_ic,IEEE80211_S_INIT,-1);
    cancelScan(this);cancelCalibration(this);task_del(systq,&core_.init_task);abortTransfers();
    auMediaConnected_=auRateReady_=auRSSIValid_=false;auRateConfig_={};auChannel_=auRSSI_=0;auOperating_={};
    if(!familyStopDrained())return;
    int cleanup=0,admission=0;
    if(!owned||!present_||sleeping_||usbError_!=kIOReturnSuccess)admission=ENXIO;
    else if(auCore_.io_error&&!familyRecovery_.eligible)admission=auCore_.io_error;
    else if(auCore_.firmware_running&&!auCore_.io_error){
        uint32_t ready=0;admission=rtw_r12au_read(&auCore_,0x80,1,&ready);
        if(!admission&&(ready&0xc7U)!=0xc6U)admission=ENXIO;
    }
    if(admission&&!core_.core_error)core_.core_error=admission;
    if(owned&&present_&&!sleeping_&&usbError_==kIOReturnSuccess&&(!auCore_.io_error||familyRecovery_.eligible)){
        if(!auCore_.io_error){if(auCoex_)cleanup=rtw_r21au_coex_stop(auCoex_);if(auPHY_){const int e=rtw_r12au_phy_stop(auPHY_);if(!cleanup)cleanup=e;}}
        if(auCore_.identified){const int e=rtw_r12au_power_off_checked(&auCore_);if(!cleanup)cleanup=e;}
    }else{auCore_.powered=auCore_.dma_prepared=auCore_.firmware_running=auCore_.mac_initialized=auCore_.associated=0;cleanup=ENXIO;}
    if(auCoex_)rtw_r21au_coex_forget(auCoex_);if(auPHY_)rtw_r12au_phy_forget(auPHY_);
    auCoex_=nullptr;auPHY_=nullptr;recordFamilyCleanup(cleanup);freeTransfers();
}
void RTWUSBController::release8812Storage()
{
    /* Parent calls after gated teardown has drained every transfer. */
    auAuthorityBound_=false;
    if(auCoexStorage_){IOFree(auCoexStorage_,auCoexSize_);auCoexStorage_=nullptr;}auCoexSize_=0;auCoex_=nullptr;
    if(auPHYStorage_){IOFree(auPHYStorage_,auPHYSize_);auPHYStorage_=nullptr;}auPHYSize_=0;auPHY_=nullptr;
}
uint16_t RTWUSBController::station8812Rates(bool basicOnly) const
{
    const auto&ic=core_.sc_ic;const auto*n=ic.ic_bss;if(!n)return 0;
    const bool five=n->ni_chan&&n->ni_chan!=IEEE80211_CHAN_ANYC&&(n->ni_chan->ic_flags&IEEE80211_CHAN_5GHZ);
    uint16_t mask=rtw8812RateMask(n->ni_rates,basicOnly)&(five?0xff0U:0xfffU);
    if(basicOnly&&!mask){mask=rtw8812RateMask(n->ni_rates,false)&(five?0xff0U:0xfffU);mask&=(uint16_t)(0U-mask);}
    return mask;
}
int RTWUSBController::set8812Channel(uint8_t channel)
{
    const rtw_r12au_operating op{channel,channel,20,RTW_R12AU_MODE_LEGACY};return set8812Operating(op);
}
int RTWUSBController::station8812Hardware(ieee80211_state state)
{
    auto&ic=core_.sc_ic;const auto*n=ic.ic_bss;
    if(!owns8812Authority()||!auCore_.powered||!auCore_.firmware_running||!auCore_.mac_initialized)return ENXIO;
    if(state!=IEEE80211_S_INIT&&(!n||!auPHY_))return ENXIO;
    const bool run=state==IEEE80211_S_RUN;const bool five=auChannel_>14;
    int e=rtw_r12au_mac_station(&auCore_,run?n->ni_bssid:nullptr,run?(n->ni_associd&0x3fff):0,run);
    if(e)return e;
    /* AU uses RXFLTMAP1 directly; Linux FIF_CONTROL changes this map without
     * importing the older92C RCR_ACF bit. Deliver only BAR for host reorder. */
    e=rtw_r12au_write(&auCore_,0x6a2,2,run&&auOperating_.mode!=RTW_R12AU_MODE_LEGACY?0x100:0);if(!e)e=rtw_r12au_write(&auCore_,0x6a4,2,run?0xffff:0);
    if(state==IEEE80211_S_INIT){if(auPHY_){int stopped=rtw_r12au_phy_stop(auPHY_);if(!e)e=stopped;}auChannel_=0;return e;}
    /* The imported stack retains BSSID, RSN MIC/replay and port admission. */
    uint16_t basic=station8812Rates(true);if(!basic&&!run)basic=(uint16_t)(five?0x10:1);
    if(!e)e=rtw_r12au_station_rates(&auCore_,basic,(ic.ic_flags&IEEE80211_F_SHSLOT)!=0,five);
    if(!e&&(state==IEEE80211_S_AUTH||state==IEEE80211_S_ASSOC)){
        const auto*b=n->ni_bssid;uint32_t word=(uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);
        e=rtw_r12au_write(&auCore_,0x618,4,word);if(!e)e=rtw_r12au_write(&auCore_,0x61c,2,(uint32_t)b[4]|((uint32_t)b[5]<<8));
    }
    /* Passive scan never resumes TX, even after a successful full RFK. AUTH
     * is the explicit transition to calibrated operating-channel management. */
    const bool passive=state==IEEE80211_S_SCAN&&n->ni_chan&&(n->ni_chan->ic_flags&IEEE80211_CHAN_PASSIVE);
    if(!e)e=passive?rtw_r12au_phy_stop(auPHY_):rtw_r12au_phy_resume_tx(auPHY_);
    /* The PHY lease returns boolean admission; retain the concrete firmware
     * or USB failure latched by its live coex health check. */
    if(e&&auCoex_){rtw_r21au_coex_status status{};rtw_r21au_coex_get_status(auCoex_,&status);if(status.error)e=status.error;}
    return e;
}
/* All independent station/EDCA entry points admit the immutable owner and
 * live MCU before touching MAC state or a firmware mailbox. Failure performs
 * only bounded direct cleanup on the unchanged usable device, then host INIT. */
int RTWUSBController::require8812Hardware()
{
    bool owned=owns8812Authority();int error=owned?0:ENXIO;
    if(auCoex_){rtw_r21au_coex_status status{};rtw_r21au_coex_get_status(auCoex_,&status);owned=owned&&status.owner_valid!=0;if(!owned)error=ENXIO;else if(status.error)error=status.error;else if(!status.active)error=ENXIO;}
    if(!error&&(!present_||sleeping_||usbError_!=kIOReturnSuccess))error=ENXIO;
    if(!error&&auCore_.io_error)error=auCore_.io_error;
    if(!error&&(!auCore_.identified||!auCore_.powered||!auCore_.firmware_running||!auCore_.mac_initialized||(auCore_.board.phy.btcoex&&!auCoex_)))error=ENXIO;
    if(!error){
        if(auCoex_)error=rtw_r21au_coex_health(auCoex_);
        else{uint32_t status=0;error=rtw_r12au_read(&auCore_,0x80,1,&status);if(!error&&(status&0xc7U)!=0xc6U)error=ENXIO;}
    }
    if(!error)return 0;
    auCore_.firmware_running=auCore_.associated=0;
    const bool usable=owned&&present_&&!sleeping_&&usbError_==kIOReturnSuccess&&!auCore_.io_error;
    if(usable){if(auCoex_)(void)rtw_r21au_coex_stop(auCoex_);if(auPHY_)(void)rtw_r12au_phy_stop(auPHY_);}
    if(auCoex_)rtw_r21au_coex_forget(auCoex_);if(auPHY_)rtw_r12au_phy_forget(auPHY_);
    if(!usable)auCore_.powered=auCore_.dma_prepared=auCore_.mac_initialized=0;
    auMediaConnected_=auRateReady_=auRSSIValid_=false;auRateConfig_={};auChannel_=0;auOperating_={};radioReady_=false;
    cancelScan(this);cancelCalibration(this);
    if(core_.sc_newstate)(void)core_.sc_newstate(&core_.sc_ic,IEEE80211_S_INIT,-1);
    core_.core_error=error;return error;
}
int RTWUSBController::state8812(ieee80211com*ic,ieee80211_state state,int argument)
{
    if(!ic||ic->ic_opmode!=IEEE80211_M_STA||!core_.sc_newstate||state<IEEE80211_S_INIT||state>IEEE80211_S_RUN)return EINVAL;
    const auto previous=ic->ic_state;const bool wasMedia=auMediaConnected_;cancelScan(this);int error=0;
    if(state!=IEEE80211_S_INIT||auCore_.powered||auCore_.firmware_running||auCore_.mac_initialized||auPHY_||auCoex_){int admitted=require8812Hardware();if(admitted)return admitted;}
    if((previous==IEEE80211_S_RUN&&state!=IEEE80211_S_RUN)||state==IEEE80211_S_INIT){
        cancelCalibration(this);
        if(auMediaConnected_&&auCore_.firmware_running&&!auCore_.io_error){uint8_t command[8]{};if(!rtw_r12au_make_media_command(0,command))error=EINVAL;else error=rtw_r12au_h2c(&auCore_,command);}
        auMediaConnected_=auRateReady_=auRSSIValid_=false;auRateConfig_={};
    }
    if(state==IEEE80211_S_INIT){
        if(auCore_.powered&&auCore_.firmware_running&&auCore_.mac_initialized&&!auCore_.io_error){int e=station8812Hardware(state);if(!error)error=e;}
        /* A failed disconnect mailbox must not retain calibrated/TX-ready
         * metadata. PHY stop invalidates that state before its bounded
         * best-effort write, including when the transport has failed. */
        if(auPHY_){rtw_r12au_phy_status status{};rtw_r12au_phy_get_status(auPHY_,&status);if(status.calibrated||status.tx_enabled){int e=rtw_r12au_phy_stop(auPHY_);if(!error)error=e;}}
        if(auCoex_){int ce=0;if(wasMedia)ce=notify8812Coex(RTW_R21CX_MEDIA_DISCONNECT);if(!ce&&(previous==IEEE80211_S_AUTH||previous==IEEE80211_S_ASSOC))ce=notify8812Coex(RTW_R21CX_ASSOC_FINISH);if(!error)error=ce;}
        auChannel_=0;
        /* No USB, RF or firmware failure may skip the host key/port flush. */
        int e=core_.sc_newstate(ic,state,argument);return error?error:e;
    }
    auto rollback=[&](int e)->int{
        if(require8812Hardware()){core_.core_error=e;return e;}
        if(auMediaConnected_&&auCore_.firmware_running&&!auCore_.io_error){uint8_t command[8]{};if(rtw_r12au_make_media_command(0,command))(void)rtw_r12au_h2c(&auCore_,command);}
        auMediaConnected_=auRateReady_=false;auRateConfig_={};radioReady_=false;
        if(auCore_.mac_initialized&&!auCore_.io_error)(void)station8812Hardware(IEEE80211_S_INIT);
        if(auPHY_)(void)rtw_r12au_phy_stop(auPHY_);
        (void)core_.sc_newstate(ic,IEEE80211_S_INIT,-1);core_.core_error=e;return e;
    };
    if(error)return rollback(error);
    if(!ic->ic_bss||!ic->ic_bss->ni_chan||ic->ic_bss->ni_chan==IEEE80211_CHAN_ANYC||!auPHY_)return rollback(ENXIO);
    const unsigned channel=ieee80211_chan2ieee(ic,ic->ic_bss->ni_chan);
    if(!(rtw_usb_wifi_profile_allowed_bands(profile_)&(channel>14?2U:1U)))return rollback(ENOTSUP);
    if(channel>255||!permitsDFSChannelByCountry((uint8_t)channel)||!rtw_r12au_phy_channel_supported(&auCore_.board.phy,(uint8_t)channel,20))return rollback(ENOTSUP);
    rtw_r12au_phy_status phy{};rtw_r12au_phy_get_status(auPHY_,&phy);
    switch(state){
    case IEEE80211_S_SCAN:
        if(!phy.calibrated||auChannel_!=channel)error=set8812Channel((uint8_t)channel);
        if(!error)error=station8812Hardware(state);
        break;
    case IEEE80211_S_AUTH:
        /* Authentication can never inherit an RX-only scan's radio state. */
        error=set8812Channel((uint8_t)channel);if(!error&&previous==IEEE80211_S_SCAN)error=notify8812Coex(RTW_R21CX_SCAN_FINISH);if(!error)error=notify8812Coex(RTW_R21CX_ASSOC_START);if(!error)error=station8812Hardware(state);break;
    case IEEE80211_S_ASSOC:
        if(!phy.calibrated||auChannel_!=channel||phy.io_error)return rollback(ENXIO);
        error=station8812Hardware(state);break;
    case IEEE80211_S_RUN: {
        if(!phy.calibrated||auChannel_!=channel||phy.io_error)return rollback(ENXIO);
        auRateReady_=false;rtw_r12au_operating operating{};error=peer8812Operating(&operating);if(error)return rollback(error);
        if(operating.primary!=auOperating_.primary||operating.center!=auOperating_.center||operating.width_mhz!=auOperating_.width_mhz||operating.mode!=auOperating_.mode){error=set8812Operating(operating);if(error)return rollback(error);}
        const auto*n=ic->ic_bss;const bool five=(n->ni_chan->ic_flags&IEEE80211_CHAN_5GHZ)!=0;
        const uint16_t rates=station8812Rates(false),basic=station8812Rates(true),aid=n->ni_associd&0x3fff;
        if(!rates||!basic||!aid||aid>2007||!n->ni_intval)return rollback(EINVAL);
        rtw_r12au_legacy_rate config{};uint8_t rate[8]{},media[8]{};
        uint8_t mcs[2]={n->ni_rxmcs[0],n->ni_rxmcs[1]};bool localSGI,peerSGI;
        unsigned peerTXPaths=auCore_.board.phy.rf_paths;
        /* net80211 retains raw peer RX MCS even under static SMPS. Dynamic
         * SMPS stays at one stream until explicit multi-stream wake/protection
         * is implemented; reserved values cannot authorize extra streams.
         * The bound applies to HT MCS and VHT NSS, never our receive paths. */
        if(operating.mode!=RTW_R12AU_MODE_LEGACY&&((n->ni_htcaps&IEEE80211_HTCAP_SMPS_MASK)>>IEEE80211_HTCAP_SMPS_SHIFT)!=IEEE80211_HTCAP_SMPS_DIS){mcs[1]=0;peerTXPaths=1;}
        if(operating.width_mhz==80){localSGI=(ic->ic_vhtcaps&IEEE80211_VHTCAP_SHORT_GI_80)!=0;peerSGI=(n->ni_vhtcaps&IEEE80211_VHTCAP_SHORT_GI_80)!=0;}
        else{unsigned flag=operating.width_mhz==40?IEEE80211_HTCAP_SGI40:IEEE80211_HTCAP_SGI20;localSGI=(ic->ic_htcaps&flag)!=0;peerSGI=(n->ni_htcaps&flag)!=0;}
        if(!rtw_r12au_build_rate_config(rates,mcs,le16toh(n->ni_vht_mcsinfo.rx_mcs_map),peerTXPaths,operating.mode,operating.width_mhz,five,localSGI,peerSGI,&config)||!rtw_r12au_make_rate_command(&config,1,rate)||!rtw_r12au_make_media_command(1,media))return rollback(EINVAL);
        auRateReady_=false;
        error=rtw_r12au_h2c(&auCore_,rate); /* AU8051 uses only RA_INFO0x40. */
        if(!error){error=rtw_r12au_h2c(&auCore_,media);if(!error)auMediaConnected_=true;}
        if(!error)error=rtw_r12au_write(&auCore_,0x554,2,n->ni_intval);
        if(!error)error=station8812Hardware(state);
        if(!error)error=rtw_r12au_phy_set_tx_rate(auPHY_,config.initial_rate);
        if(!error){auRateConfig_=config;update8812EDCA(ic);if(auCore_.io_error)error=auCore_.io_error;else if(core_.core_error)error=core_.core_error;}
        break;
    }
    case IEEE80211_S_INIT:return EINVAL;
    }
    if(error)return rollback(error);
    error=core_.sc_newstate(ic,state,argument);if(error)return rollback(error);
    if(state==IEEE80211_S_RUN){error=notify8812Coex(RTW_R21CX_ASSOC_FINISH);if(!error)error=notify8812Coex(RTW_R21CX_MEDIA_CONNECT);if(error)return rollback(error);auRateReady_=true;auRSSIValid_=false;nextCalibration(this);}
    if(state==IEEE80211_S_SCAN){if(previous!=IEEE80211_S_SCAN){error=notify8812Coex(RTW_R21CX_SCAN_START);if(error)return rollback(error);}if(auCoex_)nextCalibration(this);nextScan(this);}return 0;
}
void RTWUSBController::update8812EDCA(ieee80211com*ic)
{
    if(!ic)return;auto*core=static_cast<rtwn_softc*>(ic->ic_softc);auto*s=owner(core->sc_ops.cookie);
    if(s->require8812Hardware())return;
    const auto*n=ic->ic_bss;const bool five=n&&n->ni_chan&&n->ni_chan!=IEEE80211_CHAN_ANYC&&(n->ni_chan->ic_flags&IEEE80211_CHAN_5GHZ);
    unsigned mode=five?IEEE80211_MODE_11A:(s->station8812Rates(false)&0xff0)?IEEE80211_MODE_11G:IEEE80211_MODE_11B;
    const auto*parameters=((ic->ic_flags&IEEE80211_F_QOS)&&n&&(n->ni_flags&IEEE80211_NODE_QOS))?ic->ic_edca_ac:ieee80211_edca_table[mode];
    rtw_r12au_edca_ac a[4]{};for(unsigned i=0;i<4;++i){const auto&p=parameters[i];a[i]={p.ac_aifsn,p.ac_ecwmin,p.ac_ecwmax,p.ac_txoplimit};}
    int e=rtw_r12au_station_edca(&s->auCore_,(ic->ic_flags&IEEE80211_F_SHSLOT)!=0,five,a);if(e)core->core_error=e;
}
bool RTWUSBController::uses8812FirmwareRate() const
{
    if(!(rtw_usb_wifi_profile_allowed_bands(profile_)&(auChannel_>14?2U:1U))||!owns8812Authority()||!present_||sleeping_||usbError_!=kIOReturnSuccess||!auPHY_||!auRateReady_||!auMediaConnected_||!running_||stopping_||
       !auCore_.mac_initialized||!auCore_.firmware_running||auCore_.io_error||
       core_.sc_ic.ic_state!=IEEE80211_S_RUN||!core_.sc_ic.ic_bss)return false;
    if(auCoex_||auCore_.board.phy.btcoex){rtw_r21au_coex_status coex{};rtw_r21au_coex_get_status(auCoex_,&coex);if(!coex.active||coex.error)return false;}
    rtw_r12au_phy_status status{};rtw_r12au_phy_get_status(auPHY_,&status);
    return status.calibrated&&status.tx_enabled&&!status.io_error&&status.channel==auChannel_;
}
int RTWUSBController::tick8812()
{
    if(!owns8812Authority())return require8812Hardware();
    if(auCoex_){auto input=input8812Coex();int ce=rtw_r21au_coex_watchdog(auCoex_,&input);auCoexTxBytes_=auCoexRxBytes_=0;auCoexTxFrames_=auCoexRxFrames_=0;if(ce){radioReady_=false;(void)state8812(&core_.sc_ic,IEEE80211_S_INIT,-1);core_.core_error=ce;return ce;}}
    if(core_.sc_ic.ic_state!=IEEE80211_S_RUN)return 0;
    if(!uses8812FirmwareRate())return ENXIO;
    uint32_t ready=0;int e=rtw_r12au_read(&auCore_,0x80,1,&ready);
    /* Pinned legacy ABI: RDY/checksum/WINTINI/RAM bits must be set and
     * download-enable bit0 must be clear. A failed/all-ones USB read cannot
     * retain cached firmware, rate, PHY or host controlled-port authority. */
    if(e||(ready&0xc7U)!=0xc6U){
        if(!e)e=EIO;auCore_.firmware_running=auCore_.associated=0;radioReady_=false;
        (void)state8812(&core_.sc_ic,IEEE80211_S_INIT,-1);
        core_.core_error=e;return e;
    }
    if(!e){e=rtw_r12au_phy_tick(auPHY_);if(e==EAGAIN)e=0;}
    if(!e&&auRSSIValid_){uint8_t command[8]{};if(!rtw_r12au_make_rssi_command(auRSSI_,command))return EINVAL;
        e=rtw_r12au_h2c_rssi_monitor(&auCore_,command);
        if(e==EBUSY&&!auCore_.io_error)e=0; // Retry advisory telemetry on the next ordinary tick.
    }
    return e;
}
rtw_packet_error RTWUSBController::decode8812USBFrame(rtw_rx_iterator*it,rtw_8192eu_rx_frame*out,uint8_t*reportType) const
{
    if(out)memset(out,0,sizeof(*out));if(reportType)*reportType=0;
    if(!it||!out||!reportType||it->frames>RTW_R12AU_MAX_RX_FRAMES)return RTW_PACKETS_BAD_ARGUMENT;
    rtw_r12au_rx_iterator iterator{it->buffer,it->length,it->cursor,(unsigned)it->frames};rtw_r12au_rx_frame frame{};
    auto result=rtw_r12au_rx_next(&iterator,auCore_.board.phy.rf_paths,&frame);
    switch(result){
    case RTW_R12AU_FRAME:break;
    case RTW_R12AU_END:it->cursor=iterator.cursor;it->frames=iterator.frames;return RTW_PACKETS_DONE;
    case RTW_R12AU_TRUNCATED:return RTW_PACKETS_TRUNCATED;
    case RTW_R12AU_MALFORMED:return RTW_PACKETS_MALFORMED;
    case RTW_R12AU_TOO_MANY:return RTW_PACKETS_TOO_MANY;
    default:return RTW_PACKETS_BAD_ARGUMENT;
    }
    it->cursor=iterator.cursor;it->frames=iterator.frames;
    auto&p=out->payload;p.mpdu=frame.payload;p.mpdu_length=frame.payload_length;
    /* MAC RCR omits APP_FCS. Do not remove another four bytes here. */
    p.phy_info=frame.phy;p.phy_info_length=frame.phy_length;p.phy_status_present=frame.phy_length!=0;
    p.timestamp=frame.timestamp;p.rate_index=frame.rate;p.bandwidth_code=frame.bandwidth_mhz==80?2:frame.bandwidth_mhz==40?1:0;
    p.crc_error=frame.crc_error;p.icv_error=frame.icv_error;out->security_code=frame.cipher;out->sw_decrypt_required=frame.sw_decrypt;out->report_is_c2h=frame.is_c2h;*reportType=frame.is_c2h?5:0;
    return RTW_PACKETS_OK;
}
int RTWUSBController::sample8812RSSI(const rtw_rx_packet&p)
{
    if(auCoex_&&p.mpdu&&p.mpdu_length>=24){if(auCoexRxFrames_!=UINT32_MAX)++auCoexRxFrames_;if((p.mpdu[0]&IEEE80211_FC0_TYPE_MASK)==IEEE80211_FC0_TYPE_DATA){auCoexRxRate_=p.rate_index;if(!(p.mpdu[4]&1)&&UINT64_MAX-auCoexRxBytes_>=p.mpdu_length)auCoexRxBytes_+=p.mpdu_length;}}

    rtw_r12au_phy_sample sample{};
    if(!p.phy_status_present||!rtw_r12au_decode_phy_sample(auCore_.chip,auCore_.board.phy.rf_paths,auCckHighPower_,p.rate_index,p.phy_info,p.phy_info_length,&sample))return 0;
    const auto*n=core_.sc_ic.ic_bss;
    if(n&&core_.sc_ic.ic_state==IEEE80211_S_RUN&&rtw_signal_from_ap(p.mpdu,p.mpdu_length,n->ni_bssid)){auRSSI_=sample.rssi_percent;auRSSIValid_=true;}
    return sample.signal_dbm;
}
void RTWUSBController::handle8812C2H(const uint8_t*bytes,size_t length)
{
    if(!owns8812Authority()){++rateControl_.invalid;return;}
    if(auCoex_){int ce=rtw_r21au_coex_c2h(auCoex_,bytes,length,nowMS());if(ce!=ENOMSG){if(ce==EINVAL){++rateControl_.invalid;return;}if(ce==EAGAIN)return;if(ce){radioReady_=false;(void)state8812(&core_.sc_ic,IEEE80211_S_INIT,-1);core_.core_error=ce;}return;}}

    uint8_t rate=0;int parsed=rtw_r12au_parse_rate_report(bytes,length,auRateReady_?&auRateConfig_:nullptr,&rate);
    if(parsed<0){++rateControl_.invalid;return;}if(!parsed){++rateControl_.ignored;return;}
    if(uses8812FirmwareRate()){++rateControl_.reports;rateControl_.finalRate=rate;auCoexTxRate_=rate;if(auPHY_)(void)rtw_r12au_phy_set_tx_rate(auPHY_,rate);}
}
int RTWUSBController::send8812Frame(mbuf_t packet,ieee80211_node*node)
{
    if(!packet)return EINVAL;
    if(!(rtw_usb_wifi_profile_allowed_bands(profile_)&(auChannel_>14?2U:1U))||!owns8812Authority()){mbuf_freem(packet);return ENETDOWN;}
    rtw_r12au_phy_status phy{};if(auPHY_)rtw_r12au_phy_get_status(auPHY_,&phy);
    if(auCoex_||auCore_.board.phy.btcoex){rtw_r21au_coex_status coex{};rtw_r21au_coex_get_status(auCoex_,&coex);if(!coex.active||coex.error){mbuf_freem(packet);return ENETDOWN;}}
    if(!running_||stopping_||!present_||!node||usbError_!=kIOReturnSuccess||!phy.calibrated||!phy.tx_enabled||phy.io_error||auCore_.io_error||!auCore_.mac_initialized){mbuf_freem(packet);return ENETDOWN;}
    ieee80211_frame header{};size_t length=mbuf_pkthdr_len(packet);
    if(length<sizeof(header)||length>4096||mbuf_copydata(packet,0,sizeof(header),&header)){mbuf_freem(packet);return EINVAL;}
    const bool data=(header.i_fc[0]&IEEE80211_FC0_TYPE_MASK)==IEEE80211_FC0_TYPE_DATA;
    const bool multicast=(header.i_addr1[0]&1)!=0;
    const bool qos=data&&(header.i_fc[0]&IEEE80211_FC0_SUBTYPE_QOS);const size_t headerLength=sizeof(header)+(qos?2U:0U);uint8_t qosBytes[2]{};
    if((header.i_fc[1]&IEEE80211_FC1_DIR_MASK)==IEEE80211_FC1_DIR_DSTODS||length<headerLength||(qos&&mbuf_copydata(packet,sizeof(header),2,qosBytes))){mbuf_freem(packet);return EINVAL;}
    const unsigned tid=qos?(qosBytes[0]&IEEE80211_QOS_TID):0;if(tid>=8){mbuf_freem(packet);return EINVAL;}
    uint8_t llc[8]{};const bool eapol=data&&length>=headerLength+sizeof(llc)&&!mbuf_copydata(packet,headerLength,sizeof(llc),llc)&&
        llc[0]==0xaa&&llc[1]==0xaa&&llc[2]==3&&llc[3]==0&&llc[4]==0&&llc[5]==0&&llc[6]==0x88&&llc[7]==0x8e;
    /* RUN is not authorization. EAPOL remains usable while software WPA2
     * establishes keys; every other data frame needs the controlled port. */
    if(data&&!eapol&&!authorized()){mbuf_freem(packet);return ENETDOWN;}
    auto&ic=core_.sc_ic;
    if(header.i_fc[1]&IEEE80211_FC1_PROTECTED){auto*key=ieee80211_get_txkey(&ic,&header,node);if(!key||key->k_cipher!=IEEE80211_CIPHER_CCMP){mbuf_freem(packet);return EINVAL;}packet=ieee80211_encrypt(&ic,packet,key);if(!packet)return ENOBUFS;length=mbuf_pkthdr_len(packet);}
    unsigned index=RTW_TRANSFER_SLOTS;IOInterruptState lock=IOSimpleLockLockDisableInterrupt(completionLock_);
    for(unsigned i=RXSlots;i<RTW_TRANSFER_SLOTS;++i)if(!(ledger_.pending&(1U<<i))){index=i;break;}IOSimpleLockUnlockEnableInterrupt(completionLock_,lock);
    if(index==RTW_TRANSFER_SLOTS||length>4096){mbuf_freem(packet);return ENOBUFS;}
    const bool five=auChannel_>14;uint16_t mask=rtw8812RateMask(node->ni_rates,true)&(five?0xff0U:0xfffU);
    if(!mask)mask=rtw8812RateMask(node->ni_rates,false)&(five?0xff0U:0xfffU);
    if(!mask)mask=(uint16_t)(five?0x10:1);uint8_t rate=0;while(rate<11&&!(mask&(1U<<rate)))++rate;
    const bool firmware=data&&!multicast&&!eapol&&node==ic.ic_bss&&uses8812FirmwareRate();
    rtw_r12au_tx_options options{};options.payload_bytes=length;options.sequence=(uint16_t)(((uint16_t)header.i_seq[0]|((uint16_t)header.i_seq[1]<<8))>>4);
    options.rate=firmware?auRateConfig_.initial_rate:rate;options.rf_chains=auCore_.board.phy.rf_paths;options.data=data;options.qos=qos;options.tid=(uint8_t)tid;options.multicast=multicast;options.eapol=eapol;options.firmware_rate=firmware;options.band_5ghz=five;options.bandwidth_mhz=firmware?auRateConfig_.bandwidth_mhz:20;options.short_gi=firmware?auRateConfig_.short_gi:0;
    options.short_preamble=(ic.ic_flags&IEEE80211_F_SHPREAMBLE)&&options.rate>=1&&options.rate<=3;
    options.ack_expected=!multicast;options.retry_limit=multicast?0:7;
    if(data&&!multicast){if(length+4>(unsigned)ic.ic_rtsthreshold)options.protection=2;else if(ic.ic_flags&IEEE80211_F_USEPROT)options.protection=ic.ic_protmode==IEEE80211_PROT_CTSONLY?1:ic.ic_protmode==IEEE80211_PROT_RTSCTS?2:0;if(options.protection)options.rts_rate=rate;}
    if(firmware&&qos&&uses8812HighThroughput()&&(header.i_fc[1]&IEEE80211_FC1_PROTECTED)&&(ic.ic_flags&IEEE80211_F_RSNON)&&(node->ni_rsnprotos&IEEE80211_PROTO_RSN)&&node->ni_tx_ba[tid].ba_state==IEEE80211_BA_AGREED&&node->ni_tx_ba[tid].ba_winsize>0&&node->ni_tx_ba[tid].ba_winsize<=IEEE80211_BA_MAX_WINSZ&&!(ic.ic_tid_noack&(1U<<tid))){options.ampdu=1;options.density=(node->ni_ampdu_param&IEEE80211_AMPDU_PARAM_SS)>>2;options.max_agg=(uint8_t)((4U<<(node->ni_ampdu_param&IEEE80211_AMPDU_PARAM_LE))-1U);if(options.max_agg>node->ni_tx_ba[tid].ba_winsize)options.max_agg=(uint8_t)node->ni_tx_ba[tid].ba_winsize;}
    uint8_t descriptor[40]{};if(!rtw_r12au_make_tx(&options,firmware?&auRateConfig_:nullptr,descriptor)){mbuf_freem(packet);return EINVAL;}
    uint8_t endpoint=0;if(rtw_r12au_qsel_endpoint(&auCore_,(descriptor[5]&31),&endpoint)){mbuf_freem(packet);return EINVAL;}
    unsigned pipe=0;while(pipe<endpoints_.num_bulk_out&&endpoints_.bulk_out[pipe]!=endpoint)++pipe;
    if(pipe>=endpoints_.num_bulk_out){mbuf_freem(packet);return EINVAL;}size_t total=0;
    if(rtw_tx_total_transfer_size_for_descriptor(40,length,txMPS_[pipe],&total)!=RTW_PACKETS_OK||total>TXCapacity){mbuf_freem(packet);return EINVAL;}
    auto*t=&transfers_[index];auto*bytes=static_cast<uint8_t*>(t->memory->getBytesNoCopy());bzero(bytes,total);memcpy(bytes,descriptor,40);
    if(mbuf_copydata(packet,0,length,bytes+40)){mbuf_freem(packet);return EINVAL;}mbuf_freem(packet);
    int error=submit(index,txPipes_[pipe],(uint32_t)total);if(!error){if(auCoex_){if(auCoexTxFrames_!=UINT32_MAX)++auCoexTxFrames_;if(data&&!multicast&&UINT64_MAX-auCoexTxBytes_>=length)auCoexTxBytes_+=length;}if(options.rate>=12)++txHTPackets_;if(options.ampdu)++txAMPDUPackets_;ieee80211_release_node(&ic,node);}return error;
}
void RTWUSBController::watchdog8812(_ifnet*ifp)
{
    auto*core=static_cast<rtwn_softc*>(ifp->if_softc);ifp->if_timer=0;
    if(core->sc_tx_timer>0){if(--core->sc_tx_timer==0){++ifp->if_oerrors;task_add(systq,&core->init_task);return;}ifp->if_timer=1;}ieee80211_watchdog(ifp);
}
void RTWUSBController::initTask8812(void*p)
{
    auto*s=owner(p);auto&core=s->core_;auto&ifp=core.sc_ic.ic_if;if(core.sc_flags&RTWN_FLAG_BUSY)return;
    core.sc_flags|=RTWN_FLAG_BUSY;s->stop8812();if(ifp.if_flags&IFF_UP){int e=s->start8812();if(e&&!s->familyRecovery_.transient(e))core.core_error=e;}core.sc_flags&=~(uint32_t)RTWN_FLAG_BUSY;
}
int RTWUSBController::ioctl8812(_ifnet*ifp,u_long command,caddr_t data)
{
    auto*core=static_cast<rtwn_softc*>(ifp->if_softc);auto*s=owner(core->sc_ops.cookie);if(core->sc_flags&RTWN_FLAG_BUSY)return EBUSY;
    core->sc_flags|=RTWN_FLAG_BUSY;int error=0;
    switch(command){case SIOCSIFADDR:ifp->if_flags|=IFF_UP;/* FALLTHROUGH */
    case SIOCSIFFLAGS:if(ifp->if_flags&IFF_UP){if(!(ifp->if_flags&IFF_RUNNING))error=s->start8812();}else if(ifp->if_flags&IFF_RUNNING)s->stop8812();break;
    default:error=ieee80211_ioctl(ifp,command,data);break;}
    if(core->sc_ic.ic_opmode!=IEEE80211_M_STA){core->sc_ic.ic_opmode=IEEE80211_M_STA;error=EOPNOTSUPP;}
    if(error==ENETRESET){if((ifp->if_flags&(IFF_UP|IFF_RUNNING))==(IFF_UP|IFF_RUNNING)){s->stop8812();error=s->start8812();}else error=0;}
    core->sc_flags&=~(uint32_t)RTWN_FLAG_BUSY;return error;
}
