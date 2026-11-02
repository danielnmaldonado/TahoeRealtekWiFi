/* Actual PHY register/RF model prefix is supplied by the runner. */
static rtw_r12au_operating operating(uint8_t primary,uint8_t center,uint8_t width,rtw_r12au_mode mode){return {primary,center,width,mode};}
static void prepare(rtw_r12au_phy_board&b){for(auto&rom:b.txpwr_idx){rom[11]=0x20;rom[12]=0x3e;rom[32]=0x30;rom[33]=0xe2;rom[38]=0x40;rom[39]=0xf0;}}
#include "RTL8812AUBW80Golden.inc"
static void primary_bw80_power_vectors(){
 const unsigned rates[]={44,51,53,54,61,63};
 for(const auto&g:au_bw80_goldens){
  auto b=board(g.one,g.rfe);
  for(auto&rom:b.txpwr_idx){rom[32]=0x30;rom[33]=0xe2;rom[38]=g.diff1;rom[39]=g.diff2;}
  rtw_r12au_power_plan p{};auto op=operating((uint8_t)g.primary,(uint8_t)g.center,80,RTW_R12AU_MODE_VHT);
  assert(!rtw_r12au_phy_power_plan(&b,&op,&p));
  for(unsigned path=0;path<(g.one?1U:2U);++path)for(unsigned i=0;i<(g.one?3U:6U);++i){
   if(p.index[path][rates[i]]!=g.index[path*6+i])
    std::fprintf(stderr,"primary BW80 mismatch one=%u rfe=%u ch=%u bytes=%02x/%02x path=%u rate=%u actual=%u expected=%u\n",g.one,g.rfe,g.primary,g.diff1,g.diff2,path,rates[i],p.index[path][rates[i]],g.index[path*6+i]);
   assert(p.index[path][rates[i]]==g.index[path*6+i]);
  }
  // A missing factory differential remains unavailable. Absent2SS data is
  // irrelevant to the one-stream board; no default calibration is created.
  b.txpwr_idx[0][38]=255;assert(rtw_r12au_phy_power_plan(&b,&op,&p)==ENOTSUP);
  b.txpwr_idx[0][38]=g.diff1;b.txpwr_idx[0][39]=255;
  assert(rtw_r12au_phy_power_plan(&b,&op,&p)==(g.one?0:ENOTSUP));
 }
 puts("AU VHT80 matches24 primary struct/PHY/PG/limit vectors, signed1SS/2SS and lower/upper5GHz");
}
static void power(){
 for(bool one:{false,true}){
  auto b=board(one);prepare(b);rtw_r12au_power_plan p{};
  auto ht20=operating(6,6,20,RTW_R12AU_MODE_HT);assert(!rtw_r12au_phy_power_plan(&b,&ht20,&p));
  assert(p.valid_rates==(one?0xfffffULL:0xfffffffULL));assert(p.index[0][19]==26);if(!one)assert(p.index[1][27]==24);
  auto ht40=operating(6,8,40,RTW_R12AU_MODE_HT);assert(!rtw_r12au_phy_power_plan(&b,&ht40,&p));assert(p.index[0][19]==24);if(!one)assert(p.index[0][27]==27);
  auto vht80=operating(36,42,80,RTW_R12AU_MODE_VHT);assert(!rtw_r12au_phy_power_plan(&b,&vht80,&p));
  assert(p.index[0][44]==(one?26:30)&&p.index[0][51]==(one?26:28)&&p.index[0][53]==24);
  if(!one)assert(p.index[0][54]==29&&p.index[0][61]==27&&p.index[0][63]==23);
  auto bad=vht80;bad.center=38;assert(rtw_r12au_phy_power_plan(&b,&bad,&p)==ENOTSUP);
  bad=vht80;bad.primary=52;assert(rtw_r12au_phy_power_plan(&b,&bad,&p)==ENOTSUP);
  bad=ht40;bad.primary=11;bad.center=13;assert(rtw_r12au_phy_power_plan(&b,&bad,&p)==ENOTSUP);
  bad=vht80;bad.primary=165;bad.center=171;assert(rtw_r12au_phy_power_plan(&b,&bad,&p)==ENOTSUP);
  auto invalid=b;invalid.txpwr_idx[0][38]=255;assert(rtw_r12au_phy_power_plan(&invalid,&vht80,&p)==ENOTSUP);assert(!rtw_r12au_phy_power_plan(&invalid,&ht20,&p));
 }
}
static void tune(){
 for(bool one:{false,true})for(auto op:{operating(6,6,20,RTW_R12AU_MODE_HT),operating(6,8,40,RTW_R12AU_MODE_HT),operating(40,38,40,RTW_R12AU_MODE_HT),operating(36,38,40,RTW_R12AU_MODE_HT),operating(36,36,20,RTW_R12AU_MODE_VHT),operating(36,38,40,RTW_R12AU_MODE_VHT),operating(48,46,40,RTW_R12AU_MODE_VHT),operating(36,42,80,RTW_R12AU_MODE_VHT),operating(161,155,80,RTW_R12AU_MODE_VHT)}){
  Fixture f(one);prepare(f.b);rtw_usb_rw_transport bus{&f.m,Model::in,Model::out};rtw_waiter wait{&f.m,Model::wait};assert(!rtw_r12au_phy_init(f.memory.data(),f.memory.size()*sizeof(std::max_align_t),&f.b,&bus,&wait,&f.p));f.load();
  assert(!rtw_r12au_phy_set_operating(f.p,&op));auto status=f.status();assert(status.primary==op.primary&&status.center==op.center&&status.width_mhz==op.width_mhz&&status.mode==op.mode);
  assert((f.m.rf[0][0x18]&255)==op.center);assert((f.m.rf[0][0x18]&0xc00)==(op.width_mhz==20?0xc00U:op.width_mhz==40?0x400U:0U));
  assert(!status.calibrated&&!status.tx_enabled&&f.m.reg[0x522]==0x3f);
  assert(!rtw_r12au_phy_calibrate(f.p));assert(f.status().calibrated&&f.status().tx_iqk_mask==(one?1:3));assert(!rtw_r12au_phy_resume_tx(f.p));
  if(op.width_mhz==80&&!one&&op.primary==36){assert(f.m.raw(0xc40,4)==0x1c1e1e1e&&f.m.raw(0xc44,4)==0x1d1d181a&&f.m.raw(0xc4c,4)==0x17191b1d);}
 }
}
static void thermal(){
 for(bool one:{false,true}){
  Fixture f(one);prepare(f.b);rtw_usb_rw_transport bus{&f.m,Model::in,Model::out};rtw_waiter wait{&f.m,Model::wait};assert(!rtw_r12au_phy_init(f.memory.data(),f.memory.size()*sizeof(std::max_align_t),&f.b,&bus,&wait,&f.p));f.load();auto op=operating(36,42,80,RTW_R12AU_MODE_VHT);assert(!rtw_r12au_phy_set_operating(f.p,&op));assert(!rtw_r12au_phy_calibrate(f.p));assert(!rtw_r12au_phy_resume_tx(f.p));assert(!rtw_r12au_phy_set_tx_rate(f.p,one?53:63));
  assert(!rtw_r12au_phy_tick(f.p));assert((f.m.rf[0][0x42]&0x30000)==0x30000);assert(!f.status().thermal_samples);
  f.m.rf[0][0x42]=32U<<10;assert(!rtw_r12au_phy_tick(f.p));assert(f.status().thermal==32&&f.status().thermal_samples==1&&f.status().thermal_recalibrations==1&&f.status().calibrated&&f.status().tx_enabled);
  auto samples=f.status().thermal_samples;assert(!rtw_r12au_phy_tick(f.p));f.m.rf[0][0x42]=0;assert(rtw_r12au_phy_tick(f.p)==EAGAIN&&f.status().thermal_samples==samples&&f.status().tx_enabled);
  assert(!rtw_r12au_phy_set_tx_rate(f.p,4));assert(!rtw_r12au_phy_tick(f.p));f.m.rf[0][0x42]=1U<<10;assert(!rtw_r12au_phy_tick(f.p));assert(f.status().tx_enabled);
  for(unsigned a=0xc20;a<=0xc4c;a+=4){auto word=f.m.raw(a,4);for(unsigned j=0;j<4;++j)assert(((word>>(8*j))&255)<=63);}
 }
}
static void otp_1t1r_modes(){
 for(unsigned rfe=0;rfe<4;++rfe)for(auto op:{operating(6,6,20,RTW_R12AU_MODE_HT),operating(6,8,40,RTW_R12AU_MODE_HT),operating(36,36,20,RTW_R12AU_MODE_VHT),operating(36,38,40,RTW_R12AU_MODE_VHT),operating(36,42,80,RTW_R12AU_MODE_VHT),operating(161,155,80,RTW_R12AU_MODE_VHT)}){
  Fixture f(false,rfe,1);prepare(f.b);std::memset(f.b.txpwr_idx[1],255,sizeof(f.b.txpwr_idx[1]));f.b.swing_2g=f.b.swing_5g=255;f.m.ready_path[1]=false;
  rtw_r12au_power_plan power{};assert(!rtw_r12au_phy_power_plan(&f.b,&op,&power));assert(!(power.valid_rates&((UINT64_C(255)<<20)|(UINT64_C(1023)<<54))));
  rtw_usb_rw_transport bus{&f.m,Model::in,Model::out};rtw_waiter wait{&f.m,Model::wait};assert(!rtw_r12au_phy_init(f.memory.data(),f.memory.size()*sizeof(std::max_align_t),&f.b,&bus,&wait,&f.p));f.load();assert(f.m.rf_writes[1]==1);
  assert(!rtw_r12au_phy_set_operating(f.p,&op));assert(!rtw_r12au_phy_calibrate(f.p));assert(f.status().rf_paths==1&&f.status().tx_iqk_mask==1&&f.status().rx_iqk_mask==1&&!f.m.iqk_reads[1]);assert(!rtw_r12au_phy_resume_tx(f.p));
  assert(rtw_r12au_phy_set_tx_rate(f.p,27)==EINVAL);assert(rtw_r12au_phy_set_tx_rate(f.p,63)==EINVAL);assert(!rtw_r12au_phy_set_tx_rate(f.p,op.mode==RTW_R12AU_MODE_HT?19:52));
  assert(!rtw_r12au_phy_tick(f.p));f.m.rf[0][0x42]=32U<<10;assert(!rtw_r12au_phy_tick(f.p));assert(f.status().thermal_recalibrations==1&&f.status().calibrated&&f.status().tx_enabled&&!f.m.iqk_reads[1]);
 }
}
static void cold_and_failures(){
 unsigned failures=0;
 auto setup=[](Fixture&f,const rtw_r12au_operating&op){prepare(f.b);rtw_usb_rw_transport bus{&f.m,Model::in,Model::out};rtw_waiter wait{&f.m,Model::wait};assert(!rtw_r12au_phy_init(f.memory.data(),f.memory.size()*sizeof(std::max_align_t),&f.b,&bus,&wait,&f.p));f.load();assert(!rtw_r12au_phy_set_operating(f.p,&op));};
 for(unsigned topology:{2U,1U,0U}){bool one=topology==0;unsigned paths=topology?topology:1;
  auto op=operating(36,42,80,RTW_R12AU_MODE_VHT);Fixture measure(one,0,paths);setup(measure,op);auto before=measure.m.io;assert(!rtw_r12au_phy_set_operating(measure.p,&op));unsigned tune_io=measure.m.io-before;
  for(unsigned fault=1;fault<=tune_io;++fault){Fixture f(one,0,paths);setup(f,op);f.m.fail_at=f.m.io+fault;assert(rtw_r12au_phy_set_operating(f.p,&op)==EIO);assert(!f.status().calibrated&&!f.status().tx_enabled&&f.m.reg[0x522]==0x3f);++failures;}
  before=measure.m.io;assert(!rtw_r12au_phy_calibrate(measure.p));unsigned rfk=measure.m.io-before;
  for(unsigned fault=1;fault<=rfk;++fault){Fixture f(one,0,paths);setup(f,op);f.m.fail_at=f.m.io+fault;assert(rtw_r12au_phy_calibrate(f.p)==EIO);assert(!f.status().calibrated&&!f.status().tx_enabled&&f.m.reg[0x522]==0x3f);++failures;}
  assert(!rtw_r12au_phy_resume_tx(measure.p));assert(!rtw_r12au_phy_set_tx_rate(measure.p,paths==1?53:63));assert(!rtw_r12au_phy_tick(measure.p));measure.m.rf[0][0x42]=32U<<10;before=measure.m.io;assert(!rtw_r12au_phy_tick(measure.p));unsigned thermal_io=measure.m.io-before;
  for(unsigned fault=1;fault<=thermal_io;++fault){Fixture f(one,0,paths);setup(f,op);assert(!rtw_r12au_phy_calibrate(f.p));assert(!rtw_r12au_phy_resume_tx(f.p));assert(!rtw_r12au_phy_set_tx_rate(f.p,paths==1?53:63));assert(!rtw_r12au_phy_tick(f.p));f.m.rf[0][0x42]=32U<<10;f.m.fail_at=f.m.io+fault;assert(rtw_r12au_phy_tick(f.p)==EIO);assert(!f.status().calibrated&&!f.status().tx_enabled&&f.m.reg[0x522]==0x3f);++failures;}
  Fixture fail(one,0,paths);setup(fail,op);assert(!rtw_r12au_phy_calibrate(fail.p));assert(!rtw_r12au_phy_resume_tx(fail.p));assert(!rtw_r12au_phy_set_tx_rate(fail.p,paths==1?53:63));assert(!rtw_r12au_phy_tick(fail.p));fail.m.rf[0][0x42]=32U<<10;fail.m.txfail=true;assert(rtw_r12au_phy_tick(fail.p)==ETIMEDOUT&&!fail.status().tx_enabled);
 }
 Fixture cold;prepare(cold.b);cold.b.swing_5g=15;rtw_usb_rw_transport bus{&cold.m,Model::in,Model::out};rtw_waiter wait{&cold.m,Model::wait};assert(!rtw_r12au_phy_init(cold.memory.data(),cold.memory.size()*sizeof(std::max_align_t),&cold.b,&bus,&wait,&cold.p));cold.load();auto op=operating(36,42,80,RTW_R12AU_MODE_VHT);assert(!rtw_r12au_phy_set_operating(cold.p,&op));assert(!rtw_r12au_phy_calibrate(cold.p));assert(!rtw_r12au_phy_resume_tx(cold.p));assert(!rtw_r12au_phy_set_tx_rate(cold.p,4));
 for(unsigned i=0;i<20;++i){assert(!rtw_r12au_phy_tick(cold.p));cold.m.rf[0][0x42]=1U<<10;assert(!rtw_r12au_phy_tick(cold.p));}assert(((cold.m.raw(0xc1c,4)>>21)&0x7ff)==0x81&&cold.status().tx_enabled);
 printf("AU VHT80 and thermal: %u additional I/O failures, cold signed swing and measured RFK failure passed\n",failures);
}
#include "RTL8812AUDFSGoldens.inc"
static void dfs20(){
 const uint8_t channels[]={52,56,60,64,100,104,108,112,116,120,124,128,132,136,140};
 for(bool one:{false,true})for(unsigned rfe=0;rfe<(one?1U:4U);++rfe)for(unsigned paths=1;paths<=(one?1U:2U);++paths){
  Fixture f(one,rfe,paths);prepare(f.b);
  // Each factory group differs, so an upper-band group alias cannot pass.
  for(auto&rom:f.b.txpwr_idx)for(unsigned g=0;g<14;++g)rom[18+g]=uint8_t(10+g);
  rtw_usb_rw_transport bus{&f.m,Model::in,Model::out};rtw_waiter wait{&f.m,Model::wait};
  assert(!rtw_r12au_phy_init(f.memory.data(),f.memory.size()*sizeof(std::max_align_t),&f.b,&bus,&wait,&f.p));f.load();
  for(uint8_t ch:channels){
   assert(!rtw_r12au_phy_set_power_limit_mbm(f.p,INT32_MAX)&&!rtw_r12au_phy_set_power_reduction(f.p,0));
   auto op=operating(ch,ch,20,RTW_R12AU_MODE_HT);rtw_r12au_power_plan power{};
   assert(rtw_r12au_phy_channel_supported(&f.b,ch,20));assert(!rtw_r12au_phy_power_plan(&f.b,&op,&power));
   assert(!rtw_r12au_phy_set_operating(f.p,&op));assert((f.m.rf[0][0x18]&255)==ch);
   const AUDfsGolden*golden=nullptr;for(const auto&g:au_dfs_goldens)if(g.one==one&&g.rfe==(rfe==3?3U:0U)&&g.channel==ch)golden=&g;assert(golden);
   for(unsigned path=0;path<paths;++path)for(unsigned word=0;word<(paths==1?4U:6U);++word){
    unsigned reg=(path?0xe24U:0xc24U)+word*4U;
    if(f.m.raw(reg,4)!=golden->power[path][word]){fprintf(stderr,"AU DFS golden chip%d rfe%u paths%u ch%u reg%x got%x expected%x\n",one,rfe,paths,ch,reg,f.m.raw(reg,4),golden->power[path][word]);std::abort();}
   }
   assert(((f.m.raw(0x860,4)>>17)&0xfff)==(ch<=64?0x453U:ch<=116?0x452U:0x412U));
   assert(!f.status().calibrated&&!f.status().tx_enabled&&f.m.reg[0x522]==0x3f);
   assert(!rtw_r12au_phy_calibrate(f.p));assert(f.status().tx_iqk_mask==((1U<<paths)-1U));assert(!rtw_r12au_phy_resume_tx(f.p));
   assert(!rtw_r12au_phy_set_power_limit_mbm(f.p,1249)&&!rtw_r12au_phy_set_power_reduction(f.p,1));
   assert(rtw_r12au_phy_calibrate(f.p)==EBUSY);
   assert(!rtw_r12au_phy_set_operating(f.p,&op));
   for(unsigned path=0;path<paths;++path)for(unsigned word=0;word<(paths==1?4U:6U);++word){
    unsigned actual=f.m.raw((path?0xe24U:0xc24U)+word*4U,4);
    for(unsigned j=0;j<4;++j){unsigned base=(golden->power[path][word]>>(j*8))&255U,cap=(golden->ceiling[path][word]>>(j*8))&255U;assert(((actual>>(j*8))&255U)==(base<cap?base:cap)-2U);}
   }
   assert(!rtw_r12au_phy_calibrate(f.p)&&!rtw_r12au_phy_resume_tx(f.p));
   assert(!rtw_r12au_phy_set_tx_rate(f.p,19)&&!rtw_r12au_phy_tick(f.p));f.m.rf[0][0x42]=32U<<10;assert(!rtw_r12au_phy_tick(f.p));
   for(unsigned path=0;path<paths;++path)for(unsigned word=0;word<(paths==1?4U:6U);++word){unsigned actual=f.m.raw((path?0xe24U:0xc24U)+word*4U,4);for(unsigned j=0;j<4;++j)assert(((actual>>(j*8))&255U)+2U<=((golden->ceiling[path][word]>>(j*8))&255U));}

   auto calls=f.m.io;auto wide=operating(ch,uint8_t(ch+2),40,RTW_R12AU_MODE_HT);
   assert(rtw_r12au_phy_set_operating(f.p,&wide)==ENOTSUP&&f.m.io==calls);
  }
  auto calls=f.m.io;assert(!rtw_r12au_phy_channel_supported(&f.b,144,20));
  assert(rtw_r12au_phy_set_channel(f.p,144,20)==ENOTSUP&&f.m.io==calls);
 }
 puts("AU DFS20: all 15 primary channels,8812 RFE0..3/1T2T,8821 RFE0,measured RFK,paused scan/wider rejection");
}
static void power_reduction(){
 for(bool one:{false,true})for(uint8_t ch:{uint8_t(52),uint8_t(100),uint8_t(140)}){
  Fixture baseline(one),limited(one);baseline.load();limited.load();
  auto op=operating(ch,ch,20,RTW_R12AU_MODE_HT);
  assert(!rtw_r12au_phy_set_power_reduction(limited.p,1));
  assert(!rtw_r12au_phy_set_operating(baseline.p,&op)&&!rtw_r12au_phy_set_operating(limited.p,&op));
  for(unsigned phase=0;phase<2;++phase){
   if(phase){for(auto*f:{&baseline,&limited}){assert(!rtw_r12au_phy_calibrate(f->p));assert(!rtw_r12au_phy_resume_tx(f->p));assert(!rtw_r12au_phy_set_tx_rate(f->p,19));assert(!rtw_r12au_phy_tick(f->p));f->m.rf[0][0x42]=32U<<10;assert(!rtw_r12au_phy_tick(f->p));}}
   for(unsigned path=0;path<(one?1U:2U);++path)for(unsigned word=0;word<(one?4U:6U);++word){
    auto a=baseline.m.raw((path?0xe24:0xc24)+4*word,4),b=limited.m.raw((path?0xe24:0xc24)+4*word,4);
    for(unsigned j=0;j<4;++j)assert(((a>>(8*j))&255U)==((b>>(8*j))&255U)+2U);
   }
  }
  assert(!rtw_r12au_phy_set_power_reduction(limited.p,0)&&!rtw_r12au_phy_set_operating(limited.p,&op));
 }
 Fixture floor;floor.load();assert(!rtw_r12au_phy_set_power_reduction(floor.p,255));auto op=operating(52,52,20,RTW_R12AU_MODE_HT);
 assert(rtw_r12au_phy_set_operating(floor.p,&op)==ERANGE&&!floor.status().calibrated&&!floor.status().tx_enabled&&floor.m.reg[0x522]==0x3f);
 puts("AU association backoff:2index/dB,all active TXAGC words,thermal/RFK persistence,reset,floor rejection");
}
static void rejected_plan_keeps_live_ceiling(){
 for(bool one:{false,true}){
  Fixture control(one),rejected(one);auto op=operating(52,52,20,RTW_R12AU_MODE_HT);
  for(auto*f:{&control,&rejected}){f->load();assert(!rtw_r12au_phy_set_power_limit_mbm(f->p,1200));assert(!rtw_r12au_phy_set_operating(f->p,&op));assert(!rtw_r12au_phy_calibrate(f->p));assert(!rtw_r12au_phy_resume_tx(f->p));assert(!rtw_r12au_phy_set_tx_rate(f->p,19));}
  auto bad=operating(52,54,40,RTW_R12AU_MODE_HT);auto calls=rejected.m.io;
  assert(rtw_r12au_phy_set_operating(rejected.p,&bad)==ENOTSUP&&rejected.m.io==calls);
  assert(rejected.status().calibrated&&rejected.status().tx_enabled);
  for(auto*f:{&control,&rejected}){assert(!rtw_r12au_phy_tick(f->p));f->m.rf[0][0x42]=45U<<10;assert(!rtw_r12au_phy_tick(f->p));}
  for(unsigned path=0;path<(one?1U:2U);++path)for(unsigned word=0;word<(one?4U:6U);++word)
   assert(control.m.raw((path?0xe24U:0xc24U)+word*4U,4)==rejected.m.raw((path?0xe24U:0xc24U)+word*4U,4));
 }
 puts("AU rejected DFS geometry preserves the active thermal ceiling and calibrated operating plan");
}
static void impossible_signed_cap(){
 for(bool one:{false,true})for(int32_t cap:{INT32_MIN,int32_t(-10000)}){
  Fixture f(one);for(auto&rom:f.b.txpwr_idx){for(unsigned g=0;g<14;++g)rom[18+g]=63;rom[32]=rom[33]=0x77;}
  rtw_usb_rw_transport bus{&f.m,Model::in,Model::out};rtw_waiter wait{&f.m,Model::wait};
  assert(!rtw_r12au_phy_init(f.memory.data(),f.memory.size()*sizeof(std::max_align_t),&f.b,&bus,&wait,&f.p));f.load();
  assert(!rtw_r12au_phy_set_power_limit_mbm(f.p,cap));auto op=operating(52,52,20,RTW_R12AU_MODE_HT);
  assert(rtw_r12au_phy_set_operating(f.p,&op)==ERANGE&&!f.status().tuned&&!f.status().calibrated&&!f.status().tx_enabled);
  assert(rtw_r12au_phy_calibrate(f.p)==EBUSY&&rtw_r12au_phy_resume_tx(f.p)==EBUSY&&f.m.reg[0x522]==0x3f);
 }
 puts("AU exact signed SAR bounds reject impossible caps at maximum valid factory base");
}
int main(){primary_bw80_power_vectors();impossible_signed_cap();rejected_plan_keeps_live_ceiling();power_reduction();dfs20();power();tune();thermal();otp_1t1r_modes();cold_and_failures();puts("AU actual HT20/40 and VHT20/40/80 power/channel/RFK modes including8812 OTP1T1R passed");}
