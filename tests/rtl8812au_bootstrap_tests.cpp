#include "rtw/RTL8812AUMAC.h"
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <cstdlib>
#include <vector>
struct Event{unsigned address,width;uint32_t value;};
struct Bus{
 std::array<uint8_t,65536> regs{};std::array<uint8_t,1024> otp{};std::array<uint8_t,32768> ram{};
 std::vector<Event>writes;std::vector<unsigned>otp_reads;std::array<uint8_t,256>llt{};
 unsigned calls=0,fail_at=0,late_fail_at=0,waits=0;uint64_t delayed=0;bool otp_stuck=false,power_stuck=false,mac_off_stuck=false,llt_stuck=false,cam_stuck=false,delay_fail=false,checksum=true,init_ready=true,ram_ready=true;
 size_t expected_fw=0,written=0;bool single;
 explicit Bus(bool one=false):single(one){otp.fill(255);regs[6]=2;put(0xf0,4,one?0x01000000:0x04000000);}
 uint32_t read(unsigned a,unsigned n)const{uint32_t v=0;for(unsigned i=0;i<n;++i)v|=uint32_t(regs[a+i])<<(8*i);return v;}
 void put(unsigned a,unsigned n,uint32_t v){for(unsigned i=0;i<n;++i)regs[a+i]=uint8_t(v>>(8*i));}
 bool fail(){return ++calls==fail_at;}
 static rtw_usb_error in(void*context,const rtw_usb_setup*s,uint8_t*d,uint16_t*actual){auto&b=*static_cast<Bus*>(context);assert(s->w_length==1||s->w_length==2||s->w_length==4);if(b.fail())return RTW_USB_IO_ERROR;for(unsigned i=0;i<s->w_length;++i)d[i]=b.regs[s->w_value+i];*actual=s->w_length;return RTW_USB_OK;}
 static rtw_usb_error out(void*context,const rtw_usb_setup*s,const uint8_t*d,uint16_t*actual){auto&b=*static_cast<Bus*>(context);if(b.fail())return RTW_USB_IO_ERROR;
  unsigned a=s->w_value,n=s->w_length;uint8_t old=b.regs[0x80];uint32_t v=0;for(unsigned i=0;i<n&&i<4;++i)v|=uint32_t(d[i])<<(8*i);b.writes.push_back({a,n,v});
  if(a>=0x1000&&a<0x2000){assert((old&1)&&n<=196);unsigned page=b.regs[0x82]&7;assert(a-0x1000+n<=4096);for(unsigned i=0;i<n;++i)b.ram[page*4096+a-0x1000+i]=d[i];b.written+=n;if(b.written==b.expected_fw&&b.checksum)b.regs[0x80]|=4;}
  else{
   for(unsigned i=0;i<n;++i)b.regs[a+i]=d[i];
   if(a==0x80)b.regs[0x80]=uint8_t((d[0]&~4U)|((d[0]&4U)?0U:(old&4U)));
   if(a==3&&(d[0]&4)&&(b.regs[0x80]&2)&&!(b.regs[0x80]&1)&&b.init_ready)b.regs[0x80]|=uint8_t(0x40|(b.ram_ready?0x80:0));
   if(a==0x33&&!(d[0]&0x80)&&!b.otp_stuck){unsigned address=b.regs[0x31]|((b.regs[0x32]&3U)<<8);assert(address<(b.single?512U:1024U));b.otp_reads.push_back(address);b.regs[0x30]=b.otp[address];b.regs[0x33]|=0x80;}
   if(a==5&&(d[0]&3)&&!b.power_stuck)b.regs[5]&=uint8_t(~(b.mac_off_stuck?1U:3U));
   if(a==0x1e0&&!b.llt_stuck){b.llt[(v>>8)&255]=uint8_t(v);b.regs[0x1e3]&=0x3f;}
   if(a==0x670&&!b.cam_stuck)b.regs[0x673]&=0x7f;
  }*actual=s->w_length;return b.calls==b.late_fail_at?RTW_USB_IO_ERROR:RTW_USB_OK;
 }
 static int wait(void*context,uint32_t us){auto&b=*static_cast<Bus*>(context);assert(us&&us<=20000);++b.waits;b.delayed+=us;return !b.delay_fail;}
};
static std::array<uint8_t,512> rom(bool one){std::array<uint8_t,512>r;r.fill(255);r[0]=0x29;r[1]=0x81;for(unsigned p=0;p<(one?1U:2U);++p){unsigned off=0x10+p*42;for(unsigned i=0;i<42;++i)r[off+i]=24;r[off+11]=0;r[off+32]=0;}
 r[0xb8]=0x34;r[0xb9]=0x20;r[0xba]=0x18;r[0xbc]=0;r[0xbd]=0;r[0xbf]=0;r[0xc1]=0;r[0xc3]=0;r[0xc6]=0;r[0xc7]=0;r[0xc8]=0x63;r[0xca]=one?255:0;r[0xcb]='B';r[0xcc]='R';unsigned vid=one?0x100:0xd0,mac=one?0x107:0xd7;r[vid]=0xda;r[vid+1]=0xb;r[vid+2]=one?0x21:0x12;r[vid+3]=0x88;r[mac]=2;for(unsigned i=1;i<6;++i)r[mac+i]=uint8_t(i);return r;}
