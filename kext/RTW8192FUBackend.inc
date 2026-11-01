/* SPDX-License-Identifier: GPL-2.0-only */
/* Dedicated two-path RTL8192FU controller adapter. Actual FU OTP, power,
 * 0x4000 firmware, DMA/LLT, PHY and measured RFK precede management TX.
 * Imported net80211 retains WPA2/CCMP, replay/MIC and controlled ports. */
#include "RTL8192FUFirmware.inc"
static uint16_t rtw8192FURateMask(const ieee80211_rateset&rates,bool basicOnly)
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
rtw_usb_error RTWUSBController::fuControlIn(void*p,const rtw_usb_setup*s,uint8_t*b,uint16_t*actual)
{
    if(actual)*actual=0;
    if(!p||!s||!b||!actual||s->bm_request_type!=0xc0||s->b_request!=5||s->w_index||
       (s->w_length!=1&&s->w_length!=2&&s->w_length!=4))return RTW_USB_INVALID_ARGUMENT;
    int e=owner(p)->vendorRequest(true,s->w_value,b,s->w_length);
    if(e)return RTW_USB_IO_ERROR;*actual=s->w_length;return RTW_USB_OK;
}
rtw_usb_error RTWUSBController::fuControlOut(void*p,const rtw_usb_setup*s,const uint8_t*b,uint16_t*actual)
{
    if(actual)*actual=0;
    if(!p||!s||!b||!actual||s->bm_request_type!=0x40||s->b_request!=5||s->w_index||!s->w_length||s->w_length>254)return RTW_USB_INVALID_ARGUMENT;
    /* writeRegion copies immutable firmware bytes into bounded writable USB
     * request storage. Scalar register and254-byte firmware transfers use the
     * same verified endpoint0 transport, not a reserved-page bulk path. */
    int e=writeRegion(p,s->w_value,b,s->w_length);
    if(e)return RTW_USB_IO_ERROR;*actual=s->w_length;return RTW_USB_OK;
}
int RTWUSBController::fuWait(void*p,uint32_t us)
{
    if(!p||ioError(p)||!us)return 0;delayUS(p,us);return !ioError(p);
}
bool RTWUSBController::is8192FUFamily() const {return family_==RTW_WIFI_RTL8192FU;}
bool RTWUSBController::supports8192FUBand(bool five) const {if(five||!is8192FUFamily()||!fu_)return false;for(uint8_t ch=1;ch<=11;++ch){rtw_r92fu_operating op{ch,ch,20,0};if(rtw_r92fu_channel_supported(&fuBoard_,&op))return true;}return false;}
int RTWUSBController::initialize8192FUCore()
{
    if(!is8192FUFamily()||!fuStorage_||!rxPipe_||endpoints_.num_bulk_in!=1||endpoints_.num_bulk_out<1||endpoints_.num_bulk_out>3||ioError(this))return ENXIO;
    auto speed=rxPipe_->getSpeed();if(speed!=kUSBHostConnectionSpeedHigh&&speed!=kUSBHostConnectionSpeedFull)return ENOTSUP;
    rtw_r92fu_usb usb{};usb.bulk_in=endpoints_.bulk_in[0];usb.bulk_out_count=endpoints_.num_bulk_out;usb.max_packet=txMPS_[0];
    if(usb.max_packet!=(speed==kUSBHostConnectionSpeedHigh?512:64))return EINVAL;
    for(unsigned i=0;i<usb.bulk_out_count;++i){if(!txPipes_[i]||txMPS_[i]!=usb.max_packet)return EINVAL;usb.bulk_out[i]=endpoints_.bulk_out[i];}
    const rtw_usb_rw_transport bus{this,fuControlIn,fuControlOut};const rtw_waiter wait{this,fuWait};int e=rtw_r92fu_init(fuStorage_,fuSize_,&bus,&wait,&usb,&fu_);
    if(!e)e=rtw_r92fu_identify(fu_);if(!e)e=rtw_r92fu_board_info(fu_,&fuBoard_);return e;
}
uint32_t RTWUSBController::channel8192FUModes(uint8_t channel) const
{
    uint32_t flags=0;rtw_r92fu_operating op{channel,channel,20,1};if(!rtw_r92fu_channel_supported(&fuBoard_,&op))return 0;flags|=IEEE80211_CHAN_HT20;
    op.width_mhz=40;op.center=(uint8_t)(channel+2);if(rtw_r92fu_channel_supported(&fuBoard_,&op))flags|=IEEE80211_CHAN_HT40U;
    if(channel>2){op.center=(uint8_t)(channel-2);if(rtw_r92fu_channel_supported(&fuBoard_,&op))flags|=IEEE80211_CHAN_HT40D;}return flags;
}
void RTWUSBController::refresh8192FUModes()
{
    auto&ic=core_.sc_ic;ic.ic_modecaps&=~((1U<<IEEE80211_MODE_11AC)|(1U<<IEEE80211_MODE_11AX));ic.ic_flags&=~(uint32_t)(IEEE80211_F_HTON|IEEE80211_F_VHTON|IEEE80211_F_HEON);
    if(fuHTSupported_&&(ic.ic_curmode==IEEE80211_MODE_AUTO||ic.ic_curmode==IEEE80211_MODE_11N))ic.ic_flags|=IEEE80211_F_HTON;
}
int RTWUSBController::attach8192FU()
{
    if(fuStorage_||fu_||running_)return EBUSY;fuSize_=rtw_r92fu_size();fuStorage_=IOMalloc(fuSize_);if(!fuStorage_)return ENOMEM;bzero(fuStorage_,fuSize_);
    int e=initialize8192FUCore();if(e){release8192FUStorage();return e;}
    core_.sc_pdev=&device_;core_.ntxchains=core_.nrxchains=2;core_.avg_pwdb=-1;task_set(&core_.init_task,initTask8192FU,this,"rtw8192fu_init");
    auto&ic=core_.sc_ic;auto&ifp=ic.ic_if;memcpy(ic.ic_myaddr,fuBoard_.mac,IEEE80211_ADDR_LEN);ic.ic_phytype=IEEE80211_T_OFDM;ic.ic_opmode=IEEE80211_M_STA;ic.ic_state=IEEE80211_S_INIT;
    ic.ic_caps=IEEE80211_C_SHPREAMBLE|IEEE80211_C_SHSLOT|IEEE80211_C_RSN;ic.ic_htcaps=0;ic.ic_tx_mcs_set=0;memset(ic.ic_sup_mcs,0,sizeof(ic.ic_sup_mcs));ic.ic_flags&=~(uint32_t)IEEE80211_F_QOS;
    ic.ic_sup_rates[IEEE80211_MODE_11B]=ieee80211_std_rateset_11b;ic.ic_sup_rates[IEEE80211_MODE_11G]=ieee80211_std_rateset_11g;
    uint8_t initial=0;fuHTSupported_=false;for(uint8_t ch=1;ch<=11;++ch){rtw_r92fu_operating legacy{ch,ch,20,0};if(!rtw_r92fu_channel_supported(&fuBoard_,&legacy))continue;
        ic.ic_channels[ch].ic_freq=(uint16_t)ieee80211_ieee2mhz(ch,IEEE80211_CHAN_2GHZ);ic.ic_channels[ch].ic_flags=IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM|IEEE80211_CHAN_DYN|IEEE80211_CHAN_2GHZ;
        uint32_t modes=channel8192FUModes(ch);ic.ic_channels[ch].ic_flags|=modes;fuHTSupported_|=(modes&IEEE80211_CHAN_HT20)!=0;if(!initial)initial=ch;}
    if(!initial){release8192FUStorage();return ENOTSUP;}ic.ic_ibss_chan=&ic.ic_channels[initial];
    if(fuHTSupported_){ic.ic_caps|=IEEE80211_C_QOS|IEEE80211_C_TX_AMPDU;ic.ic_flags|=IEEE80211_F_QOS;ic.ic_htcaps=IEEE80211_HTCAP_CBW20_40|IEEE80211_HTCAP_SGI20|IEEE80211_HTCAP_SGI40|(IEEE80211_HTCAP_SMPS_DIS<<IEEE80211_HTCAP_SMPS_SHIFT);ic.ic_ampdu_params=IEEE80211_AMPDU_PARAM_LE|IEEE80211_AMPDU_PARAM_SS_16;ic.ic_sup_mcs[0]=ic.ic_sup_mcs[1]=255;ic.ic_tx_mcs_set=IEEE80211_TX_MCS_SET_DEFINED;}
    ifp.if_softc=&core_;ifp.if_flags=IFF_BROADCAST|IFF_SIMPLEX|IFF_MULTICAST;ifp.if_ioctl=ioctl8192FU;ifp.if_start=rtwn_start;ifp.if_watchdog=watchdog8192FU;memcpy(ifp.if_xname,device_.dv_xname,IFNAMSIZ);if_attach(&ifp);ieee80211_ifattach(&ifp,this);
    ic.ic_updateslot=update8192FUEDCA;ic.ic_updateedca=update8192FUEDCA;core_.sc_newstate=ic.ic_newstate;ic.ic_newstate=stateTransition;refresh8192FUModes();ieee80211_media_init(&ifp);refresh8192FUModes();
    setProperty("RTWFUChip",0x92f,16);setProperty("RTWFUCut",fuBoard_.cut,8);setProperty("RTWFURFE",fuBoard_.rfe,8);setProperty("RTWFURFPaths",2,8);return 0;
}
int RTWUSBController::start8192FU()
{
    auto&ic=core_.sc_ic;auto&ifp=ic.ic_if;if(!is8192FUFamily()||!fuStorage_||!fu_||!ic.ic_bss||ic.ic_opmode!=IEEE80211_M_STA||ioError(this))return ENXIO;rtw_r92fu_status status{};rtw_r92fu_status_read(fu_,&status);if(status.powered||status.firmware)return EBUSY;
    radioReady_=false;core_.core_error=0;fuMedia_=fuRateReady_=fuRSSIValid_=false;fuRate_={};fuChannel_=fuRSSI_=0;fuOperating_={};refresh8192FUModes();
    int e=initialize8192FUCore();if(e)goto fail;e=allocateTransfers();if(e)goto fail;e=rtw_r92fu_boot(fu_,RTW8192FUFirmware,sizeof(RTW8192FUFirmware));if(e)goto fail;
    rtw_r92fu_status_read(fu_,&status);if(status.max_bw<40){ic.ic_htcaps&=~(uint32_t)(IEEE80211_HTCAP_CBW20_40|IEEE80211_HTCAP_SGI40);for(unsigned ch=1;ch<=11;++ch)ic.ic_channels[ch].ic_flags&=~(uint32_t)IEEE80211_CHAN_HT40;}
    ic.ic_bss->ni_chan=ic.ic_ibss_chan;e=set8192FUChannel((uint8_t)ieee80211_chan2ieee(&ic,ic.ic_ibss_chan));if(e)goto fail;e=busInitialize(this);if(e)goto fail;ifq_clr_oactive(&ifp.if_snd);ifp.if_flags|=IFF_RUNNING;
    e=ic.ic_newstate(&ic,IEEE80211_S_SCAN,-1);if(e)goto fail;radioReady_=true;return 0;
fail:stop8192FU();core_.core_error=e;return e;
}
void RTWUSBController::stop8192FU()
{
    auto&ifp=core_.sc_ic.ic_if;radioReady_=false;core_.sc_tx_timer=0;ifp.if_timer=0;ifp.if_flags&=~IFF_RUNNING;ifq_clr_oactive(&ifp.if_snd);
    if(core_.sc_newstate)(void)state8192FU(&core_.sc_ic,IEEE80211_S_INIT,-1);cancelScan(this);cancelCalibration(this);task_del(systq,&core_.init_task);abortTransfers();
    if(fu_){if(present_&&!sleeping_&&usbError_==kIOReturnSuccess)(void)rtw_r92fu_stop(fu_);else rtw_r92fu_forget(fu_);}fuMedia_=fuRateReady_=fuRSSIValid_=false;fuRate_={};fuOperating_={};fuChannel_=fuRSSI_=0;freeTransfers();
}
void RTWUSBController::release8192FUStorage(){if(fuStorage_){IOFree(fuStorage_,fuSize_);fuStorage_=nullptr;}fuSize_=0;fu_=nullptr;}
uint16_t RTWUSBController::station8192FURates(bool basic) const
{
    const auto*n=core_.sc_ic.ic_bss;if(!n)return 0;uint16_t mask=rtw8192FURateMask(n->ni_rates,basic);if(basic&&!mask){mask=rtw8192FURateMask(n->ni_rates,false);mask&=(uint16_t)(0U-mask);}return mask;
}
int RTWUSBController::set8192FUOperating(const rtw_r92fu_operating&op){if(!fu_)return ENXIO;int e=rtw_r92fu_set_operating(fu_,&op);if(!e){fuOperating_=op;fuChannel_=op.primary;}else{fuOperating_={};fuChannel_=0;core_.core_error=e;}return e;}
int RTWUSBController::set8192FUChannel(uint8_t ch){const rtw_r92fu_operating op{ch,ch,20,0};return set8192FUOperating(op);}
int RTWUSBController::peer8192FUOperating(rtw_r92fu_operating*out) const
{
    if(!out)return EINVAL;const auto*n=core_.sc_ic.ic_bss;if(!n||!n->ni_chan||n->ni_chan==IEEE80211_CHAN_ANYC)return ENXIO;unsigned ch=ieee80211_chan2ieee(const_cast<ieee80211com*>(&core_.sc_ic),n->ni_chan);if(ch>255)return EINVAL;rtw_r92fu_operating op{(uint8_t)ch,(uint8_t)ch,20,(uint8_t)(fuHTSupported_&&(n->ni_flags&IEEE80211_NODE_HT)?1:0)};
    if(op.ht){if(n->ni_chw==IEEE80211_CHAN_WIDTH_40){unsigned sco=n->ni_htop0&IEEE80211_HTOP0_SCO_MASK;op.width_mhz=40;if(sco==IEEE80211_HTOP0_SCO_SCA)op.center=(uint8_t)(ch+2);else if(sco==IEEE80211_HTOP0_SCO_SCB&&ch>2)op.center=(uint8_t)(ch-2);else return EINVAL;}else if(n->ni_chw!=IEEE80211_CHAN_WIDTH_20&&n->ni_chw!=IEEE80211_CHAN_WIDTH_20_NOHT)return ENOTSUP;}
    if(!rtw_r92fu_channel_supported(&fuBoard_,&op))return ENOTSUP;*out=op;return 0;
}
int RTWUSBController::station8192FUHardware(ieee80211_state state)
{
    const auto*n=core_.sc_ic.ic_bss;const bool run=state==IEEE80211_S_RUN;if(!fu_||(!n&&state!=IEEE80211_S_INIT))return ENXIO;
    uint16_t basic=station8192FURates(true);if(!basic&&!run)basic=1;int e=rtw_r92fu_station(fu_,run,run?n->ni_bssid:nullptr,run?(n->ni_associd&0x3fff):0,run?n->ni_intval:0,basic,(core_.sc_ic.ic_flags&IEEE80211_F_SHSLOT)!=0,run&&fuOperating_.ht);
    if(state==IEEE80211_S_INIT){int stopped=rtw_r92fu_invalidate(fu_);fuChannel_=0;return e?e:stopped;}
    if(!e&&(state==IEEE80211_S_AUTH||state==IEEE80211_S_ASSOC)){for(unsigned i=0;!e&&i<6;++i)e=rtw_r92fu_write(fu_,(uint16_t)(0x618+i),1,n->ni_bssid[i]);}
    if(!e)e=state==IEEE80211_S_SCAN&&n->ni_chan&&(n->ni_chan->ic_flags&IEEE80211_CHAN_PASSIVE)?rtw_r92fu_pause(fu_):rtw_r92fu_resume(fu_);return e;
}
int RTWUSBController::state8192FU(ieee80211com*ic,ieee80211_state state,int argument)
{
    if(!ic||ic->ic_opmode!=IEEE80211_M_STA||!core_.sc_newstate||state<IEEE80211_S_INIT||state>IEEE80211_S_RUN)return EINVAL;auto previous=ic->ic_state;cancelScan(this);int error=0;rtw_r92fu_status status{};if(fu_)rtw_r92fu_status_read(fu_,&status);
    if((previous==IEEE80211_S_RUN&&state!=IEEE80211_S_RUN)||state==IEEE80211_S_INIT){cancelCalibration(this);if(fuMedia_&&fu_&&!status.error&&status.firmware){uint8_t command[8]={1,0x20};error=rtw_r92fu_h2c(fu_,command);}fuMedia_=fuRateReady_=fuRSSIValid_=false;fuRate_={};}
    if(state==IEEE80211_S_INIT){if(fu_&&status.mac&&!status.error){int e=station8192FUHardware(state);if(!error)error=e;}if(fu_){int e=rtw_r92fu_invalidate(fu_);if(!error)error=e;}fuChannel_=0;int e=core_.sc_newstate(ic,state,argument);return error?error:e;}
    auto rollback=[&](int e)->int{if(fuMedia_&&fu_){uint8_t disconnect[8]={1,0x20};(void)rtw_r92fu_h2c(fu_,disconnect);}fuMedia_=fuRateReady_=false;fuRate_={};radioReady_=false;if(fu_)(void)rtw_r92fu_invalidate(fu_);(void)core_.sc_newstate(ic,IEEE80211_S_INIT,-1);core_.core_error=e;return e;};
    if(error)return rollback(error);if(!fu_||!ic->ic_bss||!ic->ic_bss->ni_chan||ic->ic_bss->ni_chan==IEEE80211_CHAN_ANYC||!status.firmware||!status.mac||status.error)return rollback(ENXIO);
    unsigned channel=ieee80211_chan2ieee(ic,ic->ic_bss->ni_chan);rtw_r92fu_operating legacy{(uint8_t)channel,(uint8_t)channel,20,0};if(channel>255||!rtw_r92fu_channel_supported(&fuBoard_,&legacy))return rollback(ENOTSUP);
    switch(state){
    case IEEE80211_S_SCAN:if(!status.calibrated||fuChannel_!=channel||fuOperating_.ht)error=set8192FUChannel((uint8_t)channel);if(!error)error=station8192FUHardware(state);break;
    case IEEE80211_S_AUTH:error=set8192FUChannel((uint8_t)channel);if(!error)error=station8192FUHardware(state);break;
    case IEEE80211_S_ASSOC:if(!status.calibrated||fuChannel_!=channel)return rollback(ENXIO);error=station8192FUHardware(state);break;
    case IEEE80211_S_RUN:{
        if(!status.calibrated||fuChannel_!=channel)return rollback(ENXIO);fuRateReady_=false;rtw_r92fu_operating operating{};error=peer8192FUOperating(&operating);if(error)return rollback(error);
        if(operating.primary!=fuOperating_.primary||operating.center!=fuOperating_.center||operating.width_mhz!=fuOperating_.width_mhz||operating.ht!=fuOperating_.ht){error=set8192FUOperating(operating);if(error)return rollback(error);}
        const auto*n=ic->ic_bss;uint16_t rates=station8192FURates(false),basic=station8192FURates(true),aid=n->ni_associd&0x3fff;if(!rates||!basic||!aid||aid>2007||!n->ni_intval)return rollback(EINVAL);
        uint8_t mcs[2]={n->ni_rxmcs[0],n->ni_rxmcs[1]},rate[8]{},media[8]={1,0x21};unsigned sgi=operating.width_mhz==40?IEEE80211_HTCAP_SGI40:IEEE80211_HTCAP_SGI20;rtw_r92fu_rate config{};
        /* Imported net80211 preserves the peer's raw RX MCS set. Static
         * SMPS permits one stream; dynamic SMPS also stays at one until an
         * explicit per-packet multi-stream wake/protection path exists.
         * A reserved SMPS value never expands the peer's receive capacity. */
        if(operating.ht&&((n->ni_htcaps&IEEE80211_HTCAP_SMPS_MASK)>>IEEE80211_HTCAP_SMPS_SHIFT)!=IEEE80211_HTCAP_SMPS_DIS)mcs[1]=0;
        if(!rtw_r92fu_rate_build(rates,mcs,operating.ht,operating.width_mhz,(ic->ic_htcaps&sgi)!=0,(n->ni_htcaps&sgi)!=0,&config)||!rtw_r92fu_rate_command(&config,1,rate))return rollback(EINVAL);
        error=rtw_r92fu_h2c(fu_,rate);if(!error){error=rtw_r92fu_h2c(fu_,media);if(!error)fuMedia_=true;}if(!error)error=station8192FUHardware(state);if(!error){fuRate_=config;update8192FUEDCA(ic);if(core_.core_error)error=core_.core_error;}break;}
    case IEEE80211_S_INIT:return EINVAL;
    }
    if(error)return rollback(error);error=core_.sc_newstate(ic,state,argument);if(error)return rollback(error);if(state==IEEE80211_S_RUN){fuRateReady_=true;fuRSSIValid_=false;nextCalibration(this);}if(state==IEEE80211_S_SCAN)nextScan(this);return 0;
}
void RTWUSBController::update8192FUEDCA(ieee80211com*ic)
{
    if(!ic||!ic->ic_softc)return;auto*core=static_cast<rtwn_softc*>(ic->ic_softc);auto*s=owner(core->sc_ops.cookie);if(!s->fu_)return;rtw_r92fu_status status{};rtw_r92fu_status_read(s->fu_,&status);if(!status.mac)return;
    const auto*n=ic->ic_bss;unsigned mode=s->station8192FURates(false)&0xff0?IEEE80211_MODE_11G:IEEE80211_MODE_11B;const auto*p=((ic->ic_flags&IEEE80211_F_QOS)&&n&&(n->ni_flags&IEEE80211_NODE_QOS))?ic->ic_edca_ac:ieee80211_edca_table[mode];rtw_r92fu_edca ac[4]{};for(unsigned i=0;i<4;++i)ac[i]={p[i].ac_aifsn,p[i].ac_ecwmin,p[i].ac_ecwmax,p[i].ac_txoplimit};int e=rtw_r92fu_edca_update(s->fu_,(ic->ic_flags&IEEE80211_F_SHSLOT)!=0,ac);if(e)core->core_error=e;
}
bool RTWUSBController::uses8192FUFirmwareRate() const
{
    if(!is8192FUFamily()||!fu_||!fuRateReady_||!fuMedia_||!running_||stopping_||core_.core_error||core_.sc_ic.ic_state!=IEEE80211_S_RUN||!core_.sc_ic.ic_bss)return false;rtw_r92fu_status s{};rtw_r92fu_status_read(fu_,&s);return s.firmware&&s.mac&&s.calibrated&&s.tx_enabled&&!s.error&&s.primary==fuChannel_;
}
bool RTWUSBController::uses8192FUHighThroughput() const{return uses8192FUFirmwareRate()&&fuRate_.ht;}
bool RTWUSBController::accepts8192FUReceiveRate(const rtw_rx_packet&p) const {return p.rate_index<12||(uses8192FUHighThroughput()&&p.rate_index<28&&p.bandwidth_code<=1&&(p.bandwidth_code==0||fuOperating_.width_mhz==40));}
int RTWUSBController::ampdu8192FUTXStart(ieee80211com*ic,ieee80211_node*n,uint8_t tid)
{
    if(!ic||!ic->ic_softc||!n||tid>=8)return EINVAL;auto*core=static_cast<rtwn_softc*>(ic->ic_softc);auto*s=owner(core->sc_ops.cookie);if(!s)return EINVAL;const auto&ba=n->ni_tx_ba[tid];return s->uses8192FUHighThroughput()&&s->authorized()&&n==ic->ic_bss&&!(ic->ic_tid_noack&(1U<<tid))&&ba.ba_winsize>=1&&ba.ba_winsize<=IEEE80211_BA_MAX_WINSZ&&(ic->ic_flags&IEEE80211_F_RSNON)&&(n->ni_rsnprotos&IEEE80211_PROTO_RSN)?0:EINVAL;
}
int RTWUSBController::tick8192FU()
{
    if(core_.sc_ic.ic_state!=IEEE80211_S_RUN)return 0;if(!uses8192FUFirmwareRate())return ENXIO;int e=rtw_r92fu_tick(fu_);if(e==EAGAIN)e=0;if(!e&&fuRSSIValid_){uint8_t command[8]={0x42,0,0,fuRSSI_};e=rtw_r92fu_h2c(fu_,command);}return e;
}
int RTWUSBController::sample8192FURSSI(const rtw_rx_packet&p)
{
    int dbm=0;if(!p.phy_status_present||!fu_||!rtw_r92fu_decode_phy(fu_,p.rate_index,p.phy_info,p.phy_info_length,&dbm))return 0;const auto*n=core_.sc_ic.ic_bss;if(n&&core_.sc_ic.ic_state==IEEE80211_S_RUN&&rtw_signal_from_ap(p.mpdu,p.mpdu_length,n->ni_bssid)){fuRSSI_=(uint8_t)(dbm<=-100?0:dbm>=-1?100:100+dbm);fuRSSIValid_=true;}return dbm;
}
void RTWUSBController::handle8192FUC2H(const uint8_t*b,size_t n)
{
    if(!b||n<2){++rateControl_.invalid;return;}if(b[0]!=0x0c){++rateControl_.ignored;return;}if(n<6||n>256||b[3]||!fuRateReady_||(b[2]&0x7f)>=28||((b[2]&0x80)&&!fuRate_.sgi)||!(fuRate_.mask&(1U<<(b[2]&0x7f)))){++rateControl_.invalid;return;}if(uses8192FUFirmwareRate()){++rateControl_.reports;rateControl_.finalRate=b[2]&0x7f;}
}
rtw_packet_error RTWUSBController::decode8192FUUSBFrame(rtw_rx_iterator*it,rtw_8192eu_rx_frame*out,uint8_t*reportType) const
{
    if(out)memset(out,0,sizeof(*out));if(reportType)*reportType=0;
    if(!it||!out||!reportType||it->frames>RTW_R92FU_MAX_RX_FRAMES)return RTW_PACKETS_BAD_ARGUMENT;
    rtw_r92fu_rx_iterator iterator{it->buffer,it->length,it->cursor,(unsigned)it->frames};rtw_r92fu_rx_frame frame{};
    auto result=rtw_r92fu_rx_next(&iterator,2,&frame);
    switch(result){
    case RTW_R92FU_FRAME:break;
    case RTW_R92FU_END:it->cursor=iterator.cursor;it->frames=iterator.frames;return RTW_PACKETS_DONE;
    case RTW_R92FU_TRUNCATED:return RTW_PACKETS_TRUNCATED;
    case RTW_R92FU_MALFORMED:return RTW_PACKETS_MALFORMED;
    case RTW_R92FU_TOO_MANY:return RTW_PACKETS_TOO_MANY;
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
int RTWUSBController::send8192FUFrame(mbuf_t packet,ieee80211_node*node)
{
    if(!packet)return EINVAL;
    rtw_r92fu_status phy{};if(fu_)rtw_r92fu_status_read(fu_,&phy);
    if(!running_||stopping_||!present_||!node||usbError_!=kIOReturnSuccess||!phy.calibrated||!phy.tx_enabled||phy.error||phy.error||!phy.mac){mbuf_freem(packet);return ENETDOWN;}
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
    const bool five=false;uint16_t mask=rtw8192FURateMask(node->ni_rates,true)&(five?0xff0U:0xfffU);
    if(!mask)mask=rtw8192FURateMask(node->ni_rates,false)&(five?0xff0U:0xfffU);
    if(!mask)mask=(uint16_t)(five?0x10:1);uint8_t rate=0;while(rate<11&&!(mask&(1U<<rate)))++rate;
    const bool firmware=data&&!multicast&&!eapol&&node==ic.ic_bss&&uses8192FUFirmwareRate();
    rtw_r92fu_tx_options options{};options.payload_bytes=length;options.sequence=(uint16_t)(((uint16_t)header.i_seq[0]|((uint16_t)header.i_seq[1]<<8))>>4);
    options.rate=firmware?fuRate_.initial:rate;options.rf_chains=2;options.data=data;options.qos=qos;options.tid=(uint8_t)tid;options.multicast=multicast;options.eapol=eapol;options.firmware_rate=firmware;options.band_5ghz=five;options.bandwidth_mhz=firmware?fuRate_.width_mhz:20;options.short_gi=firmware?fuRate_.sgi:0;
    options.short_preamble=(ic.ic_flags&IEEE80211_F_SHPREAMBLE)&&options.rate>=1&&options.rate<=3;
    options.ack_expected=!multicast;options.retry_limit=multicast?0:7;
    if(data&&!multicast){if(length+4>(unsigned)ic.ic_rtsthreshold)options.protection=2;else if(ic.ic_flags&IEEE80211_F_USEPROT)options.protection=ic.ic_protmode==IEEE80211_PROT_CTSONLY?1:ic.ic_protmode==IEEE80211_PROT_RTSCTS?2:0;if(options.protection)options.rts_rate=rate;}
    if(firmware&&qos&&uses8192FUHighThroughput()&&(header.i_fc[1]&IEEE80211_FC1_PROTECTED)&&(ic.ic_flags&IEEE80211_F_RSNON)&&(node->ni_rsnprotos&IEEE80211_PROTO_RSN)&&node->ni_tx_ba[tid].ba_state==IEEE80211_BA_AGREED&&node->ni_tx_ba[tid].ba_winsize>0&&node->ni_tx_ba[tid].ba_winsize<=IEEE80211_BA_MAX_WINSZ&&!(ic.ic_tid_noack&(1U<<tid))){options.ampdu=1;options.density=(node->ni_ampdu_param&IEEE80211_AMPDU_PARAM_SS)>>2;options.max_agg=(uint8_t)((4U<<(node->ni_ampdu_param&IEEE80211_AMPDU_PARAM_LE))-1U);if(options.max_agg>node->ni_tx_ba[tid].ba_winsize)options.max_agg=(uint8_t)node->ni_tx_ba[tid].ba_winsize;}
    /* Primary FU v2 TX requests hardware RTS/CTS for an operational AMPDU. */
    if(options.ampdu){options.protection=2;options.rts_rate=rate;}
    uint8_t descriptor[40]{};if(!rtw_r92fu_make_tx(&options,firmware?&fuRate_:nullptr,descriptor)){mbuf_freem(packet);return EINVAL;}
    unsigned pipe=0;if(rtw_r92fu_tx_endpoint(fu_,(descriptor[5]&31),&pipe)||pipe>=endpoints_.num_bulk_out){mbuf_freem(packet);return EINVAL;}size_t total=0;
    if(rtw_tx_total_transfer_size_for_descriptor(40,length,txMPS_[pipe],&total)!=RTW_PACKETS_OK||total>TXCapacity){mbuf_freem(packet);return EINVAL;}
    auto*t=&transfers_[index];auto*bytes=static_cast<uint8_t*>(t->memory->getBytesNoCopy());bzero(bytes,total);memcpy(bytes,descriptor,40);
    if(mbuf_copydata(packet,0,length,bytes+40)){mbuf_freem(packet);return EINVAL;}mbuf_freem(packet);
    int error=submit(index,txPipes_[pipe],(uint32_t)total);if(!error){if(options.rate>=12)++txHTPackets_;if(options.ampdu)++txAMPDUPackets_;ieee80211_release_node(&ic,node);}return error;
}
void RTWUSBController::watchdog8192FU(_ifnet*ifp)
{
    auto*core=static_cast<rtwn_softc*>(ifp->if_softc);ifp->if_timer=0;
    if(core->sc_tx_timer>0){if(--core->sc_tx_timer==0){++ifp->if_oerrors;task_add(systq,&core->init_task);return;}ifp->if_timer=1;}ieee80211_watchdog(ifp);
}
void RTWUSBController::initTask8192FU(void*p)
{
    auto*s=owner(p);auto&core=s->core_;auto&ifp=core.sc_ic.ic_if;if(core.sc_flags&RTWN_FLAG_BUSY)return;
    core.sc_flags|=RTWN_FLAG_BUSY;s->stop8192FU();if(ifp.if_flags&IFF_UP){int e=s->start8192FU();if(e)core.core_error=e;}core.sc_flags&=~(uint32_t)RTWN_FLAG_BUSY;
}
int RTWUSBController::ioctl8192FU(_ifnet*ifp,u_long command,caddr_t data)
{
    auto*core=static_cast<rtwn_softc*>(ifp->if_softc);auto*s=owner(core->sc_ops.cookie);if(core->sc_flags&RTWN_FLAG_BUSY)return EBUSY;
    core->sc_flags|=RTWN_FLAG_BUSY;int error=0;
    switch(command){case SIOCSIFADDR:ifp->if_flags|=IFF_UP;/* FALLTHROUGH */
    case SIOCSIFFLAGS:if(ifp->if_flags&IFF_UP){if(!(ifp->if_flags&IFF_RUNNING))error=s->start8192FU();}else if(ifp->if_flags&IFF_RUNNING)s->stop8192FU();break;
    default:error=ieee80211_ioctl(ifp,command,data);break;}
    if(core->sc_ic.ic_opmode!=IEEE80211_M_STA){core->sc_ic.ic_opmode=IEEE80211_M_STA;error=EOPNOTSUPP;}
    if(error==ENETRESET){if((ifp->if_flags&(IFF_UP|IFF_RUNNING))==(IFF_UP|IFF_RUNNING)){s->stop8192FU();error=s->start8192FU();}else error=0;}
    core->sc_flags&=~(uint32_t)RTWN_FLAG_BUSY;return error;
}
