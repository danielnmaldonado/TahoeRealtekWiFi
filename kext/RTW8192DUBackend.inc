#include "rtw/StationChannels.h"
/* SPDX-License-Identifier: GPL-2.0-only */
/* Real RTL8192DU station adapter. net80211 owns RSN/software CCMP/replay and
 * the controlled port. The DU HAL owns MAC/8051/BB/RF/HT20. Opening one USB
 * interface is insufficient: every interface is exclusively claimed.
 * DUALMAC_DUALPHY uses one logical Ethernet station, cold-switching between
 * physical MAC0/5GHz and MAC1/2GHz with all old USB transfers drained first.
 * Both vendor interfaces remain owned for the complete controller lifetime.
 * Parent includes RTL8192DUFirmware.inc before this implementation. */
static int rtw8192DUError(rtw_92du_result value)
{
    switch(value){
    case RTW_92DU_OK:return 0;
    case RTW_92DU_BAD_ARGUMENT:case RTW_92DU_BAD_PROM:case RTW_92DU_BAD_FIRMWARE:return EINVAL;
    case RTW_92DU_UNSUPPORTED_CHIP:return EOPNOTSUPP;
    case RTW_92DU_IDENTITY_UNVERIFIED:case RTW_92DU_IO_NOT_CONFIRMED:return EPERM;
    case RTW_92DU_MCU_BUSY:return EBUSY;
    case RTW_92DU_POLL_TIMEOUT:return ETIMEDOUT;
    case RTW_92DU_WRONG_STATE:return ENXIO;
    default:return EIO;
    }
}
static uint16_t rtw8192DURateMask(const ieee80211_rateset& rates,bool basic)
{
    static const uint8_t values[12]={2,4,11,22,12,18,24,36,48,72,96,108};
    if(rates.rs_nrates>sizeof(rates.rs_rates))return 0;
    uint16_t result=0;
    for(unsigned i=0;i<rates.rs_nrates;i++){
        if(basic&&!(rates.rs_rates[i]&IEEE80211_RATE_BASIC))continue;
        const uint8_t value=rates.rs_rates[i]&IEEE80211_RATE_VAL;
        for(unsigned j=0;j<12;j++)if(value==values[j])result|=(uint16_t)(1U<<j);
    }
    return result;
}
rtw_usb_error RTWUSBController::duControlIn(void *p,const rtw_usb_setup *setup,uint8_t *bytes,uint16_t *actual)
{
    auto*s=owner(p);if(!actual)return RTW_USB_INVALID_ARGUMENT;*actual=0;
    if(!setup||!bytes||setup->bm_request_type!=0xc0||setup->b_request!=5||setup->w_index||
       (setup->w_length!=1&&setup->w_length!=2&&setup->w_length!=4))return RTW_USB_INVALID_ARGUMENT;
    if(!s->duWholeDeviceOwned_||ioError(p))return RTW_USB_IO_ERROR;
    const uint32_t value=s->readRegister(setup->w_value,(uint8_t)setup->w_length);
    if(ioError(p))return RTW_USB_IO_ERROR;
    for(unsigned i=0;i<setup->w_length;i++)bytes[i]=(uint8_t)(value>>(8*i));
    *actual=setup->w_length;return RTW_USB_OK;
}
rtw_usb_error RTWUSBController::duControlOut(void *p,const rtw_usb_setup *setup,const uint8_t *bytes,uint16_t *actual)
{
    auto*s=owner(p);if(!actual)return RTW_USB_INVALID_ARGUMENT;*actual=0;
    if(!setup||!bytes||setup->bm_request_type!=0x40||setup->b_request!=5||setup->w_index||
       !setup->w_length||setup->w_length>RTW_USB_MAX_VENDOR_PAYLOAD)return RTW_USB_INVALID_ARGUMENT;
    if(!s->duWholeDeviceOwned_||ioError(p)||writeRegion(p,setup->w_value,bytes,setup->w_length))return RTW_USB_IO_ERROR;
    *actual=setup->w_length;return RTW_USB_OK;
}
int RTWUSBController::duWait(void *p,uint32_t us)
{
    auto*s=owner(p);if(!s->duWholeDeviceOwned_||ioError(p))return 0;
    delayUS(p,us);return ioError(p)==0;
}
bool RTWUSBController::claim8192DUDevice()
{
    if(duWholeDeviceOwned_)return true;
    if(!usb_||!usbOpened_||!usb_->isOpen(this))return false;
    auto*device=usb_->getDevice();const auto*config=usb_->getConfigurationDescriptor();
    const auto*current=usb_->getInterfaceDescriptor();
    if(!device||!config||!current||current->bInterfaceNumber!=0||
       config->bNumInterfaces<1||config->bNumInterfaces>2)return false;
    if(!device->open(this))return false;
    device->retain();duDevice_=device;
    /* Device::open permits child-interface sessions. Own every child as
     * well; never seize an interface already opened by another driver. */
    OSIterator*iterator=device->getChildIterator(gIOServicePlane);
    bool ok=iterator!=nullptr,self=false;unsigned count=0,seen=0;
    if(iterator){
        while(auto*object=iterator->getNextObject()){
            auto*child=OSDynamicCast(IOUSBHostInterface,object);if(!child)continue;
            const auto*desc=child->getInterfaceDescriptor();
            if(!desc||child->getDevice()!=device||desc->bInterfaceClass!=0xff||
               desc->bInterfaceSubClass!=0xff||desc->bInterfaceProtocol!=0xff||desc->bInterfaceNumber>=config->bNumInterfaces||
               (seen&(1U<<desc->bInterfaceNumber))){ok=false;break;}
            seen|=1U<<desc->bInterfaceNumber;++count;
            if(child==usb_){self=true;continue;}
            if(duSibling_||!child->open(this)){ok=false;break;}
            child->retain();duSibling_=child;
        }
        iterator->release();
    }
    ok=ok&&self&&count==config->bNumInterfaces&&seen==((1U<<config->bNumInterfaces)-1U);
    if(!ok){release8192DUStorage();return false;}
    duWholeDeviceOwned_=true;return true;
}
bool RTWUSBController::is8192DUUSBProvider(IOService *provider) const
{
    if(lifecycleLock_)IOLockLock(lifecycleLock_);
    const bool matches=family_==RTW_WIFI_RTL8192DU&&provider&&
        (provider==usb_||provider==duSibling_||provider==duDevice_);
    if(lifecycleLock_)IOLockUnlock(lifecycleLock_);return matches;
}
void RTWUSBController::release8192DUStorage()
{
    if(duPHYStorage_){IOFree(duPHYStorage_,duPHYSize_);duPHYStorage_=nullptr;duPHYSize_=0;duCore_.phy=nullptr;}
    for(auto&port:duPorts_){
        if(port.rx){port.rx->release();port.rx=nullptr;}
        for(auto*&pipe:port.tx)if(pipe){pipe->release();pipe=nullptr;}
    }
    duPortCount_=0;
    close8192DUProviders();
}
void RTWUSBController::close8192DUProviders()
{
    // Parent calls outside its command gate after quiesce/abort, including
    // when callbacks still retain transfer/PHY storage. Null first: close()
    // can synchronously trigger a second provider-close/termination message.
    if(lifecycleLock_)IOLockLock(lifecycleLock_);
    auto*sibling=duSibling_;auto*device=duDevice_;
    duSibling_=nullptr;duDevice_=nullptr;duWholeDeviceOwned_=false;
    if(lifecycleLock_)IOLockUnlock(lifecycleLock_);
    if(sibling){if(sibling->isOpen(this))sibling->close(this);sibling->release();}
    if(device){if(device->isOpen(this))device->close(this);device->release();}
}
int RTWUSBController::prepare8192DUPorts(const uint8_t *configuration,size_t length)
{
    if(!duWholeDeviceOwned_||duPortCount_)return EBUSY;
    const unsigned count=duROM_.board.mode==RTW_92DU_DUAL_MAC?2:1;
    duPortROM_[0]=duROM_;memcpy(duStationAddress_,duROM_.board.mac,6);
    if(count==2){
        if(!duSibling_)return ENXIO;
        duPortROM_[1]=duROM_;
        const auto result=rtw_92du_identify(&duPlan_,duROM_.board.sys_cfg,duROM_.logical,
            sizeof(duROM_.logical),1,&duPortROM_[1].board);
        if(result!=RTW_92DU_OK)return rtw8192DUError(result);
    }
    for(unsigned index=0;index<count;index++){
        auto&port=duPorts_[index];auto*interface=index?duSibling_:usb_;
        if(!interface||!interface->getInterfaceDescriptor()||interface->getInterfaceDescriptor()->bInterfaceNumber!=index)return ENXIO;
        const auto result=rtw_92du_usb_interface_endpoints(&duPlan_,&duPortROM_[index].board,configuration,length,&port.endpoints);
        if(result!=RTW_92DU_OK)return rtw8192DUError(result);
        port.rx=interface->copyPipe(port.endpoints.bulk_in[0]);if(!port.rx)return ENXIO;
        const auto*receive=port.rx->getEndpointDescriptor();
        if(!receive||(receive->bmAttributes&3)!=2||
           (USBToHost16(receive->wMaxPacketSize)!=64&&USBToHost16(receive->wMaxPacketSize)!=512))return EINVAL;
        for(unsigned p=0;p<port.endpoints.num_bulk_out;p++){
            port.tx[p]=interface->copyPipe(port.endpoints.bulk_out[p]);if(!port.tx[p])return ENXIO;
            const auto*d=port.tx[p]->getEndpointDescriptor();if(!d||(d->bmAttributes&3)!=2)return EINVAL;
            port.mps[p]=USBToHost16(d->wMaxPacketSize);if(port.mps[p]!=64&&port.mps[p]!=512)return EINVAL;
        }
    }
    duPortCount_=(uint8_t)count;
    // DMDP MAC1 owns 2GHz in the Linux PHY baseline; begin scanning there.
    return select8192DUPort(count==2?1:0);
}
int RTWUSBController::select8192DUPort(uint8_t index)
{
    if(index>=duPortCount_||running_||!duWholeDeviceOwned_)return EBUSY;
    const IOInterruptState lock=IOSimpleLockLockDisableInterrupt(completionLock_);
    const bool drained=rtw_transfer_drained(&ledger_);IOSimpleLockUnlockEnableInterrupt(completionLock_,lock);
    if(!drained)return EBUSY;
    auto&port=duPorts_[index];if(!port.rx||!port.tx[0])return ENXIO;
    if(rxPipe_)rxPipe_->release();rxPipe_=port.rx;rxPipe_->retain();
    for(unsigned p=0;p<4;p++){
        if(txPipes_[p])txPipes_[p]->release();txPipes_[p]=port.tx[p];
        if(txPipes_[p])txPipes_[p]->retain();txMPS_[p]=port.mps[p];
    }
    endpoints_=port.endpoints;
    core_.ac2idx[EDCA_AC_BK]=core_.ac2idx[EDCA_AC_BE]=endpoints_.num_bulk_out>=3?2:endpoints_.num_bulk_out==2?1:0;
    core_.ac2idx[EDCA_AC_VI]=endpoints_.num_bulk_out>=3?1:0;core_.ac2idx[EDCA_AC_VO]=0;
    duActivePort_=index;duROM_=duPortROM_[index];return 0;
}
int RTWUSBController::initialize8192DUCore()
{
    if(!duWholeDeviceOwned_||!duPHYStorage_||!usb_||!usb_->getConfigurationDescriptor())return ENXIO;
    const rtw_usb_rw_transport raw{this,duControlIn,duControlOut};
    if(rtw_92du_usb_map_init(&duUSB_,&raw,duROM_.board.mac_index)!=RTW_USB_OK)return EINVAL;
    const rtw_waiter waiter{this,duWait};const rtw_92du_boot_options options{1,1,1000};
    auto result=rtw_92du_boot_init(&duCore_,&duROM_.board,rtw_92du_usb_bus(&duUSB_),&waiter,&options);
    if(result!=RTW_92DU_OK)return rtw8192DUError(result);
    const auto*c=usb_->getConfigurationDescriptor();
    return rtw8192DUError(rtw_92du_mac_init_for_interface(&duMAC_,&duCore_,&duPlan_,(const uint8_t*)c,USBToHost16(c->wTotalLength)));
}
int RTWUSBController::attach8192DU()
{
    if(family_!=RTW_WIFI_RTL8192DU||!profile_||!claim8192DUDevice())return EBUSY;
    auto*device=usb_->getDevice();const auto*d=device?device->getDeviceDescriptor():nullptr;
    const auto*c=usb_->getConfigurationDescriptor();const auto*i=usb_->getInterfaceDescriptor();
    if(!d||!c||!i)return ENXIO;
    const size_t length=USBToHost16(c->wTotalLength);if(length<9||length>4096)return EINVAL;
    const auto status=rtw_backend_usb_preflight(USBToHost16(d->idVendor),USBToHost16(d->idProduct),
        USBToHost16(d->bcdDevice),(const uint8_t*)c,length,i->bInterfaceNumber,&duPlan_);
    if((status!=RTW_BACKEND_PREFLIGHT_STATION&&status!=RTW_BACKEND_PREFLIGHT_DIAGNOSTICS)||duPlan_.profile!=profile_)
        return EPERM;
    const rtw_usb_rw_transport raw{this,duControlIn,duControlOut};const rtw_waiter waiter{this,duWait};
    const rtw_92du_boot_options options{1,1,10};
    const auto result=rtw_92du_read_rom(&duPlan_,&raw,&waiter,&options,0,&duROM_);
    if(result!=RTW_92DU_OK)return rtw8192DUError(result);
    const int ports=prepare8192DUPorts((const uint8_t*)c,length);if(ports)return ports;
    duPHYSize_=rtw_92du_phy_size();duPHYStorage_=IOMalloc(duPHYSize_);if(!duPHYStorage_)return ENOMEM;
    bzero(duPHYStorage_,duPHYSize_);
    core_.sc_pdev=&device_;core_.ntxchains=core_.nrxchains=duROM_.board.rf_paths;core_.avg_pwdb=-1;
    task_set(&core_.init_task,initTask8192DU,this,"rtw8192du_init");
    auto&ic=core_.sc_ic;auto&ifp=ic.ic_if;memcpy(ic.ic_myaddr,duStationAddress_,IEEE80211_ADDR_LEN);
    ic.ic_phytype=IEEE80211_T_OFDM;ic.ic_opmode=IEEE80211_M_STA;ic.ic_state=IEEE80211_S_INIT;
    ic.ic_caps=IEEE80211_C_SHPREAMBLE|IEEE80211_C_SHSLOT|IEEE80211_C_RSN|IEEE80211_C_QOS|IEEE80211_C_SCANALLBAND;ic.ic_flags|=IEEE80211_F_QOS;
    ic.ic_htcaps=IEEE80211_HTCAP_SGI20|(IEEE80211_HTCAP_SMPS_DIS<<IEEE80211_HTCAP_SMPS_SHIFT);
    ic.ic_ampdu_params=IEEE80211_AMPDU_PARAM_LE|IEEE80211_AMPDU_PARAM_SS_16;
    memset(ic.ic_sup_mcs,0,sizeof(ic.ic_sup_mcs));ic.ic_sup_mcs[0]=0xff;
    if(duROM_.board.rf_paths==2)ic.ic_sup_mcs[1]=0xff;
    ic.ic_tx_mcs_set=IEEE80211_TX_MCS_SET_DEFINED;
    ic.ic_sup_rates[IEEE80211_MODE_11A]=ieee80211_std_rateset_11a;
    ic.ic_sup_rates[IEEE80211_MODE_11B]=ieee80211_std_rateset_11b;ic.ic_sup_rates[IEEE80211_MODE_11G]=ieee80211_std_rateset_11g;
    for(unsigned ch=1;ch<=11;ch++){
        ic.ic_channels[ch].ic_freq=(uint16_t)ieee80211_ieee2mhz(ch,IEEE80211_CHAN_2GHZ);
        ic.ic_channels[ch].ic_flags=IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM|IEEE80211_CHAN_DYN|IEEE80211_CHAN_2GHZ|IEEE80211_CHAN_HT20;
    }
    static const uint8_t five[]={36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165};
    for(uint8_t ch:five){
        if(!permitsDFSChannelByCountry(ch))continue;
        ic.ic_channels[ch].ic_freq=(uint16_t)ieee80211_ieee2mhz(ch,IEEE80211_CHAN_5GHZ);
        ic.ic_channels[ch].ic_flags=IEEE80211_CHAN_5GHZ|IEEE80211_CHAN_OFDM|IEEE80211_CHAN_PASSIVE|IEEE80211_CHAN_HT20;
    }
    ic.ic_ibss_chan=&ic.ic_channels[1];ifp.if_softc=&core_;ifp.if_flags=IFF_BROADCAST|IFF_SIMPLEX|IFF_MULTICAST;
    ifp.if_ioctl=ioctl8192DU;ifp.if_start=rtwn_start;ifp.if_watchdog=watchdog8192DU;memcpy(ifp.if_xname,device_.dv_xname,IFNAMSIZ);
    if_attach(&ifp);ieee80211_ifattach(&ifp,this);
    ic.ic_updateslot=update8192DUEDCA;ic.ic_updateedca=update8192DUEDCA;
    ic.ic_ampdu_rx_start=ampdu8192DURXStart;ic.ic_ampdu_rx_stop=ampdu8192DURXStop;
    core_.sc_newstate=ic.ic_newstate;ic.ic_newstate=stateTransition;ieee80211_media_init(&ifp);
    setProperty("RTW8192DUCut",(unsigned long long)duROM_.cut,8);
    setProperty("RTW8192DUMACMode",(unsigned long long)duROM_.board.mode,8);setProperty("RTW8192DUWholeDeviceOwned",true);return 0;
}
rtw_92du_bss RTWUSBController::station8192DUParams() const
{
    rtw_92du_bss b{};const auto&ic=core_.sc_ic;const auto*n=ic.ic_bss;
    if(!n||!n->ni_chan||n->ni_chan==IEEE80211_CHAN_ANYC)return b;
    b.channel=(uint8_t)ieee80211_chan2ieee(const_cast<ieee80211com*>(&ic),n->ni_chan);memcpy(b.bssid,n->ni_bssid,6);
    b.aid=n->ni_associd&0x3fff;b.beacon_interval=n->ni_intval;
    b.short_slot=b.channel>14||(ic.ic_flags&IEEE80211_F_SHSLOT)!=0;
    const uint16_t allowed=b.channel>14?0xff0:0xfff;b.basic_rates=rtw8192DURateMask(n->ni_rates,true)&allowed;
    if(!b.basic_rates){const uint16_t rates=rtw8192DURateMask(n->ni_rates,false)&allowed;b.basic_rates=rates&(uint16_t)(0U-rates);}
    return b;
}
bool RTWUSBController::permits8192DUChannel(uint8_t channel) const
{
    if(family_!=RTW_WIFI_RTL8192DU||!duWholeDeviceOwned_||!permitsDFSChannelByCountry(channel))return false;
    if(duROM_.board.mode==RTW_92DU_DUAL_MAC&&duPortCount_==2){
        if(!rtw_92du_phy_receive_ready(&duCore_)||duCore_.last_error!=RTW_92DU_OK)return false;
        return (channel>=1&&channel<=11)||rtw_station_5ghz_channel(channel);
    }
    return rtw_92du_phy_supports_channel(&duCore_,channel);
}
bool RTWUSBController::supports8192DUBand(bool five) const
{
    return permits8192DUChannel(five?36:1);
}
int RTWUSBController::start8192DU()
{
    const int lifecycle=beginFamilyStart();if(lifecycle)return lifecycle;

    auto&ic=core_.sc_ic;auto&ifp=ic.ic_if;
    if(ic.ic_opmode!=IEEE80211_M_STA||!ic.ic_bss||ioError(this)||!duWholeDeviceOwned_||!duPHYStorage_)return ENXIO;
    duRSSIValid_=false;duRSSIPWDB_=0;core_.core_error=0;
    if(duROM_.board.mode==RTW_92DU_DUAL_MAC){
        const uint8_t target=ieee80211_chan2ieee(&ic,ic.ic_ibss_chan)>14?0:1;
        const int selected=select8192DUPort(target);if(selected){failFamilyStart(selected);return selected;}
    }
    int error=initialize8192DUCore();if(error){failFamilyStart(error);return error;}
    error=allocateTransfers();if(error)goto fail;
    error=start8192DUHardware();if(error)goto fail;
    ic.ic_bss->ni_chan=ic.ic_ibss_chan;
    error=set8192DUChannel((uint8_t)ieee80211_chan2ieee(&ic,ic.ic_ibss_chan));if(error)goto fail;
    error=rtw8192DUError(rtw_92du_mac_set_state(&duMAC_,RTW_92DU_ST_INIT,nullptr));if(error)goto fail;
    familyRecovery_.diag.stage=rtw::StationRecovery::RX;error=busInitialize(this);if(error)goto fail;
    familyRecovery_.diag.stage=rtw::StationRecovery::Start;
    ifq_clr_oactive(&ifp.if_snd);ifp.if_flags|=IFF_RUNNING;
    error=ic.ic_newstate(&ic,IEEE80211_S_SCAN,-1);if(error)goto fail;
    radioReady_=true;
    setProperty("RTW8192DUFirmwareHeaderVersion",(unsigned long long)duCore_.firmware_version,16);
    setProperty("RTW8192DUFirmwareReused",duCore_.firmware_reused!=0);finishFamilyStart();return 0;
fail:
    setProperty("RTW8192DULastHALResult",(unsigned long long)duCore_.last_error,32);
    failFamilyStart(error);stop8192DU();if(!familyRecovery_.transient(error))core_.core_error=error;return error;
}
int RTWUSBController::start8192DUHardware()
{
    if(!rxPipe_)return ENXIO;
    int error=rtw8192DUError(rtw_92du_power_on(&duCore_));
    if(!error)error=rtw8192DUError(rtw_92du_mac_prepare(&duMAC_));
    if(!error)error=rtw8192DUError(rtw_92du_firmware_start(&duCore_,RTW8192DUFirmware,sizeof(RTW8192DUFirmware)));
    if(!error)error=rtw8192DUError(rtw_92du_phy_init(&duCore_,&duROM_,duPHYStorage_,duPHYSize_,
        rxPipe_->getSpeed()==kUSBHostConnectionSpeedHigh?RTW_92DU_USB_HIGH:RTW_92DU_USB_FULL));
    if(!error)error=rtw8192DUError(rtw_92du_mac_start(&duMAC_));
    if(!error)error=rtw8192DUError(rtw_92du_mac_set_address(&duMAC_,duStationAddress_));
    if(!error)error=rtw8192DUError(rtw_92du_phy_load_tables(&duCore_));
    if(!error)error=rtw8192DUError(rtw_92du_phy_set_power_reduction(&duCore_,duPowerReductionDB_));
    if(!error)error=rtw8192DUError(rtw_92du_phy_set_power_limit_mbm(&duCore_,duPowerLimitMBM_));return error;
}
int RTWUSBController::switch8192DUBand(uint8_t channel)
{
    if(duROM_.board.mode!=RTW_92DU_DUAL_MAC)return 0;
    const uint8_t target=channel>14?0:1;if(target==duActivePort_)return 0;
    if(!permits8192DUChannel(channel)||duPortCount_!=2||!duMAC_.started)return ENXIO;
    int error=rtw8192DUError(rtw_92du_mac_set_state(&duMAC_,RTW_92DU_ST_INIT,nullptr));if(error)return error;
    // No pipe change or power teardown is safe until every old callback drained.
    const auto recovery=wpaRecovery_;abortTransfers();
    const IOInterruptState lock=IOSimpleLockLockDisableInterrupt(completionLock_);
    const bool drained=rtw_transfer_drained(&ledger_);IOSimpleLockUnlockEnableInterrupt(completionLock_,lock);
    if(!drained)return EBUSY;
    error=rtw8192DUError(rtw_92du_mac_stop(&duMAC_));
    if(!error)error=select8192DUPort(target);
    if(!error)error=initialize8192DUCore();
    if(!error)error=start8192DUHardware();
    if(!error)error=rtw8192DUError(rtw_92du_phy_set_channel(&duCore_,channel));
    if(!error)error=rtw8192DUError(rtw_92du_phy_calibrate(&duCore_));
    if(!error)error=busInitialize(this);
    if(error){duMAC_.joined=duMAC_.rate_ready=0;return error;}
    wpaRecovery_=recovery;duRSSIValid_=false;radioReady_=true;
    setProperty("RTW8192DUActiveMAC",(unsigned long long)duActivePort_,8);return 0;
}
void RTWUSBController::stop8192DU()
{
    familyRecovery_.inStop=true;
    auto&ic=core_.sc_ic;auto&ifp=ic.ic_if;
    core_.sc_tx_timer=0;ifp.if_timer=0;ifp.if_flags&=~IFF_RUNNING;ifq_clr_oactive(&ifp.if_snd);
    if(core_.sc_newstate)(void)core_.sc_newstate(&ic,IEEE80211_S_INIT,-1);
    cancelScan(this);cancelCalibration(this);task_del(systq,&core_.init_task);abortTransfers();
    duMAC_.rate_ready=duMAC_.joined=0;duRSSIValid_=false;duPowerReductionDB_=0;duPowerLimitMBM_=INT32_MAX;
    if(!familyStopDrained())return;
    int cleanup=0;
    const int original=rtw8192DUError(duCore_.last_error);if(original&&!core_.core_error)core_.core_error=original;
    if(present_&&!sleeping_&&usbError_==kIOReturnSuccess&&duWholeDeviceOwned_)cleanup=rtw8192DUError(rtw_92du_mac_stop_checked(&duMAC_));
    else{duCore_.power_ready=duCore_.firmware_ready=0;duMAC_.prepared=duMAC_.started=0;cleanup=ENXIO;}
    recordFamilyCleanup(cleanup);freeTransfers();
}
int RTWUSBController::set8192DUPowerLimitMBM(int32_t mbm)
{
    duPowerLimitMBM_=mbm;
    return duCore_.phy?rtw8192DUError(rtw_92du_phy_set_power_limit_mbm(&duCore_,mbm)):0;
}
int RTWUSBController::set8192DUPowerReduction(uint8_t db)
{
    duPowerReductionDB_=db;
    return duCore_.phy?rtw8192DUError(rtw_92du_phy_set_power_reduction(&duCore_,db)):0;
}
int RTWUSBController::set8192DUChannel(uint8_t channel)
{
    const bool switched=duROM_.board.mode==RTW_92DU_DUAL_MAC&&duActivePort_!=(channel>14?0:1);
    int error=switch8192DUBand(channel);
    if(error||switched)return error; // a switch already calibrated this channel
    if(!error)error=rtw8192DUError(rtw_92du_phy_set_channel(&duCore_,channel));
    if(!error)error=rtw8192DUError(rtw_92du_phy_calibrate(&duCore_));return error;
}
int RTWUSBController::state8192DU(ieee80211com *ic,ieee80211_state state,int argument)
{
    if(!ic||ic->ic_opmode!=IEEE80211_M_STA||!core_.sc_newstate)return EINVAL;
    cancelScan(this);int error=0;
    if(state!=IEEE80211_S_RUN){cancelCalibration(this);duRSSIValid_=false;}
    if(state==IEEE80211_S_INIT){
        if(duMAC_.started&&duCore_.last_error==RTW_92DU_OK)
            error=rtw8192DUError(rtw_92du_mac_set_state(&duMAC_,RTW_92DU_ST_INIT,nullptr));
        duMAC_.joined=duMAC_.rate_ready=0;
        if(duCore_.phy){const int paused=rtw8192DUError(rtw_92du_phy_pause(&duCore_));if(!error)error=paused;}
        /* Flush host keys, queues and the controlled port even after USB loss. */
        const int generic=core_.sc_newstate(ic,state,argument);
        if(generic)ic->ic_state=IEEE80211_S_INIT;
        if(!error)error=generic;if(error)core_.core_error=error;return error;
    }
    const auto failClosed=[&](int reason){
        radioReady_=false;duRSSIValid_=false;
        (void)state8192DU(ic,IEEE80211_S_INIT,-1);
        core_.core_error=reason;return reason;
    };
    const auto bss=station8192DUParams();
    if(!bss.channel||!permits8192DUChannel(bss.channel)||!duMAC_.started||duCore_.last_error!=RTW_92DU_OK)
        return failClosed(duCore_.last_error==RTW_92DU_OK?ENXIO:rtw8192DUError(duCore_.last_error));
    switch(state){
    case IEEE80211_S_SCAN:
        error=switch8192DUBand(bss.channel);
        if(!error)error=rtw8192DUError(rtw_92du_mac_set_state(&duMAC_,RTW_92DU_ST_SCAN,nullptr));
        if(!error)error=rtw8192DUError(rtw_92du_phy_set_scan_channel(&duCore_,bss.channel));
        if(!error&&bss.channel<=11)error=rtw8192DUError(rtw_92du_phy_activate(&duCore_));break;
    case IEEE80211_S_AUTH:
        duMAC_.rate_ready=0;error=set8192DUChannel(bss.channel);
        if(!error)error=rtw8192DUError(rtw_92du_mac_set_state(&duMAC_,RTW_92DU_ST_AUTH,&bss));
        if(!error)error=rtw8192DUError(rtw_92du_phy_activate(&duCore_));break;
    case IEEE80211_S_ASSOC:
        error=rtw8192DUError(rtw_92du_mac_set_state(&duMAC_,RTW_92DU_ST_ASSOC,&bss));break;
    case IEEE80211_S_RUN:{
        const auto*n=ic->ic_bss;const bool ht=(n->ni_flags&IEEE80211_NODE_HT)&&(n->ni_flags&IEEE80211_NODE_QOS);
        rtw_92du_peer_rates peer{};peer.legacy=rtw8192DURateMask(n->ni_rates,false);
        peer.ht=ht;peer.mcs=ht?(uint16_t)((unsigned)n->ni_rxmcs[0]|((unsigned)n->ni_rxmcs[1]<<8)):0;
        peer.sgi20=ht&&(ic->ic_htcaps&IEEE80211_HTCAP_SGI20)&&(n->ni_htcaps&IEEE80211_HTCAP_SGI20);
        peer.smps=ht?(uint8_t)((n->ni_htcaps&IEEE80211_HTCAP_SMPS_MASK)>>IEEE80211_HTCAP_SMPS_SHIFT):3;
        rtw_92du_rate_config rates{};
        error=rtw8192DUError(rtw_92du_build_rate_config(&duCore_,bss.channel,&peer,&rates));
        if(!error)error=rtw8192DUError(rtw_92du_mac_set_rates(&duMAC_,&rates));
        if(!error)error=rtw8192DUError(rtw_92du_mac_set_state(&duMAC_,RTW_92DU_ST_RUN,&bss));
        if(!error){update8192DUEDCA(ic);error=core_.core_error;}break;
    }
    default:return EINVAL;
    }
    if(!error)error=core_.sc_newstate(ic,state,argument);
    if(error)return failClosed(error);
    if(state==IEEE80211_S_RUN){duRSSIValid_=false;nextCalibration(this);}
    if(state==IEEE80211_S_SCAN)nextScan(this);return 0;
}
void RTWUSBController::update8192DUEDCA(ieee80211com *ic)
{
    if(!ic)return;auto*core=static_cast<rtwn_softc*>(ic->ic_softc);auto*s=owner(core->sc_ops.cookie);
    if(!s->duMAC_.started)return;
    const auto bss=s->station8192DUParams();if(!bss.channel)return;
    const auto*node=ic->ic_bss;
    const unsigned mode=bss.channel>14?IEEE80211_MODE_11A:
        (rtw8192DURateMask(node->ni_rates,false)&0xff0U)?IEEE80211_MODE_11G:IEEE80211_MODE_11B;
    const auto*parameters=((ic->ic_flags&IEEE80211_F_QOS)&&(node->ni_flags&IEEE80211_NODE_QOS))?
        ic->ic_edca_ac:ieee80211_edca_table[mode];
    rtw_92du_edca_ac values[4]{};
    for(unsigned i=0;i<4;i++){
        const auto&ac=parameters[i];values[i].aifsn=ac.ac_aifsn;values[i].ecwmin=ac.ac_ecwmin;
        values[i].ecwmax=ac.ac_ecwmax;values[i].txop=ac.ac_txoplimit;
    }
    const int error=rtw8192DUError(rtw_92du_mac_edca(&s->duMAC_,bss.channel,bss.short_slot,values));
    if(error)core->core_error=error;
}
bool RTWUSBController::uses8192DUFirmwareRate() const
{
    return family_==RTW_WIFI_RTL8192DU&&duWholeDeviceOwned_&&running_&&!stopping_&&
        core_.sc_ic.ic_state==IEEE80211_S_RUN&&core_.sc_ic.ic_bss&&
        duMAC_.started&&duMAC_.rate_ready&&duMAC_.joined&&rtw_92du_phy_tx_active(&duCore_);
}
int RTWUSBController::tick8192DU()
{
    if(!running_||stopping_||core_.sc_ic.ic_state!=IEEE80211_S_RUN)return 0;
    int error=rtw8192DUError(rtw_92du_mac_health(&duMAC_));
    if(!error&&!uses8192DUFirmwareRate())error=ENXIO;
    if(!error)error=rtw8192DUError(rtw_92du_phy_tick(&duCore_));
    if(!error&&duRSSIValid_)error=rtw8192DUError(rtw_92du_mac_report_rssi(&duMAC_,duRSSIPWDB_));
    if(error){radioReady_=false;(void)state8192DU(&core_.sc_ic,IEEE80211_S_INIT,-1);core_.core_error=error;}
    return error;
}
int RTWUSBController::ampdu8192DURXStart(ieee80211com *ic,ieee80211_node *node,uint8_t tid)
{
    if(!ic||!node||tid>=16)return EINVAL;
    auto*core=static_cast<rtwn_softc*>(ic->ic_softc);auto*s=owner(core->sc_ops.cookie);const auto&ba=node->ni_rx_ba[tid];
    return s->uses8192DUFirmwareRate()&&s->duMAC_.rates.ht&&s->authorized()&&node==ic->ic_bss&&
        ba.ba_buf&&ba.ba_winsize>=1&&ba.ba_winsize<=IEEE80211_BA_MAX_WINSZ?0:EINVAL;
}
void RTWUSBController::ampdu8192DURXStop(ieee80211com *,ieee80211_node *,uint8_t){}
rtw_packet_error RTWUSBController::decode8192DUUSBFrame(rtw_rx_iterator *it,rtw_8192eu_rx_frame *out,uint8_t *reportType) const
{
    if(out)memset(out,0,sizeof(*out));if(reportType)*reportType=0;
    if(!it||!out||!reportType||it->cursor>it->length||(!it->buffer&&it->length))return RTW_PACKETS_BAD_ARGUMENT;
    if(it->cursor==it->length)return RTW_PACKETS_DONE;
    if(it->cursor||it->frames)return RTW_PACKETS_MALFORMED;
    rtw_92du_rx_frame frame{};const auto result=rtw_92du_parse_rx(it->buffer,it->length,&frame);
    if(result!=RTW_PACKETS_OK)return result;
    if(frame.ht&&(frame.packet.bandwidth_code||duROM_.board.rf_paths<1||duROM_.board.rf_paths>2||
       frame.packet.rate_index>=12U+8U*duROM_.board.rf_paths))return RTW_PACKETS_MALFORMED;
    out->payload=frame.packet;if(!frame.ht)out->payload.bandwidth_code=0;
    if(!frame.fcs_valid)out->payload.crc_error=1;
    out->security_code=frame.security_code;out->sw_decrypt_required=frame.sw_decrypt_required;
    it->cursor=it->length;++it->frames;return RTW_PACKETS_OK;
}
int RTWUSBController::sample8192DURSSI(const rtw_rx_packet& packet)
{
    rtw_92du_rx_frame frame{};frame.packet=packet;rtw_92du_rx_signal signal{};
    if(!rtw_92du_parse_signal(&frame,duROM_.board.rf_paths,(uint8_t)rtw_92du_phy_cck_high_power(&duCore_),&signal))return 0;
    if(!rtw_signal_valid_dbm(signal.dbm))return 0;
    const auto&ic=core_.sc_ic;
    if(ic.ic_state==IEEE80211_S_RUN&&ic.ic_bss&&rtw_signal_from_ap(packet.mpdu,packet.mpdu_length,ic.ic_bss->ni_bssid)){
        duRSSIPWDB_=duRSSIValid_?(uint8_t)(((unsigned)duRSSIPWDB_*19U+signal.pwdb)/20U):signal.pwdb;duRSSIValid_=true;
    }
    return signal.dbm;
}
int RTWUSBController::send8192DUFrame(mbuf_t packet,ieee80211_node *node)
{
    if(!packet)return EINVAL;
    if(!running_||stopping_||!present_||ioError(this)||!node||core_.core_error||
       !duMAC_.started||!rtw_92du_phy_tx_active(&duCore_)){mbuf_freem(packet);return ENETDOWN;}
    ieee80211_frame header{};size_t length=mbuf_pkthdr_len(packet);
    if(length<sizeof(header)||length>4096||mbuf_copydata(packet,0,sizeof(header),&header)){mbuf_freem(packet);return EINVAL;}
    auto&ic=core_.sc_ic;const unsigned type=header.i_fc[0]&IEEE80211_FC0_TYPE_MASK;
    const bool data=type==IEEE80211_FC0_TYPE_DATA,multicast=(header.i_addr1[0]&1)!=0;
    const bool qos=data&&(header.i_fc[0]&IEEE80211_FC0_SUBTYPE_QOS);
    if((type!=IEEE80211_FC0_TYPE_DATA&&type!=IEEE80211_FC0_TYPE_MGT)||
       (header.i_fc[1]&IEEE80211_FC1_DIR_MASK)==IEEE80211_FC1_DIR_DSTODS){mbuf_freem(packet);return EINVAL;}
    const size_t headerLength=sizeof(header)+(qos?2U:0U);uint8_t qosBytes[2]{},llc[8]{};
    if(length<headerLength||(qos&&mbuf_copydata(packet,sizeof(header),2,qosBytes))){mbuf_freem(packet);return EINVAL;}
    const unsigned tid=qos?(qosBytes[0]&IEEE80211_QOS_TID):0;if(tid>7){mbuf_freem(packet);return EINVAL;}
    const bool eapol=data&&length>=headerLength+sizeof(llc)&&!mbuf_copydata(packet,headerLength,sizeof(llc),llc)&&
        llc[0]==0xaa&&llc[1]==0xaa&&llc[2]==3&&!llc[3]&&!llc[4]&&!llc[5]&&llc[6]==0x88&&llc[7]==0x8e;
    if(data&&(node!=ic.ic_bss||(!eapol&&!authorized()))){mbuf_freem(packet);return EACCES;}
    if(header.i_fc[1]&IEEE80211_FC1_PROTECTED){
        auto*key=ieee80211_get_txkey(&ic,&header,node);
        if(!key||key->k_cipher!=IEEE80211_CIPHER_CCMP){mbuf_freem(packet);return EINVAL;}
        packet=ieee80211_encrypt(&ic,packet,key);if(!packet)return ENOBUFS;length=mbuf_pkthdr_len(packet);
    }
    unsigned index=RTW_TRANSFER_SLOTS;IOInterruptState lock=IOSimpleLockLockDisableInterrupt(completionLock_);
    for(unsigned i=RXSlots;i<RTW_TRANSFER_SLOTS;i++)if(!(ledger_.pending&(1U<<i))){index=i;break;}
    IOSimpleLockUnlockEnableInterrupt(completionLock_,lock);
    if(index==RTW_TRANSFER_SLOTS||length>4096){mbuf_freem(packet);return ENOBUFS;}
    const bool firmware=data&&!multicast&&!eapol&&uses8192DUFirmwareRate()&&(!duMAC_.rates.ht||qos);
    const unsigned ac=data?(qos?ieee80211_up_to_ac(&ic,(int)tid):EDCA_AC_BE):EDCA_AC_VO;
    if(ac>3){mbuf_freem(packet);return EINVAL;}
    const int mapped=core_.ac2idx[ac];
    if(mapped<0||mapped>=endpoints_.num_bulk_out){mbuf_freem(packet);return EINVAL;}
    const unsigned pipe=(unsigned)mapped;const uint8_t channel=rtw_92du_phy_channel(&duCore_);
    uint16_t basics=rtw8192DURateMask(node->ni_rates,true)&(channel>14?0xff0U:0xfffU);
    if(!basics)basics=rtw8192DURateMask(node->ni_rates,false)&(channel>14?0xff0U:0xfffU);
    uint8_t basic=channel>14?4:0;for(unsigned i=0;i<12;i++)if(basics&(1U<<i)){basic=(uint8_t)i;break;}
    rtw_92du_tx_layout layout{};if(!rtw_92du_tx_transfer_layout(length,txMPS_[pipe],&layout)||layout.total>TXCapacity){mbuf_freem(packet);return EINVAL;}
    rtw_92du_tx_options options{};options.rf_paths=duROM_.board.rf_paths;options.is_5ghz=channel>14;
    options.rate=firmware?duMAC_.rates.initial_rate:basic;options.raid=firmware?duMAC_.rates.raid:channel>14?5:6;
    options.firmware_rate=firmware;options.multicast=multicast;options.qos=qos;
    static const uint8_t queues[4]={0,2,5,7};
    const unsigned nativeAC=(tid==1||tid==2)?EDCA_AC_BK:(tid==4||tid==5)?EDCA_AC_VI:tid>=6?EDCA_AC_VO:EDCA_AC_BE;
    options.qsel=data?(qos&&ac==nativeAC?(uint8_t)tid:queues[ac]):0x12;
    options.sequence=(uint16_t)(((unsigned)header.i_seq[0]|((unsigned)header.i_seq[1]<<8))>>4);
    options.short_gi=firmware&&duMAC_.rates.ht&&duMAC_.rates.sgi20;
    options.short_preamble=(ic.ic_flags&IEEE80211_F_SHPREAMBLE)&&options.rate>=1&&options.rate<=3;
    options.packet_offset=layout.packet_offset;
    if(data&&!multicast){
        const unsigned htProtection=node->ni_htop1&IEEE80211_HTOP1_PROT_MASK;
        if(length+4>(unsigned)ic.ic_rtsthreshold||(firmware&&duMAC_.rates.ht&&(htProtection==1||htProtection==3)))options.protection=1;
        else if(ic.ic_flags&IEEE80211_F_USEPROT)options.protection=ic.ic_protmode==IEEE80211_PROT_CTSONLY?2:ic.ic_protmode==IEEE80211_PROT_RTSCTS?1:0;
    }
    if(options.protection){options.rts_rate=basic;options.rts_short=(ic.ic_flags&IEEE80211_F_SHPREAMBLE)&&basic>=1&&basic<=3;}
    auto*transfer=&transfers_[index];auto*bytes=static_cast<uint8_t*>(transfer->memory->getBytesNoCopy());bzero(bytes,layout.total);
    if(!rtw_92du_make_tx_descriptor(length,&options,bytes)||mbuf_copydata(packet,0,length,bytes+layout.mpdu_offset)){mbuf_freem(packet);return EINVAL;}
    mbuf_freem(packet);const int error=submit(index,txPipes_[pipe],(uint32_t)layout.total);
    if(!error){if(firmware&&duMAC_.rates.ht)++txHTPackets_;ieee80211_release_node(&ic,node);}return error;
}
void RTWUSBController::watchdog8192DU(_ifnet *ifp)
{
    auto*core=static_cast<rtwn_softc*>(ifp->if_softc);ifp->if_timer=0;
    if(core->sc_tx_timer>0){if(--core->sc_tx_timer==0){ifp->if_oerrors++;task_add(systq,&core->init_task);return;}ifp->if_timer=1;}
    ieee80211_watchdog(ifp);
}
void RTWUSBController::initTask8192DU(void *p)
{
    auto*s=owner(p);auto&core=s->core_;auto&ifp=core.sc_ic.ic_if;
    if(core.sc_flags&RTWN_FLAG_BUSY)return;
    core.sc_flags|=RTWN_FLAG_BUSY;s->stop8192DU();
    if(ifp.if_flags&IFF_UP){const int error=s->start8192DU();if(error&&!s->familyRecovery_.transient(error))core.core_error=error;}
    core.sc_flags&=~(uint32_t)RTWN_FLAG_BUSY;
}
int RTWUSBController::ioctl8192DU(_ifnet *ifp,u_long command,caddr_t data)
{
    auto*core=static_cast<rtwn_softc*>(ifp->if_softc);auto*s=owner(core->sc_ops.cookie);
    if(core->sc_flags&RTWN_FLAG_BUSY)return EBUSY;
    core->sc_flags|=RTWN_FLAG_BUSY;int error=0;
    switch(command){
    case SIOCSIFADDR:ifp->if_flags|=IFF_UP; /* FALLTHROUGH */
    case SIOCSIFFLAGS:
        if(ifp->if_flags&IFF_UP){if(!(ifp->if_flags&IFF_RUNNING))error=s->start8192DU();}
        else if(ifp->if_flags&IFF_RUNNING)s->stop8192DU();break;
    default:error=ieee80211_ioctl(ifp,command,data);break;
    }
    if(core->sc_ic.ic_opmode!=IEEE80211_M_STA){core->sc_ic.ic_opmode=IEEE80211_M_STA;error=EOPNOTSUPP;}
    if(error==ENETRESET){
        if((ifp->if_flags&(IFF_UP|IFF_RUNNING))==(IFF_UP|IFF_RUNNING)){s->stop8192DU();error=s->start8192DU();}
        else error=0;
    }
    core->sc_flags&=~(uint32_t)RTWN_FLAG_BUSY;return error;
}
