/* SPDX-License-Identifier: GPL-2.0-only */
/* RTL8723BU station adapter, explicit typed rtl8xxxu context. The imported
 * net80211 stack owns MLME, software CCMP and controlled-port authorization.
 * HT20/1SS only; physical channel/calibration precede any management TX. */
#include "RTL8723BUFirmware.inc"
static uint16_t rtw8723RateMask(const ieee80211_rateset&r,bool basic){
 static const uint8_t rates[12]={2,4,11,22,12,18,24,36,48,72,96,108};uint16_t mask=0;if(r.rs_nrates>sizeof(r.rs_rates))return 0;
 for(unsigned i=0;i<r.rs_nrates;i++){if(basic&&!(r.rs_rates[i]&IEEE80211_RATE_BASIC))continue;for(unsigned j=0;j<12;j++)if((r.rs_rates[i]&IEEE80211_RATE_VAL)==rates[j])mask|=(uint16_t)(1U<<j);}return mask;
}
bool RTWUSBController::is8723Family()const{return family_==RTW_WIFI_RTL8723BU;}
bool RTWUSBController::supports8723Band(bool five)const{if(five||!is8723Family()||!r23BoardKnown_)return false;for(uint8_t ch=1;ch<=11;ch++)if(rtw_8723bu_channel_supported(&r23Core_,ch,20))return true;return false;}
bool RTWUSBController::permits8723Channel(uint8_t ch)const{return is8723Family()&&r23BoardKnown_&&rtw_8723bu_channel_supported(&r23Core_,ch,20);}
int RTWUSBController::r23Read(void*p,uint16_t a,unsigned w,uint32_t*out){if(!p||!out||(w!=1&&w!=2&&w!=4)||a>65536U-w)return EINVAL;*out=owner(p)->readRegister(a,(uint8_t)w);return ioError(p);}
int RTWUSBController::r23Write(void*p,uint16_t a,unsigned w,uint32_t v){if(!p||(w!=1&&w!=2&&w!=4)||a>65536U-w)return EINVAL;return owner(p)->writeRegister(a,(uint8_t)w,v);}
int RTWUSBController::r23Region(void*p,uint16_t a,const uint8_t*b,size_t n){
 if(!p||!b||!n||n>4096||a>65536U-n)return EINVAL;
 // Use the controller's bounded mutable control-request staging buffer.
 while(n){size_t chunk=n>196?196:n;int e=writeRegion(p,a,b,chunk);if(e)return e;a=(uint16_t)(a+chunk);b+=chunk;n-=chunk;}return 0;
}
int RTWUSBController::bootstrap8723(bool compareMAC){
 if(!is8723Family()||!profile_||!profile_->primary_verified||!r23Storage_||!rxPipe_||endpoints_.num_bulk_in!=1||endpoints_.num_bulk_out<1||endpoints_.num_bulk_out>4||ioError(this))return ENXIO;
 rtw_8723bu_usb usb{};usb.bulk_in=endpoints_.bulk_in[0];usb.bulk_out_count=endpoints_.num_bulk_out;usb.high_speed=rxPipe_->getSpeed()==kUSBHostConnectionSpeedHigh;
 if(rxPipe_->getSpeed()!=kUSBHostConnectionSpeedHigh&&rxPipe_->getSpeed()!=kUSBHostConnectionSpeedFull)return ENOTSUP;usb.bulk_max_packet=txMPS_[0];
 for(unsigned i=0;i<usb.bulk_out_count;i++){if(!txPipes_[i]||txMPS_[i]!=usb.bulk_max_packet)return EINVAL;usb.bulk_out[i]=endpoints_.bulk_out[i];}
 const rtw_8723bu_bus bus{this,r23Read,r23Write,r23Region,delayUS};int e=rtw_8723bu_init(&r23Core_,profile_->vendor,profile_->product,&bus,&usb,r23Storage_,r23PrivateSize_);
 if(!e)e=rtw_8723bu_identify(&r23Core_);if(!e)e=rtw_8723bu_read_board(&r23Core_);if(e)return e;
 if(!validateFamilyRecoveryBoard())return ENODEV;
 if(compareMAC&&(!r23BoardKnown_||memcmp(r23MACAddress_,r23Core_.board.mac,6)))return ENODEV;
 if(!compareMAC){memcpy(r23MACAddress_,r23Core_.board.mac,6);r23BoardKnown_=true;}return 0;
}
int RTWUSBController::attach8723(){
 if(!is8723Family()||r23Storage_)return EINVAL;r23PrivateSize_=rtw_8723bu_private_size();if(!r23PrivateSize_)return EINVAL;r23Storage_=IOMalloc(r23PrivateSize_);if(!r23Storage_)return ENOMEM;
 int e=bootstrap8723(false);if(e){release8723Storage();return e;}
 uint8_t first=0;for(uint8_t ch=1;ch<=11;ch++)if(permits8723Channel(ch)){first=ch;break;}if(!first){release8723Storage();return ENOTSUP;}
 core_.sc_pdev=&device_;core_.ntxchains=core_.nrxchains=1;core_.avg_pwdb=-1;task_set(&core_.init_task,initTask8723,this,"rtw8723_init");
 auto&ic=core_.sc_ic;auto&ifp=ic.ic_if;memcpy(ic.ic_myaddr,r23MACAddress_,6);ic.ic_phytype=IEEE80211_T_OFDM;ic.ic_opmode=IEEE80211_M_STA;ic.ic_state=IEEE80211_S_INIT;
 ic.ic_caps=IEEE80211_C_SHPREAMBLE|IEEE80211_C_SHSLOT|IEEE80211_C_RSN|IEEE80211_C_QOS;ic.ic_flags|=IEEE80211_F_QOS;
 ic.ic_htcaps=IEEE80211_HTCAP_SGI20|(IEEE80211_HTCAP_SMPS_DIS<<IEEE80211_HTCAP_SMPS_SHIFT);ic.ic_ampdu_params=IEEE80211_AMPDU_PARAM_LE|IEEE80211_AMPDU_PARAM_SS_16;
 memset(ic.ic_sup_mcs,0,sizeof(ic.ic_sup_mcs));ic.ic_sup_mcs[0]=255;ic.ic_tx_mcs_set=IEEE80211_TX_MCS_SET_DEFINED;
 ic.ic_sup_rates[IEEE80211_MODE_11B]=ieee80211_std_rateset_11b;ic.ic_sup_rates[IEEE80211_MODE_11G]=ieee80211_std_rateset_11g;
 for(uint8_t ch=1;ch<=11;ch++)if(permits8723Channel(ch)){ic.ic_channels[ch].ic_freq=(uint16_t)ieee80211_ieee2mhz(ch,IEEE80211_CHAN_2GHZ);ic.ic_channels[ch].ic_flags=IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM|IEEE80211_CHAN_DYN|IEEE80211_CHAN_2GHZ|IEEE80211_CHAN_HT20;}
 ic.ic_ibss_chan=&ic.ic_channels[first];ifp.if_softc=&core_;ifp.if_flags=IFF_BROADCAST|IFF_SIMPLEX|IFF_MULTICAST;ifp.if_ioctl=ioctl8723;ifp.if_start=rtwn_start;ifp.if_watchdog=watchdog8723;
 memcpy(ifp.if_xname,device_.dv_xname,IFNAMSIZ);if_attach(&ifp);ieee80211_ifattach(&ifp,this);ic.ic_updateslot=update8723EDCA;ic.ic_updateedca=update8723EDCA;
 ic.ic_ampdu_rx_start=ampduRXStart;ic.ic_ampdu_rx_stop=ampduRXStop;core_.sc_newstate=ic.ic_newstate;ic.ic_newstate=stateTransition;ieee80211_media_init(&ifp);
 setProperty("RTL8723BUCut",(unsigned long long)r23Core_.board.cut,8);setProperty("RTL8723BURFE",(unsigned long long)r23Core_.board.rfe_option,8);return 0;
}
unsigned RTWUSBController::r23RadioState(void*p,uint8_t ch){auto*s=owner(p);if(!s||!s->permits8723Channel(ch)||s->r23Core_.channel!=ch||ioError(s)||s->r23Core_.last_error||!s->r23Core_.powered||!s->r23Core_.firmware_running||!s->r23Core_.mac_initialized||!s->r23Core_.phy_loaded)return 0;return s->r23Core_.calibrated?2:1;}
int RTWUSBController::set8723Channel(uint8_t ch,bool calibrate){if(!permits8723Channel(ch))return ENOTSUP;int e=rtw_8723bu_set_channel(&r23Core_,ch,20);if(!e&&calibrate)e=rtw_8723bu_calibrate(&r23Core_);if(e)core_.core_error=e;return e;}
int RTWUSBController::start8723(){
    const int lifecycle=beginFamilyStart();if(lifecycle)return lifecycle;

 auto&ic=core_.sc_ic;auto&ifp=ic.ic_if;if(!is8723Family()||!r23BoardKnown_||!r23Storage_||!ic.ic_bss||ic.ic_opmode!=IEEE80211_M_STA||ioError(this))return ENXIO;
 if(r23Core_.powered||r23Core_.firmware_running)return EBUSY;radioReady_=r23RateReady_=false;r23Rates_={};core_.core_error=0;
 int e=allocateTransfers();if(e)goto failed;e=bootstrap8723(true);if(e)goto failed;e=rtw_8723bu_boot(&r23Core_,RTW8723BUFirmware,sizeof(RTW8723BUFirmware));if(e)goto failed;
 ic.ic_bss->ni_chan=ic.ic_ibss_chan;e=set8723Channel((uint8_t)ieee80211_chan2ieee(&ic,ic.ic_ibss_chan),true);if(e)goto failed;
 familyRecovery_.diag.stage=rtw::StationRecovery::RX;e=busInitialize(this);if(e)goto failed;familyRecovery_.diag.stage=rtw::StationRecovery::Start;ifq_clr_oactive(&ifp.if_snd);ifp.if_flags|=IFF_RUNNING;
 e=ic.ic_newstate(&ic,IEEE80211_S_SCAN,-1);if(e)goto failed;radioReady_=true;finishFamilyStart();return 0;
failed:failFamilyStart(e);stop8723();if(!familyRecovery_.transient(e))core_.core_error=e;return e;
}
void RTWUSBController::stop8723(){
 familyRecovery_.inStop=true;
 auto&ifp=core_.sc_ic.ic_if;radioReady_=r23RateReady_=false;r23Rates_={};core_.sc_tx_timer=0;ifp.if_timer=0;ifp.if_flags&=~IFF_RUNNING;ifq_clr_oactive(&ifp.if_snd);
 if(core_.sc_newstate)(void)core_.sc_newstate(&core_.sc_ic,IEEE80211_S_INIT,-1);cancelScan(this);cancelCalibration(this);task_del(systq,&core_.init_task);abortTransfers();
 if(!familyStopDrained())return;
 int cleanup=0;
 if(present_&&!sleeping_&&usbError_==kIOReturnSuccess&&r23Core_.identified)cleanup=rtw_8723bu_stop_checked(&r23Core_);
 else{r23Core_.calibrated=r23Core_.associated=r23Core_.powered=r23Core_.firmware_running=r23Core_.mac_initialized=r23Core_.phy_loaded=0;r23Core_.channel=0;cleanup=ENXIO;}
 recordFamilyCleanup(cleanup);freeTransfers();
}
void RTWUSBController::release8723Storage(){if(r23Storage_){IOFree(r23Storage_,r23PrivateSize_);r23Storage_=nullptr;}r23PrivateSize_=0;r23Core_={};r23BoardKnown_=r23RateReady_=false;r23Rates_={};}
uint16_t RTWUSBController::station8723Rates(bool basic)const{auto*n=core_.sc_ic.ic_bss;if(!n)return 0;uint16_t rates=rtw8723RateMask(n->ni_rates,basic);if(basic&&!rates){rates=rtw8723RateMask(n->ni_rates,false);rates&=(uint16_t)(0U-rates);}return rates;}
int RTWUSBController::station8723Hardware(ieee80211_state state){
 if(state==IEEE80211_S_INIT)return rtw_8723bu_station_state(&r23Core_,0,nullptr);
 auto&ic=core_.sc_ic;const auto*n=ic.ic_bss;if(!n||!n->ni_chan||n->ni_chan==IEEE80211_CHAN_ANYC)return EINVAL;
 if(state==IEEE80211_S_SCAN){rtw_8723bu_station scan{};scan.passive_scan=(n->ni_chan->ic_flags&IEEE80211_CHAN_PASSIVE)!=0;return rtw_8723bu_station_state(&r23Core_,1,&scan);}
 unsigned channel=ieee80211_chan2ieee(&ic,n->ni_chan);if(channel>255||r23RadioState(this,(uint8_t)channel)!=2)return ENXIO;
 rtw_8723bu_station p{};memcpy(p.bssid,n->ni_bssid,6);p.channel=(uint8_t)channel;p.short_slot=(ic.ic_flags&IEEE80211_F_SHSLOT)!=0;p.short_preamble=(ic.ic_flags&IEEE80211_F_SHPREAMBLE)!=0;
 p.ht=(n->ni_flags&IEEE80211_NODE_HT)!=0;p.sgi20=p.ht&&(ic.ic_htcaps&n->ni_htcaps&IEEE80211_HTCAP_SGI20);p.mcs=p.ht?n->ni_rxmcs[0]:0;
 p.aid=n->ni_associd&0x3fff;p.beacon_tu=n->ni_intval;p.basic_rates=station8723Rates(true);p.legacy_rates=station8723Rates(false);
 if(state==IEEE80211_S_RUN){int e=rtw_8723bu_prepare_rates(p.legacy_rates,p.mcs,p.ht,p.sgi20,&r23Rates_);if(e)return e;}
 return rtw_8723bu_station_state(&r23Core_,state==IEEE80211_S_RUN?3:2,&p);
}
int RTWUSBController::state8723(ieee80211com*ic,ieee80211_state state,int argument){
 if(!ic||!core_.sc_newstate)return EINVAL;
 if(ic->ic_state==IEEE80211_S_SCAN)cancelScan(this);
 if(ic->ic_state==IEEE80211_S_RUN&&state!=IEEE80211_S_RUN){cancelCalibration(this);r23RateReady_=false;r23Rates_={};}
 if(state==IEEE80211_S_INIT){int e=0;if(!ioError(this)&&r23Core_.mac_initialized&&!r23Core_.last_error)e=station8723Hardware(state);r23Core_.associated=0;r23RateReady_=false;int callback=core_.sc_newstate(ic,state,argument);return e?e:callback;}
 auto rollback=[&](int e)->int{stop8723();core_.core_error=e;return e;};
 if(!ic->ic_bss||!ic->ic_bss->ni_chan||ic->ic_bss->ni_chan==IEEE80211_CHAN_ANYC)return rollback(EINVAL);
 unsigned channel=ieee80211_chan2ieee(ic,ic->ic_bss->ni_chan);if(channel>255||!permits8723Channel((uint8_t)channel))return rollback(ENOTSUP);
 int e=0;bool passive=state==IEEE80211_S_SCAN&&(ic->ic_bss->ni_chan->ic_flags&IEEE80211_CHAN_PASSIVE);
 if(state==IEEE80211_S_SCAN||state==IEEE80211_S_AUTH){e=set8723Channel((uint8_t)channel,!passive);if(!e)e=station8723Hardware(state);}
 else if(r23RadioState(this,(uint8_t)channel)!=2)return rollback(ENXIO);
 else if(state==IEEE80211_S_ASSOC||state==IEEE80211_S_RUN)e=station8723Hardware(state);
 if(e)return rollback(e);e=core_.sc_newstate(ic,state,argument);if(e)return rollback(e);
 if(state==IEEE80211_S_RUN){r23RateReady_=true;update8723EDCA(ic);if(core_.core_error||r23Core_.last_error)return rollback(core_.core_error?core_.core_error:r23Core_.last_error);nextCalibration(this);}
 if(state==IEEE80211_S_SCAN)nextScan(this);return 0;
}
void RTWUSBController::update8723EDCA(ieee80211com*ic){if(!ic)return;auto*core=static_cast<rtwn_softc*>(ic->ic_softc);auto*s=owner(core->sc_ops.cookie);if(!s->r23Core_.associated||r23RadioState(s,s->r23Core_.channel)!=2)return;
 const auto*n=ic->ic_bss;const auto*parameters=((ic->ic_flags&IEEE80211_F_QOS)&&n&&(n->ni_flags&IEEE80211_NODE_QOS))?ic->ic_edca_ac:ieee80211_edca_table[(s->station8723Rates(false)&0xff0)?IEEE80211_MODE_11G:IEEE80211_MODE_11B];rtw_8723bu_edca a[4]{};for(unsigned i=0;i<4;i++){const auto&p=parameters[i];a[i]={p.ac_aifsn,p.ac_ecwmin,p.ac_ecwmax,p.ac_txoplimit};}int e=rtw_8723bu_set_edca(&s->r23Core_,(ic->ic_flags&IEEE80211_F_SHSLOT)!=0,a);if(e)core->core_error=e;
}
bool RTWUSBController::uses8723FirmwareRate()const{return is8723Family()&&running_&&!stopping_&&present_&&!sleeping_&&usbError_==kIOReturnSuccess&&r23RateReady_&&r23Core_.associated&&core_.sc_ic.ic_state==IEEE80211_S_RUN&&core_.sc_ic.ic_bss&&r23RadioState(const_cast<RTWUSBController*>(this),r23Core_.channel)==2;}
int RTWUSBController::tick8723(){
 if(core_.sc_ic.ic_state!=IEEE80211_S_RUN)return 0;if(!uses8723FirmwareRate())return ENXIO;
 uint32_t ready=0;int e=r23Read(this,0x80,1,&ready);
 /* 8051 RDY/checksum/RAM/start flags are insufficient while DL_EN is set. */
 if(!e&&(ready&0xc7U)!=0xc6U)e=EIO;if(!e)e=rtw_8723bu_tick(&r23Core_);
 if(e){r23Core_.firmware_running=r23Core_.calibrated=0;r23RateReady_=false;}return e;
}
rtw_packet_error RTWUSBController::decode8723USBFrame(rtw_rx_iterator*it,rtw_8192eu_rx_frame*out,uint8_t*report)const{if(report)*report=0;if(!out||!report)return RTW_PACKETS_BAD_ARGUMENT;auto e=rtw_8723bu_rx_next(it,out);if(e==RTW_PACKETS_OK)*report=out->report_is_c2h?7:0;return e;}
int RTWUSBController::sample8723RSSI(const rtw_rx_packet&p){int dbm=0;if(!p.phy_status_present||rtw_8723bu_rssi(p.rate_index,p.phy_info,p.phy_info_length,&dbm))return 0;const auto*n=core_.sc_ic.ic_bss;if(n&&core_.sc_ic.ic_state==IEEE80211_S_RUN&&rtw_signal_from_ap(p.mpdu,p.mpdu_length,n->ni_bssid)){r23Core_.rssi_percent=(uint8_t)(dbm<=-100?0:dbm>=0?100:dbm+100);r23Core_.rssi_valid=1;}return dbm;}
void RTWUSBController::handle8723C2H(const uint8_t*b,size_t n){int e=rtw_8723bu_c2h(&r23Core_,b,n);if(e==ENOTSUP){++rateControl_.ignored;return;}if(e){++rateControl_.invalid;if(r23Core_.last_error)core_.core_error=e;return;}if(n>=2&&b[0]==0x0c&&uses8723FirmwareRate()){++rateControl_.reports;rateControl_.finalRate=r23Core_.tx_rate;}}

