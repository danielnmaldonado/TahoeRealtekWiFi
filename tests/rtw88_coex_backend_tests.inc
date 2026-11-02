/* SPDX-License-Identifier: BSD-3-Clause */
struct BTFixture {
 Fixture f;IOTimerEventSource timer;std::vector<std::max_align_t>storage;
 explicit BTFixture(rtw_wifi_family family=RTW_WIFI_RTL8822CU,bool shared=false):f(family),storage((RTWUSBController::R88StorageBytes+sizeof(std::max_align_t)-1)/sizeof(std::max_align_t)){
  testNow=1000;phyMimoCalls=0;phyMimoFail=0;noMbuf=false;
  auto&b=f.c.r88Core_;b.physical_valid=1;b.board.btcoex=1;b.board.rf_board_option=0x20;
  b.board.share_ant=b.board.bt_setting=shared;b.board.rfe_option=family==RTW_WIFI_RTL8822BU?3:0;
  b.firmware.bus.coex_read=testIndirectRead;b.firmware.bus.coex_write=testIndirectWrite;
  regs[0x80]=0x78;regs[0x81]=0xc0;regs[0xaa]=2;regs[0x770]=1;
  const uint8_t local[6]={2,0xcc,0,0,0,0xdd},ap[6]={2,0xaa,0,0,0,0xbb};
  memcpy(f.c.core_.sc_ic.ic_myaddr,local,6);memcpy(f.c.r88MACAddress_,local,6);memcpy(b.board.mac,local,6);memcpy(f.n.ni_bssid,ap,6);
  f.n.ni_flags|=IEEE80211_NODE_QOS;for(unsigned i=0;i<4;++i)f.c.core_.sc_ic.ic_edca_ac[i]=ieee80211_edca_table[IEEE80211_MODE_11G][i];
  f.c.r88Storage_=storage.data();f.c.r88CoexTimer_=&timer;
  f.c.remember88Authority(true);assert(!f.c.coex88Start()&&f.c.coex88Ready()&&timer.queued);
 }
 void ack(bool v1=false,bool success=true){assert(f.c.r88SMPSPending_&&!f.c.r88SMPSWaitingUSB_);uint8_t b[12]{};b[1]=0xad;
  if(v1){b[0]=255;b[2]=15;b[10]=f.c.r88SMPSTag_;b[11]=success?0:0x40;}else{b[0]=3;b[2]=success?0:0x40;b[8]=f.c.r88SMPSTag_;}
  f.c.handle88C2H(b,v1?12:9);assert(!f.c.r88SMPSPending_);
 }
 void run(){f.run();if(f.c.family_==RTW_WIFI_RTL8822CU)ack();}
 ~BTFixture(){f.c.stop88();f.c.r88Storage_=nullptr;f.c.r88CoexTimer_=nullptr;}
};
static void controllerTraces(){
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})for(bool shared:{false,true}){
  BTFixture b(family,shared);b.run();auto&c=b.f.c;auto*coex=c.r88Coex_;const auto calls=ioCalls;const auto powers=std::count(events.begin(),events.end(),91);
  c.recoveryDrained_=false;c.stop88();assert(ioCalls==calls&&c.r88Coex_==coex&&!b.timer.queued);
  assert(c.familyRecovery_.pendingStop&&!c.running_);c.recoveryDrained_=true;c.stop88();
  assert(!c.r88Coex_&&!c.running_&&!c.familyRecovery_.pendingStop&&std::count(events.begin(),events.end(),91)==powers+1);
 }
 unsigned cases=0;
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})for(bool shared:{false,true}){
  BTFixture b(family,shared);auto&f=b.f;
  assert(!f.state(IEEE80211_S_SCAN));assert(f.c.r88CoexScanning_);b.run();assert(!f.c.r88CoexScanning_&&f.c.coex88Ready());
  assert(f.c.r88Core_.firmware.h2c_box==(commands.size()%4));
  const uint8_t a2dp[9]={9,0,1,0x49,0,20,0,0,40};f.c.handle88C2H(a2dp,9);if(f.c.r88SMPSPending_)b.ack(true);
  rtw88_coex_status status{};rtw88_coex_get_status(f.c.r88Coex_,&status);assert(status.active&&status.bt_info_events==1&&status.bt_profiles==1);
  for(unsigned channel:{36U,165U,6U}){if(channel>14)f.five(channel);else{f.channel.number=6;f.channel.ic_flags=0;}assert(!f.state(IEEE80211_S_SCAN));b.run();assert(f.c.coex88Ready());}
  testNow+=2000;auto before=ioCalls;assert(!f.c.coex88Tick()&&ioCalls>before&&b.timer.queued);
  f.c.stop88();before=ioCalls;assert(!b.timer.queued&&!f.c.r88Coex_&&!f.c.running_);RTWUSBController::coex88TimerAction(&f.c,&b.timer);assert(ioCalls==before);++cases;
 }
 std::printf("B/C actual coex controller: %u family/antenna scan/AUTH/RUN/BT-info/band/RFK/timer/stop traces passed\n",cases);
}
static void smpsFrames(){
 BTFixture b;auto&f=b.f;f.run();assert(f.c.r88SMPSPending_&&!f.c.r88SMPSWaitingUSB_);
 for(unsigned iteration=0;iteration<64;++iteration){
  if(iteration)assert(!f.c.send88SMPS(f.c.r88CoexOneSS_));
  const auto&bytes=f.c.submittedBytes;assert(bytes.size()==75&&bytes[48]==0xd0&&bytes[72]==7&&bytes[73]==1&&bytes[74]==f.c.r88SMPSDesired_);
  assert(!memcmp(bytes.data()+52,f.n.ni_bssid,6)&&!memcmp(bytes.data()+58,f.c.core_.sc_ic.ic_myaddr,6)&&!memcmp(bytes.data()+64,f.n.ni_bssid,6));
  assert(get32(bytes.data()+8)&(1U<<19));assert((get32(bytes.data()+24)&0xfff)==f.c.r88SMPSTag_);assert(((get32(bytes.data()+36)>>12)&0xfff)==f.c.r88SMPSSequence_);
  assert(f.c.r88SMPSTag_==uint8_t(((iteration+1)*4)&0xfc));b.ack((iteration&1)!=0);
  unsigned expected=f.c.r88SMPSDesired_?0U:12U;assert((f.c.core_.sc_ic.ic_htcaps&12)==expected);
 }
 assert(f.c.r88SMPSTag_==0);assert(f.c.send88SMPS(false)==EAGAIN);testNow+=2000;assert(!f.c.send88SMPS(false));b.ack();
 std::puts("B/C actual SMPS:64 shifted CCX tokens including0,V0/V1 ACK,AP addresses,host sequence and retired-token reuse guard passed");
}
static void lifecycle(){
 for(bool missing:{false,true}){BTFixture b;b.run();auto&f=b.f;if(missing)f.c.present_=false;else f.c.sleeping_=true;auto before=ioCalls;f.c.stop88();assert(ioCalls==before&&!f.c.r88Coex_&&!b.timer.queued);RTWUSBController::coex88TimerAction(&f.c,&b.timer);assert(ioCalls==before);}
 {BTFixture b;b.run();auto&f=b.f;auto before=ioCalls;f.c.r88Core_.board.share_ant^=1;assert(!f.c.coex88Ready()&&ioCalls==before);assert(f.c.send88Frame(packet(false,false),&f.n)==ENETDOWN&&ioCalls==before);}
 std::puts("B/C actual coex lifecycle:missing USB/sleep/timer cancellation/live binding guard passed");
}
static void smpsAuthority(){
 for(int kind:{0,1,2,3}){BTFixture b;auto&f=b.f;f.run();assert(f.c.r88SMPSPending_);
  uint8_t report[9]={3,0,0,0,0,0,0,0,f.c.r88SMPSTag_};
  if(kind==0){report[8]^=4;f.c.handle88C2H(report,9);assert(f.c.r88SMPSPending_);b.ack();}
  if(kind==1){f.n.ni_associd^=1;f.c.handle88C2H(report,9);assert(f.c.r88SMPSPending_);f.n.ni_associd^=1;b.ack();}
  if(kind==2){f.n.ni_bssid[5]^=1;f.c.handle88C2H(report,9);assert(f.c.r88SMPSPending_);f.n.ni_bssid[5]^=1;b.ack();}
  if(kind==3){unsigned old=f.c.core_.sc_ic.ic_htcaps;b.ack(false,false);assert(f.c.core_.core_error==ENOLINK&&f.c.core_.sc_ic.ic_htcaps==old&&!f.c.uses88FirmwareRate()&&b.timer.due==testNow+1);unsigned before=ioCalls;const uint8_t bt[9]={9,0,1,0x49,0,20,0,0,40};f.c.handle88C2H(bt,9);assert(ioCalls==before);RTWUSBController::coex88TimerAction(&f.c,&b.timer);assert(f.c.quiesces==1&&!f.c.running_);}
 }
 {BTFixture b;b.f.run();testNow+=500;RTWUSBController::coex88TimerAction(&b.f.c,&b.timer);assert(b.f.c.quiesces==1&&b.f.c.core_.core_error==ETIMEDOUT&&!b.f.c.running_);}
 {BTFixture b;b.run();auto&f=b.f;const unsigned caps=f.c.core_.sc_ic.ic_htcaps;assert(!RTWUSBController::coex88Mimo(&f.c,!f.c.r88CoexOneSS_));assert(f.c.r88SMPSPending_&&f.c.core_.sc_ic.ic_htcaps==caps);assert(f.c.r88Rates_.rates.tx_streams==(f.c.r88CoexOneSS_?1:2));b.ack();}
 {BTFixture b;b.run();auto&f=b.f;noMbuf=true;assert(RTWUSBController::coex88Mimo(&f.c,!f.c.r88CoexOneSS_)==ENOBUFS&&f.c.core_.core_error==ENOBUFS&&!f.c.uses88FirmwareRate());noMbuf=false;}
 std::puts("B/C actual SMPS:USB submission is not ACK;wrong token/association/AP,NACK,timeout,RA/MIMO and allocation failures passed");
}
static void runtimeFailures(){
 unsigned total=0;
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})for(bool shared:{false,true}){
  unsigned count=0;{BTFixture measure(family,shared);auto before=ioCalls;measure.f.run();count=ioCalls-before;}
  for(unsigned step=1;step<=count;++step){BTFixture b(family,shared);failIO=ioCalls+step;int error=b.f.state(IEEE80211_S_AUTH);if(!error)error=b.f.state(IEEE80211_S_ASSOC);if(!error)error=b.f.state(IEEE80211_S_RUN);
   if(error!=EIO)std::fprintf(stderr,"BT failure family%u shared%d step%u/%u error%d\n",unsigned(family),shared,step,count,error);
   assert(error==EIO&&!b.f.c.uses88FirmwareRate()&&!b.f.c.coex88Ready()&&!b.f.c.r88Coex_&&!b.timer.queued&&b.f.c.core_.sc_ic.ic_state==IEEE80211_S_INIT);++total;
  }
 }
 std::printf("B/C actual coexistence controller:%u AUTH/ASSOC/RUN injected I/O failures passed\n",total);
}
static void admissionCase(Fixture&f,IOTimerEventSource*timer,unsigned action,unsigned kind){
 auto&c=f.c;if(action<3||action==6)f.run();else if(action==3)assert(!f.state(IEEE80211_S_SCAN));else{assert(!f.state(IEEE80211_S_AUTH));if(action==5)assert(!f.state(IEEE80211_S_ASSOC));}
 f.n.ni_port_valid=true;auto before=ioCalls;auto sent=commands.size();events.clear();
 switch(kind){case 0:c.r88Core_.firmware.bus.cookie=&f.n;break;case 1:++c.r88Core_.power_epoch;break;case 2:c.r88Core_.board.share_ant^=1;break;case 3:c.r88Core_.raw_bus.cookie=&f.n;break;case 4:c.r88Core_.board.btcoex^=1;break;case 5:c.r88PHY_.boot=nullptr;break;case 6:c.r88PHY_.mac=nullptr;break;case 7:c.r88PHY_.tables_loaded=0;break;case 8:failIO=ioCalls+1;break;default:{const uint16_t bad[]={0xc079,0xffff,0xc070};regs[0x80]=(uint8_t)bad[kind-9];regs[0x81]=(uint8_t)(bad[kind-9]>>8);}}
 if(kind<8){assert(!c.coex88Ready()&&ioCalls==before);if(action<3||action==6)assert(c.send88Frame(packet(false,false),&f.n)==ENETDOWN&&ioCalls==before);}
 int expected=kind==8?EIO:ENXIO;if(action==0)c.stop88();else if(action==6)RTWUSBController::update88EDCA(&c.core_.sc_ic);else{const ieee80211_state targets[]={IEEE80211_S_INIT,IEEE80211_S_INIT,IEEE80211_S_SCAN,IEEE80211_S_AUTH,IEEE80211_S_ASSOC,IEEE80211_S_RUN};assert(f.state(targets[action])==expected);}
 assert(!f.n.ni_port_valid&&c.core_.sc_ic.ic_state==IEEE80211_S_INIT&&!c.r88Coex_&&!c.radioReady_&&(!timer||!timer->queued));
 assert(commands.size()==sent&&std::count(events.begin(),events.end(),91)==0); // Poison cleanup dispatch; fake I/O alone cannot certify it.
 assert(ioCalls-before==(kind>=8?1U:0U)&&!c.r88Core_.firmware.firmware_running&&!c.r88Station_.initialized&&c.core_.core_error==expected);
}
static void cleanupAuthority(){
 unsigned cases=0;
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})for(unsigned ch:{6U,36U,165U})for(unsigned action=0;action<7;++action)for(unsigned kind=0;kind<12;++kind){
  {Fixture f(family);if(ch>14)f.five(ch);admissionCase(f,nullptr,action,kind);++cases;}
  for(bool shared:{false,true}){BTFixture b(family,shared);if(ch>14)b.f.five(ch);admissionCase(b.f,&b.timer,action,kind);++cases;}
 }
 printf("B/C actual admission: %u BT/no-BT,channels6/36/165,7-action owner/MCU/I-O cases; no unauthorized firmware/boot dispatch,host port cleared\n",cases);
}


