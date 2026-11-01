/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/RTW8814BHAL.h"
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cstdio>
#include <vector>
struct Bus {uint8_t reg[65536]{},otp[1024]{};unsigned calls=0,writes=0,packets=0,allocs=0,fail=0,live=0;bool ack=true;unsigned fw_packets=0,h2c_packets=0,allocation_failure=0;Bus*foreign=nullptr;rtwb14_context*context=nullptr;bool mutate_read=false,mutate_allocate=false;};
static int rd(void*p,uint16_t a,unsigned w,uint32_t*v){auto&b=*(Bus*)p;++b.calls;if(b.fail==b.calls)return -EIO;assert(w&&a<=65536U-w);*v=0;for(unsigned i=0;i<w;i++)*v|=uint32_t(b.reg[a+i])<<(i*8);if(b.mutate_read){b.mutate_read=false;b.context->bus.cookie=b.foreign;}return 0;}
static int wr(void*p,uint16_t a,unsigned w,uint32_t v){auto&b=*(Bus*)p;++b.calls;if(b.fail==b.calls)return -EIO;++b.writes;assert(w&&a<=65536U-w);for(unsigned i=0;i<w;i++)b.reg[a+i]=uint8_t(v>>(i*8));if(b.ack){if(a==5)b.reg[5]&=~3U;if(a==0x208)b.reg[0x208]&=~1U;if(a==0x1208&&w==4)b.reg[0x120b]&=~0x80U;if(a==3&&(v&4U)&&(b.reg[0x80]&0x50U)==0x50U){b.reg[0x80]=0x78;b.reg[0x81]=0xc0;}if(a==0x30&&w==4){unsigned o=(v>>8)&1023U;b.reg[0x30]=b.otp[o];b.reg[0x33]|=0x80;}}return 0;}
static int send(void*p,unsigned kind,const uint8_t*b,size_t n){auto&s=*(Bus*)p;++s.calls;if(s.fail==s.calls)return -EIO;assert(b&&n&&n<=31744);++s.packets;if(kind)++s.h2c_packets;if(n==497)assert(b[n-1]==0);uint8_t desc[48];assert(!rtwb14_control_descriptor(kind,n,desc,48));std::memcpy(s.reg+0x8000,desc,48);s.reg[0x201]|=0x80U;++s.fw_packets;return 0;}
static void delay(void*,uint32_t){}
static void* allocate(void*p,size_t n){auto&b=*(Bus*)p;++b.allocs;if(b.allocation_failure==b.allocs)return nullptr;void*x=std::malloc(n);if(x){++b.live;std::memset(x,0xa5,n);}if(b.mutate_allocate){b.mutate_allocate=false;b.context->bus.cookie=b.foreign;}return x;}
static void release(void*p,void*b,size_t){assert(((Bus*)p)->live);--((Bus*)p)->live;std::free(b);}
static rtwb14_bus ops(Bus&b){return {&b,rd,wr,send,delay,allocate,release};}
static void ready(Bus&b){b.reg[0xfc]=0x11;b.reg[0xf1]=0x10;b.reg[0x100]=0xea;b.reg[0xff]=0x20;for(unsigned i=0;i<1024;i++)b.otp[i]=uint8_t(i);}
static void le32(uint8_t*b,uint32_t x){for(unsigned i=0;i<4;i++)b[i]=uint8_t(x>>(8*i));}
static std::vector<uint8_t> firmware(unsigned abi=11){std::vector<uint8_t>b(64+496+520+72,0x5a);std::memset(b.data(),0,64);b[0]=0x14;b[1]=0x88;b[24]=0x18;le32(b.data()+28,abi);le32(b.data()+32,0x80200000);le32(b.data()+36,488);le32(b.data()+48,512);le32(b.data()+52,64);le32(b.data()+56,0x80100000);le32(b.data()+60,0x80030000);return b;}
extern "C" void rtwb14_test_vendor_goldens(void);
int main(){
 rtwb14_test_vendor_goldens();
 for(unsigned id=0;id<256;id++){Bus b;ready(b);b.reg[0xfc]=uint8_t(id);auto o=ops(b);rtwb14_context c{};int e=rtwb14_open(&c,&o,3);assert((id==0x11)==(e==0));if(id!=0x11)assert(e==ENODEV&&!b.allocs&&!b.writes);rtwb14_close(&c);assert(!b.live);}
 for(unsigned pipes=0;pipes<7;pipes++){Bus b;ready(b);auto o=ops(b);rtwb14_context c{};int e=rtwb14_open(&c,&o,pipes);if(pipes<3||pipes>5){assert(e==EINVAL&&!b.calls);continue;}assert(!e);assert(!rtwb14_mac_boot(&c)&&c.state==RTWB14_MAC_READY);uint8_t otp[1024]{};assert(!rtwb14_read_otp(&c,0,otp,sizeof(otp)));assert(!std::memcmp(otp,b.otp,sizeof(otp)));auto old=b.calls;assert(rtwb14_read_otp(&c,1023,otp,2)==EINVAL&&old==b.calls);rtwb14_close(&c);assert(!b.live);}
 Bus base;ready(base);auto bo=ops(base);rtwb14_context good{};assert(!rtwb14_open(&good,&bo,3));assert(!rtwb14_mac_boot(&good));unsigned total=base.calls;rtwb14_close(&good);
 for(unsigned f=1;f<=total;f++){Bus b;ready(b);b.fail=f;auto o=ops(b);rtwb14_context c{};int e=rtwb14_open(&c,&o,3);if(!e)e=rtwb14_mac_boot(&c);assert(e==EIO);assert(b.calls==f);auto n=b.calls;assert(rtwb14_mac_boot(&c)!=0&&b.calls==n);rtwb14_close(&c);assert(!b.live);}
 auto image=firmware();rtwb14_fw_image inspected{};assert(!rtwb14_firmware_inspect(image.data(),image.size(),11,&inspected));assert(rtwb14_firmware_inspect(image.data(),image.size(),15,&inspected)==EINVAL);
 // Normal serialized ownership lifecycle; pure lease calls perform no I/O.
 {Bus b;ready(b);auto o=ops(b);rtwb14_context c{};int first=1,second=2;
  assert(!rtwb14_open(&c,&o,3)&&!rtwb14_mac_boot(&c)&&!rtwb14_firmware_download(&c,image.data(),image.size()));
  auto calls=b.calls;assert(!rtwb14_phy_claim(&c,&first)&&rtwb14_phy_owned(&c,&first));
  assert(rtwb14_phy_claim(&c,&second)==EBUSY);rtwb14_phy_release(&c,&second);assert(rtwb14_phy_owned(&c,&first));
  rtwb14_phy_release(&c,&first);assert(!rtwb14_phy_owned(&c,&first)&&!rtwb14_phy_claim(&c,&second)&&calls==b.calls);
  rtwb14_close(&c);assert(!rtwb14_phy_owned(&c,&second));
  b=Bus{};ready(b);
  assert(!rtwb14_open(&c,&o,3)&&!rtwb14_mac_boot(&c)&&!rtwb14_firmware_download(&c,image.data(),image.size()));
  calls=b.calls;assert(!rtwb14_phy_owned(&c,&second)&&calls==b.calls);rtwb14_close(&c);assert(!b.live);
 }
 Bus fwbus;ready(fwbus);auto fo=ops(fwbus);rtwb14_context fc{};assert(!rtwb14_open(&fc,&fo,3));assert(!rtwb14_mac_boot(&fc));assert(!rtwb14_firmware_download(&fc,image.data(),image.size()));assert(fc.state==RTWB14_FIRMWARE_READY&&fwbus.fw_packets==3&&!fwbus.h2c_packets);assert(!rtwb14_live_firmware(&fc));unsigned fwtotal=fwbus.calls,allocations=fwbus.allocs;rtwb14_close(&fc);assert(!fwbus.live);
 for(unsigned f=total+1;f<=fwtotal-2;f++){Bus b;ready(b);b.fail=f;auto o=ops(b);rtwb14_context c{};assert(!rtwb14_open(&c,&o,3));assert(!rtwb14_mac_boot(&c));assert(rtwb14_firmware_download(&c,image.data(),image.size())==EIO);assert(b.calls==f);auto prior=b.calls;assert(rtwb14_register_write(&c,0x1d70,4,0)!=0&&b.calls==prior);rtwb14_close(&c);assert(!b.live);}
 for(unsigned bad:{0xc079U,0xffffU,0xc070U}){Bus b;ready(b);auto o=ops(b);rtwb14_context c{};assert(!rtwb14_open(&c,&o,3));assert(!rtwb14_mac_boot(&c));assert(!rtwb14_firmware_download(&c,image.data(),image.size()));b.reg[0x80]=uint8_t(bad);b.reg[0x81]=uint8_t(bad>>8);auto prior=b.writes;assert(rtwb14_register_write(&c,0x1d70,4,0)==ENXIO&&prior==b.writes&&c.state==RTWB14_FAULT&&!c.power_owned);auto calls=b.calls;assert(rtwb14_power_off(&c)==ENXIO&&calls==b.calls);rtwb14_close(&c);assert(!b.live);}
 {Bus b;ready(b);auto o=ops(b);rtwb14_context c{};assert(!rtwb14_open(&c,&o,3));auto clone=c;auto calls=b.calls;assert(rtwb14_mac_boot(&clone)==EINVAL&&calls==b.calls);rtwb14_close(&clone);assert(b.live);assert(!rtwb14_mac_boot(&c));rtwb14_close(&c);assert(!b.live);}
 {Bus b,foreign;ready(b);ready(foreign);auto o=ops(b);rtwb14_context c{};assert(!rtwb14_open(&c,&o,3));c.bus.cookie=&foreign;auto calls=b.calls;assert(rtwb14_mac_boot(&c)==EINVAL&&b.calls==calls&&!foreign.calls);rtwb14_close(&c);assert(!b.live&&!foreign.live);}
 {Bus b;ready(b);auto o=ops(b);rtwb14_context c{};assert(!rtwb14_open(&c,&o,3));c.state=RTWB14_FIRMWARE_READY;c.power_owned=1;b.reg[0x80]=0x78;b.reg[0x81]=0xc0;auto calls=b.calls;assert(rtwb14_register_write(&c,0x1d70,4,0)==EINVAL&&calls==b.calls);rtwb14_close(&c);assert(!b.live);}
 std::printf("B14 boot+firmware modeled bus operations:%u; all mutation/ownership checks passed\n",fwtotal);

 for(unsigned fail=1;fail<=allocations;fail++){Bus b;ready(b);b.allocation_failure=fail;auto o=ops(b);rtwb14_context c{};int e=rtwb14_open(&c,&o,3);if(!e)e=rtwb14_mac_boot(&c);if(!e)e=rtwb14_firmware_download(&c,image.data(),image.size());assert(e==ENOMEM);rtwb14_close(&c);assert(!b.live);}
 for(unsigned mps:{512U,1024U})for(unsigned n:{1U,463U,464U,975U,976U,31744U}){std::vector<uint8_t> data(n,0x42),wire(48+n+1);size_t bytes=0;assert(!rtwb14_control_packet(0,data.data(),n,uint16_t(mps),wire.data(),wire.size(),&bytes));assert(bytes%mps&&bytes==48+n+((48+n)%mps==0));assert(!rtwb14_descriptor_check(wire.data(),bytes));assert(!std::memcmp(wire.data()+48,data.data(),n));if(bytes>48+n)assert(!wire.back());}
 {auto malformed=image;for(unsigned i=0;i<64;i++){auto changed=malformed;changed[i]^=0xff;rtwb14_fw_image im{};int e=rtwb14_firmware_inspect(changed.data(),changed.size(),11,&im);if(i==0||i==1||i==28||i==32||i==36||i==48||i==52||i==56||i==60)assert(e);}}
 for(unsigned when=0;when<2;when++){Bus b,foreign;ready(b);ready(foreign);auto o=ops(b);rtwb14_context c{};b.context=&c;b.foreign=&foreign;b.mutate_read=!when;b.mutate_allocate=!!when;assert(rtwb14_open(&c,&o,3)==EINVAL&&!foreign.calls&&!b.writes);rtwb14_close(&c);assert(!b.live);}
 {Bus b;ready(b);rtwb14_context c{};c.bus=ops(b);assert(!rtwb14_open(&c,&c.bus,3));rtwb14_close(&c);assert(!b.live);}
 uint8_t desc[49]{};for(unsigned kind=0;kind<2;kind++){assert(!rtwb14_control_descriptor(kind,4096,desc+1,48));assert(!rtwb14_descriptor_check(desc+1,48));desc[5]^=1;assert(rtwb14_descriptor_check(desc+1,48));}
 uint8_t phy[1024],logical[1024];std::memset(phy,0xff,sizeof(phy));phy[0]=0x0e;phy[1]=0x29;phy[2]=0x81;assert(!rtwb14_decode_otp(phy,1024,logical,1024));assert(logical[0]==0x29&&logical[1]==0x81&&logical[2]==0xff);
 {uint8_t raw[1024],out[1024];std::memset(raw,0x1f,sizeof(raw));raw[927]=0x0f;std::memset(out,0xa5,sizeof(out));assert(rtwb14_decode_otp(raw,sizeof(raw),out,sizeof(out))==EINVAL);for(auto x:out)assert(x==0xa5);}
 {uint8_t raw[1024],out[1024];std::memset(raw,0xff,sizeof(raw));raw[0]=0xef;raw[1]=0xfe;raw[2]=0xab;raw[3]=0xcd;assert(!rtwb14_decode_otp(raw,sizeof(raw),out,sizeof(out)));assert(out[1016]==0xab&&out[1017]==0xcd);}
 std::puts("RTL8814B actual HALMAC: all256 identities;3/4/5 USB mappings;OTP1024;every boot bus fault; unaligned IE/checksum codecs passed");
}
