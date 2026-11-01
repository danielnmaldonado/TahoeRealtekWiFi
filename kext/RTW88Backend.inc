#include "rtw/StationChannels.h"
/* SPDX-License-Identifier: BSD-3-Clause
 * Native B/C USB station backend. Distinct board/MAC/PHY/FW contexts; the
 * imported net80211 owns MLME,software CCMP,replay checks and controlled port.
 * HT20 up to actual RF stream count. Individual TX MPDUs,software RX reorder;
 * VHT,40/80MHz and TX AMPDU are not advertised until MLME/RA integration. */
#include "RTL8821CUFirmware.inc"
#include "RTL8822BUFirmware.inc"
#include "RTL8822CUFirmware.inc"
static uint16_t rtw88RateMask(const ieee80211_rateset&r,bool basic){
 static const uint8_t values[12]={2,4,11,22,12,18,24,36,48,72,96,108};uint16_t result=0;
 if(r.rs_nrates>sizeof(r.rs_rates))return 0;for(unsigned i=0;i<r.rs_nrates;i++){if(basic&&!(r.rs_rates[i]&IEEE80211_RATE_BASIC))continue;for(unsigned j=0;j<12;j++)if((r.rs_rates[i]&IEEE80211_RATE_VAL)==values[j])result|=(uint16_t)(1U<<j);}return result;
}
#include "RTW88CoexBackend.inc"
bool RTWUSBController::is88Family() const{return family_==RTW_WIFI_RTL8821CU||family_==RTW_WIFI_RTL8822BU||family_==RTW_WIFI_RTL8822CU;}
bool RTWUSBController::supports88Band(bool five) const{return is88Family()&&(rtw_usb_wifi_profile_allowed_bands(profile_)&(five?2U:1U))&&r88BoardKnown_&&r88Core_.board.features_valid&&r88Core_.board.rf_paths>=1&&r88Core_.board.rf_paths<=(family_==RTW_WIFI_RTL8821CU?1:2)&&(five||r88Core_.board.rf_paths!=0);}
bool RTWUSBController::permits88Channel(uint8_t ch) const{
 /* Verified board/RFE and profile band gates precede this station geometry.
  * All5GHz discovery remains passive; DFS is initially HT20-only. */
 return permitsDFSChannelByCountry(ch)&&supports88Band(ch>14)&&((ch>=1&&ch<=11)||rtw_station_5ghz_channel(ch));
}
int RTWUSBController::r88Read(void*p,uint16_t a,unsigned w,uint32_t*out){
 if(!p||!out||(w!=1&&w!=2&&w!=4)||a>65536U-w)return EINVAL;
 auto*s=owner(p);*out=0;
 if(!s->permits88Transport(a==0xfc&&w==1)){s->clear88Discovery(true);return ENXIO;}
 const uint8_t code=a==0xfc&&s->r88ResolvedIdentity_?rtw_chip_identity_code(s->r88ResolvedIdentity_):0;
 *out=s->readRegister(a,(uint8_t)w);int e=ioError(p);
 if(!e&&(!s->permits88Transport(a==0xfc&&w==1)||(code&&(*out&255U)!=code))){s->clear88Discovery(true);return ENODEV;}
 return e;
}
int RTWUSBController::r88Write(void*p,uint16_t a,unsigned w,uint32_t v){
 if(!p||(w!=1&&w!=2&&w!=4)||a>65536U-w)return EINVAL;auto*s=owner(p);
 if(!s->permits88Transport()){s->clear88Discovery(true);return ENXIO;}
 int e=s->writeRegister(a,(uint8_t)w,v);
 if(!e&&!s->permits88Transport()){s->clear88Discovery(true);return ENODEV;}return e;
}
int RTWUSBController::r88Bulk(void*p,uint8_t queue,const uint8_t*bytes,size_t length){
 auto*s=owner(p);if(!s||!bytes||!length||length>48U+4097U||(queue!=16&&queue!=19))return EINVAL;
 if(!s->permits88Transport()){s->clear88Discovery(true);return ENXIO;}
 if(ioError(p)||s->r88MAC_.fifo.family!=s->family_)return ENXIO;
 uint8_t pipe=0;int e=rtw88_mac_queue_endpoint(&s->r88MAC_.fifo,queue,&pipe);if(e)return e;
 if(pipe>=s->endpoints_.num_bulk_out||!s->txPipes_[pipe]||!s->txMPS_[pipe])return ENXIO;
 const size_t wire=length+(length%s->txMPS_[pipe]==0?1:0);auto*m=IOBufferMemoryDescriptor::withCapacity(wire,kIODirectionOut);
 if(!m||!m->getBytesNoCopy()){if(m)m->release();return ENOMEM;}memcpy(m->getBytesNoCopy(),bytes,length);if(wire!=length)static_cast<uint8_t*>(m->getBytesNoCopy())[length]=0;
 auto result=m->prepare(kIODirectionOut);uint32_t actual=0;if(result==kIOReturnSuccess){result=s->txPipes_[pipe]->io(m,(uint32_t)wire,actual,1000);m->complete(kIODirectionOut);}m->release();
 if(result!=kIOReturnSuccess||actual!=wire){s->fault(result==kIOReturnSuccess?kIOReturnUnderrun:result);return EIO;}return 0;
}
int RTWUSBController::bootstrap88(bool compareMAC){
 if(!is88Family()||!r88Storage_||!rxPipe_||endpoints_.num_bulk_in!=1||endpoints_.num_bulk_out<2||endpoints_.num_bulk_out>4||ioError(this))return ENXIO;
 for(unsigned i=0;i<endpoints_.num_bulk_out;i++)if(!txPipes_[i]||!txMPS_[i]||txMPS_[i]!=txMPS_[0])return EINVAL;
 const rtw88_fw_bus bus{this,r88Read,r88Write,r88Bulk,delayUS,nullptr,nullptr};int e=0;
 if(r88ResolvedIdentity_){
  clear88Discovery(false);
  if(!matches88Discovery()||!rtw_chip_identity_valid(r88ResolvedIdentity_,profile_,&bus)){clear88Discovery(true);return ENODEV;}
  e=rtw_chip_identity_revalidate(r88ResolvedIdentity_,&bus);if(e){clear88Discovery(true);return e;}
 }
 coex88Stop();rtw88_phy_radio_invalidate(&r88PHY_);r88PHY_={};r88MAC_={};r88Station_={};r88PacketSequence_=0;
 e=prepare88Identity(bus);if(e)return e;remember88Authority(false);if(r88Core_.firmware.family!=family_)return ENODEV;
 e=rtw88_mac_prepare(family_,endpoints_.num_bulk_out,&r88MAC_.fifo);if(e)return e;
 e=rtw88_boot_identify(&r88Core_);if(e)return e;e=rtw88_boot_power_on(&r88Core_);if(e)return e;
 /* Linux main.c rtw_chip_efuse_enable requests the hardware report before
  * firmware download/CPU boot. A later read of19 cannot create that report. */
 e=recheck88Identity();if(e)return e;
 e=r88Write(this,0x1a0,1,0xfd);if(e)return e;
 const uint8_t*image=family_==RTW_WIFI_RTL8821CU?RTW8821CUFirmware:family_==RTW_WIFI_RTL8822BU?RTW8822BUFirmware:RTW8822CUFirmware;
 size_t bytes=family_==RTW_WIFI_RTL8821CU?sizeof(RTW8821CUFirmware):family_==RTW_WIFI_RTL8822BU?sizeof(RTW8822BUFirmware):sizeof(RTW8822CUFirmware);
 auto*scratch=static_cast<uint8_t*>(r88Storage_)+RTW88_PHY_RADIO_SCRATCH_BYTES;
 e=rtw88_fw_download(&r88Core_.firmware,image,bytes,scratch,RTW88_FW_SCRATCH_BYTES);if(e)return e;
 e=rtw88_boot_read_board(&r88Core_,scratch+RTW88_FW_SCRATCH_BYTES,RTW88_EFUSE_SCRATCH_BYTES);if(e)return e;
 e=rtw88_boot_read_features(&r88Core_);if(e)return e;
 if(compareMAC&&(!r88BoardKnown_||memcmp(r88MACAddress_,r88Core_.board.mac,6)||core_.ntxchains!=r88Core_.board.tx_streams))return ENODEV;
 if(!compareMAC){memcpy(r88MACAddress_,r88Core_.board.mac,6);r88BoardKnown_=true;}remember88Authority(true);return publish88Identity();
}
int RTWUSBController::attach88(){
 if(!is88Family()||r88Storage_)return EINVAL;r88Storage_=IOMalloc(R88StorageBytes);if(!r88Storage_)return ENOMEM;bzero(r88Storage_,R88StorageBytes);
 int e=bootstrap88(false);if(e){if(r88Core_.firmware.identified&&!ioError(this))(void)rtw88_boot_stop(&r88Core_);release88Storage();return e;}
 if(r88Core_.board.btcoex){e=rtw88_coex_board_supported(&r88Core_);if(e){(void)rtw88_boot_stop(&r88Core_);release88Storage();return e;}}
 /* OTP requires actual powered/running firmware. Read it before interface
  * attach,then power down; start re-reads and compares the immutable MAC. */
 e=rtw88_boot_stop(&r88Core_);if(e){release88Storage();return e;}
 if(r88Core_.board.btcoex){r88CoexTimer_=IOTimerEventSource::timerEventSource(this,coex88TimerAction);if(!r88CoexTimer_){release88Storage();return ENOMEM;}if(!loop_||loop_->addEventSource(r88CoexTimer_)!=kIOReturnSuccess){r88CoexTimer_->release();r88CoexTimer_=nullptr;release88Storage();return EIO;}}
 core_.sc_pdev=&device_;core_.ntxchains=core_.nrxchains=r88Core_.board.tx_streams;core_.avg_pwdb=-1;task_set(&core_.init_task,initTask88,this,"rtw88_init");
 auto&ic=core_.sc_ic;auto&ifp=ic.ic_if;memcpy(ic.ic_myaddr,r88MACAddress_,6);ic.ic_phytype=IEEE80211_T_OFDM;ic.ic_opmode=IEEE80211_M_STA;ic.ic_state=IEEE80211_S_INIT;
 ic.ic_caps=IEEE80211_C_SHPREAMBLE|IEEE80211_C_SHSLOT|IEEE80211_C_RSN|IEEE80211_C_QOS|IEEE80211_C_SCANALLBAND;ic.ic_flags|=IEEE80211_F_QOS;
 ic.ic_htcaps=IEEE80211_HTCAP_SGI20|(IEEE80211_HTCAP_SMPS_DIS<<IEEE80211_HTCAP_SMPS_SHIFT);ic.ic_ampdu_params=IEEE80211_AMPDU_PARAM_LE|IEEE80211_AMPDU_PARAM_SS_16;
 memset(ic.ic_sup_mcs,0,sizeof(ic.ic_sup_mcs));ic.ic_sup_mcs[0]=255;if(core_.nrxchains>1)ic.ic_sup_mcs[1]=255;ic.ic_tx_mcs_set=IEEE80211_TX_MCS_SET_DEFINED;
 ic.ic_sup_rates[IEEE80211_MODE_11A]=ieee80211_std_rateset_11a;ic.ic_sup_rates[IEEE80211_MODE_11B]=ieee80211_std_rateset_11b;ic.ic_sup_rates[IEEE80211_MODE_11G]=ieee80211_std_rateset_11g;
 for(uint8_t ch=1;ch<=11;ch++){if(!permits88Channel(ch))continue;ic.ic_channels[ch].ic_freq=(uint16_t)ieee80211_ieee2mhz(ch,IEEE80211_CHAN_2GHZ);ic.ic_channels[ch].ic_flags=IEEE80211_CHAN_CCK|IEEE80211_CHAN_OFDM|IEEE80211_CHAN_DYN|IEEE80211_CHAN_2GHZ|IEEE80211_CHAN_HT20;}
 static const uint8_t five[]={36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165};for(uint8_t ch:five){if(!permits88Channel(ch))continue;ic.ic_channels[ch].ic_freq=(uint16_t)ieee80211_ieee2mhz(ch,IEEE80211_CHAN_5GHZ);ic.ic_channels[ch].ic_flags=IEEE80211_CHAN_5GHZ|IEEE80211_CHAN_OFDM|IEEE80211_CHAN_PASSIVE|IEEE80211_CHAN_HT20;}
 ic.ic_ibss_chan=&ic.ic_channels[supports88Band(false)?1:36];ifp.if_softc=&core_;ifp.if_flags=IFF_BROADCAST|IFF_SIMPLEX|IFF_MULTICAST;ifp.if_ioctl=ioctl88;ifp.if_start=rtwn_start;ifp.if_watchdog=watchdog88;
 memcpy(ifp.if_xname,device_.dv_xname,IFNAMSIZ);if_attach(&ifp);ieee80211_ifattach(&ifp,this);ic.ic_updateslot=update88EDCA;ic.ic_updateedca=update88EDCA;ic.ic_ampdu_rx_start=ampduRXStart;ic.ic_ampdu_rx_stop=ampduRXStop;
 core_.sc_newstate=ic.ic_newstate;ic.ic_newstate=stateTransition;ieee80211_media_init(&ifp);setProperty("RTW88RFPaths",(unsigned long long)r88Core_.board.rf_paths,8);setProperty("RTW88Cut",(unsigned long long)r88Core_.board.cut,8);setProperty("RTW88RFE",(unsigned long long)r88Core_.board.rfe_option,8);return 0;
}
int RTWUSBController::start88(){
 auto&ic=core_.sc_ic;auto&ifp=ic.ic_if;if(!is88Family()||!r88BoardKnown_||!r88Storage_||!ic.ic_bss||ic.ic_opmode!=IEEE80211_M_STA||ioError(this))return ENXIO;
 if(r88Core_.firmware.powered||r88Core_.firmware.firmware_running)return EBUSY;
 radioReady_=r88Media_=r88RateReady_=r88RSSIValid_=false;r88Rates_={};r88Channel_=r88RSSI_=0;core_.core_error=0;
 int e=allocateTransfers();if(e)goto fail;e=bootstrap88(true);if(e)goto fail;
 {rtw88_usb_speed speed=rxPipe_->getSpeed()>=kUSBHostConnectionSpeedSuper?RTW88_USB_SUPER_SPEED:rxPipe_->getSpeed()==kUSBHostConnectionSpeedHigh?RTW88_USB_HIGH_SPEED:RTW88_USB_FULL_SPEED;
  e=rtw88_mac_initialize(&r88MAC_,&r88Core_.firmware,endpoints_.num_bulk_out,speed);if(e)goto fail;}
 e=rtw88_phy_load_boot_tables(&r88PHY_,&r88Core_,&r88MAC_);if(e)goto fail;
 e=rtw88_phy_radio_initialize(&r88PHY_,r88Storage_,RTW88_PHY_RADIO_SCRATCH_BYTES,RTW88_PHY_REGD_WORLD);if(e)goto fail;
 e=rtw88_phy_radio_set_h2c_sequence(&r88PHY_,&r88PacketSequence_);if(e)goto fail;
 r88RX_={family_,r88Core_.board.active_rf_paths,r88Core_.board.rfe_option,0,0,0};
 if(family_==RTW_WIFI_RTL8822CU){uint32_t a=0,b=0,c=0;e=r88Read(this,0x1a98,4,&a);if(!e)e=r88Read(this,0x1aa8,4,&b);if(!e)e=r88Read(this,0x1a70,4,&c);if(e)goto fail;r88RX_.cck_gain_low=(uint8_t)(((a>>6)&3U)*16U+((c>>24)&15U));r88RX_.cck_gain_high=(uint8_t)(((a>>14)&3U)*16U+((b>>16)&15U));if(r88RX_.cck_gain_low>r88RX_.cck_gain_high){e=EIO;goto fail;}}
 e=rtw88_station_initialize(&r88Station_,&r88MAC_,r88RadioState,this,r88MACAddress_);if(e)goto fail;
 ic.ic_bss->ni_chan=ic.ic_ibss_chan;e=set88Channel((uint8_t)ieee80211_chan2ieee(&ic,ic.ic_ibss_chan),false);if(e)goto fail;
 e=busInitialize(this);if(e)goto fail;e=send88Startup();if(e)goto fail;e=coex88Start();if(e)goto fail;
 ifq_clr_oactive(&ifp.if_snd);ifp.if_flags|=IFF_RUNNING;e=ic.ic_newstate(&ic,IEEE80211_S_SCAN,-1);if(e)goto fail;radioReady_=true;return 0;
fail:stop88();core_.core_error=e;return e;
}
void RTWUSBController::remember88Authority(bool complete){
 r88AuthorityBus_=r88Core_.firmware.bus;r88AuthorityRaw_=r88Core_.raw_bus;
 memcpy(&r88AuthorityBoard_,&r88Core_.board,sizeof(r88AuthorityBoard_));r88AuthorityEpoch_=r88Core_.power_epoch;
 r88AuthorityBound_=true;r88AuthorityComplete_=complete;
}
bool RTWUSBController::owns88Authority(bool operating) const{
 if(!owns88Discovery())return false;
 const auto&f=r88Core_.firmware;
 #define R88_SAME_BUS(a,b) ((a).cookie==(b).cookie&&(a).read==(b).read&&(a).write==(b).write&&(a).bulk_write==(b).bulk_write&&(a).delay_us==(b).delay_us&&(a).coex_read==(b).coex_read&&(a).coex_write==(b).coex_write)
 bool owned=r88AuthorityBound_&&present_&&!sleeping_&&usbError_==kIOReturnSuccess&&f.identified&&f.family==family_&&R88_SAME_BUS(f.bus,r88AuthorityBus_)&&R88_SAME_BUS(r88Core_.raw_bus,r88AuthorityRaw_);
 #undef R88_SAME_BUS
 if(owned&&r88AuthorityComplete_)owned=r88Core_.power_epoch==r88AuthorityEpoch_&&!memcmp(&r88Core_.board,&r88AuthorityBoard_,sizeof(r88AuthorityBoard_));
 if(owned&&(operating||r88Station_.initialized))owned=r88PHY_.boot==&r88Core_&&r88PHY_.mac==&r88MAC_&&r88MAC_.device==&f&&r88MAC_.mac_ready&&r88PHY_.tables_loaded&&r88Core_.board_valid&&r88Core_.physical_valid&&f.powered&&f.firmware_running;
 if(owned&&r88Coex_)owned=r88PHY_.boot==&r88Core_&&r88PHY_.mac==&r88MAC_&&rtw88_coex_ready(r88Coex_,&r88PHY_);
 return owned;
}
bool RTWUSBController::cleanup88Authorized(bool operating){
 auto&f=r88Core_.firmware;bool owned=owns88Authority(operating);
 int e=f.last_error?f.last_error:ENXIO;
 if(owned&&!f.last_error){if(!f.firmware_running)return true;uint32_t value=0;e=f.bus.read?f.bus.read(f.bus.cookie,0x80,2,&value):ENXIO;if(!e&&(value&0xcfffU)==0xc078U)return true;if(!e)e=ENXIO;}
 /* Lost authority permits host cleanup only, including the final power gate. */
 clear88Discovery(!owns88Discovery());r88AuthorityBound_=false;if(!f.last_error)f.last_error=e;f.powered=f.firmware_running=0;
 if(r88Coex_)rtw88_coex_forget(r88Coex_);coex88Stop();rtw88_phy_radio_invalidate(&r88PHY_);
 r88PHY_.tables_loaded=r88MAC_.mac_ready=r88Station_.initialized=0;r88Media_=r88RateReady_=r88RSSIValid_=radioReady_=false;r88Rates_={};
 if(!core_.core_error)core_.core_error=e;return false;
}
void RTWUSBController::stop88(){
 clear88Discovery(false);
 bool hardware=cleanup88Authorized();
 auto&ifp=core_.sc_ic.ic_if;radioReady_=false;core_.sc_tx_timer=0;ifp.if_timer=0;ifp.if_flags&=~IFF_RUNNING;ifq_clr_oactive(&ifp.if_snd);
 /* Flush MLME keys and controlled port even when USB is absent/suspended. */
 if(core_.sc_newstate)(void)state88(&core_.sc_ic,IEEE80211_S_INIT,-1);cancelScan(this);cancelCalibration(this);coex88Stop();task_del(systq,&core_.init_task);abortTransfers();
 rtw88_phy_radio_invalidate(&r88PHY_);r88PHY_.tables_loaded=r88MAC_.mac_ready=r88Station_.initialized=0;
 if(hardware&&cleanup88Authorized())(void)rtw88_boot_stop(&r88Core_);
 else r88Core_.firmware.powered=r88Core_.firmware.firmware_running=r88Core_.board_valid=0;
 r88Media_=r88RateReady_=r88RSSIValid_=false;r88Rates_={};r88Channel_=r88RSSI_=0;freeTransfers();
}
void RTWUSBController::release88Storage(){clear88Discovery(true);coex88Stop();if(r88CoexTimer_){r88CoexTimer_->cancelTimeout();if(loop_)loop_->removeEventSource(r88CoexTimer_);r88CoexTimer_->release();r88CoexTimer_=nullptr;}rtw88_phy_radio_invalidate(&r88PHY_);if(r88Storage_){IOFree(r88Storage_,R88StorageBytes);r88Storage_=nullptr;}r88BoardKnown_=false;}
int RTWUSBController::send88Startup(){
 if(!r88MAC_.mac_ready||!r88PHY_.tables_loaded||!r88Core_.board_valid)return ENXIO;
 const auto&fifo=r88MAC_.fifo;if(fifo.firmware_tx_page<fifo.reserved_boundary||fifo.firmware_tx_page-fifo.reserved_boundary>255)return EINVAL;
 uint8_t packet[80]{};if(rtw88_usb_make_control_descriptor(family_,32,RTW88_USB_CONTROL_H2C,packet,48)!=RTW88_USB_TX_OK)return EINVAL;
 packet[48]=1;packet[49]=255;packet[50]=13;packet[52]=12;packet[54]=(uint8_t)r88PacketSequence_;packet[55]=(uint8_t)(r88PacketSequence_>>8);packet[58]=(uint8_t)(fifo.firmware_tx_page-fifo.reserved_boundary);
 int e=r88Bulk(this,19,packet,sizeof(packet));if(e)return e;++r88PacketSequence_;
 memset(packet+48,0,32);packet[48]=1;packet[49]=255;packet[50]=17;packet[52]=16;packet[54]=(uint8_t)r88PacketSequence_;packet[55]=(uint8_t)(r88PacketSequence_>>8);packet[56]=r88Core_.board.rfe_option;packet[57]=r88Core_.board.active_rf_paths==1?4:2;packet[58]=r88Core_.board.cut;packet[59]=r88Core_.board.active_rf_paths==1?0x11:0x33;
 e=r88Bulk(this,19,packet,sizeof(packet));if(!e)++r88PacketSequence_;return e;
}
unsigned RTWUSBController::r88RadioState(void*p,uint8_t ch){auto*s=owner(p);if(!s||!s->permits88Channel(ch)||s->r88Channel_!=ch)return 0;auto status=rtw88_phy_radio_get_status(&s->r88PHY_);if(status.error||!status.channel_ready||status.primary_channel!=ch||status.center_channel!=ch||status.width_mhz!=20||status.primary_index)return 0;return rtw88_phy_radio_ready(&s->r88PHY_)?2:1;}
int RTWUSBController::set88Channel(uint8_t ch,bool calibrate){
 if(!permits88Channel(ch)||!r88PHY_.tables_loaded)return ENOTSUP;int e=r88Write(this,0x522,1,255);if(!e)e=rtw88_phy_radio_tune(&r88PHY_,ch,ch,20,0);if(!e&&calibrate){if(r88Coex_)e=rtw88_coex_rfk(r88Coex_,1);if(!e)e=rtw88_phy_radio_calibrate(&r88PHY_);if(r88Coex_){int end=rtw88_coex_rfk(r88Coex_,0);if(!e)e=end;}if(!e&&family_==RTW_WIFI_RTL8822CU&&r88Coex_)e=rtw88_phy_radio_set_txrx_1ss(&r88PHY_,r88CoexOneSS_);}if(!e){r88Channel_=ch;r88RX_.current_width=0;}else{r88Channel_=0;core_.core_error=e;}return e;
}
uint16_t RTWUSBController::station88Rates(bool basic) const{const auto*n=core_.sc_ic.ic_bss;if(!n)return 0;uint16_t rates=rtw88RateMask(n->ni_rates,basic)&(r88Channel_>14?0xff0U:0xfffU);if(basic&&!rates){rates=rtw88RateMask(n->ni_rates,false)&(r88Channel_>14?0xff0U:0xfffU);rates&=(uint16_t)(0U-rates);}return rates;}
int RTWUSBController::station88Hardware(ieee80211_state state){
 if(state==IEEE80211_S_INIT)return rtw88_station_configure(&r88Station_,RTW88_STATION_INIT,nullptr);
 const auto&ic=core_.sc_ic;const auto*n=ic.ic_bss;if(!n||!n->ni_chan||n->ni_chan==IEEE80211_CHAN_ANYC)return EINVAL;
 rtw88_station_params params{};memcpy(params.mac,r88MACAddress_,6);memcpy(params.bssid,n->ni_bssid,6);params.channel=r88Channel_;params.short_slot=(ic.ic_flags&IEEE80211_F_SHSLOT)!=0;params.ht=state==IEEE80211_S_RUN&&(n->ni_flags&IEEE80211_NODE_HT)!=0;params.passive_scan=state==IEEE80211_S_SCAN&&(n->ni_chan->ic_flags&IEEE80211_CHAN_PASSIVE);params.aid=n->ni_associd&0x3fff;params.basic_rates=station88Rates(true);if(!params.basic_rates&&state!=IEEE80211_S_RUN)params.basic_rates=r88Channel_>14?16:1;
 return rtw88_station_configure(&r88Station_,(rtw88_station_state)state,&params);
}
int RTWUSBController::state88(ieee80211com*ic,ieee80211_state state,int arg){
 if(!ic||!core_.sc_newstate||ic->ic_opmode!=IEEE80211_M_STA||state<IEEE80211_S_INIT||state>IEEE80211_S_RUN)return EINVAL;
 auto previous=ic->ic_state;cancelScan(this);int error=0;
 if(!cleanup88Authorized(state!=IEEE80211_S_INIT)){r88Channel_=0;(void)core_.sc_newstate(ic,IEEE80211_S_INIT,-1);return r88Core_.firmware.last_error?r88Core_.firmware.last_error:ENXIO;}
 if(state!=IEEE80211_S_RUN){r88SMPSPending_=r88SMPSWaitingUSB_=false;r88SMPSDeadline_=0;}
 if(r88CoexScanning_&&state!=IEEE80211_S_SCAN&&!ioError(this)&&!r88Core_.firmware.last_error)error=coex88Notify(RTW88_CX_SCAN_FINISH,state);
 if((previous==IEEE80211_S_RUN&&state!=IEEE80211_S_RUN)||state==IEEE80211_S_INIT){cancelCalibration(this);(void)rtw88_phy_radio_set_associated(&r88PHY_,0);if(r88Media_&&r88Core_.firmware.firmware_running&&!r88Core_.firmware.last_error&&!ioError(this)){uint8_t cmd[8]{};if(!rtw_8814au_make_media_status(0,cmd))error=EINVAL;else error=rtw88_station_h2c(&r88Station_,cmd);}r88Media_=r88RateReady_=r88RSSIValid_=false;r88Rates_={};if(previous==IEEE80211_S_RUN&&r88Coex_&&!ioError(this)&&!r88Core_.firmware.last_error){int e=coex88Notify(RTW88_CX_MEDIA_DISCONNECT,state);if(!error)error=e;}}
 if(state==IEEE80211_S_INIT){if(r88Station_.initialized&&!ioError(this)&&!r88Core_.firmware.last_error){int e=station88Hardware(state);if(!error)error=e;}r88Channel_=0;int e=core_.sc_newstate(ic,state,arg);return error?error:e;}
 auto rollback=[&](int e)->int{bool hardware=cleanup88Authorized();r88Media_=r88RateReady_=false;r88Rates_={};radioReady_=false;r88Channel_=0;(void)rtw88_phy_radio_set_associated(&r88PHY_,0);coex88Stop();if(hardware&&cleanup88Authorized()){(void)r88Write(this,0x522,1,255);if(r88Core_.firmware.identified)(void)rtw88_boot_stop(&r88Core_);}else r88Core_.firmware.powered=r88Core_.firmware.firmware_running=0;rtw88_phy_radio_invalidate(&r88PHY_);r88PHY_.tables_loaded=r88MAC_.mac_ready=r88Station_.initialized=0;(void)core_.sc_newstate(ic,IEEE80211_S_INIT,-1);core_.core_error=e;return e;};
 if(error)return rollback(error);if(!ic->ic_bss||!ic->ic_bss->ni_chan||ic->ic_bss->ni_chan==IEEE80211_CHAN_ANYC)return rollback(EINVAL);
 unsigned channel=ieee80211_chan2ieee(ic,ic->ic_bss->ni_chan);if(channel>255||!permits88Channel((uint8_t)channel))return rollback(ENOTSUP);
 if(r88Coex_&&(state==IEEE80211_S_SCAN||state==IEEE80211_S_AUTH)){if((r88Channel_>14)!=(channel>14))error=coex88Notify(RTW88_CX_SWITCH_BAND,state,(uint8_t)channel);if(!error&&state==IEEE80211_S_SCAN&&(!r88CoexScanning_||(r88Channel_>14)!=(channel>14)))error=coex88Notify(RTW88_CX_SCAN_START,state,(uint8_t)channel);if(!error&&state==IEEE80211_S_AUTH)error=coex88Notify(RTW88_CX_ASSOC_START,state,(uint8_t)channel);if(error)return rollback(error);}
 const bool passive=state==IEEE80211_S_SCAN&&(ic->ic_bss->ni_chan->ic_flags&IEEE80211_CHAN_PASSIVE);
 if(state==IEEE80211_S_SCAN||state==IEEE80211_S_AUTH){error=set88Channel((uint8_t)channel,!passive);if(!error)error=station88Hardware(state);}
 else if(r88RadioState(this,(uint8_t)channel)!=2)return rollback(ENXIO);
 else if(state==IEEE80211_S_ASSOC)error=station88Hardware(state);
 else if(state==IEEE80211_S_RUN){const auto*n=ic->ic_bss;
  if(!n->ni_intval)return rollback(EINVAL);rtw88_station_rate rate{};error=prepare88Rate(&rate);
  uint8_t media[8]{};if(!error&&!rtw_8814au_make_media_status(1,media))error=EINVAL;
  if(!error)error=rtw88_station_h2c(&r88Station_,rate.command);if(!error)error=rtw88_station_h2c(&r88Station_,media);if(!error)r88Media_=true;
  if(!error)error=r88Write(this,0x554,2,n->ni_intval);if(!error)error=station88Hardware(state);if(!error){r88Rates_=rate;update88EDCA(ic);if(r88Core_.firmware.last_error)error=r88Core_.firmware.last_error;else if(core_.core_error)error=core_.core_error;}}
 if(error)return rollback(error);error=core_.sc_newstate(ic,state,arg);if(error)return rollback(error);
 if(state==IEEE80211_S_RUN){error=rtw88_phy_radio_set_associated(&r88PHY_,1);if(!error&&r88Coex_)error=coex88Notify(RTW88_CX_ASSOC_FINISH,state);if(!error&&r88Coex_)error=coex88Notify(RTW88_CX_MEDIA_CONNECT,state);if(!error&&r88Coex_&&family_==RTW_WIFI_RTL8822CU&&!r88SMPSPending_)error=send88SMPS(r88CoexOneSS_);if(error)return rollback(error);r88RateReady_=true;nextCalibration(this);coex88Schedule();}if(state==IEEE80211_S_SCAN)nextScan(this);return 0;
}
void RTWUSBController::update88EDCA(ieee80211com*ic){if(!ic||!ic->ic_softc)return;auto*core=static_cast<rtwn_softc*>(ic->ic_softc);auto*s=owner(core->sc_ops.cookie);if(!s->r88Station_.initialized||!s->r88Channel_)return;if(!s->cleanup88Authorized(true)){s->r88Channel_=0;if(s->core_.sc_newstate)(void)s->core_.sc_newstate(ic,IEEE80211_S_INIT,-1);return;}if(r88RadioState(s,s->r88Channel_)!=2)return;const auto*n=ic->ic_bss;const auto*parameters=((ic->ic_flags&IEEE80211_F_QOS)&&n&&(n->ni_flags&IEEE80211_NODE_QOS))?ic->ic_edca_ac:ieee80211_edca_table[s->r88Channel_>14?IEEE80211_MODE_11A:(s->station88Rates(false)&0xff0)?IEEE80211_MODE_11G:IEEE80211_MODE_11B];rtw88_station_edca a[4]{};for(unsigned i=0;i<4;i++){const auto&p=parameters[i];a[i]={p.ac_aifsn,p.ac_ecwmin,p.ac_ecwmax,p.ac_txoplimit};}int e=rtw88_station_set_edca(&s->r88Station_,(ic->ic_flags&IEEE80211_F_SHSLOT)!=0,a);if(e)core->core_error=e;}
bool RTWUSBController::uses88FirmwareRate() const{return is88Family()&&!core_.core_error&&coex88Ready()&&running_&&!stopping_&&present_&&!sleeping_&&usbError_==kIOReturnSuccess&&r88Media_&&r88RateReady_&&r88Station_.initialized&&r88MAC_.mac_ready&&!r88Core_.firmware.last_error&&core_.sc_ic.ic_state==IEEE80211_S_RUN&&core_.sc_ic.ic_bss&&r88RadioState(const_cast<RTWUSBController*>(this),r88Channel_)==2;}
/* Linux rtw88/reg.h: 3081 FW_READY excludes CPU clock bits12/13. This
 * matches rtw88_fw_download; the legacy 8051 0xc6 format does not apply. */