static void pack(Bus&b,const std::array<uint8_t,512>&r){size_t cursor=0;for(unsigned block=0;block<64;++block){bool used=false;for(unsigned j=0;j<8;++j)used|=r[block*8+j]!=255;if(!used)continue;assert(cursor+10<512);if(block<16)b.otp[cursor++]=uint8_t(block<<4);else{b.otp[cursor++]=uint8_t(((block&7)<<5)|15);b.otp[cursor++]=uint8_t((block&0x78)<<1);}for(unsigned j=0;j<8;++j)b.otp[cursor++]=r[block*8+j];}for(unsigned i=1018;i<1022;++i)b.otp[i]=0;}
struct Fixture{
 Bus b;rtw_r12au_boot c{};std::array<uint8_t,512>logical;
 Fixture(bool one=false,unsigned endpoints=3,rtw_r12au_usb_speed speed=RTW_R12AU_USB_HIGH):b(one),logical(rom(one)){pack(b,logical);rtw_usb_rw_transport bus{&b,Bus::in,Bus::out};rtw_waiter wait{&b,Bus::wait};rtw_r12au_usb_config usb{speed,uint16_t(speed==RTW_R12AU_USB_SUPER?1024:speed==RTW_R12AU_USB_HIGH?512:64),0x81,{2,3,4,5},uint8_t(endpoints)};assert(!rtw_r12au_boot_init(&c,one?RTW_R12AU_CHIP_8821AU:RTW_R12AU_CHIP_8812AU,&bus,&wait,&usb));assert(!b.calls);}
 void identify(){assert(!rtw_r12au_identify(&c));assert(c.identified&&!c.powered&&b.regs[0xcf]==0);assert(c.board.phy.rf_paths==(b.single?1:2));assert(c.board.channel_plan==0x34&&c.board.mac[0]==2);assert(c.board.eeprom_vendor==0xbda&&c.board.eeprom_product==(b.single?0x8821:0x8812));assert(std::memcmp(c.logical_rom,logical.data(),512)==0);}
 void seed(unsigned phase){uint8_t hints[4]={};assert(!rtw_r12au_decode_board(c.chip,b.read(0xf0,4),logical.data(),b.single?nullptr:hints,0,&c.board));c.identified=1;std::memcpy(c.logical_rom,logical.data(),512);if(phase>=1){c.powered=1;b.put(0x100,2,0x63f);b.regs[0x522]=0x3f;}if(phase>=2){c.dma_prepared=1;c.rsvd_boundary=c.acq_pages=uint8_t(b.single?248:247);}if(phase>=3){c.firmware_running=1;b.regs[0x80]=0xc6;}}
};
static std::vector<uint8_t> firmware(bool one,size_t payload=8500){std::vector<uint8_t>v(32+payload);for(size_t i=32;i<v.size();++i)v[i]=uint8_t(i*31);v[0]=1;v[1]=one?0x21:0x95;v[2]=one?0x13:0x10;v[4]=42;v[6]=4;v[12]=uint8_t(payload);v[13]=uint8_t(payload>>8);return v;}
static void happy(){
 for(bool one:{false,true})for(unsigned endpoints=2;endpoints<=4;++endpoints)for(auto speed:{RTW_R12AU_USB_FULL,RTW_R12AU_USB_HIGH,RTW_R12AU_USB_SUPER}){
  Fixture f(one,endpoints,speed);f.identify();assert(f.b.otp_reads.size()==(one?512:516));if(one){for(auto a:f.b.otp_reads)assert(a<512);}else for(unsigned i=0;i<4;++i)assert(f.b.otp_reads[512+i]==1018+i);
  assert(!rtw_r12au_power_on(&f.c));assert(f.c.powered&&f.b.read(0x100,2)==0x63f&&f.b.regs[0x522]==0x3f);if(one)assert(f.b.regs[0x7c]&0x40);
  assert(!rtw_r12au_dma_prepare(&f.c));unsigned boundary=one?248:247;assert(f.c.rsvd_boundary==boundary);for(unsigned i=0;i<256;++i)assert(f.b.llt[i]==(i==boundary-1?255:i==255?boundary:i+1));
  auto image=firmware(one);f.b.expected_fw=image.size()-32;rtw_boot_report r{};assert(!rtw_r12au_download_fw(&f.c,image.data(),image.size(),&r));assert(f.c.firmware_running&&r.firmware_ready&&r.checksum_seen&&r.pages_written==3&&r.bytes_written==8500&&r.version==42&&r.subversion==4);assert((f.b.regs[0x80]&0xc7)==0xc6);assert(!std::memcmp(image.data()+32,f.b.ram.data(),8500));assert(f.b.regs[0x1d]&(one?1:8));
  f.b.regs[0x283]=0xd5;f.b.regs[0x290]=0xff;
  assert(!rtw_r12au_mac_init(&f.c));assert(f.b.regs[0x283]==0xd5);assert(f.b.regs[0x290]==(speed==RTW_R12AU_USB_SUPER?0x0e:speed==RTW_R12AU_USB_HIGH?0x1e:0x2e));assert(f.c.mac_initialized&&!f.c.associated&&f.b.regs[0x522]==0x3f);assert((f.b.read(0x100,2)&0xc0)==0xc0&&f.b.read(0x680,2)==0);assert(f.b.regs[0x609]&0x40);assert(!(f.b.regs[0x60b]&0x80));assert(f.b.regs[0x60f]==4);assert(f.b.read(0x116,2)==15999);assert(f.b.read(0x514,2)==0x100a);assert(f.b.regs[0x456]==(one?0x5e:0x70));assert(f.b.regs[0x114]==boundary);
  unsigned hq=one?8:16,nq=endpoints>=3?hq:0,eq=one&&endpoints==4?4:0,pub=boundary-hq-nq-eq-1;assert(f.b.read(0x200,4)==(0x80000000U|hq|(pub<<16)));assert(f.b.read(0x214,4)==(nq|(eq<<16)));assert(f.b.read(0x280,2)==(speed==RTW_R12AU_USB_SUPER?0x1a06:0x2005));
  uint8_t ep=0;assert(!rtw_r12au_qsel_endpoint(&f.c,0,&ep)&&ep==(endpoints==2?3:4));assert(!rtw_r12au_qsel_endpoint(&f.c,0x12,&ep)&&ep==(endpoints==4?5:2));assert(rtw_r12au_qsel_endpoint(&f.c,15,&ep)==EINVAL&&!ep);
  const uint8_t bssid[6]={2,0xaa,0xbb,0xcc,0xdd,0xee};assert(!rtw_r12au_mac_station(&f.c,bssid,123,1));assert(f.c.associated&&f.b.read(0x6a8,2)==123&&(f.b.read(0x100,4)&0x30000)==0x20000);assert(!std::memcmp(f.b.regs.data()+0x618,bssid,6));assert(f.b.regs[0x608]&0xc0);assert(f.b.regs[0x522]==0x3f);assert(!rtw_r12au_mac_station(&f.c,nullptr,0,0)&&!f.c.associated);
  uint8_t command[8]={0x40,1,2,3,4,5,6,7};f.b.regs[0x1cc]=0;for(unsigned i=0;i<5;++i){size_t n=f.b.writes.size();assert(!rtw_r12au_h2c(&f.c,command));assert(f.b.writes[n].address==0x1f0+4*(i%4)&&f.b.writes[n+1].address==0x1d0+4*(i%4));}assert(f.c.h2c_box==1);
  assert(!rtw_r12au_power_off(&f.c));assert(!f.c.powered&&!f.c.dma_prepared&&!f.c.firmware_running&&!f.c.mac_initialized&&!f.c.associated&&!(f.b.regs[0x100]&0xc0));
 }
}
static void firmware_boundaries(){
 for(bool one:{false,true})for(size_t size:{size_t(1),size_t(196),size_t(197),size_t(4095),size_t(4096),size_t(4097),size_t(32768)}){
  Fixture f(one);f.seed(2);auto image=firmware(one,size);f.b.expected_fw=size;rtw_boot_report report{};assert(!rtw_r12au_download_fw(&f.c,image.data(),image.size(),&report));assert(report.firmware_ready&&report.bytes_written==size&&report.pages_written==(size+4095)/4096);assert(!std::memcmp(image.data()+32,f.b.ram.data(),size));
 }
}
static void actual_firmware(const char*path,bool one){
 std::ifstream in(path,std::ios::binary);assert(in);std::vector<uint8_t>image((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());Fixture f(one);f.seed(2);f.b.expected_fw=image.size()-32;rtw_boot_report report{};assert(!rtw_r12au_download_fw(&f.c,image.data(),image.size(),&report));assert(report.firmware_ready&&!std::memcmp(image.data()+32,f.b.ram.data(),image.size()-32));std::printf("RTL%u actual firmware: version %u.%u, %zu bytes, %u pages verified in USB model\n",one?8821:8812,report.version,report.subversion,report.bytes_written,report.pages_written);
}
static void inputs(){
 Fixture f;auto old=f.c;auto bad=f.c.usb;bad.bulk_out_count=1;assert(rtw_r12au_boot_init(&f.c,f.c.chip,&f.c.bus,&f.c.waiter,&bad)==EINVAL);assert(!f.b.calls);assert(!std::memcmp(&old,&f.c,sizeof(old)));
 assert(!rtw_r12au_boot_init(&f.c,f.c.chip,&f.c.bus,&f.c.waiter,&f.c.usb));assert(f.c.bus.context==&f.b&&f.c.usb.bulk_out_count==3);
 assert(rtw_r12au_dma_prepare(&f.c)==EINVAL);assert(rtw_r12au_mac_init(&f.c)==EINVAL);assert(rtw_r12au_power_on(&f.c)==EINVAL);assert(!f.b.calls);
 for(bool one:{false,true}){auto image=firmware(one);rtw_fw_image fw{};assert(!rtw_r12au_fw_inspect(one?RTW_R12AU_CHIP_8821AU:RTW_R12AU_CHIP_8812AU,image.data(),image.size(),&fw));assert(rtw_r12au_fw_inspect(one?RTW_R12AU_CHIP_8812AU:RTW_R12AU_CHIP_8821AU,image.data(),image.size(),&fw)==EBADMSG);image[12]^=1;assert(rtw_r12au_fw_inspect(one?RTW_R12AU_CHIP_8821AU:RTW_R12AU_CHIP_8812AU,image.data(),image.size(),&fw)==EBADMSG);}
 /* Linux rtw88xxau_hw_reset retires resident RAM firmware before the cold
  * flow. This catches the permanent EBUSY dead end on an identified warm AU. */
 for(bool one:{false,true}){Fixture warm(one);warm.seed(0);warm.b.regs[0x80]=0xc6;
  assert(!rtw_r12au_power_on(&warm.c));assert(warm.c.powered&&!warm.c.firmware_running&&!warm.c.mac_initialized);
  assert(warm.c.warm_reset_attempts==1&&warm.c.warm_reset_stage==5&&warm.c.warm_reset_mcu==0xc6&&!warm.c.warm_reset_error);
  assert(!(warm.b.regs[0x80]&0x80));
  /* Independently transcribed primary reset boundaries. A stale-MCU bypass
   * that merely clears RAM_DL_SEL cannot satisfy the BB/RF/MAC ordering. */
  const Event reset[]={{0x80,1,0},{0x8b4,4,0x40},{2,1,0},{0x1f,1,0},{0x100,2,0},{5,1,2},{5,1,1},{3,1,4},{3,1,0x94}};
  size_t cursor=0;
  for(const auto&want:reset){while(cursor<warm.b.writes.size()&&(warm.b.writes[cursor].address!=want.address||warm.b.writes[cursor].width!=want.width||warm.b.writes[cursor].value!=want.value))++cursor;assert(cursor<warm.b.writes.size());++cursor;}
  assert(!rtw_r12au_dma_prepare(&warm.c));auto image=firmware(one);warm.b.expected_fw=image.size()-32;rtw_boot_report report{};
  assert(!rtw_r12au_download_fw(&warm.c,image.data(),image.size(),&report)&&report.firmware_ready);
  assert(!rtw_r12au_mac_init(&warm.c)&&warm.c.mac_initialized);
 }
 Fixture single(true);single.b.put(0x68,4,1U<<18);single.identify();assert(single.c.board.phy.btcoex==1&&single.c.board.phy.rf_paths==1);assert(single.c.board.phy.cut==0);
 auto ff=rom(true);ff[0xc3]=255;rtw_r12au_board decoded{};assert(!rtw_r12au_decode_board(RTW_R12AU_CHIP_8821AU,single.c.board.sys_cfg,ff.data(),nullptr,1,&decoded));assert(decoded.bt_setting==255&&decoded.share_ant==1);
 Fixture two;two.b.otp[1019]=0x20;assert(!rtw_r12au_identify(&two.c));assert(two.c.board.phy.rf_paths==1&&two.c.board.phy.cut==1);
}
static void warm_reset_failures(){
 for(bool one:{false,true}){
  Fixture base(one);base.seed(0);base.b.regs[0x80]=0xc6;assert(!rtw_r12au_power_on(&base.c));unsigned total=base.b.calls;
  for(bool ambiguous:{false,true})for(unsigned step=1;step<=total;++step){
   Fixture f(one);f.seed(0);f.b.regs[0x80]=0xc6;
   if(ambiguous)f.b.late_fail_at=step;else f.b.fail_at=step;
   const int e=rtw_r12au_power_on(&f.c);
   /* The late-error hook applies only to writes. Successful reads may reach
    * the complete state, but every reported transfer error must close it. */
   if(!e){assert(ambiguous&&f.c.powered&&f.c.warm_reset_stage==5);continue;}
   assert(e==EIO&&f.c.io_error==EIO&&!f.c.powered&&!f.c.firmware_running&&!f.c.mac_initialized);
   if(f.c.warm_reset_stage&&f.c.warm_reset_stage!=5)assert(f.c.warm_reset_error==EIO);
  }
  Fixture stuck(one);stuck.seed(0);stuck.b.regs[0x80]=0xc6;stuck.b.mac_off_stuck=true;
  assert(rtw_r12au_power_on(&stuck.c)==ETIMEDOUT&&stuck.c.io_error==ETIMEDOUT&&!stuck.c.powered);
  assert(stuck.c.warm_reset_stage==3&&stuck.c.warm_reset_error==ETIMEDOUT&&stuck.b.delayed==4999);
  const auto attempts=stuck.c.warm_reset_attempts;assert(rtw_r12au_power_on(&stuck.c)==ETIMEDOUT&&stuck.c.warm_reset_attempts==attempts);
  /* Shutdown/recovery must retain the original reset result for diagnostics. */
  stuck.b.mac_off_stuck=false;assert(!rtw_r12au_power_off_checked(&stuck.c));
  assert(stuck.c.io_error==ETIMEDOUT&&stuck.c.warm_reset_error==ETIMEDOUT&&stuck.c.warm_reset_stage==3);
  std::printf("RTL%u resident MCU reset: %u early/ambiguous I/O positions and bounded MAC-off timeout\n",one?8821:8812,total);
 }
}
static int phase(Fixture&f,unsigned p,const std::vector<uint8_t>&image){switch(p){case 0:return rtw_r12au_identify(&f.c);case 1:return rtw_r12au_power_on(&f.c);case 2:return rtw_r12au_dma_prepare(&f.c);case 3:{f.b.expected_fw=image.size()-32;rtw_boot_report r{};return rtw_r12au_download_fw(&f.c,image.data(),image.size(),&r);}default:return rtw_r12au_mac_init(&f.c);}}
static void error_sweep(){
 for(bool one:{false,true})for(unsigned p=0;p<5;++p){auto image=firmware(one);Fixture base(one);if(p)base.seed(p-1);assert(!phase(base,p,image));unsigned total=base.b.calls;
  for(unsigned step=1;step<=total;++step){Fixture f(one);if(p)f.seed(p-1);f.b.fail_at=step;int e=phase(f,p,image);if(e!=EIO){std::fprintf(stderr,"chip%u phase%u I/O%u/%u error%d\n",one?8821:8812,p,step,total,e);std::abort();}assert(f.c.io_error==EIO);if(p==0)assert(!f.c.identified);if(p==1)assert(!f.c.powered);if(p==2)assert(!f.c.dma_prepared);if(p==3)assert(!f.c.firmware_running&&!(f.b.regs[0x80]&1));if(p==4)assert(!f.c.mac_initialized&&f.b.regs[0x522]==0x3f&&!(f.b.regs[0x100]&0xc0));if(p==0&&step>3)assert(f.b.regs[0xcf]==0);}
  std::printf("RTL%u phase%u: %u individual I/O failures\n",one?8821:8812,p,total);
 }
}
static void ambiguous_writes_and_station(){
 for(bool one:{false,true}){
  auto image=firmware(one);for(unsigned p=0;p<5;++p){Fixture base(one);if(p)base.seed(p-1);assert(!phase(base,p,image));unsigned total=base.b.calls;
   for(unsigned step=1;step<=total;++step){Fixture f(one);if(p)f.seed(p-1);f.b.late_fail_at=step;int e=phase(f,p,image);if(e){assert(e==EIO&&f.c.io_error==EIO);if(p==0)assert(!f.c.identified&&f.b.regs[0xcf]==0);if(p==1)assert(!f.c.powered);if(p==2)assert(!f.c.dma_prepared);if(p==3)assert(!f.c.firmware_running&&!(f.b.regs[0x80]&1));if(p==4)assert(!f.c.mac_initialized&&!(f.b.regs[0x100]&0xc0));}}
  }
  Fixture base(one);base.seed(3);assert(!rtw_r12au_mac_init(&base.c));auto calls=base.b.calls;uint8_t bad[6]={1,2,3,4,5,6},bssid[6]={2,3,4,5,6,7};assert(rtw_r12au_mac_station(&base.c,bad,1,1)==EINVAL);assert(rtw_r12au_mac_station(&base.c,bssid,0,1)==EINVAL);assert(rtw_r12au_mac_station(&base.c,nullptr,123,0)==EINVAL);assert(base.b.calls==calls);assert(!rtw_r12au_mac_station(&base.c,bssid,123,1));unsigned steps=base.b.calls-calls;
  for(unsigned step=1;step<=steps;++step){Fixture f(one);f.seed(3);assert(!rtw_r12au_mac_init(&f.c));f.b.fail_at=f.b.calls+step;assert(rtw_r12au_mac_station(&f.c,bssid,123,1)==EIO);assert(!f.c.mac_initialized&&!f.c.associated&&!(f.b.regs[0x100]&0xc0)&&f.b.regs[0x522]==0x3f);}
  Fixture h(one);h.seed(3);uint8_t command[8]={};for(unsigned step=1;step<=3;++step){h.c.io_error=0;h.c.h2c_box=0;h.b.fail_at=h.b.calls+step;assert(rtw_r12au_h2c(&h.c,command)==EIO&&!h.c.h2c_box);}
 }
 Fixture alias;alias.seed(2);auto image=firmware(false);auto before=image;assert(rtw_r12au_download_fw(&alias.c,image.data(),image.size(),reinterpret_cast<rtw_boot_report*>(image.data()+32))==EINVAL);assert(image==before&&!alias.b.calls);auto context=alias.c;assert(rtw_r12au_download_fw(&alias.c,image.data(),image.size(),reinterpret_cast<rtw_boot_report*>(&alias.c))==EINVAL);assert(!std::memcmp(&context,&alias.c,sizeof(context))&&!alias.b.calls);
}
static void timeouts(){
 for(bool one:{false,true}){
  Fixture otp(one);otp.b.otp_stuck=true;assert(rtw_r12au_identify(&otp.c)==ETIMEDOUT&&!otp.c.identified&&otp.b.regs[0xcf]==0&&otp.b.waits==99);
  Fixture pwr(one);pwr.seed(0);pwr.b.power_stuck=true;assert(rtw_r12au_power_on(&pwr.c)==ETIMEDOUT&&!pwr.c.powered&&pwr.b.waits<=1000);
  Fixture llt(one);llt.seed(1);llt.b.llt_stuck=true;assert(rtw_r12au_dma_prepare(&llt.c)==ETIMEDOUT&&!llt.c.dma_prepared&&llt.b.waits==21);
  Fixture cam(one);cam.seed(3);cam.b.cam_stuck=true;assert(rtw_r12au_mac_init(&cam.c)==ETIMEDOUT&&!cam.c.mac_initialized&&cam.b.regs[0x522]==0x3f);
  for(unsigned fault=0;fault<3;++fault){Fixture fw(one);fw.seed(2);auto image=firmware(one);fw.b.expected_fw=image.size()-32;fw.b.checksum=fault!=0;fw.b.init_ready=fault!=1;fw.b.ram_ready=fault!=2;rtw_boot_report r{};assert(rtw_r12au_download_fw(&fw.c,image.data(),image.size(),&r)==ETIMEDOUT&&!fw.c.firmware_running&&!r.firmware_ready&&!(fw.b.regs[0x80]&1));}
  Fixture h2c(one);h2c.seed(3);h2c.b.regs[0x1cc]=1;uint8_t cmd[8]={};assert(rtw_r12au_h2c(&h2c.c,cmd)==ETIMEDOUT&&!h2c.c.h2c_box&&h2c.b.waits==30);auto calls=h2c.b.calls;assert(rtw_r12au_h2c(&h2c.c,cmd)==ETIMEDOUT&&h2c.b.calls==calls);
  Fixture shutdown(one);shutdown.seed(3);shutdown.c.io_error=EIO;assert(rtw_r12au_power_off(&shutdown.c)==EIO&&!shutdown.c.firmware_running&&!shutdown.c.powered&&shutdown.b.calls>1&&!(shutdown.b.regs[0x100]&0xc0));
 }
}
static void checked_cleanup(){for(bool one:{false,true}){Fixture clean(one);clean.seed(3);clean.c.io_error=ETIMEDOUT;assert(rtw_r12au_power_off_checked(&clean.c)==0&&clean.c.io_error==ETIMEDOUT&&!clean.c.powered);Fixture failed(one);failed.seed(3);failed.b.fail_at=failed.b.calls+1;assert(rtw_r12au_power_off_checked(&failed.c)==EIO);failed.b.fail_at=0;assert(rtw_r12au_power_off_checked(&failed.c)==EIO);}}
static void advisory_rssi(){for(bool one:{false,true})for(unsigned box=0;box<4;++box){
 Fixture f(one);f.seed(3);f.c.associated=1;f.c.h2c_box=uint8_t(box);f.b.regs[0x1cc]=uint8_t(1U<<box);
 const uint8_t command[8]={0x42,0,0,57};const auto writes=f.b.writes.size();
 assert(rtw_r12au_h2c_rssi_monitor(&f.c,command)==EBUSY);
 assert(!f.c.io_error&&f.c.firmware_running&&f.c.associated&&f.c.h2c_box==box&&f.b.writes.size()==writes&&f.b.waits==30);
 f.b.regs[0x1cc]=0;assert(!rtw_r12au_h2c_rssi_monitor(&f.c,command)&&f.c.h2c_box==(box+1)%4);
 for(unsigned position=1;position<=3;++position){Fixture bad(one);bad.seed(3);bad.b.fail_at=position;
  assert(rtw_r12au_h2c_rssi_monitor(&bad.c,command)==EIO&&bad.c.io_error==EIO&&!bad.c.h2c_box);}
 Fixture delay(one);delay.seed(3);delay.b.regs[0x1cc]=1;delay.b.delay_fail=true;
 assert(rtw_r12au_h2c_rssi_monitor(&delay.c,command)==EIO&&delay.c.io_error==EIO);
 Fixture invalid(one);invalid.seed(3);uint8_t bad[8]={0x40};
 assert(rtw_r12au_h2c_rssi_monitor(&invalid.c,bad)==EINVAL&&!invalid.b.calls);
 for(unsigned index=0;index<8;++index){std::memcpy(bad,command,8);bad[index]=index==0?1:index==3?101:1;
  assert(rtw_r12au_h2c_rssi_monitor(&invalid.c,bad)==EINVAL&&!invalid.b.calls);}
 for(int error:{EIO,ETIMEDOUT,EBUSY}){invalid.c.io_error=error;
  assert(rtw_r12au_h2c_rssi_monitor(&invalid.c,command)==error&&!invalid.b.calls);}
 }std::puts("AU optional RSSI: both families/all boxes preserve busy station, retry next tick; hard/sticky I/O and mandatory H2C remain fatal");}
int main(int argc,char**argv){advisory_rssi();warm_reset_failures();checked_cleanup();assert(argc==1||argc==3);if(argc==3){actual_firmware(argv[1],false);actual_firmware(argv[2],true);}inputs();happy();firmware_boundaries();timeouts();error_sweep();ambiguous_writes_and_station();std::puts("Typed AU bootstrap/MAC: distinct OTP,8051,power,LLT,DMA,USB,station,H2C,rollback passed");}