int RTWUSBController::send8723Frame(mbuf_t packet,ieee80211_node*node){
 if(!packet)return EINVAL;if(!running_||stopping_||!present_||sleeping_||!node||usbError_!=kIOReturnSuccess||r23Core_.last_error||r23RadioState(this,r23Core_.channel)!=2){mbuf_freem(packet);return ENETDOWN;}
 ieee80211_frame header{};size_t length=mbuf_pkthdr_len(packet);if(length<sizeof(header)||length>4096||mbuf_copydata(packet,0,sizeof(header),&header)){mbuf_freem(packet);return EINVAL;}
 auto&ic=core_.sc_ic;bool data=(header.i_fc[0]&IEEE80211_FC0_TYPE_MASK)==IEEE80211_FC0_TYPE_DATA,multicast=(header.i_addr1[0]&1)!=0,qos=data&&(header.i_fc[0]&IEEE80211_FC0_SUBTYPE_QOS);size_t headerLength=sizeof(header)+(qos?2:0);uint8_t qosBytes[2]{},llc[8]{};
 if((header.i_fc[1]&IEEE80211_FC1_DIR_MASK)==IEEE80211_FC1_DIR_DSTODS||length<headerLength||(qos&&mbuf_copydata(packet,sizeof(header),2,qosBytes))){mbuf_freem(packet);return EINVAL;}uint8_t tid=qos?qosBytes[0]&15U:0;if(tid>7){mbuf_freem(packet);return EINVAL;}
 bool eapol=data&&length>=headerLength+8&&!mbuf_copydata(packet,headerLength,8,llc)&&llc[0]==0xaa&&llc[1]==0xaa&&llc[2]==3&&!llc[3]&&!llc[4]&&!llc[5]&&llc[6]==0x88&&llc[7]==0x8e;
 if(data&&(ic.ic_state!=IEEE80211_S_RUN||(!eapol&&!authorized()))){mbuf_freem(packet);return ENETDOWN;}
 if(header.i_fc[1]&IEEE80211_FC1_PROTECTED){auto*key=ieee80211_get_txkey(&ic,&header,node);if(!key||key->k_cipher!=IEEE80211_CIPHER_CCMP){mbuf_freem(packet);return EINVAL;}packet=ieee80211_encrypt(&ic,packet,key);if(!packet)return ENOBUFS;length=mbuf_pkthdr_len(packet);}
 unsigned index=RTW_TRANSFER_SLOTS;IOInterruptState lock=IOSimpleLockLockDisableInterrupt(completionLock_);for(unsigned i=RXSlots;i<RTW_TRANSFER_SLOTS;i++)if(!(ledger_.pending&(1U<<i))){index=i;break;}IOSimpleLockUnlockEnableInterrupt(completionLock_,lock);if(index==RTW_TRANSFER_SLOTS||length>4096){mbuf_freem(packet);return ENOBUFS;}
 bool firmware=data&&!multicast&&!eapol&&node==ic.ic_bss&&uses8723FirmwareRate()&&(!(r23Rates_.mask&0xff000U)||qos);uint16_t mask=rtw8723RateMask(node->ni_rates,true);if(!mask)mask=rtw8723RateMask(node->ni_rates,false);if(!mask)mask=1;uint8_t rate=0;while(rate<11&&!(mask&(1U<<rate)))++rate;
 uint8_t initial=19;if(firmware){while(initial&&!(r23Rates_.mask&(1U<<initial)))--initial;}else initial=rate;
 rtw_8723bu_tx_options options{};options.sequence=(uint16_t)(((uint16_t)header.i_seq[0]|((uint16_t)header.i_seq[1]<<8))>>4);options.qsel=data?tid:18;options.data=data&&!multicast;options.multicast=multicast;options.rate=initial;options.raid=firmware?r23Rates_.raid:(rate<=3?8:7);options.firmware_rate=firmware;options.short_preamble=(ic.ic_flags&IEEE80211_F_SHPREAMBLE)&&rate>0&&rate<=3;
 uint8_t descriptor[40]{};if(!rtw_8723bu_make_tx_descriptor(length,&options,descriptor)){mbuf_freem(packet);return EINVAL;}
 if(data&&!multicast&&(length+4>(unsigned)ic.ic_rtsthreshold||(ic.ic_flags&IEEE80211_F_USEPROT)))if(!rtw_92e_tx_protection(1,descriptor)){mbuf_freem(packet);return EINVAL;}
 uint8_t address=0,pipe=255;if(rtw_8723bu_tx_endpoint(&r23Core_,options.qsel,&address)){mbuf_freem(packet);return EINVAL;}for(unsigned i=0;i<endpoints_.num_bulk_out;i++)if(endpoints_.bulk_out[i]==address)pipe=(uint8_t)i;
 if(pipe>=endpoints_.num_bulk_out||!txPipes_[pipe]){mbuf_freem(packet);return EINVAL;}size_t total=40U+length;const uint16_t mps=txMPS_[pipe];if(mps!=64&&mps!=512){mbuf_freem(packet);return EINVAL;}if(total%mps==0)++total;if(total>TXCapacity){mbuf_freem(packet);return EINVAL;}
 auto*t=&transfers_[index];auto*bytes=static_cast<uint8_t*>(t->memory->getBytesNoCopy());bzero(bytes,total);memcpy(bytes,descriptor,40);if(mbuf_copydata(packet,0,length,bytes+40)){mbuf_freem(packet);return EINVAL;}mbuf_freem(packet);int error=submit(index,txPipes_[pipe],(uint32_t)total);if(!error){if(options.rate>=12)++txHTPackets_;ieee80211_release_node(&ic,node);}return error;
}
void RTWUSBController::watchdog8723(_ifnet*ifp){auto*core=static_cast<rtwn_softc*>(ifp->if_softc);ifp->if_timer=0;if(core->sc_tx_timer>0){if(--core->sc_tx_timer==0){++ifp->if_oerrors;task_add(systq,&core->init_task);return;}ifp->if_timer=1;}ieee80211_watchdog(ifp);}
void RTWUSBController::initTask8723(void*p){auto*s=owner(p);auto&core=s->core_;auto&ifp=core.sc_ic.ic_if;if(core.sc_flags&RTWN_FLAG_BUSY)return;core.sc_flags|=RTWN_FLAG_BUSY;s->stop8723();if(ifp.if_flags&IFF_UP){int e=s->start8723();if(e&&!s->familyRecovery_.transient(e))core.core_error=e;}core.sc_flags&=~(uint32_t)RTWN_FLAG_BUSY;}
int RTWUSBController::ioctl8723(_ifnet*ifp,u_long command,caddr_t data){auto*core=static_cast<rtwn_softc*>(ifp->if_softc);auto*s=owner(core->sc_ops.cookie);if(core->sc_flags&RTWN_FLAG_BUSY)return EBUSY;core->sc_flags|=RTWN_FLAG_BUSY;int error=0;switch(command){case SIOCSIFADDR:ifp->if_flags|=IFF_UP;/* FALLTHROUGH */case SIOCSIFFLAGS:if(ifp->if_flags&IFF_UP){if(!(ifp->if_flags&IFF_RUNNING))error=s->start8723();}else if(ifp->if_flags&IFF_RUNNING)s->stop8723();break;default:error=ieee80211_ioctl(ifp,command,data);break;}if(core->sc_ic.ic_opmode!=IEEE80211_M_STA){core->sc_ic.ic_opmode=IEEE80211_M_STA;error=EOPNOTSUPP;}if(error==ENETRESET){if((ifp->if_flags&(IFF_UP|IFF_RUNNING))==(IFF_UP|IFF_RUNNING)){s->stop8723();error=s->start8723();}else error=0;}core->sc_flags&=~(uint32_t)RTWN_FLAG_BUSY;return error;}