int RTWUSBController::tick88(){if(core_.core_error)return core_.core_error;if(core_.sc_ic.ic_state!=IEEE80211_S_RUN)return 0;if(!uses88FirmwareRate())return ENXIO;uint32_t ready=0;int e=r88Read(this,0x80,2,&ready);if(!e&&(ready&0xcfffU)!=0xc078U){r88Core_.firmware.firmware_running=0;e=EIO;}if(!e)e=rtw88_phy_radio_thermal_track(&r88PHY_);if(!e&&r88RSSIValid_){uint8_t command[8]{};if(!rtw_8814au_make_rssi_monitor(r88RSSI_,command))return EINVAL;e=rtw88_station_h2c(&r88Station_,command);}return e;}
rtw_packet_error RTWUSBController::decode88USBFrame(rtw_rx_iterator*it,rtw_8192eu_rx_frame*out,uint8_t*report) const{if(out)memset(out,0,sizeof(*out));if(report)*report=0;if(!out||!report)return RTW_PACKETS_BAD_ARGUMENT;rtw_8814au_rx_frame frame{};auto e=rtw88_rx_next(&r88RX_,it,&frame);if(e!=RTW_PACKETS_OK)return e;out->payload=frame.packet;if(!frame.is_c2h){if(out->payload.mpdu_length<=4)return RTW_PACKETS_MALFORMED;out->payload.mpdu_length-=4;}out->security_code=frame.security_code;out->sw_decrypt_required=frame.sw_decrypt_required;out->report_is_c2h=frame.is_c2h;*report=frame.is_c2h?6:0;return e;}
int RTWUSBController::sample88RSSI(const rtw_rx_packet&p){const auto*ap=core_.sc_ic.ic_bss;if(ap&&core_.sc_ic.ic_state==IEEE80211_S_RUN&&p.mpdu&&p.mpdu_length>=24&&(p.mpdu[0]&IEEE80211_FC0_TYPE_MASK)==IEEE80211_FC0_TYPE_DATA&&!(p.mpdu[4]&1)&&rtw_signal_from_ap(p.mpdu,p.mpdu_length,ap->ni_bssid)){r88CoexRXBytes_+=p.mpdu_length;if(r88CoexRXPackets_!=UINT32_MAX)++r88CoexRXPackets_;r88CoexRXRate_=p.rate_index;}rtw88_rx_phy_sample sample{};if(!rtw88_rx_parse_phy(&r88RX_,&p,&sample))return 0;const auto*n=core_.sc_ic.ic_bss;if(n&&core_.sc_ic.ic_state==IEEE80211_S_RUN&&rtw_signal_from_ap(p.mpdu,p.mpdu_length,n->ni_bssid)){r88RSSI_=sample.rssi;r88RSSIValid_=true;}return sample.signal_dbm;}
void RTWUSBController::handle88C2H(const uint8_t*b,size_t n){if(core_.core_error)return;if(handle88SMPSReport(b,n))return;if(r88Coex_){auto input=coex88Input(core_.sc_ic.ic_state);int error=rtw88_coex_c2h(r88Coex_,b,n,&input);if(!error){coex88Schedule();return;}if(error==EINVAL){++rateControl_.invalid;return;}if(error!=ENOMSG){core_.core_error=error;coex88Schedule();return;}}uint8_t rate=0;int parsed=rtw88_rx_parse_rate_report(b,n,r88RateReady_?&r88Rates_.rates:nullptr,&rate);if(parsed<0){++rateControl_.invalid;return;}if(!parsed){++rateControl_.ignored;return;}if(uses88FirmwareRate()){int e=rtw88_phy_radio_set_tx_rate(&r88PHY_,rate);if(e){core_.core_error=e;return;}++rateControl_.reports;rateControl_.finalRate=rate;r88CoexTXRate_=rate;}}
int RTWUSBController::send88Frame(mbuf_t packet,ieee80211_node*node){
 if(!packet)return EINVAL;if(core_.core_error||!coex88Ready()||!running_||stopping_||!present_||sleeping_||!node||usbError_!=kIOReturnSuccess||r88Core_.firmware.last_error||r88RadioState(this,r88Channel_)!=2||!r88Station_.initialized){mbuf_freem(packet);return ENETDOWN;}
 ieee80211_frame header{};size_t length=mbuf_pkthdr_len(packet);if(length<sizeof(header)||length>4096||mbuf_copydata(packet,0,sizeof(header),&header)){mbuf_freem(packet);return EINVAL;}
 auto&ic=core_.sc_ic;bool data=(header.i_fc[0]&IEEE80211_FC0_TYPE_MASK)==IEEE80211_FC0_TYPE_DATA,multicast=(header.i_addr1[0]&1)!=0,qos=data&&(header.i_fc[0]&IEEE80211_FC0_SUBTYPE_QOS);size_t headerLength=sizeof(header)+(qos?2:0);uint8_t qosBytes[2]{},llc[8]{};
 if((header.i_fc[1]&IEEE80211_FC1_DIR_MASK)==IEEE80211_FC1_DIR_DSTODS||length<headerLength||(qos&&mbuf_copydata(packet,sizeof(header),2,qosBytes))){mbuf_freem(packet);return EINVAL;}uint8_t tid=qos?qosBytes[0]&15U:0;if(tid>7){mbuf_freem(packet);return EINVAL;}
 bool eapol=data&&length>=headerLength+8&&!mbuf_copydata(packet,headerLength,8,llc)&&llc[0]==0xaa&&llc[1]==0xaa&&llc[2]==3&&!llc[3]&&!llc[4]&&!llc[5]&&llc[6]==0x88&&llc[7]==0x8e;
 if(data&&(ic.ic_state!=IEEE80211_S_RUN||(!eapol&&!authorized()))){mbuf_freem(packet);return ENETDOWN;}
 if(header.i_fc[1]&IEEE80211_FC1_PROTECTED){auto*key=ieee80211_get_txkey(&ic,&header,node);if(!key||key->k_cipher!=IEEE80211_CIPHER_CCMP){mbuf_freem(packet);return EINVAL;}packet=ieee80211_encrypt(&ic,packet,key);if(!packet)return ENOBUFS;length=mbuf_pkthdr_len(packet);}
 unsigned index=RTW_TRANSFER_SLOTS;IOInterruptState lock=IOSimpleLockLockDisableInterrupt(completionLock_);for(unsigned i=RXSlots;i<RTW_TRANSFER_SLOTS;i++)if(!(ledger_.pending&(1U<<i))){index=i;break;}IOSimpleLockUnlockEnableInterrupt(completionLock_,lock);if(index==RTW_TRANSFER_SLOTS||length>4096){mbuf_freem(packet);return ENOBUFS;}
 bool firmware=data&&!multicast&&!eapol&&node==ic.ic_bss&&uses88FirmwareRate()&&(!r88Rates_.rates.ht||qos);uint16_t mask=rtw88RateMask(node->ni_rates,true)&(r88Channel_>14?0xff0U:0xfffU);if(!mask)mask=rtw88RateMask(node->ni_rates,false)&(r88Channel_>14?0xff0U:0xfffU);if(!mask)mask=r88Channel_>14?16:1;uint8_t rate=0;while(rate<11&&!(mask&(1U<<rate)))++rate;
 rtw88_usb_tx_options options{};options.sequence=(uint16_t)(((uint16_t)header.i_seq[0]|((uint16_t)header.i_seq[1]<<8))>>4);options.queue=data?tid:18;options.multicast=multicast;options.band_5ghz=r88Channel_>14;options.rate=firmware?r88Rates_.rates.initial_rate:rate;options.rate_id=firmware?r88Rates_.rates.rate_id:(options.band_5ghz?7:8);options.firmware_rate=firmware;options.short_gi=firmware&&r88Rates_.rates.sgi20;options.disable_fallback=!firmware;options.disable_queue_sequence=1;
 bool smps=false;if(!data&&r88SMPSPending_&&length==27&&header.i_fc[0]==0xd0&&options.sequence==r88SMPSSequence_&&!memcmp(header.i_addr1,r88SMPSBSSID_,6)&&!memcmp(header.i_addr3,r88SMPSBSSID_,6)){uint8_t body[3]{};if(!mbuf_copydata(packet,24,3,body)&&body[0]==7&&body[1]==1&&body[2]==r88SMPSDesired_){smps=true;options.request_report=1;options.report_tag=r88SMPSTag_;}}
 if(data&&!multicast&&(length+4>(unsigned)ic.ic_rtsthreshold||(ic.ic_flags&IEEE80211_F_USEPROT)))options.rts=1;
 uint8_t descriptor[48]{};if(rtw88_usb_make_tx_descriptor(family_,length,&options,descriptor,sizeof(descriptor))!=RTW88_USB_TX_OK){mbuf_freem(packet);return EINVAL;}
 uint8_t pipe=0;if(rtw88_mac_queue_endpoint(&r88MAC_.fifo,options.queue,&pipe)||pipe>=endpoints_.num_bulk_out||!txPipes_[pipe]){mbuf_freem(packet);return EINVAL;}size_t total=48U+length;const uint16_t mps=txMPS_[pipe];if(mps!=64&&mps!=512&&mps!=1024){mbuf_freem(packet);return EINVAL;}if(total%mps==0)++total;if(total>TXCapacity){mbuf_freem(packet);return EINVAL;}
 auto*t=&transfers_[index];auto*bytes=static_cast<uint8_t*>(t->memory->getBytesNoCopy());bzero(bytes,total);memcpy(bytes,descriptor,48);if(mbuf_copydata(packet,0,length,bytes+48)){mbuf_freem(packet);return EINVAL;}mbuf_freem(packet);int error=submit(index,txPipes_[pipe],(uint32_t)total);if(!error){if(options.rate>=12)++txHTPackets_;if(data&&!multicast){r88CoexTXBytes_+=length;if(r88CoexTXPackets_!=UINT32_MAX)++r88CoexTXPackets_;}if(smps){r88SMPSWaitingUSB_=false;r88SMPSDeadline_=nowMS()+500;r88SMPSTagRetire_[r88SMPSTag_>>2]=r88SMPSDeadline_+500;coex88Schedule();}ieee80211_release_node(&ic,node);}return error;
}
void RTWUSBController::watchdog88(_ifnet*ifp){auto*core=static_cast<rtwn_softc*>(ifp->if_softc);ifp->if_timer=0;if(core->sc_tx_timer>0){if(--core->sc_tx_timer==0){++ifp->if_oerrors;task_add(systq,&core->init_task);return;}ifp->if_timer=1;}ieee80211_watchdog(ifp);}
void RTWUSBController::initTask88(void*p){auto*s=owner(p);auto&core=s->core_;auto&ifp=core.sc_ic.ic_if;if(core.sc_flags&RTWN_FLAG_BUSY)return;core.sc_flags|=RTWN_FLAG_BUSY;s->stop88();if(ifp.if_flags&IFF_UP){int e=s->start88();if(e)core.core_error=e;}core.sc_flags&=~(uint32_t)RTWN_FLAG_BUSY;}
int RTWUSBController::ioctl88(_ifnet*ifp,u_long command,caddr_t data){auto*core=static_cast<rtwn_softc*>(ifp->if_softc);auto*s=owner(core->sc_ops.cookie);if(core->sc_flags&RTWN_FLAG_BUSY)return EBUSY;core->sc_flags|=RTWN_FLAG_BUSY;int error=0;switch(command){case SIOCSIFADDR:ifp->if_flags|=IFF_UP;/* FALLTHROUGH */case SIOCSIFFLAGS:if(ifp->if_flags&IFF_UP){if(!(ifp->if_flags&IFF_RUNNING))error=s->start88();}else if(ifp->if_flags&IFF_RUNNING)s->stop88();break;default:error=ieee80211_ioctl(ifp,command,data);break;}if(core->sc_ic.ic_opmode!=IEEE80211_M_STA){core->sc_ic.ic_opmode=IEEE80211_M_STA;error=EOPNOTSUPP;}if(error==ENETRESET){if((ifp->if_flags&(IFF_UP|IFF_RUNNING))==(IFF_UP|IFF_RUNNING)){s->stop88();error=s->start88();}else error=0;}core->sc_flags&=~(uint32_t)RTWN_FLAG_BUSY;return error;}