static void recoveryDefersTimerHardwareUntilResume(){
 unsigned cases=0;
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})for(bool shared:{false,true}){
  BTFixture b(family,shared);b.run();auto&c=b.f.c;
  const unsigned before=ioCalls;const auto sent=commands.size();
  c.rxRecovery_.pending=true;failIO=ioCalls+1;b.timer.queued=false;
  RTWUSBController::coex88TimerAction(&c,&b.timer);
  assert(ioCalls==before&&commands.size()==sent&&c.quiesces==0&&c.running_);
  assert(b.timer.queued&&b.timer.due==testNow+100);
  testNow+=100;RTWUSBController::coex88TimerAction(&c,&b.timer);
  assert(ioCalls==before&&c.quiesces==0&&b.timer.due==testNow+100);
  failIO=0;c.rxRecovery_.pending=false;testNow+=100;
  RTWUSBController::coex88TimerAction(&c,&b.timer);
  assert(ioCalls>before&&c.quiesces==0&&c.running_&&b.timer.queued);
  failIO=ioCalls+1;RTWUSBController::coex88TimerAction(&c,&b.timer);
  assert(c.quiesces==1&&!c.running_);failIO=0;++cases;
 }
 std::printf("B/C RX recovery: %u family/antenna timer pause/resume/error traces passed\n",cases);
}

