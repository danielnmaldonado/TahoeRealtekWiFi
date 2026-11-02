#include "rtw/StationChannels.h"
/* SPDX-License-Identifier: BSD-3-Clause
 * RTL8814AU station adapter. net80211 owns scanning, authentication, RSN,
 * software CCMP and the controlled port; all hardware uses the 8814 backend.
 * rtwn_start is reused solely as the existing host mbuf/management dispatcher.
 */
static uint16_t rtw8814RateMask(const ieee80211_rateset &rates,bool basicOnly)
{
    static const uint8_t values[12]={2,4,11,22,12,18,24,36,48,72,96,108};
    if(rates.rs_nrates>sizeof(rates.rs_rates))return 0;
    uint16_t mask=0;
    for(unsigned i=0;i<rates.rs_nrates;i++) {
        if(basicOnly&&!(rates.rs_rates[i]&IEEE80211_RATE_BASIC))continue;
        uint8_t rate=rates.rs_rates[i]&~IEEE80211_RATE_BASIC;
        for(unsigned j=0;j<12;j++)if(rate==values[j])mask|=(uint16_t)(1u<<j);
    }
    return mask;
}
rtw_8814au_station_params RTWUSBController::station8814Params() const
{
    rtw_8814au_station_params p{};const auto&ic=core_.sc_ic;
    memcpy(p.mac,ic.ic_myaddr,sizeof(p.mac));p.short_slot=(ic.ic_flags&IEEE80211_F_SHSLOT)!=0;
    if(const auto*n=ic.ic_bss){
        memcpy(p.bssid,n->ni_bssid,sizeof(p.bssid));p.aid=n->ni_associd&0x3fff;
        p.band_5g=n->ni_chan&&n->ni_chan!=IEEE80211_CHAN_ANYC&&(n->ni_chan->ic_flags&IEEE80211_CHAN_5GHZ)!=0;
        p.ht=(n->ni_flags&IEEE80211_NODE_HT)!=0;
        p.passive_scan=n->ni_chan&&n->ni_chan!=IEEE80211_CHAN_ANYC&&(n->ni_chan->ic_flags&IEEE80211_CHAN_PASSIVE)!=0;
        if(p.band_5g)p.short_slot=1;
        p.basic_rates=rtw8814RateMask(n->ni_rates,true)&(p.band_5g?0xff0U:0xfffU);
        if(!p.basic_rates){uint16_t mask=rtw8814RateMask(n->ni_rates,false)&(p.band_5g?0xff0U:0xfffU);p.basic_rates=mask&(uint16_t)(0u-mask);}
    }
    return p;
}
int RTWUSBController::attach8814()
{
    const rtw_8814au_bus_ops ops{this,acRead,acWrite,acBulk,delayUS};
    rtw_8814au_init(&acCore_,&ops);
    int e=rtw_8814au_identify_read_board(&acCore_,acSpeed());if(e)return e;
    /* This station port caps active stream metadata to2T2R even on USB3.
     * Preserve the original ROM, four physical RF paths and actual USB speed;
     * MAC/DMA still use SuperSpeed programming where applicable. */
    acCore_.board.tx_streams=acCore_.board.rx_streams=2;
    acCore_.board.antenna_tx=acCore_.board.antenna_rx=3;
    if(!capture8814RecoveryIdentity())return ENODEV;
    acPHYSize_=rtw_8814au_phy_size();
    acPHYStorage_=IOMalloc(acPHYSize_);if(!acPHYStorage_)return ENOMEM;
    bzero(acPHYStorage_,acPHYSize_);
    /* PHY context initialization requires powered MAC and running firmware;
     * start8814 performs it after those stages. Attach only owns storage. */
    core_.sc_pdev=&device_;core_.ntxchains=acCore_.board.tx_streams;core_.nrxchains=acCore_.board.rx_streams;
    core_.avg_pwdb=-1;task_set(&core_.init_task,initTask8814,this,"rtw8814_init");
    auto&ic=core_.sc_ic;auto&ifp=ic.ic_if;
    memcpy(ic.ic_myaddr,acCore_.board.mac,IEEE80211_ADDR_LEN);
    ic.ic_phytype=IEEE80211_T_OFDM;ic.ic_opmode=IEEE80211_M_STA;ic.ic_state=IEEE80211_S_INIT;
    ic.ic_caps=IEEE80211_C_SHPREAMBLE|IEEE80211_C_SHSLOT|IEEE80211_C_RSN|IEEE80211_C_SCANALLBAND;
    /* HT20: net80211 owns BA negotiation, reordering and software CCMP.
     * Individual USB MPDUs request bounded radio aggregation only after BA. */
    ic.ic_caps|=IEEE80211_C_QOS|IEEE80211_C_TX_AMPDU;ic.ic_flags|=IEEE80211_F_QOS;
    ic.ic_htcaps=IEEE80211_HTCAP_SGI20|(IEEE80211_HTCAP_SMPS_DIS<<IEEE80211_HTCAP_SMPS_SHIFT);
    ic.ic_ampdu_params=IEEE80211_AMPDU_PARAM_LE|IEEE80211_AMPDU_PARAM_SS_16;
    memset(ic.ic_sup_mcs,0,sizeof(ic.ic_sup_mcs));ic.ic_sup_mcs[0]=0xff;
    if(core_.nrxchains>1)ic.ic_sup_mcs[1]=0xff;
    ic.ic_tx_mcs_set=IEEE80211_TX_MCS_SET_DEFINED;
    ic.ic_sup_rates[IEEE80211_MODE_11A]=ieee80211_std_rateset_11a;
    ic.ic_sup_rates[IEEE80211_MODE_11B]=ieee80211_std_rateset_11b;
    ic.ic_sup_rates[IEEE80211_MODE_11G]=ieee80211_std_rateset_11g;
    uint8_t initial=0;
    for(unsigned ch=1;ch<=11;ch++){
        if(!rtw_8814au_phy_channel_supported(&acCore_,(uint8_t)ch,20))continue;
        if(!initial)initial=(uint8_t)ch;
        ic.ic_channels[ch].ic_freq=ieee80211_ieee2mhz(ch,IEEE80211_CHAN_2GHZ);
        ic.ic_channels[ch].ic_flags=IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM|IEEE80211_CHAN_DYN|IEEE80211_CHAN_2GHZ|IEEE80211_CHAN_HT20;
    }
    /* Passive observation, including DFS. Net80211 skips these channels during
     * active scans; the PHY keeps REG_TXPAUSE set during passive scans.
     * AUTH switches to calibrated operating power before transmission. */
    static const uint8_t passive5g[]=RTW_STATION_5GHZ_CHANNELS;
    for(uint8_t ch:passive5g){
        if(!permitsDFSChannelByCountry(ch))continue;
        if(!rtw_8814au_phy_channel_supported(&acCore_,ch,20))continue;
        if(!initial)initial=ch;
        ic.ic_channels[ch].ic_freq=ieee80211_ieee2mhz(ch,IEEE80211_CHAN_5GHZ);
        ic.ic_channels[ch].ic_flags=IEEE80211_CHAN_5GHZ|
            IEEE80211_CHAN_OFDM|IEEE80211_CHAN_PASSIVE|IEEE80211_CHAN_HT20;
    }
    if(!initial)return ENOTSUP;
    ic.ic_ibss_chan=&ic.ic_channels[initial];
    ifp.if_softc=&core_;ifp.if_flags=IFF_BROADCAST|IFF_SIMPLEX|IFF_MULTICAST;
    ifp.if_ioctl=ioctl8814;ifp.if_start=rtwn_start;ifp.if_watchdog=watchdog8814;
    memcpy(ifp.if_xname,device_.dv_xname,IFNAMSIZ);
    if_attach(&ifp);ieee80211_ifattach(&ifp,this);
    ic.ic_updateslot=update8814EDCA;ic.ic_updateedca=update8814EDCA;
    ic.ic_ampdu_rx_start=ampduRXStart;ic.ic_ampdu_rx_stop=ampduRXStop;
    core_.sc_newstate=ic.ic_newstate;ic.ic_newstate=stateTransition;
    ieee80211_media_init(&ifp);
    setProperty("RTW8814Cut",(unsigned long long)acCore_.board.cut,8);
    setProperty("RTW8814RFE",(unsigned long long)acCore_.board.rfe_option,8);
    setProperty("RTW8814PhysicalRFPaths",(unsigned long long)acCore_.board.physical_rf_paths,8);
    setProperty("RTW8814TXStreams",(unsigned long long)acCore_.board.tx_streams,8);
    return 0;
}
int RTWUSBController::start8814()
{
    auto&ic=core_.sc_ic;auto&ifp=ic.ic_if;
    if(ic.ic_opmode!=IEEE80211_M_STA||!ic.ic_bss||!acCore_.board_valid||!acPHYStorage_)return ENXIO;
    /* Only the manual recovery gate can retire an earlier core failure after
     * checked teardown and identity revalidation. Watchdog/ioctl cannot skip it. */
    if(core_.core_error)return core_.core_error;
    if(rxRecovery_.initialDrainError!=kIOReturnSuccess)return EIO;
    if(!present_||sleeping_||closing_||usbError_!=kIOReturnSuccess||!completionLock_)return ENXIO;
    IOInterruptState lock=IOSimpleLockLockDisableInterrupt(completionLock_);
    const bool drained=rtw_transfer_drained(&ledger_)!=0&&!rxRecovery_.initialDrainMask;
    IOSimpleLockUnlockEnableInterrupt(completionLock_,lock);
    if(!drained)return EBUSY;
    /* A previous stop can defer power-off while abort callbacks still own
     * the old epoch. Finish that checked stop before any new bootstrap. */
    if(acRecoveryDiag_.cleanupResult==EBUSY)stop8814();
    if(acCore_.io_error)return acCore_.io_error;
    if(acRecoveryBlocked_)return acRecoveryDiag_.cleanupResult?(int)acRecoveryDiag_.cleanupResult:EIO;
    acStarting_=true;auto&d=acRecoveryDiag_;++d.starts;d.result=0;d.stage=AUAllocate;
    acMediaConnected_=acRateReady_=false;acRateConfig_={};acChannel_=0;acRSSI_=0;
    int e=allocateTransfers();if(e)goto fail;
    d.stage=AUPower;e=rtw_8814au_power_on(&acCore_);if(e)goto fail;
    d.stage=AUFirmware;e=rtw_8814au_download_fw(&acCore_,RTW8814AUFirmware,sizeof(RTW8814AUFirmware));if(e)goto fail;
    d.stage=AUMAC;e=rtw_8814au_init_mac(&acCore_,endpoints_.num_bulk_out,acSpeed());if(e)goto fail;
    d.stage=AUPHYContext;e=rtw_8814au_phy_init(&acCore_,acPHYStorage_,acPHYSize_);if(e)goto fail;
    d.stage=AUPHYTables;e=rtw_8814au_phy_load_tables(&acCore_);if(e)goto fail;
    ic.ic_bss->ni_chan=ic.ic_ibss_chan;
    e=set8814Channel((uint8_t)ieee80211_chan2ieee(&ic,ic.ic_ibss_chan));if(e)goto fail;
    {
        d.stage=AUStation;auto params=station8814Params();
        e=rtw_8814au_station_initialize(&acCore_,&params);if(e)goto fail;
    }
    d.stage=AURX;e=busInitialize(this);if(e)goto fail;
    d.stage=AUStartupInfo;e=rtw_8814au_send_startup_info(&acCore_);if(e)goto fail;
    ifq_clr_oactive(&ifp.if_snd);ifp.if_flags|=IFF_RUNNING;
    d.stage=AUScan;e=ic.ic_newstate(&ic,IEEE80211_S_SCAN,-1);if(e)goto fail;
    radioReady_=true;acStarting_=false;acFailureEligible_=false;d.stage=AUReady;
    d.consecutiveRecoveries=0;
    if(acRecoveryRestartPending_){++d.recoverySuccesses;acRecoveryRestartPending_=false;}
    return 0;
fail:
    record8814Failure(d.stage,e);d.result=(uint32_t)e;acStarting_=false;
    stop8814();if(acCore_.io_error)core_.core_error=acCore_.io_error;
    return e;
}
void RTWUSBController::stop8814()
{
    /* Retire only the deferred marker before abort invokes completion drain;
     * a completion-side finalizer must not recursively enter this same stop. */
    if(acRecoveryDiag_.cleanupResult==EBUSY)acRecoveryDiag_.cleanupResult=0;
    auto&ic=core_.sc_ic;auto&ifp=ic.ic_if;
    core_.sc_tx_timer=0;ifp.if_timer=0;ifp.if_flags&=~IFF_RUNNING;ifq_clr_oactive(&ifp.if_snd);
    /* Generic INIT must flush host keys and authorization even after USB loss. */
    if(core_.sc_newstate){
        if(rxRecovery_.initialDrainError!=kIOReturnSuccess)
            (void)core_.sc_newstate(&ic,IEEE80211_S_INIT,-1);
        else (void)state8814(&ic,IEEE80211_S_INIT,-1);
    }
    cancelScan(this);cancelCalibration(this);task_del(systq,&core_.init_task);
    abortTransfers();
    IOInterruptState lock=IOSimpleLockLockDisableInterrupt(completionLock_);
    const bool drained=rtw_transfer_drained(&ledger_)!=0&&!rxRecovery_.initialDrainMask;
    IOSimpleLockUnlockEnableInterrupt(completionLock_,lock);
    if(!drained){
        /* A missing abort callback retains its old USB epoch. Do not power
         * cycle or release that epoch while hardware ownership is pending. */
        acRecoveryDiag_.cleanupResult=EBUSY;
        acMediaConnected_=acRateReady_=false;acChannel_=0;acRateConfig_={};
        return;
    }
    if(present_&&!sleeping_&&usbError_==kIOReturnSuccess&&rxRecovery_.initialDrainError==kIOReturnSuccess){
        if(acCore_.phy&&acCore_.mac_initialized)(void)rtw_8814au_phy_stop(&acCore_);
        const int cleanup=rtw_8814au_stop_checked(&acCore_);
        if(!acRecoveryDiag_.cleanupResult)acRecoveryDiag_.cleanupResult=(uint32_t)cleanup;
        if(cleanup)acRecoveryBlocked_=true;
    }else {
        acCore_.powered=acCore_.fw_running=acCore_.mac_initialized=acCore_.phy_initialized=0;
        if(!acRecoveryDiag_.cleanupResult)acRecoveryDiag_.cleanupResult=ENXIO;
        acRecoveryBlocked_=true;
    }
    acMediaConnected_=acRateReady_=false;acChannel_=0;acRateConfig_={};
    freeTransfers();
}
int RTWUSBController::set8814Channel(uint8_t channel)
{
    const bool band5g=rtw_station_5ghz_channel(channel)!=0;
    if(!band5g&&(channel<1||channel>11))return EINVAL;
    if(!permitsDFSChannelByCountry(channel))return EACCES;
    if(!rtw_8814au_phy_channel_supported(&acCore_,channel,20))return ENOTSUP;
    if(acStarting_)acRecoveryDiag_.stage=AUChannel;
    int e=rtw_8814au_phy_set_channel(&acCore_,channel,20);
    const uint32_t stage=e?AUChannel:AURFK;
    if(!e){if(acStarting_)acRecoveryDiag_.stage=AURFK;e=rtw_8814au_phy_calibrate(&acCore_);}
    if(!e)acChannel_=channel;
    else {acChannel_=0;core_.core_error=e;if(!acStarting_)record8814Failure(stage,e);}
    return e;
}
int RTWUSBController::state8814(ieee80211com *ic,ieee80211_state state,int argument)
{
    if(!ic||ic->ic_opmode!=IEEE80211_M_STA||!core_.sc_newstate)return EINVAL;
    const auto previous=ic->ic_state;
    cancelScan(this);
    int error=0;
    if((previous==IEEE80211_S_RUN&&state!=IEEE80211_S_RUN)||state==IEEE80211_S_INIT){
        cancelCalibration(this);
        if(acMediaConnected_&&acCore_.fw_running&&!acCore_.io_error){
            uint8_t command[8]{};
            if(!rtw_8814au_make_media_status(0,command))error=EINVAL;
            else error=rtw_8814au_h2c_mailbox(&acCore_,command);
        }
        acMediaConnected_=acRateReady_=false;acRateConfig_={};
    }
    if(state==IEEE80211_S_INIT){
        if(acCore_.powered&&acCore_.fw_running&&acCore_.mac_initialized&&!acCore_.io_error){
            int e=rtw_8814au_station_configure(&acCore_,RTW_8814AU_STATION_INIT,nullptr);if(!error)error=e;
        }
        int e=core_.sc_newstate(ic,state,argument);return error?error:e;
    }
    if(error)return error;
    if(!ic->ic_bss||!ic->ic_bss->ni_chan||ic->ic_bss->ni_chan==IEEE80211_CHAN_ANYC)return EINVAL;
    const unsigned channel=ieee80211_chan2ieee(ic,ic->ic_bss->ni_chan);
    const bool passive5g=channel<=255&&rtw_station_5ghz_channel((uint8_t)channel)!=0;
    if(!passive5g&&(channel<1||channel>11))return EINVAL;
    if(!permitsDFSChannelByCountry((uint8_t)channel))return EACCES;
    if(!rtw_8814au_phy_channel_supported(&acCore_,(uint8_t)channel,20))return ENOTSUP;
    auto params=station8814Params();
    switch(state){
    case IEEE80211_S_SCAN:
        if(passive5g){
            /* Pause station TX queues and clear the previous BSSID filters
             * while observing passive scan channels. */
            params.passive_scan=1;
            error=rtw_8814au_phy_set_passive_5g_channel(&acCore_,(uint8_t)channel,20);
            if(!error){acChannel_=(uint8_t)channel;error=rtw_8814au_station_configure(&acCore_,RTW_8814AU_STATION_SCAN,&params);}
            break;
        }
        /* Leaving 5 GHz invalidated the old 2.4 GHz calibration. Do not
         * resume regular scanning until 2.4 GHz is recalibrated. */
        error=acCore_.phy_initialized&&acChannel_>=1&&acChannel_<=11 ?
            rtw_8814au_phy_set_scan_channel(&acCore_,(uint8_t)channel,20) :
            set8814Channel((uint8_t)channel);
        if(!error){acChannel_=(uint8_t)channel;error=rtw_8814au_station_configure(&acCore_,RTW_8814AU_STATION_SCAN,&params);}
        break;
    case IEEE80211_S_AUTH:
        error=set8814Channel((uint8_t)channel);
        if(!error)error=rtw_8814au_station_configure(&acCore_,RTW_8814AU_STATION_AUTH,&params);
        break;
    case IEEE80211_S_ASSOC:
        if(!acCore_.phy_initialized||acChannel_!=channel)return ENXIO;
        error=rtw_8814au_station_configure(&acCore_,RTW_8814AU_STATION_ASSOC,&params);
        break;
    case IEEE80211_S_RUN: {
        if(!acCore_.phy_initialized||acChannel_!=channel)return ENXIO;
        const uint16_t rates=rtw8814RateMask(ic->ic_bss->ni_rates,false)&(params.band_5g?0xff0U:0xfffU);
        if(!rates||!params.basic_rates||!params.aid||params.aid>2007)return EINVAL;
        uint8_t mcs[2]{},low[8]{},high[8]{},media[8]{};
        const auto*peer=ic->ic_bss;
        const bool ht=(peer->ni_flags&IEEE80211_NODE_HT)!=0;
        if(ht){mcs[0]=peer->ni_rxmcs[0];mcs[1]=peer->ni_rxmcs[1];}
        rtw_8814au_rate_config config{};
        if(!rtw_8814au_build_rate_config(rates,mcs,acCore_.board.tx_streams,ht,
           (ic->ic_htcaps&IEEE80211_HTCAP_SGI20)!=0,(peer->ni_htcaps&IEEE80211_HTCAP_SGI20)!=0,&config)||
           !rtw_8814au_build_ra_commands(&config,1,low,high)||!rtw_8814au_make_media_status(1,media))return EINVAL;
        acRateReady_=false;
        error=rtw_8814au_h2c_mailbox(&acCore_,low);
        if(!error)error=rtw_8814au_h2c_mailbox(&acCore_,high);
        if(!error){error=rtw_8814au_h2c_mailbox(&acCore_,media);if(!error)acMediaConnected_=true;}
        if(!error)error=rtw_8814au_write(&acCore_,0x554,2,ic->ic_bss->ni_intval);
        if(!error)error=rtw_8814au_station_configure(&acCore_,RTW_8814AU_STATION_RUN,&params);
        if(!error){acRateConfig_=config;acRateConfig_.init_ra_level=0;update8814EDCA(ic);if(acCore_.io_error)error=acCore_.io_error;else if(core_.core_error)error=core_.core_error;}
        if(error){
            if(acMediaConnected_&&!acCore_.io_error){uint8_t disconnect[8]{};if(rtw_8814au_make_media_status(0,disconnect))(void)rtw_8814au_h2c_mailbox(&acCore_,disconnect);}
            acMediaConnected_=false;acRateReady_=false;acRateConfig_={};
            (void)rtw_8814au_station_configure(&acCore_,RTW_8814AU_STATION_INIT,nullptr);
            return error;
        }
        break;
    }
    case IEEE80211_S_INIT: return EINVAL;
    }
    if(error)return error;
    error=core_.sc_newstate(ic,state,argument);
    if(error&&state==IEEE80211_S_RUN){
        if(acMediaConnected_&&!acCore_.io_error){uint8_t disconnect[8]{};if(rtw_8814au_make_media_status(0,disconnect))(void)rtw_8814au_h2c_mailbox(&acCore_,disconnect);}
        acMediaConnected_=acRateReady_=false;acRateConfig_={};
        (void)rtw_8814au_station_configure(&acCore_,RTW_8814AU_STATION_INIT,nullptr);
    }
    if(!error&&state==IEEE80211_S_RUN){acRateReady_=true;acRSSI_=0;nextCalibration(this);}
    if(!error&&state==IEEE80211_S_SCAN)nextScan(this);
    return error;
}
void RTWUSBController::update8814EDCA(ieee80211com *ic)
{
    if(!ic)return;
    auto*core=static_cast<rtwn_softc*>(ic->ic_softc);auto*s=owner(core->sc_ops.cookie);
    if(!s->acCore_.powered||!s->acCore_.mac_initialized)return;
    rtw_8814au_edca_ac values[4]{};
    for(unsigned i=0;i<4;i++){
        const auto&p=ic->ic_edca_ac[i];values[i].aifsn=p.ac_aifsn;values[i].ecwmin=p.ac_ecwmin;
        values[i].ecwmax=p.ac_ecwmax;values[i].txop=p.ac_txoplimit;
    }
    const auto params=s->station8814Params();
    int e=rtw_8814au_station_edca_band(&s->acCore_,(ic->ic_flags&IEEE80211_F_SHSLOT)!=0,params.band_5g,values);
    if(e)core->core_error=e;
}
void RTWUSBController::watchdog8814(_ifnet *ifp)
{
    auto*core=static_cast<rtwn_softc*>(ifp->if_softc);
    ifp->if_timer=0;
    if(core->sc_tx_timer>0){if(--core->sc_tx_timer==0){ifp->if_oerrors++;task_add(systq,&core->init_task);return;}ifp->if_timer=1;}
    ieee80211_watchdog(ifp);
}
void RTWUSBController::initTask8814(void *p)
{
    auto*s=owner(p);auto&core=s->core_;auto&ifp=core.sc_ic.ic_if;
    if(core.sc_flags&RTWN_FLAG_BUSY)return;
    core.sc_flags|=RTWN_FLAG_BUSY;s->stop8814();
    if(ifp.if_flags&IFF_UP){int e=s->start8814();if(e&&e!=EAGAIN&&e!=EBUSY)core.core_error=e;}
    core.sc_flags&=~RTWN_FLAG_BUSY;
}
int RTWUSBController::ioctl8814(_ifnet *ifp,u_long command,caddr_t data)
{
    auto*core=static_cast<rtwn_softc*>(ifp->if_softc);auto*s=owner(core->sc_ops.cookie);
    if(core->sc_flags&RTWN_FLAG_BUSY)return EBUSY;
    core->sc_flags|=RTWN_FLAG_BUSY;int error=0;
    switch(command){
    case SIOCSIFADDR: ifp->if_flags|=IFF_UP; /* FALLTHROUGH */
    case SIOCSIFFLAGS:
        if(ifp->if_flags&IFF_UP){if(!(ifp->if_flags&IFF_RUNNING))error=s->start8814();}
        else if(ifp->if_flags&IFF_RUNNING)s->stop8814();
        break;
    default:error=ieee80211_ioctl(ifp,command,data);break;
    }
    if(core->sc_ic.ic_opmode!=IEEE80211_M_STA){core->sc_ic.ic_opmode=IEEE80211_M_STA;error=EOPNOTSUPP;}
    if(error==ENETRESET){
        if((ifp->if_flags&(IFF_UP|IFF_RUNNING))==(IFF_UP|IFF_RUNNING)){s->stop8814();error=s->start8814();}
        else error=0;
    }
    core->sc_flags&=~RTWN_FLAG_BUSY;return error;
}
