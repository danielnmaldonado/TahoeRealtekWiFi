#include "rtw/RTL8812AUPHY.h"
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <vector>
struct Model{
 std::array<uint8_t,65536> reg{};std::array<uint8_t,4096> page1{};
 uint32_t rf[2][256]{};unsigned io=0,fail_at=0,rf_writes[2]{},wait_calls=0,iqk_reads[2]{};
 uint64_t wait_us=0;bool ready=true,txfail=false,rxfail=false,lck_done=true,fail_wait=false,ready_path[2]={true,true};
 unsigned iqk_x=0x201;uint8_t rf_addr=0;bool saw_unpause=false,observe_pause=false;
 bool c1()const{return(reg[0x82f]&0x80)!=0;}
 uint8_t&byte(unsigned a){return c1()&&((a>=0xc00&&a<0xd00)||(a>=0xe00&&a<0xf00))?page1[a]:reg[a];}
 uint32_t raw(unsigned a,unsigned n){uint32_t v=0;for(unsigned i=0;i<n;++i)v|=uint32_t(byte(a+i))<<(i*8);return v;}
 void put(unsigned a,unsigned n,uint32_t v){for(unsigned i=0;i<n;++i)byte(a+i)=uint8_t(v>>(i*8));}
 bool fail(){return ++io==fail_at;}
 uint32_t read(unsigned a,unsigned n){
  if(n==4&&(a==0xd08||a==0xd04||a==0xd48||a==0xd44))return rf[a>=0xd40][rf_addr];
  if(n==4&&(a==0xd00||a==0xd40)){
   unsigned path=a>=0xd40;++iqk_reads[path];unsigned control=0xcb8+path*0x200;uint32_t selector=0;for(unsigned i=0;i<4;++i)selector|=uint32_t(page1[control+i])<<(8*i);
   unsigned xy=(selector==0x04000000||selector==0x08000000)?0x7ff:iqk_x;
   return (ready&&ready_path[path]?1U<<10:0)|(txfail?1U<<12:0)|(rxfail?1U<<11:0)|(xy<<16);
  }
  return raw(a,n);
 }
 void write(unsigned a,unsigned n,uint32_t v){
  bool page=c1();put(a,n,v);
  if(a==0x8b0)rf_addr=uint8_t(v);
  if(n==4&&(a==0xc90||a==0xe90)&&!page){unsigned path=a==0xe90;++rf_writes[path];rf[path][(v>>20)&255]=v&0xfffff;}
  if(observe_pause&&a<=0x522&&a+n>0x522&&reg[0x522]==0)saw_unpause=true;
 }
 static rtw_usb_error in(void*c,const rtw_usb_setup*s,uint8_t*d,uint16_t*n){auto&m=*static_cast<Model*>(c);if(m.fail())return RTW_USB_IO_ERROR;uint32_t v=m.read(s->w_value,s->w_length);for(unsigned i=0;i<s->w_length;++i)d[i]=uint8_t(v>>(8*i));*n=s->w_length;return RTW_USB_OK;}
 static rtw_usb_error out(void*c,const rtw_usb_setup*s,const uint8_t*d,uint16_t*n){auto&m=*static_cast<Model*>(c);if(m.fail())return RTW_USB_IO_ERROR;uint32_t v=0;for(unsigned i=0;i<s->w_length;++i)v|=uint32_t(d[i])<<(8*i);m.write(s->w_value,s->w_length,v);*n=s->w_length;return RTW_USB_OK;}
 static int wait(void*c,uint32_t us){auto&m=*static_cast<Model*>(c);++m.wait_calls;m.wait_us+=us;if(m.fail_wait)return 0;if(us>=150000&&m.lck_done)m.rf[0][0x18]&=~0x8000U;return 1;}
 Model(){put(0x41a,2,0x30);}
};
static rtw_r12au_phy_board board(bool single=false,unsigned rfe=0){rtw_r12au_phy_board b{};b.chip=single?RTW_R12AU_CHIP_8821AU:RTW_R12AU_CHIP_8812AU;b.rf_paths=single?1:2;b.rfe=uint8_t(rfe);b.cut=single?0:1;b.thermal=0x18;b.crystal=0x20;
 for(auto &p:b.txpwr_idx){std::memset(p,24,sizeof(p));p[11]=0;p[32]=0;}return b;}
