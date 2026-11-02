/* SPDX-License-Identifier: BSD-3-Clause */
#include "rtw/RTW88PHYOperating.h"
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <array>
#include "RTW88OperatingTestBus.h"
static int liveOperation(unsigned op,rtw88_phy_context&phy,std::array<uint8_t,RTW88_PHY_RADIO_SCRATCH_BYTES>&scratch,uint8_t ch){switch(op){case 0:return rtw88_phy_radio_initialize(&phy,scratch.data(),scratch.size(),RTW88_PHY_REGD_WORLD);case 1:return rtw88_phy_radio_tune(&phy,ch,ch,20,0);case 2:return rtw88_phy_radio_calibrate(&phy);case 3:return rtw88_phy_radio_thermal_track(&phy);default:return rtw88_phy_radio_set_txrx_1ss(&phy,1);}}
// Primary reg.h/mac.c: exact3081 readiness, clock selector ignored.
static void liveHealthRegression(){
 unsigned checks=0;alignas(8)std::array<uint8_t,RTW88_PHY_RADIO_SCRATCH_BYTES>scratch{};
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})for(uint8_t ch:{6,36})for(unsigned op=0;op<5;++op){if((op==4&&family!=RTW_WIFI_RTL8822CU)||(op==0&&ch==36))continue;
  for(unsigned health:{0xc078U,0xd078U,0xe078U,0xf078U,0xc079U,0xffffU,0xc070U,0x10000U})for(bool mid:{false,true}){
   if(mid&&(op==3||(health&0xcfffU)==0xc078U))continue;
   OperatingBus bus(family);rtw88_boot_context boot{};rtw88_mac_context mac{};rtw88_phy_context phy{};operating_fixture(bus,boot,mac,phy);
   if(op){assert(!rtw88_phy_radio_initialize(&phy,scratch.data(),scratch.size(),RTW88_PHY_REGD_WORLD));assert(!rtw88_phy_radio_tune(&phy,ch,ch,20,0));assert(!rtw88_phy_radio_calibrate(&phy));}
   bus.reg[0x522]=0x15;if(mid)bus.mcu_after_pause=int(health);else{bus.put(0x80,health,2);bus.mcu_read_failure=health==0x10000U;}
   auto reads=bus.reads,writes=bus.writes,bulks=bus.bulks;if(op){(void)rtw88_phy_radio_ready(&phy);(void)rtw88_phy_radio_get_status(&phy);assert(!rtw88_phy_radio_set_tx_rate(&phy,19));assert(bus.reads==reads&&bus.writes==writes);}
   int e=liveOperation(op,phy,scratch,ch);bool good=(health&0xcfffU)==0xc078U;
   if(good){assert(!e&&boot.firmware.firmware_running&&bus.reads>reads&&bus.reg[0x522]==0x15);}
   else{int expected=health==0x10000U?EIO:ENXIO;assert(e==expected&&!rtw88_phy_radio_ready(&phy));auto s=rtw88_phy_radio_get_status(&phy);assert(s.error==expected&&!s.parameters_ready&&!s.channel_ready&&!s.calibrated_ready);assert(!boot.firmware.firmware_running&&boot.firmware.last_error==expected&&!mac.mac_ready&&!phy.tables_loaded);if(mid)assert(!bus.unpaused_after_fault&&bus.reg[0x522]==255);else assert(bus.reads==reads+1&&bus.writes==writes&&bus.bulks==bulks&&bus.reg[0x522]==0x15);}
   ++checks;
  }
 }
 printf("B/C actual PHY: %u live-MCU/clock/I-O/mid-operation cases; pure ready/rate zero-I-O\n",checks);
}