static void wideCoexistence(){
 unsigned cases=0;
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})for(bool shared:{false,true})for(unsigned width:{20U,40U,80U}){
  BTFixture b(family,shared);auto&f=b.f;f.five();f.n.ni_flags|=IEEE80211_NODE_VHT;f.n.ni_vht_mcsinfo.rx_mcs_map=0xfffa;
  f.n.ni_chw=width==80?IEEE80211_CHAN_WIDTH_80:width==40?IEEE80211_CHAN_WIDTH_40:IEEE80211_CHAN_WIDTH_20;f.n.ni_htop0=IEEE80211_HTOP0_SCO_SCA;f.n.ni_vht_chan1=42;b.run();
  auto input=f.c.coex88Input(IEEE80211_S_RUN);assert(input.primary==36&&input.center==(width==80?42:width==40?38:36)&&input.width_mhz==width&&f.c.r88Rates_.vht&&f.c.coex88Ready());
  if(family==RTW_WIFI_RTL8822CU){assert(!RTWUSBController::coex88Mimo(&f.c,1));b.ack();assert(f.c.r88Rates_.rates.tx_streams==1&&f.c.r88Rates_.vht&&f.c.r88Rates_.width_mhz==width&&f.c.r88RX_.rf_paths==1);assert(!RTWUSBController::coex88Mimo(&f.c,0));b.ack();assert(f.c.r88Rates_.rates.tx_streams==2&&f.c.r88Rates_.vht&&f.c.r88RX_.rf_paths==2);}
  ++cases;
 }
 printf("B/C actual wide coexistence: %u family/antenna/width calibrated tuple and VHT RA/MIMO/SMPS cases passed\n",cases);
}
int main(){wideCoexistence();recoveryDefersTimerHardwareUntilResume();controllerTraces();cleanupAuthority();smpsFrames();smpsAuthority();lifecycle();runtimeFailures();}
