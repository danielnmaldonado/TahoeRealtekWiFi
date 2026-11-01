/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/RTL8723BU.h"
#include <array>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include "../kext/RTL8723BUFirmware.inc"
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL%u: %s\n",__LINE__,#x);std::exit(1);}}while(0)
struct fake_8723 {
 unsigned calls=0,fail_at=0;std::array<uint8_t,65536> memory{};std::array<uint32_t,256> rf{};std::array<uint8_t,512> otp{};struct write_event{uint16_t address;unsigned width;uint32_t value;};std::vector<write_event> writes;std::vector<std::array<uint8_t,8>> h2c;uint64_t delayed=0;unsigned rf_address=0,fw_bytes=0;bool simulate=false,power_ready=true,fw_ready=true,iqk_success=true,mailbox_busy=false;
 static int read(void*p,uint16_t a,unsigned w,uint32_t*v){auto&f=*(fake_8723*)p;++f.calls;if(f.calls==f.fail_at)return EIO;*v=0;for(unsigned i=0;i<w;i++)*v|=uint32_t(f.memory[a+i])<<(8*i);
  if(f.simulate){
   if(a==4&&w==4&&f.power_ready)*v=(*v|(1U<<17))&~0x300U;
   if(a==5&&w==1)*v&=~3U;
   if(a==0x224&&w==4)*v&=~(1U<<16);
   if(a==0x80&&f.fw_ready&&f.fw_bytes)*v|=4U|64U|128U;
   if(a==0x1cc&&w==1&&!f.mailbox_busy)*v=0;
   if(a==0x1cc&&w==1&&f.mailbox_busy)*v=15;
   if(a==0x8a0||a==0x8b8){*v=f.rf[f.rf_address];if(f.rf_address==0x18)*v&=~0x8000U;}
   if(a==0xe94||a==0xea4)*v=0x1000000;
   if(a==0xeac)*v=f.iqk_success?0:(1U<<28)|(1U<<27);
   if(a==0x30&&w==4){unsigned offset=(*v>>8)&1023;*v=(*v&~255U)|(1U<<31)|(offset<512?f.otp[offset]:255U);}
  }return 0;}
 static int write(void*p,uint16_t a,unsigned w,uint32_t v){auto&f=*(fake_8723*)p;++f.calls;if(f.calls==f.fail_at)return EIO;f.writes.push_back({a,w,v});uint32_t old=0;for(unsigned i=0;i<w;i++)old|=uint32_t(f.memory[a+i])<<(8*i);for(unsigned i=0;i<w;i++)f.memory[a+i]=uint8_t(v>>(8*i));if(a==0x840&&w==4)f.rf[(v>>20)&255]=v&0xfffffU;if(a==0x824&&w==4&&(v&0x80000000U)&&!(old&0x80000000U))f.rf_address=(v>>23)&255;if(a>=0x1d0&&a<=0x1dc&&w==4&&!(a&3U)){std::array<uint8_t,8> cmd{};for(unsigned i=0;i<4;i++){cmd[i]=uint8_t(v>>(8*i));cmd[i+4]=f.memory[0x1f0+a-0x1d0+i];}f.h2c.push_back(cmd);}return 0;}
 static int region(void*p,uint16_t,const uint8_t*,size_t n){auto&f=*(fake_8723*)p;++f.calls;if(f.calls==f.fail_at)return EIO;f.fw_bytes+=unsigned(n);return 0;}
 static void delay(void*p,uint32_t u){((fake_8723*)p)->delayed+=u;}
 rtw_8723bu_bus bus(){return{this,read,write,region,delay};}
};
static void valid_board_image(std::array<uint8_t,512>&rom){rom.fill(255);rom[0]=0x29;rom[1]=0x81;for(unsigned i=0;i<6;i++){rom[0x107+i]=uint8_t(i?0x10+i:2);rom[0x3a+i]=uint8_t(20+i);}for(unsigned i=0;i<5;i++)rom[0x40+i]=uint8_t(24+i);rom[0x45]=0x2f;rom[0xb8]=0x20;rom[0xb9]=23;rom[0xba]=31;rom[0xc1]=0x21;rom[0xc3]=1;rom[0xc9]=0;rom[0xca]=0;}
static void encode_otp(const std::array<uint8_t,512>&rom,std::array<uint8_t,512>&out){out.fill(255);unsigned at=0;for(unsigned section=0;section<64;section++){unsigned mask=15;for(unsigned word=0;word<4;word++)if(rom[section*8+word*2]!=255||rom[section*8+word*2+1]!=255)mask&=~(1U<<word);if(mask==15)continue;if(section<16)out[at++]=uint8_t((section<<4)|mask);else{out[at++]=uint8_t(((section&7)<<5)|15);out[at++]=uint8_t(((section&0x78)<<1)|mask);}for(unsigned word=0;word<4;word++)if(!(mask&(1U<<word))){out[at++]=rom[section*8+word*2];out[at++]=rom[section*8+word*2+1];}CHECK(at<512);}}
static void read_factory_from_otp(){
 fake_8723 f;f.simulate=true;rtw_8723bu c{};auto bus=f.bus();rtw_8723bu_usb usb{0x81,{2,3,0},2,1,512};std::vector<uint64_t> storage((rtw_8723bu_private_size()+7)/8);
 CHECK(rtw_8723bu_init(&c,0xbda,0xb720,&bus,&usb,storage.data(),storage.size()*8)==0);fake_8723::write(&f,0xf0,4,2U<<12);fake_8723::write(&f,0x68,4,4U|(1U<<18));CHECK(rtw_8723bu_identify(&c)==0);
 std::array<uint8_t,512> rom;valid_board_image(rom);encode_otp(rom,f.otp);fake_8723::write(&f,0x34,4,0x12340201);
 CHECK(rtw_8723bu_read_board(&c)==0&&c.board_valid);CHECK(!std::memcmp(c.board.logical,rom.data(),512));CHECK(f.memory[0xcf]==0&&f.memory[0x35]==2);
}
static void boot_hardware_boundary(){
 fake_8723 f;f.simulate=true;rtw_8723bu c{};auto bus=f.bus();rtw_8723bu_usb usb{0x81,{2,3,0},2,1,512};std::vector<uint64_t> storage((rtw_8723bu_private_size()+7)/8);
 CHECK(rtw_8723bu_init(&c,0xbda,0xb720,&bus,&usb,storage.data(),storage.size()*8)==0);
 fake_8723::write(&f,0xf0,4,2U<<12);fake_8723::write(&f,0x68,4,4U|(1U<<18));CHECK(rtw_8723bu_identify(&c)==0);
 std::array<uint8_t,512> rom;valid_board_image(rom);CHECK(rtw_8723bu_parse_board(&c,rom.data(),rom.size())==0);
 CHECK(rtw_8723bu_boot(&c,RTW8723BUFirmware,sizeof(RTW8723BUFirmware))==0);
 CHECK(c.powered&&c.firmware_running&&c.mac_initialized&&c.phy_loaded&&!c.calibrated&&c.channel==0);
 CHECK(c.firmware_version==35&&f.fw_bytes==sizeof(RTW8723BUFirmware)-32&&f.memory[0x522]==255);
 // Primary one-antenna ownership/PTA/TDMA initialization really reaches HW/FW.
 CHECK(f.memory[0x778]==1&&f.memory[0x76e]==0x0c&&f.memory[0xcf]==0);
 CHECK(rtw_8723bu_set_channel(&c,6,20)==0&&!c.calibrated&&c.channel==6&&f.memory[0x522]==255);
 CHECK(rtw_8723bu_calibrate(&c)==0&&c.calibrated&&f.memory[0x522]==255);
 CHECK((f.rf[0x18]&0x3ffU)==6);CHECK(rtw_8723bu_set_channel(&c,6,40)==EINVAL&&c.calibrated);
 CHECK(rtw_8723bu_set_channel(&c,11,20)==0&&!c.calibrated);f.iqk_success=false;
 CHECK(rtw_8723bu_calibrate(&c)==ETIMEDOUT&&!c.calibrated&&f.memory[0x522]==255);f.iqk_success=true;
 CHECK(rtw_8723bu_calibrate(&c)==0&&c.calibrated);
 rtw_8723bu_rates rates{};CHECK(rtw_8723bu_prepare_rates(0xfff,255,1,1,&rates)==0);CHECK(rates.mask==0xfffff&&rates.raid==3&&rates.sgi&&rates.command[0]==0x40&&rates.command[1]==2&&rates.command[2]==0x83);
 for(unsigned legacy:{0xfffU,0xff0U})for(unsigned sgi:{0U,1U})for(unsigned mcs=1;mcs<256;++mcs){CHECK(rtw_8723bu_prepare_rates(uint16_t(legacy),uint8_t(mcs),1,int(sgi),&rates)==0);CHECK(rates.mask==(legacy|(mcs<<12))&&rates.sgi==sgi);CHECK(((unsigned(rates.command[5])|unsigned(rates.command[6])<<8)>>4)==mcs);}
 CHECK(rtw_8723bu_prepare_rates(0xfff,0,1,0,&rates)==EINVAL);
 rtw_8723bu_station station{{2,3,4,5,6,7},11,1,1,1,1,255,1,100,0x15f,0xfff,0};
 CHECK(rtw_8723bu_station_state(&c,2,&station)==0&&!c.associated);
 CHECK(rtw_8723bu_station_state(&c,3,&station)==0&&c.associated);
 CHECK(f.memory[0x6a2]==0&&f.memory[0x6a3]==1&&!(f.memory[0x609]&0x10U)); // BAR subtype8; PS-POLL subtype10 must stay blocked.
 CHECK(!f.h2c.empty()&&f.h2c.back()[0]==1&&f.h2c.back()[1]==0x21&&f.h2c.back()[2]==2);
 const uint8_t bt_combo[]={9,0,1,0x69,0,40,0,0,0};CHECK(rtw_8723bu_c2h(&c,bt_combo,sizeof(bt_combo))==0);
 CHECK(f.memory[0x6c0]==0x55&&f.memory[0x6c4]==0x5a); // Linux coexistence type3,HID+A2DP.
 rtw_8723bu_edca ac[4]{};for(auto&v:ac)v={2,3,4,0};ac[0].txop_32us=0xffff;CHECK(rtw_8723bu_set_edca(&c,1,ac)==0);CHECK(f.memory[0x508]==0x1c&&f.memory[0x509]==0x43&&f.memory[0x50a]==0xff&&f.memory[0x50b]==0xff);
 ac[0].txop_32us=0;CHECK(rtw_8723bu_set_edca(&c,1,ac)==0);CHECK(f.memory[0x50a]==0&&f.memory[0x50b]==0); // Actual8723 full TXOP field is replaced, not inherited.
 unsigned station_before=f.calls;ac[0].aifsn=0;CHECK(rtw_8723bu_set_edca(&c,1,ac)==EINVAL&&f.calls==station_before);
 uint8_t rate_report[]={0x0c,0,0x93,2,0,0,0,0,0};CHECK(rtw_8723bu_c2h(&c,rate_report,sizeof(rate_report))==0&&c.tx_rate==19&&c.tx_sgi);rate_report[8]=1;CHECK(rtw_8723bu_c2h(&c,rate_report,sizeof(rate_report))==EINVAL);
 uint8_t phy[28]{};int dbm=0;phy[5]=uint8_t((6U<<5)|3U);CHECK(rtw_8723bu_rssi(0,phy,sizeof(phy),&dbm)==0&&dbm==-40);phy[4]=100;CHECK(rtw_8723bu_rssi(19,phy,sizeof(phy),&dbm)==0&&dbm==-60);phy[5]=0;CHECK(rtw_8723bu_rssi(0,phy,sizeof(phy),&dbm)==0&&dbm==0);
 CHECK(rtw_8723bu_station_state(&c,0,nullptr)==0&&!c.associated&&f.memory[0x6a2]==0&&f.memory[0x6a3]==0);
 uint8_t endpoint=0;CHECK(rtw_8723bu_tx_endpoint(&c,0,&endpoint)==0&&endpoint==3);CHECK(rtw_8723bu_tx_endpoint(&c,7,&endpoint)==0&&endpoint==2);CHECK(rtw_8723bu_tx_endpoint(&c,0x12,&endpoint)==0&&endpoint==2);
 unsigned before=f.calls;const uint8_t command[]={0x42,0,0,50,0,0,0,0};CHECK(rtw_8723bu_h2c(&c,command,sizeof(command))==0);CHECK(f.calls==before+3);
 f.mailbox_busy=true;before=f.calls;CHECK(rtw_8723bu_h2c(&c,command,2)==EBUSY);CHECK(f.calls==before+100);CHECK(!c.calibrated);
 CHECK(rtw_8723bu_stop(&c)==EBUSY&&!c.powered&&!c.firmware_running&&!c.calibrated);
}
static void identity_factory(){
 fake_8723 f;rtw_8723bu c{};auto bus=f.bus();rtw_8723bu_usb usb{0x81,{2,3,0},2,1,512};std::vector<uint64_t> storage((rtw_8723bu_private_size()+7)/8);
 CHECK(rtw_8723bu_init(&c,0xbda,0xb720,&bus,&usb,storage.data(),storage.size()*8)==0);
 fake_8723::write(&f,0xf0,4,(2U<<12)|(1U<<18));fake_8723::write(&f,0x68,4,4U|(1U<<18));fake_8723::write(&f,0xf4,4,3U<<28);
 unsigned before=f.calls;CHECK(rtw_8723bu_identify(&c)==0);CHECK(c.identified&&c.board.cut==2&&c.board.has_wifi&&c.board.has_bluetooth&&c.board.rom_revision==3);CHECK(f.calls==before+3);
 std::array<uint8_t,512> rom;rom.fill(255);rom[0]=0x29;rom[1]=0x81;
 for(unsigned i=0;i<6;i++){rom[0x107+i]=uint8_t(i?0x10+i:2);rom[0x3a+i]=uint8_t(20+i);}
 for(unsigned i=0;i<5;i++)rom[0x40+i]=uint8_t(24+i);rom[0x45]=0x2f;rom[0xb8]=0x20;rom[0xb9]=23;rom[0xba]=31;rom[0xc1]=0x21;rom[0xc3]=1;rom[0xc9]=0;rom[0xca]=0;
 before=f.calls;CHECK(rtw_8723bu_parse_board(&c,rom.data(),rom.size())==0);CHECK(f.calls==before&&c.board_valid&&c.board.crystal_cap==23);CHECK(c.board.ofdm_delta==-1&&c.board.ht20_delta==2&&c.board.cck_power[5]==25&&c.board.ht40_power[4]==28);
 for(unsigned ch=1;ch<=11;ch++)CHECK(rtw_8723bu_channel_supported(&c,uint8_t(ch),20));CHECK(!rtw_8723bu_channel_supported(&c,36,20));CHECK(!rtw_8723bu_channel_supported(&c,1,40));CHECK(f.calls==before);
 rom[0x107]|=1;CHECK(rtw_8723bu_parse_board(&c,rom.data(),rom.size())==EINVAL&&!c.board_valid);rom[0x107]&=~1U;rom[0]=0;CHECK(rtw_8723bu_parse_board(&c,rom.data(),rom.size())==EINVAL);CHECK(f.calls==before);
 std::array<uint8_t,512> physical,logical;physical.fill(255);physical[0]=0x0e;physical[1]=0x29;physical[2]=0x81;physical[3]=0x2f;physical[4]=0x2e;physical[5]=0x11;physical[6]=0x12;
 rom[0]=0x29;CHECK(rtw_8723bu_parse_board(&c,rom.data(),rom.size())==0);
 std::array<uint8_t,40> rejected_fw{};CHECK(rtw_8723bu_boot(&c,rejected_fw.data(),rejected_fw.size())==EINVAL);CHECK(f.calls==before&&!c.powered&&!c.firmware_running&&!c.calibrated);
 CHECK(rtw_8723bu_decode_efuse(physical.data(),physical.size(),logical.data(),logical.size())==0);CHECK(logical[0]==0x29&&logical[1]==0x81&&logical[0x88]==0x11&&logical[0x89]==0x12);
 CHECK(rtw_8723bu_decode_efuse(physical.data(),physical.size(),physical.data(),physical.size())==EINVAL);CHECK(f.calls==before);
 std::array<uint8_t,40> firmware{};firmware[0]=1;firmware[1]=0x53;CHECK(rtw_8723bu_inspect_firmware(firmware.data(),firmware.size())==0);firmware[1]=0x92;CHECK(rtw_8723bu_inspect_firmware(firmware.data(),firmware.size())==EINVAL);CHECK(f.calls==before);
}
struct core_fixture {
 fake_8723 f;rtw_8723bu c{};std::vector<uint64_t>storage;std::array<uint8_t,512>rom;
 core_fixture(unsigned pipes=2){f.simulate=true;auto bus=f.bus();rtw_8723bu_usb usb{0x81,{2,3,4},uint8_t(pipes),1,512};storage.resize((rtw_8723bu_private_size()+7)/8);CHECK(rtw_8723bu_init(&c,0xbda,0xb720,&bus,&usb,storage.data(),storage.size()*8)==0);fake_8723::write(&f,0xf0,4,2U<<12);fake_8723::write(&f,0x68,4,4U|(1U<<18));CHECK(rtw_8723bu_identify(&c)==0);valid_board_image(rom);encode_otp(rom,f.otp);CHECK(rtw_8723bu_parse_board(&c,rom.data(),rom.size())==0);}
 int boot(){return rtw_8723bu_boot(&c,RTW8723BUFirmware,sizeof(RTW8723BUFirmware));}
};
static void actual_boot_and_otp_failures(){
 unsigned boot_io=0,otp_io=0;
 {core_fixture x(3);CHECK(x.boot()==0);CHECK(rtw_8723bu_set_channel(&x.c,6,20)==0&&rtw_8723bu_calibrate(&x.c)==0);uint8_t ep=0;CHECK(rtw_8723bu_tx_endpoint(&x.c,7,&ep)==0&&ep==2);CHECK(rtw_8723bu_tx_endpoint(&x.c,5,&ep)==0&&ep==3);CHECK(rtw_8723bu_tx_endpoint(&x.c,0,&ep)==0&&ep==4);}
 {core_fixture x;auto before=x.f.calls;CHECK(x.boot()==0);boot_io=x.f.calls-before;}
 for(unsigned cut=1;cut<=boot_io;cut++){core_fixture x;x.f.fail_at=x.f.calls+cut;CHECK(x.boot()==EIO);CHECK(!x.c.calibrated&&x.c.last_error==EIO);
  // Download failure must clear the enable bit even after a scalar/bulk
  // fault. A healthy callback after the single failed transfer permits the
  // port's bounded cleanup; no second firmware write may be attempted.
  if(x.f.memory[0x80]&1U)std::fprintf(stderr,"cold-boot failure%u leaves firmware-download enabled\n",cut);CHECK(!(x.f.memory[0x80]&1U));CHECK(rtw_8723bu_stop(&x.c)==EIO&&!x.c.powered&&!x.c.calibrated);
 }
 {core_fixture x;auto before=x.f.calls;CHECK(rtw_8723bu_read_board(&x.c)==0);otp_io=x.f.calls-before;}
 for(unsigned cut=1;cut<=otp_io;cut++){core_fixture x;x.f.fail_at=x.f.calls+cut;CHECK(rtw_8723bu_read_board(&x.c)==EIO&&!x.c.board_valid&&!x.c.calibrated);}
 {core_fixture x;CHECK(x.boot()==0);x.rom[0x3a+2]=255;CHECK(rtw_8723bu_parse_board(&x.c,x.rom.data(),512)==0);auto before=x.f.calls;CHECK(!rtw_8723bu_channel_supported(&x.c,6,20));CHECK(rtw_8723bu_set_channel(&x.c,6,20)==ENOTSUP&&x.f.calls==before);}
 {core_fixture x;CHECK(x.boot()==0);for(int value:{0,63}){x.rom[0x40]=uint8_t(value);x.rom[0x45]=value?0x77:0x88;CHECK(rtw_8723bu_parse_board(&x.c,x.rom.data(),512)==0);CHECK(rtw_8723bu_set_channel(&x.c,1,20)==0);CHECK(x.f.memory[0xe00]==value&&x.f.memory[0xe10]==value);}}
 {core_fixture x;x.f.fw_ready=false;CHECK(x.boot()==EAGAIN&&!x.c.firmware_running&&!x.c.calibrated);CHECK(x.f.delayed<2000000&&x.f.calls<20000&&!(x.f.memory[0x80]&1U));}
 {core_fixture x;x.f.power_ready=false;CHECK(x.boot()==EBUSY&&!x.c.powered&&!x.c.calibrated);CHECK(x.f.delayed<2000000&&x.f.calls<20000);}
 {core_fixture x;CHECK(x.boot()==0&&rtw_8723bu_set_channel(&x.c,1,20)==0&&rtw_8723bu_calibrate(&x.c)==0);rtw_8723bu_station p{};std::memcpy(p.bssid,x.c.board.mac,6);p.bssid[5]=0x42;p.channel=1;p.aid=1;p.beacon_tu=100;p.basic_rates=0x150;p.legacy_rates=0xfff;CHECK(rtw_8723bu_station_state(&x.c,3,&p)==0);CHECK(rtw_8723bu_station_state(&x.c,1,nullptr)==0);CHECK(x.f.memory[0x440]==0xf1&&x.f.memory[0x441]==0xff&&(x.f.memory[0x442]&15U)==15U);}
 std::printf("8723BU: all %u cold-boot and %u OTP I/O failure positions passed\n",boot_io,otp_io);
}
int main(){
 actual_boot_and_otp_failures();
 read_factory_from_otp();
 boot_hardware_boundary();
 identity_factory();
 fake_8723 f;rtw_8723bu c{};auto bus=f.bus();rtw_8723bu_usb usb{0x81,{2,3,0},2,1,512};
 std::vector<uint64_t> storage((rtw_8723bu_private_size()+7)/8);
 CHECK(rtw_8723bu_init(&c,0x0bda,0xb720,&bus,&usb,storage.data(),storage.size()*8)==0);
 CHECK(f.calls==0&&!c.identified&&!c.calibrated&&c.vid==0xbda&&c.pid==0xb720);
 CHECK(rtw_8723bu_init(&c,0x7392,0xa611,&bus,&usb,storage.data(),storage.size()*8)==0);CHECK(f.calls==0);
 CHECK(rtw_8723bu_init(&c,0x20f4,0x108a,&bus,&usb,storage.data(),storage.size()*8)==ENODEV);CHECK(f.calls==0&&c.private_state==nullptr);
 CHECK(rtw_8723bu_init(&c,0x2357,0x0108,&bus,&usb,storage.data(),storage.size()*8)==ENODEV);CHECK(f.calls==0);
 std::printf("8723BU: %u typed core checks passed\n",checks);
}
