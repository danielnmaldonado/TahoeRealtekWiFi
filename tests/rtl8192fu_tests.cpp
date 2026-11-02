/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtl8192fu_model.hpp"
#include <algorithm>
#ifndef RTW_R92FU_CHECKED_STOP
#define rtw_r92fu_stop_checked rtw_r92fu_stop
#endif
static void checked_cleanup_separates_original(){FUFixture f;f.identify();assert(!f.boot());uint32_t v=0;f.m.fail=f.m.calls+1;assert(rtw_r92fu_read(f.c,0x80,1,&v)==EIO);f.m.fail=0;assert(rtw_r92fu_stop_checked(f.c)==0);assert(f.status().error==EIO&&!f.status().powered);}
static void inputs(){
 FUFixture zero;zero.identify();zero.m.zero_coefficients=true;assert(zero.boot()==ETIMEDOUT&&!zero.status().calibrated&&!zero.status().tx_enabled);
 assert(rtw_r92fu_size()>512);rtw_r92fu*radio=nullptr;assert(rtw_r92fu_init(nullptr,0,nullptr,nullptr,nullptr,&radio)==EINVAL&&!radio);
 auto rom=fu_rom();rtw_r92fu_board b{};assert(!rtw_r92fu_decode_rom(rom.data(),512,&b));assert(b.rfe==5&&b.mac[5]==0x55&&b.pid==0xf192&&b.vid==0xbda);
 for(unsigned offset:{0U,0xb8U,0xb9U,0xbaU,0xcaU,0x107U}){auto bad=rom;bad[offset]=offset==0x107?1:255;assert(rtw_r92fu_decode_rom(bad.data(),512,&b));}
 assert(rtw_r92fu_decode_rom(rom.data(),511,&b)==EINVAL);
 rtw_r92fu_firmware fw{};assert(!rtw_r92fu_validate_firmware(RTW8192FUFirmware,sizeof(RTW8192FUFirmware),&fw));assert(fw.version==7&&fw.subversion==3&&fw.payload_bytes==31104);
 std::vector<uint8_t>bad(RTW8192FUFirmware,RTW8192FUFirmware+sizeof(RTW8192FUFirmware));for(unsigned offset:{0U,1U,2U,3U,12U}){auto other=bad;other[offset]^=0xff;assert(rtw_r92fu_validate_firmware(other.data(),other.size(),&fw)==EINVAL);}
 assert(rtw_r92fu_validate_firmware(bad.data(),bad.size()-1,nullptr)==EINVAL);
 assert(!rtw_r92fu_decode_rom(rom.data(),512,&b));rtw_r92fu_operating op{6,6,20,1};assert(rtw_r92fu_channel_supported(&b,&op));b.txpower[0][8]=63;assert(!rtw_r92fu_channel_supported(&b,&op));
}
static void success(){
 for(unsigned endpoints:{1U,2U,3U}){
 FUFixture good(endpoints);good.identify();assert(!good.boot());auto s=good.status();assert(s.firmware&&s.mac&&s.phy&&s.calibrated&&!s.tx_enabled&&s.tx_iqk_mask==3&&s.rx_iqk_mask==3);assert(s.primary==1&&(good.m.rf[0][0x18]&255)==1&&(good.m.rf[1][0x18]&255)==1&&good.m.reg[0x522]==255);
 assert(!memcmp(good.m.reg.data()+0x610,fu_rom().data()+0x107,6));
 assert(good.m.firmware_max_chunk==254);
 assert(good.m.firmware.size()==sizeof(RTW8192FUFirmware)-32&&!memcmp(good.m.firmware.data(),RTW8192FUFirmware+32,good.m.firmware.size()));
 for(auto op:{rtw_r92fu_operating{6,6,20,0},rtw_r92fu_operating{6,6,20,1},rtw_r92fu_operating{6,8,40,1},rtw_r92fu_operating{6,4,40,1}}){assert(!rtw_r92fu_set_operating(good.c,&op));assert((good.m.rf[0][0x18]&255)==op.center&&(good.m.rf[1][0x18]&255)==op.center);assert((good.m.rf[0][0x18]&0xc00)==(op.width_mhz==40?0x400U:0xc00U));assert(!good.status().tx_enabled);assert(!rtw_r92fu_resume(good.c)&&good.status().tx_enabled);}
 uint8_t bssid[6]={2,1,2,3,4,5};assert(!rtw_r92fu_station(good.c,1,bssid,1,100,0x15,1,1));assert(good.m.read(0x6a2,2)==0x100&&good.m.read(0x6a4,2)==0xffff&&(good.m.read(0x440)&0xfffff)==0x15);assert(!rtw_r92fu_resume(good.c));
 assert(!rtw_r92fu_tick(good.c));good.m.rf[0][0x42]=32U<<10;assert(!rtw_r92fu_tick(good.c)&&good.status().thermal==32&&good.status().thermal_samples==1);
 assert(!rtw_r92fu_station(good.c,0,nullptr,0,0,0x15,1,0));assert(!good.m.read(0x6a2,2)&&!good.m.read(0x6a4,2));
 unsigned pipe=9;assert(!rtw_r92fu_tx_endpoint(good.c,0,&pipe)&&pipe<endpoints);assert(!rtw_r92fu_tx_endpoint(good.c,18,&pipe)&&pipe==0);assert(rtw_r92fu_tx_endpoint(good.c,31,&pipe)==EINVAL);
 assert(!rtw_r92fu_stop(good.c));assert(!good.status().firmware&&!good.status().tx_enabled&&good.m.reg[0x522]==255);
 }
}
static void rfe_and_trim(){
 {FUFixture f;auto rom=fu_rom();rom[0xb9]=0;f.m.packed(rom);f.m.set(0x28,0x7e);f.m.set(0x24,0x7e000000);f.identify();assert(!f.boot());assert((f.m.read(0x28)&0x7e)==0&&(f.m.read(0x24)&0x7e000000)==0);}
 for(uint8_t rfe:{1,5,7,8,9,12}){FUFixture f;auto rom=fu_rom();rom[0xca]=rfe;f.m.packed(rom);f.m.otp[0x1ea]=0x21;f.m.otp[0x1ec]=0x43;f.m.otp[0x1ee]=0x65;f.identify();assert(!f.boot());rtw_r92fu_operating op{6,8,40,1};assert(!rtw_r92fu_set_operating(f.c,&op));assert((f.m.rf[0][0x55]&(1U<<5))&&(f.m.rf[1][0x55]&(1U<<5)));assert(!(f.m.rf[0][0xdf]&(1U<<7))&&!(f.m.rf[0][0xef]&(1U<<7))&&f.status().calibrated);}
}
static void failures(){
 unsigned total=0;
 unsigned identify=0,boot=0,tune=0;{FUFixture f;f.identify();identify=f.m.calls;auto before=f.m.calls;assert(!f.boot());boot=f.m.calls-before;before=f.m.calls;rtw_r92fu_operating op{6,8,40,1};assert(!rtw_r92fu_set_operating(f.c,&op));tune=f.m.calls-before;}
 for(bool ambiguous:{false,true}){
  for(unsigned fault=1;fault<=identify;++fault){FUFixture f;f.m.fail=fault;f.m.ambiguous=ambiguous;assert(rtw_r92fu_identify(f.c)==EIO);assert(!f.status().identified&&!f.status().tx_enabled&&f.m.reg[0xcf]==0);++total;}
  for(unsigned fault=1;fault<=boot;++fault){FUFixture f;f.identify();f.m.fail=f.m.calls+fault;f.m.ambiguous=ambiguous;int e=f.boot();if(e!=EIO){printf("boot fault %u/%u ambiguous=%d e=%d\n",fault,boot,ambiguous,e);abort();}assert(!f.status().firmware&&!f.status().calibrated&&!f.status().tx_enabled&&f.m.reg[0x522]==255);++total;}
  for(unsigned fault=1;fault<=tune;++fault){FUFixture f;f.identify();assert(!f.boot());f.m.fail=f.m.calls+fault;f.m.ambiguous=ambiguous;rtw_r92fu_operating op{6,8,40,1};assert(rtw_r92fu_set_operating(f.c,&op)==EIO);assert(!f.status().calibrated&&!f.status().tx_enabled&&f.m.reg[0x522]==255);++total;}
 }
 for(unsigned mode=0;mode<6;++mode){FUFixture f;f.identify();if(mode==0)f.m.iqk_ready=false;if(mode==1)f.m.iqk_fail=true;if(mode==2)f.m.lck_ready=false;if(mode==3)f.m.firmware_ready=false;if(mode==4)f.m.llt_ready=false;if(mode==5)f.m.checksum_ready=false;assert(f.boot());assert(!f.status().firmware&&!f.status().calibrated&&!f.status().tx_enabled&&f.m.reg[0x522]==255);}
 printf("FU %u individual before/after-effect I/O faults (%u identify,%u boot,%u HT40/RFK), measured IQK/LCK/FW/LLT failures passed\n",total,identify,boot,tune);
}
static void rates(){
 {rtw_r92fu_tx_options options{};options.payload_bytes=64;options.rf_chains=2;options.multicast=options.data=1;options.bandwidth_mhz=20;uint8_t d[40];assert(rtw_r92fu_tx_descriptor(&options,d));assert((d[4]&0x7f)==0);}

 for(unsigned mask=1;mask<4096;++mask)for(bool ht:{false,true})for(unsigned width:{20U,40U}){if(!ht&&width==40)continue;uint8_t mcs[2]={255,255};rtw_r92fu_rate rate{};assert(rtw_r92fu_rate_build(uint16_t(mask),mcs,ht,width,1,1,&rate));assert(rate.mask==(ht?(mask|0xffff000U):mask));uint8_t h2c[8];assert(rtw_r92fu_rate_command(&rate,1,h2c)&&h2c[0]==0x40&&h2c[3]==(width==40?1:0)&&h2c[2]==(rate.rate_id|(rate.sgi?0x80:0)));rtw_r92fu_tx_options o{};o.payload_bytes=100;o.data=o.ack_expected=o.firmware_rate=1;o.rate=rate.initial;o.rf_chains=2;o.bandwidth_mhz=uint8_t(width);o.short_gi=rate.sgi;o.retry_limit=7;uint8_t descriptor[40];assert(rtw_r92fu_make_tx(&o,&rate,descriptor)&&(descriptor[6]&31)==rate.rate_id);o.band_5ghz=1;assert(!rtw_r92fu_make_tx(&o,&rate,descriptor));}
 FUFixture f;f.identify();assert(!f.boot());uint8_t sample[32]={};sample[1]=62;sample[13]=(7U<<5)|2;int dbm=0;assert(rtw_r92fu_decode_phy(f.c,0,sample,32,&dbm)&&dbm==-48);sample[0]=1;sample[1]=50;sample[2]=60;assert(rtw_r92fu_decode_phy(f.c,27,sample,32,&dbm)&&dbm==-50);sample[0]=255;assert(!rtw_r92fu_decode_phy(f.c,4,sample,32,&dbm));
}
static void failed_cleanup_stays_failed(){FUFixture f;f.identify();assert(!f.boot());f.m.fail=f.m.calls+1;assert(rtw_r92fu_stop_checked(f.c)==EIO);f.m.fail=0;assert(rtw_r92fu_stop_checked(f.c)==EIO);}
static void shutdown_polls_are_checked(){for(unsigned mode=0;mode<3;++mode){FUFixture f;f.identify();assert(!f.boot());if(mode==0)f.m.power_ready=false;if(mode==1)f.m.fifo_idle=false;if(mode==2)f.m.set(0x5f8,1);assert(rtw_r92fu_stop_checked(f.c)==ETIMEDOUT);assert(!f.status().powered&&!f.status().firmware);}}
static void complete_rom_is_identity(){auto first=fu_rom(),changed=first;changed[0xf8]=0x31;rtw_r92fu_board a{},b{};assert(!rtw_r92fu_decode_rom(first.data(),first.size(),&a));assert(!rtw_r92fu_decode_rom(changed.data(),changed.size(),&b));assert(memcmp(&a,&b,sizeof(a))!=0);}
int main(){complete_rom_is_identity();shutdown_polls_are_checked();checked_cleanup_separates_original();failed_cleanup_stays_failed();inputs();success();rfe_and_trim();rates();failures();puts("Dedicated FU actual OTP/0x4000-FW/MAC/PHY/measured RFK/HT20-40/runtime and strict wire passed");}