struct Fixture{
 Model m;std::vector<std::max_align_t> memory;rtw_r12au_phy*p=nullptr;rtw_r12au_phy_board b;
 Fixture(bool single=false,unsigned rfe=0,unsigned paths=0):memory((rtw_r12au_phy_size()+sizeof(std::max_align_t)-1)/sizeof(std::max_align_t)),b(board(single,rfe)){if(paths)b.rf_paths=uint8_t(paths);rtw_usb_rw_transport bus{&m,Model::in,Model::out};rtw_waiter wait{&m,Model::wait};assert(!rtw_r12au_phy_init(memory.data(),memory.size()*sizeof(std::max_align_t),&b,&bus,&wait,&p));assert(p&&m.io==0);}
 void load(){assert(!rtw_r12au_phy_load_tables(p));assert(m.reg[0x522]==0x3f);}
 void tune(uint8_t ch){assert(!rtw_r12au_phy_set_channel(p,ch,20));assert(m.reg[0x522]==0x3f);assert((m.rf[0][0x18]&255)==ch);if(b.rf_paths==2)assert((m.rf[1][0x18]&255)==ch);assert((m.rf[0][0x18]&0xc00)==0xc00);}
 rtw_r12au_phy_status status(){rtw_r12au_phy_status s{};rtw_r12au_phy_get_status(p,&s);return s;}
};
static void success(){
 for(bool single:{false,true})for(unsigned rfe=0;rfe<(single?1U:4U);++rfe){Fixture f(single,rfe);assert(rtw_r12au_phy_resume_tx(f.p)==EBUSY);assert(rtw_r12au_phy_calibrate(f.p)==EBUSY);f.load();assert(rtw_r12au_phy_load_tables(f.p)==EBUSY);assert(f.m.rf_writes[0]>100);if(single)assert(f.m.rf_writes[1]==0);else assert(f.m.rf_writes[1]>100);
  auto n=f.m.io;assert(rtw_r12au_phy_set_channel(f.p,144,20)==ENOTSUP);assert(rtw_r12au_phy_set_channel(f.p,36,80)==ENOTSUP);assert(f.m.io==n);
  for(uint8_t ch:{uint8_t(1),uint8_t(6),uint8_t(11),uint8_t(36),uint8_t(40),uint8_t(44),uint8_t(48),uint8_t(149),uint8_t(153),uint8_t(157),uint8_t(161),uint8_t(165),uint8_t(6)}){
   f.tune(ch);assert(f.status().tuned&&!f.status().calibrated);uint32_t cca=f.m.raw(0x838,4);f.m.observe_pause=true;
   int result=rtw_r12au_phy_calibrate(f.p);if(result){std::fprintf(stderr,"calib failed chip%u rfe%u ch%u e%d masks%u/%u io%u\n",unsigned(f.b.chip),rfe,ch,result,f.status().tx_iqk_mask,f.status().rx_iqk_mask,f.m.io);std::abort();}
   assert(f.m.reg[0x522]==0x3f&&!f.m.saw_unpause);assert(f.m.raw(0x838,4)==cca);assert(!f.m.c1());assert(f.status().calibrated&&!f.status().tx_enabled);assert(f.status().tx_iqk_mask==(single?1:3)&&f.status().rx_iqk_mask==(single?1:3));
   f.m.observe_pause=false;assert(!rtw_r12au_phy_resume_tx(f.p));assert(f.m.reg[0x522]==0&&f.status().tx_enabled);assert(!rtw_r12au_phy_stop(f.p));assert(f.m.reg[0x522]==0x3f&&!f.status().calibrated);assert(rtw_r12au_phy_resume_tx(f.p)==EBUSY);
  }
  if(single)assert(f.m.rf_writes[1]==0);
 }
}
static void rom_decode(){
 std::array<uint8_t,512> map{};map[0]=0x29;map[1]=0x81;map[0xb9]=0x25;map[0xba]=0x1a;map[0xbc]=0x11;map[0xbd]=0xd;map[0xbf]=0xb;map[0xd7]=2;map[0x107]=4;
 rtw_r12au_phy_board a{},b{};
 assert(!rtw_r12au_phy_parse_board(RTW_R12AU_CHIP_8812AU,1,2,1,map.data(),&a));
 assert(!rtw_r12au_phy_parse_board(RTW_R12AU_CHIP_8821AU,0,1,1,map.data(),&b));
 assert(a.chip==RTW_R12AU_CHIP_8812AU&&a.rf_paths==2&&!a.ext_pa_2g&&!a.ext_pa_5g&&!a.ext_lna_2g&&!a.ext_lna_5g&&!a.btcoex);
 assert(b.chip==RTW_R12AU_CHIP_8821AU&&b.rf_paths==1&&b.ext_pa_2g&&b.ext_pa_5g&&b.ext_lna_2g&&b.ext_lna_5g&&b.btcoex);
 assert(b.gpa_type==1&&b.apa_type==0&&b.glna_type==1&&b.alna_type==3&&b.rfe==0&&b.crystal==0x25&&b.thermal==0x1a);
 for(unsigned address:{0xb9U,0xbaU,0xbcU,0xbdU,0xbfU,0xc1U}){auto copy=map;copy[address]=255;std::memset(&a,255,sizeof(a));assert(rtw_r12au_phy_parse_board(RTW_R12AU_CHIP_8812AU,1,2,0,copy.data(),&a)==ENOTSUP);assert(a.chip==0&&a.rf_paths==0);}
 //8821has its own identityMAC location and RFE0table; OTP0xCAisunused.
 map[0xd7]=255;map[0xca]=255;assert(!rtw_r12au_phy_parse_board(RTW_R12AU_CHIP_8821AU,0,1,0,map.data(),&b));assert(rtw_r12au_phy_parse_board(RTW_R12AU_CHIP_8812AU,1,2,0,map.data(),&a)==EINVAL);
 map[0xca]=0;assert(rtw_r12au_phy_parse_board(RTW_R12AU_CHIP_8812AU,1,2,0,map.data(),&a)==EINVAL);
 auto saved=map;assert(rtw_r12au_phy_parse_board(RTW_R12AU_CHIP_8821AU,0,1,0,map.data(),reinterpret_cast<rtw_r12au_phy_board*>(map.data()))==EINVAL);assert(saved==map);
 assert(rtw_r12au_phy_parse_board(RTW_R12AU_CHIP_8821AU,0,2,0,map.data(),&b)==ENOTSUP);
}
static void initial_band_traits(){
 Fixture f;f.b.swing_2g=3;f.b.ext_lna_2g=1;rtw_usb_rw_transport bus{&f.m,Model::in,Model::out};rtw_waiter wait{&f.m,Model::wait};assert(!rtw_r12au_phy_init(f.memory.data(),f.memory.size()*sizeof(std::max_align_t),&f.b,&bus,&wait,&f.p));f.load();f.tune(6);
 //Band may already read2.4GHz at coldboot. The actualROM swing must still
 //be applied on the first channel, not delayed until the first bandswitch.
 assert((f.m.raw(0xc1c,4)>>21)==0xb6);
}
static void power_vectors(){
 //GoldenTXAGC words independently calculated from pinnedPGBCD rows,
 //EEPROMbase24,and minimumFCC/MKK/ETSIlimit for20MHz. No arbitrarydefault.
 struct Golden{bool one;unsigned rfe;uint32_t cck,ofdm6,ofdm54,ofdm5g6,ofdm5g54;};
 const Golden vectors[]={{false,0,0x16161616,0x1a1a1a1a,0x181a1a1a,0x18181818,0x18181818},{false,3,0x16161616,0x1c1c1c1c,0x181a1c1c,0x1a1a1a1a,0x181a1a1a},{true,0,0x14141414,0x1c1c1c1c,0x181a1c1c,0x1c1c1c1c,0x181a1c1c}};
 for(auto v:vectors){Fixture f(v.one,v.rfe);f.load();f.tune(6);assert(f.m.raw(0xc20,4)==v.cck&&f.m.raw(0xc24,4)==v.ofdm6&&f.m.raw(0xc28,4)==v.ofdm54);f.tune(36);assert(f.m.raw(0xc24,4)==v.ofdm5g6&&f.m.raw(0xc28,4)==v.ofdm5g54);if(!v.one){assert(f.m.raw(0xe24,4)==v.ofdm5g6&&f.m.raw(0xe28,4)==v.ofdm5g54);}}
}
static void invalid(){
 Fixture f;auto n=f.memory.size()*sizeof(std::max_align_t);rtw_usb_rw_transport bus{&f.m,Model::in,Model::out};rtw_waiter wait{&f.m,Model::wait};rtw_r12au_phy*q=f.p;
 assert(rtw_r12au_phy_init(f.memory.data(),n,nullptr,&bus,&wait,&q)==EINVAL&&!q);
 for(auto field:{&rtw_r12au_phy_board::crystal,&rtw_r12au_phy_board::thermal,&rtw_r12au_phy_board::gpa_type,&rtw_r12au_phy_board::ext_lna_2g}){auto b=f.b;b.*field=255;assert(rtw_r12au_phy_init(f.memory.data(),n,&b,&bus,&wait,&q)==ENOTSUP&&!q);}
 auto b=f.b;b.rf_paths=0;assert(rtw_r12au_phy_init(f.memory.data(),n,&b,&bus,&wait,&q)==ENOTSUP&&!q);b=board(true);b.rfe=1;assert(rtw_r12au_phy_init(f.memory.data(),n,&b,&bus,&wait,&q)==ENOTSUP&&!q);assert(!f.m.io);
 Fixture bad;bad.load();bad.b.txpwr_idx[0][8]=255;assert(!rtw_r12au_phy_init(bad.memory.data(),n,&bad.b,&bus,&wait,&q));
 // A fresh context carrying missingOTP must fail beforechannelwrites.
 assert(!rtw_r12au_phy_load_tables(q));auto calls=f.m.io;assert(rtw_r12au_phy_set_channel(q,6,20)==ENOTSUP);assert(f.m.io==calls);
}
static void transport_failures(){
 for(bool one:{false,true}){
  Fixture base(one);base.load();unsigned load_steps=base.m.io;
  for(unsigned step=1;step<=load_steps;++step){Fixture f(one);f.m.fail_at=step;assert(rtw_r12au_phy_load_tables(f.p)==EIO);auto status=f.status();assert(!status.loaded&&!status.calibrated&&!status.tx_enabled&&status.io_error==EIO);if(step>1)assert(f.m.reg[0x522]==0x3f);assert(rtw_r12au_phy_resume_tx(f.p)==EIO);}
  auto before=base.m.io;base.tune(36);unsigned tune_steps=base.m.io-before;
  for(unsigned step=1;step<=tune_steps;++step){Fixture f(one);f.load();f.m.fail_at=f.m.io+step;assert(rtw_r12au_phy_set_channel(f.p,36,20)==EIO);assert(!f.status().tuned&&!f.status().calibrated&&!f.status().tx_enabled);assert(f.m.reg[0x522]==0x3f);}
  std::printf("RTL%u: exercised %u table-load and %u channel I/O failures\n",one?8821:8812,load_steps,tune_steps);
 }
 Fixture a,b(true);a.load();b.load();a.tune(6);b.tune(36);assert(!rtw_r12au_phy_calibrate(a.p));b.m.txfail=true;assert(rtw_r12au_phy_calibrate(b.p)==ETIMEDOUT);assert(a.status().calibrated&&!b.status().calibrated);assert(!rtw_r12au_phy_resume_tx(a.p));assert(a.m.reg[0x522]==0&&b.m.reg[0x522]==0x3f);assert((a.m.rf[0][0x18]&255)==6&&(b.m.rf[0][0x18]&255)==36);
 Fixture empty;empty.load();empty.m.put(0x41a,2,0);assert(rtw_r12au_phy_set_channel(empty.p,36,20)==ETIMEDOUT);assert(!empty.status().tuned&&empty.m.reg[0x522]==0x3f);
}
static void faults(){
 for(bool single:{false,true}){
  for(unsigned fault=0;fault<3;++fault){Fixture f(single);f.load();f.tune(36);if(fault==0)f.m.ready=false;if(fault==1)f.m.txfail=true;if(fault==2)f.m.rxfail=true;f.m.observe_pause=true;assert(rtw_r12au_phy_calibrate(f.p)==ETIMEDOUT);assert(!f.status().calibrated&&!f.status().tx_enabled);assert(f.m.reg[0x522]==0x3f&&!f.m.saw_unpause);assert(rtw_r12au_phy_resume_tx(f.p)==EBUSY);assert(!f.m.c1());}
  Fixture base(single);base.load();base.tune(36);auto before=base.m.io;assert(!rtw_r12au_phy_calibrate(base.p));auto steps=base.m.io-before;
  for(unsigned step=1;step<=steps;++step){Fixture f(single);f.load();f.tune(36);f.m.observe_pause=true;f.m.fail_at=f.m.io+step;int e=rtw_r12au_phy_calibrate(f.p);if(e!=EIO){std::fprintf(stderr,"expectedEIO chip%u step%u/%u e%d\n",unsigned(f.b.chip),step,steps,e);std::abort();}assert(f.status().io_error==EIO&&!f.status().calibrated&&!f.status().tx_enabled);assert(!f.m.saw_unpause);assert(rtw_r12au_phy_resume_tx(f.p)==EIO);}
  std::printf("RTL%u: exercised %u RFK I/O failures\n",single?8821:8812,steps);
 }
 Fixture fallback;fallback.load();fallback.tune(6);fallback.m.iqk_x=0x300;assert(rtw_r12au_phy_calibrate(fallback.p)==ETIMEDOUT);assert(!fallback.status().calibrated&&fallback.status().rx_iqk_mask==0);
 Fixture f;f.load();f.tune(6);f.m.lck_done=false;auto a=f.m.rf[0][0x18],b=f.m.rf[0][0xb4];assert(rtw_r12au_phy_calibrate(f.p)==ETIMEDOUT);assert(f.m.rf[0][0x18]==a&&f.m.rf[0][0xb4]==b);assert(f.m.reg[0x522]==0x3f);
 Fixture wait;wait.m.fail_wait=true;assert(rtw_r12au_phy_load_tables(wait.p)==EIO);assert(wait.status().io_error==EIO&&!wait.status().loaded&&wait.m.reg[0x522]==0x3f);
}
static void usb_rfe_default(){
 std::array<uint8_t,512> map{};map[0]=0x29;map[1]=0x81;map[0xb9]=32;map[0xba]=24;map[0xd7]=2;map[0xca]=255;
 // rtw8812a_read_rfe_type explicitly assigns USB RFE0 for this encoding,
 // before bit7's amplifier inference. Both no-amplifier and external boards
 // therefore decode RFE0, rather than accidentally inferring RFE4 or3.
 for(uint8_t amps:{uint8_t(0),uint8_t(255)}){map[0xbc]=amps?0x33:0;map[0xbd]=map[0xbf]=amps?0x88:0;rtw_r12au_phy_board b{};assert(!rtw_r12au_phy_parse_board(RTW_R12AU_CHIP_8812AU,1,2,0,map.data(),&b));assert(b.rfe==0);}
}
static void all_swing_selectors(){
 static const unsigned swing[]={0x200,0x16a,0x101,0xb6};
 for(bool one:{false,true})for(unsigned value=0;value<256;++value){
  Fixture f(one);f.b.swing_2g=f.b.swing_5g=uint8_t(value);rtw_usb_rw_transport bus{&f.m,Model::in,Model::out};rtw_waiter wait{&f.m,Model::wait};assert(!rtw_r12au_phy_init(f.memory.data(),f.memory.size()*sizeof(std::max_align_t),&f.b,&bus,&wait,&f.p));f.load();
  for(uint8_t ch:{uint8_t(6),uint8_t(36)}){f.tune(ch);assert((f.m.raw(0xc1c,4)>>21)==swing[value&3]);if(!one)assert((f.m.raw(0xe1c,4)>>21)==swing[(value>>2)&3]);}
 }
 std::array<uint8_t,512> map{};map[0]=0x29;map[1]=0x81;map[0xb9]=32;map[0xba]=24;map[0xd7]=map[0x107]=2;map[0xc6]=map[0xc7]=255;
 for(bool one:{false,true}){rtw_r12au_phy_board b{};assert(!rtw_r12au_phy_parse_board(one?RTW_R12AU_CHIP_8821AU:RTW_R12AU_CHIP_8812AU,one?0:1,one?1:2,0,map.data(),&b));assert(b.swing_2g==255&&b.swing_5g==255);}
}
static void single_chain_8812(){
 for(unsigned rfe=0;rfe<4;++rfe){Fixture f(false,rfe,1);f.m.ready_path[1]=false;assert(rtw_r12au_phy_channel_supported(&f.b,6,20));assert(rtw_r12au_phy_channel_supported(&f.b,36,20));f.load();
  // The only page-C LSSI B write disables it. No RF_B table may be loaded.
  assert(f.m.rf_writes[0]>100&&f.m.rf_writes[1]==1&&f.m.raw(0xe90,4)==0);
  assert((f.m.raw(0x808,4)&255)==0x11&&(f.m.raw(0x80c,4)&65535)==0x1111);
  for(uint8_t ch:{uint8_t(6),uint8_t(36),uint8_t(149)}){f.tune(ch);assert(!rtw_r12au_phy_calibrate(f.p));auto s=f.status();assert(s.rf_paths==1&&s.tx_iqk_mask==1&&s.rx_iqk_mask==1&&s.calibrated);assert(f.m.iqk_reads[0]&&!f.m.iqk_reads[1]);assert(!rtw_r12au_phy_resume_tx(f.p));assert(!rtw_r12au_phy_stop(f.p));}
 }
 Fixture failed(false,0,1);failed.load();failed.tune(36);failed.m.ready_path[0]=false;assert(rtw_r12au_phy_calibrate(failed.p)==ETIMEDOUT);assert(!failed.status().calibrated&&!failed.status().tx_enabled&&failed.m.reg[0x522]==0x3f);
 Fixture two;two.load();two.tune(36);two.m.ready_path[1]=false;assert(rtw_r12au_phy_calibrate(two.p)==ETIMEDOUT);assert(!two.status().calibrated&&!two.status().tx_enabled);
}
static void unsupported_coexistence(){
 for(bool one:{false,true}){Fixture f(one);f.b.btcoex=1;auto n=f.memory.size()*sizeof(std::max_align_t);rtw_usb_rw_transport bus{&f.m,Model::in,Model::out};rtw_waiter wait{&f.m,Model::wait};rtw_r12au_phy*q=f.p;rtw_r12au_operating op{36,42,80,RTW_R12AU_MODE_VHT};rtw_r12au_power_plan power{};
  if(!one){assert(!rtw_r12au_phy_channel_supported(&f.b,6,20));assert(rtw_r12au_phy_power_plan(&f.b,&op,&power)==ENOTSUP);assert(rtw_r12au_phy_init(f.memory.data(),n,&f.b,&bus,&wait,&q)==ENOTSUP&&!q);assert(!f.m.io);}
  else{assert(rtw_r12au_phy_channel_supported(&f.b,6,20));assert(!rtw_r12au_phy_init(f.memory.data(),n,&f.b,&bus,&wait,&q));assert(!f.m.io);assert(!rtw_r12au_phy_load_tables(q));assert(!rtw_r12au_phy_set_channel(q,6,20));assert(!rtw_r12au_phy_calibrate(q));auto calls=f.m.io;assert(rtw_r12au_phy_resume_tx(q)==EBUSY&&f.m.io==calls&&f.m.reg[0x522]==0x3f);}
 }
}

int main(){usb_rfe_default();all_swing_selectors();single_chain_8812();unsupported_coexistence();for(bool one:{false,true}){auto b=board(one);assert(rtw_r12au_phy_channel_supported(&b,6,20));assert(rtw_r12au_phy_channel_supported(&b,36,20));assert(!rtw_r12au_phy_channel_supported(&b,144,20));b.txpwr_idx[0][8]=255;assert(!rtw_r12au_phy_channel_supported(&b,6,20));assert(rtw_r12au_phy_channel_supported(&b,1,20));}rom_decode();invalid();initial_band_traits();power_vectors();success();transport_failures();faults();std::puts("RTL8812AU/8821AU PHY model: USB RFE defaults,all swing selectors,8812 OTP1T1R,coex rejection,tables,both bands,power,IQK/LCK,failures passed");}