#include "RTW88DFSGoldens.inc"
static void dfs_profiles(){
 unsigned checks=0;
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})for(unsigned rfe=0;rfe<=6;++rfe){
  bool known=family==RTW_WIFI_RTL8821CU?(rfe==0||rfe==2||rfe==4||rfe==6):family==RTW_WIFI_RTL8822BU?(rfe==2||rfe==3||rfe==5):true;if(!known)continue;
  OperatingBus bus(family);rtw88_boot_context boot{};rtw88_mac_context mac{};rtw88_phy_context phy{};operating_fixture(bus,boot,mac,phy);
  boot.board.rfe_option=uint8_t(rfe);boot.board.rfe_btg=uint8_t(family==RTW_WIFI_RTL8821CU&&(rfe==2||rfe==4));
  for(auto&rom:boot.board.txpwr_idx_table)for(unsigned g=0;g<14;++g)rom[18+g]=uint8_t(24+g);
  alignas(8) std::array<uint8_t,RTW88_PHY_RADIO_SCRATCH_BYTES>scratch{};
  assert(!rtw88_phy_radio_initialize(&phy,scratch.data(),scratch.size(),RTW88_PHY_REGD_WORLD));
  for(const auto&golden:bc_dfs_goldens){if(golden.family!=family||golden.rfe!=rfe)continue;
   auto ch=golden.channel;assert(!rtw88_phy_radio_tune(&phy,ch,ch,20,0));
   assert((bus.rf[0][0x18]&255U)==ch&&!rtw88_phy_radio_ready(&phy));
   auto actual=family==RTW_WIFI_RTL8822CU?(bus.get(0x18e8)>>10)&127U:bus.reg[0x1d13];
   if(actual!=golden.index){fprintf(stderr,"B/C DFS golden family%u rfe%u ch%u MCS7 got%u expected%u\n",unsigned(family),rfe,ch,actual,golden.index);assert(false);}
   assert(!rtw88_phy_radio_calibrate(&phy)&&rtw88_phy_radio_ready(&phy));
   assert(!rtw88_phy_radio_set_power_limit_mbm(&phy,1749)&&!rtw88_phy_radio_set_power_reduction(&phy,0));assert(rtw88_phy_radio_calibrate(&phy)==ENXIO);assert(!rtw88_phy_radio_tune(&phy,ch,ch,20,0));
   unsigned drop=0;
   actual=family==RTW_WIFI_RTL8822CU?(bus.get(0x18e8)>>10)&127U:bus.reg[0x1d13];assert(actual+drop==golden.limited);
   assert(!rtw88_phy_radio_calibrate(&phy));bus.rf[0][0x42]=(44U<<10)|(44U<<1);assert(!rtw88_phy_radio_thermal_track(&phy)&&!rtw88_phy_radio_thermal_track(&phy));
   actual=family==RTW_WIFI_RTL8822CU?(bus.get(0x18e8)>>10)&127U:bus.reg[0x1d13];assert(actual+drop<=golden.ceiling);
   assert(!rtw88_phy_radio_set_power_limit_mbm(&phy,INT32_MAX)&&!rtw88_phy_radio_set_power_reduction(&phy,0));++checks;
  }
 }
 printf("B/C DFS20: %u independent primary MCS7 power/register and actual RFK cases across14 supported RFEs\n",checks);
}
static void power_reduction(){
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}){
  OperatingBus base(family),limit(family);rtw88_boot_context boots[2]{};rtw88_mac_context macs[2]{};rtw88_phy_context phys[2]{};
  alignas(8) std::array<uint8_t,RTW88_PHY_RADIO_SCRATCH_BYTES>scratch[2]{};
  for(unsigned i=0;i<2;++i){auto&b=i?limit:base;operating_fixture(b,boots[i],macs[i],phys[i]);for(auto&rom:boots[i].board.txpwr_idx_table)for(unsigned g=0;g<14;++g)rom[18+g]=60;assert(!rtw88_phy_radio_initialize(&phys[i],scratch[i].data(),scratch[i].size(),RTW88_PHY_REGD_WORLD));if(i)assert(!rtw88_phy_radio_set_power_reduction(&phys[i],1));assert(!rtw88_phy_radio_tune(&phys[i],52,52,20,0));}
  for(unsigned phase=0;phase<2;++phase){
   if(phase)for(unsigned i=0;i<2;++i){assert(!rtw88_phy_radio_calibrate(&phys[i]));assert(!rtw88_phy_radio_thermal_track(&phys[i]));assert(!rtw88_phy_radio_thermal_track(&phys[i]));}
   if(family==RTW_WIFI_RTL8822CU){assert(((base.get(0x18e8)>>10)&127U)==((limit.get(0x18e8)>>10)&127U)+4U);assert(((base.get(0x41e8)>>10)&127U)==((limit.get(0x41e8)>>10)&127U)+4U);}
   else for(unsigned path=0;path<(family==RTW_WIFI_RTL8821CU?1U:2U);++path)for(unsigned rate=4;rate<(family==RTW_WIFI_RTL8821CU?20U:28U);++rate)assert(unsigned(base.reg[0x1d00+path*0x80+rate])==unsigned(limit.reg[0x1d00+path*0x80+rate])+2U);
  }
  assert(!rtw88_phy_radio_set_power_reduction(&phys[1],0)&&!rtw88_phy_radio_tune(&phys[1],52,52,20,0));
  assert(!rtw88_phy_radio_set_power_reduction(&phys[1],255));assert(rtw88_phy_radio_tune(&phys[1],52,52,20,0)==ERANGE&&!rtw88_phy_radio_ready(&phys[1])&&!boots[1].firmware.powered);
 }
 puts("B/C power backoff:21C/22B2index and22C4index/dB,thermal persistence,reset,floor rejection");
}
static void impossible_signed_cap(){
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})for(int32_t cap:{INT32_MIN,int32_t(-10000)}){
  OperatingBus bus(family);rtw88_boot_context boot{};rtw88_mac_context mac{};rtw88_phy_context phy{};operating_fixture(bus,boot,mac,phy);
  for(auto&rom:boot.board.txpwr_idx_table){for(unsigned g=0;g<14;++g)rom[18+g]=uint8_t(family==RTW_WIFI_RTL8822CU?127:63);rom[32]=rom[33]=0x77;}
  alignas(8)std::array<uint8_t,RTW88_PHY_RADIO_SCRATCH_BYTES>scratch{};
  assert(!rtw88_phy_radio_initialize(&phy,scratch.data(),scratch.size(),RTW88_PHY_REGD_WORLD));assert(!rtw88_phy_radio_set_power_limit_mbm(&phy,cap));
  assert(rtw88_phy_radio_tune(&phy,52,52,20,0)==ERANGE);
  assert(!rtw88_phy_radio_ready(&phy)&&!boot.firmware.powered&&!rtw88_phy_radio_get_status(&phy).channel_ready);
  assert(rtw88_phy_radio_calibrate(&phy)==ENXIO&&bus.reg[0x522]==255);
 }
 puts("B/C exact signed SAR: impossible caps at maximum valid OTP base fail before TX readiness");
}
static void thermal_headroom_and_retune(){
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU})for(unsigned base:{45U,63U}){
  OperatingBus buses[]={OperatingBus(family),OperatingBus(family),OperatingBus(family)};
  rtw88_boot_context boots[3]{};rtw88_mac_context macs[3]{};rtw88_phy_context phys[3]{};
  alignas(8)std::array<uint8_t,RTW88_PHY_RADIO_SCRATCH_BYTES>scratch[3]{};
  for(unsigned mode=0;mode<3;++mode){auto&bus=buses[mode];operating_fixture(bus,boots[mode],macs[mode],phys[mode]);if(family==RTW_WIFI_RTL8822BU){bus.put(0xc1c,0x200U<<21);bus.put(0xe1c,0x180U<<21);}for(auto&rom:boots[mode].board.txpwr_idx_table)for(unsigned g=0;g<14;++g)rom[18+g]=uint8_t(base);
   assert(!rtw88_phy_radio_initialize(&phys[mode],scratch[mode].data(),scratch[mode].size(),RTW88_PHY_REGD_WORLD));
   if(mode==1)assert(!rtw88_phy_radio_set_power_reduction(&phys[mode],3));if(mode==2)assert(!rtw88_phy_radio_set_power_limit_mbm(&phys[mode],1000));
   assert(!rtw88_phy_radio_tune(&phys[mode],52,52,20,0)&&!rtw88_phy_radio_calibrate(&phys[mode])&&!rtw88_phy_radio_set_tx_rate(&phys[mode],19));
   bus.rf[0][0x42]=63U<<10;assert(!rtw88_phy_radio_thermal_track(&phys[mode])&&!rtw88_phy_radio_thermal_track(&phys[mode]));
  }
  for(unsigned mode=1;mode<3;++mode)for(unsigned path=0;path<(family==RTW_WIFI_RTL8821CU?1U:2U);++path){
   auto&control=buses[0];auto&limited=buses[mode];unsigned agc=0xc94+path*0x200,swing=0xc1c+path*0x200,mask=family==RTW_WIFI_RTL8821CU?0x7eU:0x3e000000U;
   assert((control.get(agc)&mask)==(limited.get(agc)&mask));assert((control.get(swing)&0xffe00000U)==(limited.get(swing)&0xffe00000U));
   for(unsigned rate=4;rate<(family==RTW_WIFI_RTL8821CU?20U:28U);++rate){unsigned a=control.reg[0x1d00+path*0x80+rate],b=limited.reg[0x1d00+path*0x80+rate];if(mode==1)assert(a==b+6U);else assert(a>=b);}
  }
  if(family==RTW_WIFI_RTL8822BU&&base==63)for(unsigned path=0;path<2;++path){
   // Primary RFE2 HT1S reference24: absolute1000mBm gives63+(20-24)=59.
   assert(buses[0].reg[0x1d13+path*0x80]==63&&buses[1].reg[0x1d13+path*0x80]==57&&buses[2].reg[0x1d13+path*0x80]==59);
   for(auto&bus:buses)assert(!(bus.get(0xc94+path*0x200)&0x3e000000U));
  }
  // Start with a nonzero primary thermal AGC, then replace its power plan.
  auto&bus=buses[0];auto&phy=phys[0];if(base==45)assert(bus.get(0xc94)&(family==RTW_WIFI_RTL8821CU?0x7eU:0x3e000000U));assert(!rtw88_phy_radio_set_power_limit_mbm(&phy,1000));assert(!rtw88_phy_radio_tune(&phy,52,52,20,0));
  for(unsigned path=0;path<(family==RTW_WIFI_RTL8821CU?1U:2U);++path){unsigned mask=family==RTW_WIFI_RTL8821CU?0x7eU:0x3e000000U;assert(!(bus.get(0xc94+path*0x200)&mask));if(family==RTW_WIFI_RTL8822BU)assert((bus.get(0xc1c+path*0x200)>>21)==(path?0x180U:0x200U));}
  assert(!rtw88_phy_radio_calibrate(&phy)&&rtw88_phy_radio_ready(&phy));
 }
 puts("21C/22B wire power: capped and uncapped thermal AGC/swing match; changed plan clears stale gain");
}
static void cu_retune_clears_thermal_gain(){
 OperatingBus bus(RTW_WIFI_RTL8822CU);rtw88_boot_context boot{};rtw88_mac_context mac{};rtw88_phy_context phy{};operating_fixture(bus,boot,mac,phy);
 alignas(8)std::array<uint8_t,RTW88_PHY_RADIO_SCRATCH_BYTES>scratch{};
 assert(!rtw88_phy_radio_initialize(&phy,scratch.data(),scratch.size(),RTW88_PHY_REGD_WORLD));assert(!rtw88_phy_radio_tune(&phy,52,52,20,0)&&!rtw88_phy_radio_calibrate(&phy));
 for(auto&path:bus.rf)path[0x42]=(63U<<10)|(63U<<1);
 assert(!rtw88_phy_radio_thermal_track(&phy)&&!rtw88_phy_radio_thermal_track(&phy));assert((bus.get(0x18a0)&127U)||(bus.get(0x41a0)&127U));
 assert(!rtw88_phy_radio_set_power_limit_mbm(&phy,1749)&&!rtw88_phy_radio_tune(&phy,52,52,20,0));assert(!(bus.get(0x18a0)&127U)&&!(bus.get(0x41a0)&127U));
 assert(!rtw88_phy_radio_calibrate(&phy)&&rtw88_phy_radio_ready(&phy));
 puts("22C changed power plan clears both stale signed thermal gain fields before RFK readiness");
}
static void wide_profiles(){
 unsigned checks=0;alignas(8)std::array<uint8_t,RTW88_PHY_RADIO_SCRATCH_BYTES>scratch{};
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})for(unsigned rfe=0;rfe<=6;++rfe){
  bool known=family==RTW_WIFI_RTL8821CU?(rfe==0||rfe==2||rfe==4||rfe==6):family==RTW_WIFI_RTL8822BU?(rfe==2||rfe==3||rfe==5):true;if(!known)continue;
  for(unsigned paths=1;paths<=(family==RTW_WIFI_RTL8821CU?1U:2U);++paths){
   OperatingBus bus(family);rtw88_boot_context boot{};rtw88_mac_context mac{};rtw88_phy_context phy{};operating_fixture(bus,boot,mac,phy);
   boot.board.rfe_option=(uint8_t)rfe;boot.board.rfe_btg=(uint8_t)(family==RTW_WIFI_RTL8821CU&&(rfe==2||rfe==4));
   boot.board.features_valid=1;boot.board.active_rf_paths=boot.board.tx_streams=(uint8_t)paths;
   assert(!rtw88_phy_radio_initialize(&phy,scratch.data(),scratch.size(),RTW88_PHY_REGD_WORLD));
   for(auto op:{std::array<uint8_t,4>{36,36,20,0},std::array<uint8_t,4>{36,38,40,2},std::array<uint8_t,4>{48,46,40,1},std::array<uint8_t,4>{36,42,80,4},std::array<uint8_t,4>{40,42,80,2},std::array<uint8_t,4>{44,42,80,1},std::array<uint8_t,4>{48,42,80,3},std::array<uint8_t,4>{149,155,80,4},std::array<uint8_t,4>{161,155,80,3}}){
    assert(!rtw88_phy_radio_tune(&phy,op[0],op[1],op[2],op[3])&&!rtw88_phy_radio_ready(&phy));
    for(unsigned path=0;path<paths;++path)assert((bus.rf[path][0x18]&255U)==op[1]);
    auto s=rtw88_phy_radio_get_status(&phy);assert(s.primary_channel==op[0]&&s.center_channel==op[1]&&s.width_mhz==op[2]&&s.primary_index==op[3]);
    assert(!rtw88_phy_radio_calibrate(&phy)&&rtw88_phy_radio_ready(&phy));s=rtw88_phy_radio_get_status(&phy);assert(s.iqk_complete);
    if(family==RTW_WIFI_RTL8822CU)assert(s.dac_complete&&(s.dpk_paths&((1U<<paths)-1U))==((1U<<paths)-1U));
    assert(!rtw88_phy_radio_set_tx_rate(&phy,(uint8_t)(paths==1?52:62))&&!rtw88_phy_radio_thermal_track(&phy));
    if(op[2]>20)assert(!rtw88_phy_radio_set_tx_rate(&phy,(uint8_t)(paths==1?53:63))&&!rtw88_phy_radio_thermal_track(&phy));
    if(paths==1)assert(rtw88_phy_radio_set_tx_rate(&phy,54)==EINVAL);
    ++checks;
   }
   // Existing absolute and relative policy guards cannot be bypassed by width.
   assert(!rtw88_phy_radio_set_power_limit_mbm(&phy,1700));auto writes=bus.writes;
   assert(rtw88_phy_radio_tune(&phy,36,42,80,4)==ENOTSUP&&bus.writes==writes&&!rtw88_phy_radio_ready(&phy));
   assert(!rtw88_phy_radio_set_power_limit_mbm(&phy,INT32_MAX)&&!rtw88_phy_radio_set_power_reduction(&phy,1));writes=bus.writes;
   assert(rtw88_phy_radio_tune(&phy,36,38,40,2)==ENOTSUP&&bus.writes==writes&&!rtw88_phy_radio_ready(&phy));
  }
 }
 printf("B/C actual wide PHY: %u RF/width/primary-index/RFE/active-chain/RFK/IQK-DAC-DPK/VHT-thermal cases; power-policy guards preserved\n",checks);
}
int main(){
 wide_profiles();
 impossible_signed_cap();thermal_headroom_and_retune();cu_retune_clears_thermal_gain();
 power_reduction();dfs_profiles();
 liveHealthRegression();
 assert(!rtw88_phy_radio_validate_channel(6,6,20,0));assert(!rtw88_phy_radio_validate_channel(36,36,20,0));
 assert(!rtw88_phy_radio_validate_channel(36,38,40,2));assert(!rtw88_phy_radio_validate_channel(48,42,80,3));
 assert(rtw88_phy_radio_validate_channel(6,6,80,0)==EINVAL);assert(rtw88_phy_radio_validate_channel(36,42,80,1)==EINVAL);
 assert(rtw88_phy_radio_validate_channel(6,8,40,1)==EINVAL);assert(rtw88_phy_radio_validate_channel(14,12,40,1)==EINVAL);
 assert(!rtw88_phy_radio_validate_channel(165,171,80,4));
 rtw88_phy_context context{};alignas(8) std::array<uint8_t,RTW88_PHY_RADIO_SCRATCH_BYTES>scratch{};
 assert(!rtw88_phy_radio_ready(&context));assert(rtw88_phy_radio_initialize(&context,scratch.data(),scratch.size(),RTW88_PHY_REGD_WORLD)==ENXIO);
 assert(!rtw88_phy_radio_get_status(&context).calibrated_ready);
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}){
  OperatingBus bus(family);rtw88_boot_context boot{};rtw88_mac_context mac{};rtw88_phy_context phy{};operating_fixture(bus,boot,mac,phy);
  int error=rtw88_phy_radio_initialize(&phy,scratch.data(),scratch.size(),RTW88_PHY_REGD_WORLD);
  if(error)std::fprintf(stderr,"init family%u error%d reads%u writes%u\n",unsigned(family),error,bus.reads,bus.writes);
  uint16_t packetSequence=40;assert(!rtw88_phy_radio_set_h2c_sequence(&phy,&packetSequence));assert(!error&&rtw88_phy_radio_get_status(&phy).parameters_ready&&!rtw88_phy_radio_ready(&phy));assert(rtw88_phy_radio_set_associated(&phy,1)==ENXIO);
  for(auto primary:{1,6,11,14,36,64,100,144,149,165,177}){assert(!rtw88_phy_radio_tune(&phy,uint8_t(primary),uint8_t(primary),20,0));assert((bus.rf[0][0x18]&255)==unsigned(primary));assert(!rtw88_phy_radio_ready(&phy));}
  for(auto channels:{std::array<uint8_t,4>{6,8,40,2},std::array<uint8_t,4>{36,38,40,2},std::array<uint8_t,4>{149,151,40,2},std::array<uint8_t,4>{36,42,80,4},std::array<uint8_t,4>{48,42,80,3},std::array<uint8_t,4>{165,171,80,4}}){assert(!rtw88_phy_radio_tune(&phy,channels[0],channels[1],channels[2],channels[3]));assert((bus.rf[0][0x18]&255)==channels[1]);assert(!rtw88_phy_radio_ready(&phy));}
  assert(rtw88_phy_radio_set_tx_rate(&phy,0)==EINVAL);assert(!rtw88_phy_radio_set_tx_rate(&phy,0x13));assert(rtw88_phy_radio_set_tx_rate(&phy,0xff)==EINVAL);if(family==RTW_WIFI_RTL8821CU)assert(rtw88_phy_radio_set_tx_rate(&phy,0x14)==EINVAL);
  error=rtw88_phy_radio_calibrate(&phy);if(error)std::fprintf(stderr,"cal family%u error%d reads%u writes%u bulk%u\n",unsigned(family),error,bus.reads,bus.writes,bus.bulks);
  assert(!error&&rtw88_phy_radio_ready(&phy));assert(bus.bulks==1&&rtw88_phy_radio_get_status(&phy).iqk_complete&&packetSequence==41&&bus.packets.front()[54]==40);
  if(family==RTW_WIFI_RTL8822CU)assert(rtw88_phy_radio_get_status(&phy).dac_complete&&rtw88_phy_radio_get_status(&phy).dpk_paths==3);
  if(family==RTW_WIFI_RTL8822CU){
   bus.reg[0x522]=0x15;assert(!rtw88_phy_radio_set_txrx_1ss(&phy,1));assert(rtw88_phy_radio_get_status(&phy).txrx_1ss&&rtw88_phy_radio_ready(&phy));assert((bus.get(0x4100)&0xfffff)==0x11111&&((bus.get(0x824)>>16)&15)==1&&bus.reg[0x522]==0x15);assert(rtw88_phy_radio_set_tx_rate(&phy,20)==EINVAL);
   assert(!rtw88_phy_radio_set_txrx_1ss(&phy,0));assert(!rtw88_phy_radio_get_status(&phy).txrx_1ss&&rtw88_phy_radio_ready(&phy));assert((bus.get(0x4100)&0xfffff)==0x33312&&((bus.get(0x824)>>16)&15)==3&&bus.reg[0x522]==0x15);assert(!rtw88_phy_radio_set_tx_rate(&phy,27));
   auto reads=bus.reads,writes=bus.writes;assert(rtw88_phy_radio_set_txrx_1ss(&phy,2)==EINVAL&&reads==bus.reads&&writes==bus.writes);
  }else{auto reads=bus.reads,writes=bus.writes;assert(rtw88_phy_radio_set_txrx_1ss(&phy,1)==ENOTSUP&&reads==bus.reads&&writes==bus.writes);}
  assert(!rtw88_phy_radio_set_associated(&phy,1));if(family==RTW_WIFI_RTL8821CU){assert(!rtw88_phy_radio_calibrate(&phy));assert((bus.packets.back()[56]&2)!=0);}assert(!rtw88_phy_radio_thermal_track(&phy));assert(!rtw88_phy_radio_thermal_track(&phy));assert(rtw88_phy_radio_ready(&phy));
  ++boot.power_epoch;assert(!rtw88_phy_radio_ready(&phy));assert(!rtw88_phy_radio_get_status(&phy).channel_ready);--boot.power_epoch;assert(rtw88_phy_radio_ready(&phy));
  bus.fail_read=int(bus.reads)+1;assert(rtw88_phy_radio_thermal_track(&phy)==EIO&&!rtw88_phy_radio_ready(&phy));
  rtw88_phy_radio_invalidate(&phy);assert(!rtw88_phy_radio_ready(&phy));
 }
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU}){
  OperatingBus bus(family);rtw88_boot_context boot{};rtw88_mac_context mac{};rtw88_phy_context phy{};operating_fixture(bus,boot,mac,phy);
  assert(!rtw88_phy_radio_initialize(&phy,scratch.data(),scratch.size(),RTW88_PHY_REGD_WORLD));assert(!rtw88_phy_radio_tune(&phy,6,6,20,0));
  bus.rf[0][8]=0xabcde;bus.iqk_timeout=true; // stale completion must not satisfy a new IQK
  assert(rtw88_phy_radio_calibrate(&phy)==ETIMEDOUT&&!rtw88_phy_radio_ready(&phy));assert(!boot.firmware.powered&&!boot.firmware.firmware_running);
 }
 unsigned faults=0;
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}){
  unsigned available=0;{OperatingBus base(family);rtw88_boot_context b{};rtw88_mac_context m{};rtw88_phy_context p{};operating_fixture(base,b,m,p);assert(!rtw88_phy_radio_initialize(&p,scratch.data(),scratch.size(),RTW88_PHY_REGD_WORLD));available=base.writes;}
  for(int position:{1,2,3,10,30,50}){
   if(unsigned(position)>available)continue;
   OperatingBus bus(family);rtw88_boot_context boot{};rtw88_mac_context mac{};rtw88_phy_context phy{};operating_fixture(bus,boot,mac,phy);bus.fail_write=position;
   assert(rtw88_phy_radio_initialize(&phy,scratch.data(),scratch.size(),RTW88_PHY_REGD_WORLD)==EIO&&!rtw88_phy_radio_ready(&phy));assert(!boot.firmware.powered);++faults;
  }
  {OperatingBus bus(family);rtw88_boot_context boot{};rtw88_mac_context mac{};rtw88_phy_context phy{};operating_fixture(bus,boot,mac,phy);
   assert(!rtw88_phy_radio_initialize(&phy,scratch.data(),scratch.size(),RTW88_PHY_REGD_WORLD));bus.fail_read=int(bus.reads)+1;
   assert(rtw88_phy_radio_tune(&phy,6,6,20,0)==EIO&&!rtw88_phy_radio_ready(&phy));++faults;}
 }
 for(int failure:{0,1,2}){
  OperatingBus bus(RTW_WIFI_RTL8822CU);rtw88_boot_context boot{};rtw88_mac_context mac{};rtw88_phy_context phy{};operating_fixture(bus,boot,mac,phy);
  if(failure==0)bus.dac_timeout=true;if(failure==1)bus.bad_samples=true;
  if(failure<2){int result=rtw88_phy_radio_initialize(&phy,scratch.data(),scratch.size(),RTW88_PHY_REGD_WORLD);assert(result==(failure==0?ETIMEDOUT:EIO));}
  else{assert(!rtw88_phy_radio_initialize(&phy,scratch.data(),scratch.size(),RTW88_PHY_REGD_WORLD));assert(!rtw88_phy_radio_tune(&phy,6,6,20,0));bus.rfk_timeout=true;bus.reg[0x2d9c]=0;assert(rtw88_phy_radio_calibrate(&phy)==ETIMEDOUT);}
  assert(!rtw88_phy_radio_ready(&phy)&&!boot.firmware.powered);++faults;
 }
 {OperatingBus bus(RTW_WIFI_RTL8822BU);rtw88_boot_context boot{};rtw88_mac_context mac{};rtw88_phy_context phy{};operating_fixture(bus,boot,mac,phy);std::memset(boot.board.txpwr_idx_table,0,sizeof(boot.board.txpwr_idx_table));boot.board.txpwr_idx_table[0][11]=boot.board.txpwr_idx_table[1][11]=0x80;
  assert(!rtw88_phy_radio_initialize(&phy,scratch.data(),scratch.size(),RTW88_PHY_REGD_WORLD));assert(!rtw88_phy_radio_tune(&phy,6,6,20,0));assert(bus.get(0x1d0c)==0);}
 {unsigned override_at=0;{OperatingBus bus(RTW_WIFI_RTL8822CU);rtw88_boot_context boot{};rtw88_mac_context mac{};rtw88_phy_context phy{};operating_fixture(bus,boot,mac,phy);assert(!rtw88_phy_radio_initialize(&phy,scratch.data(),scratch.size(),RTW88_PHY_REGD_WORLD));assert(!rtw88_phy_radio_tune(&phy,6,6,20,0));assert(!rtw88_phy_radio_calibrate(&phy));override_at=bus.gnt_override_write;assert(override_at);}
  OperatingBus bus(RTW_WIFI_RTL8822CU);rtw88_boot_context boot{};rtw88_mac_context mac{};rtw88_phy_context phy{};operating_fixture(bus,boot,mac,phy);assert(!rtw88_phy_radio_initialize(&phy,scratch.data(),scratch.size(),RTW88_PHY_REGD_WORLD));assert(!rtw88_phy_radio_tune(&phy,6,6,20,0));bus.fail_write=int(override_at)+1;assert(rtw88_phy_radio_calibrate(&phy)==EIO);assert(!(bus.get(0x70)&(1U<<26))&&bus.coex==0x12345678);++faults;}
 {OperatingBus bus(RTW_WIFI_RTL8822CU);rtw88_boot_context boot{};rtw88_mac_context mac{};rtw88_phy_context phy{};operating_fixture(bus,boot,mac,phy);boot.board.features_valid=1;boot.board.active_rf_paths=boot.board.tx_streams=1;assert(!rtw88_phy_radio_initialize(&phy,scratch.data(),scratch.size(),RTW88_PHY_REGD_WORLD));assert(!rtw88_phy_radio_tune(&phy,6,6,20,0));assert(!rtw88_phy_radio_calibrate(&phy)&&rtw88_phy_radio_ready(&phy));assert((rtw88_phy_radio_get_status(&phy).dpk_paths&1)!=0);assert(rtw88_phy_radio_set_tx_rate(&phy,20)==EINVAL);}
 std::printf("rtw88 operating PHY:3 chip TRX/power/20-40-80MHz/IQK-DAC-DPK/thermal paths,%u I/O/status failures and stale-IQK rejection passed\n",faults);

}
