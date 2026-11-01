#include "rtw/RTL8812AUBoot.h"
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <vector>
struct Write {unsigned reg,width;uint32_t value;};
struct Bus {
    std::array<uint8_t,65536> regs{};
    std::array<uint8_t,1024> otp{};
    std::vector<Write> writes;
    unsigned operations=0,failAt=0,waits=0;bool stuckOTP=false,stuckPower=false,waitFailure=false;
    size_t firmwareBytes=0;bool checksumReady=true,initReady=true;
    Bus(){otp.fill(0xff);regs[6]=2;regs[0xf3]=4;}
    bool fail(){return ++operations==failAt;}
    uint32_t read(unsigned a,unsigned n){uint32_t v=0;for(unsigned i=0;i<n;i++)v|=uint32_t(regs[a+i])<<(i*8);return v;}
    void set(unsigned a,unsigned n,uint32_t v){for(unsigned i=0;i<n;i++)regs[a+i]=uint8_t(v>>(i*8));}
    static rtw_usb_error in(void *arg,const rtw_usb_setup*s,uint8_t*out,uint16_t*actual){
        auto&b=*static_cast<Bus*>(arg);if(b.fail())return RTW_USB_IO_ERROR;
        for(unsigned i=0;i<s->w_length;i++)out[i]=b.regs[s->w_value+i];*actual=s->w_length;return RTW_USB_OK;
    }
    static rtw_usb_error out(void *arg,const rtw_usb_setup*s,const uint8_t*data,uint16_t*actual){
        auto&b=*static_cast<Bus*>(arg);if(b.fail())return RTW_USB_IO_ERROR;
        const uint8_t oldMCU=b.regs[0x80];
        uint32_t v=0;for(unsigned i=0;i<s->w_length;i++)b.regs[s->w_value+i]=data[i];
        for(unsigned i=0;i<s->w_length&&i<4;i++)v|=uint32_t(data[i])<<(i*8);
        b.writes.push_back({s->w_value,s->w_length,v});
        if(s->w_value>=0x1000&&s->w_value<0x2000){assert(s->w_length<=254);b.firmwareBytes+=s->w_length;}
        if(s->w_value==0x80){
            b.regs[0x80]=uint8_t((data[0]&~4U)|((data[0]&4U)?0U:(oldMCU&4U)));
            if((oldMCU&1)&&!(data[0]&1)&&b.firmwareBytes&&b.checksumReady)b.regs[0x80]|=4;
        }
        if(s->w_value==3&&(data[0]&4)&&(b.regs[0x80]&2)&&b.initReady)b.regs[0x80]|=0xc0;
        if(s->w_value==0x33&&!(data[0]&0x80)&&!b.stuckOTP){
            unsigned address=b.regs[0x31]|((b.regs[0x32]&3)<<8);
            b.regs[0x30]=b.otp[address];b.regs[0x33]|=0x80;
        }
        if(s->w_value==5&&(data[0]&1)&&!b.stuckPower)b.regs[5]&=uint8_t(~1U);
        *actual=s->w_length;return RTW_USB_OK;
    }
    static int wait(void *arg,uint32_t us){auto&b=*static_cast<Bus*>(arg);assert(us&&us<=10000);++b.waits;return !b.waitFailure;}
    rtw_8812au_boot context(){rtw_usb_rw_transport t{this,in,out};rtw_waiter w{this,wait};rtw_8812au_boot c;rtw_8812au_boot_init(&c,&t,&w);return c;}
};
static std::array<uint8_t,512> rom(){std::array<uint8_t,512> p;p.fill(0xff);p[0]=0x29;p[1]=0x81;p[0xb8]=9;p[0xb9]=0x21;p[0xba]=31;p[0xbc]=0x33;p[0xbd]=0x88;p[0xbf]=0x88;p[0xc1]=0x21;p[0xc3]=1;p[0xca]=0x80;p[0xcb]='B';p[0xcc]='R';p[0xd0]=0xda;p[0xd1]=0x0b;p[0xd2]=0x12;p[0xd3]=0x88;p[0xd7]=2;for(unsigned i=1;i<6;i++)p[0xd7+i]=uint8_t(i);return p;}
static void encode(Bus& b,const std::array<uint8_t,512>& p){size_t at=0;for(unsigned block=0;block<64;block++){bool used=false;for(unsigned j=0;j<8;j++)used|=p[8*block+j]!=0xff;if(!used)continue;assert(at+10<512);if(block<16)b.otp[at++]=uint8_t(block<<4);else{b.otp[at++]=uint8_t(((block&7)<<5)|15);b.otp[at++]=uint8_t((block&0x78)<<1);}for(unsigned j=0;j<8;j++)b.otp[at++]=p[8*block+j];}}
static void parsed_board(){
 auto p=rom();uint8_t hints[4]{};rtw_8812au_board b{};
 assert(!rtw_8812au_parse_board(0x04000000,p.data(),hints,&b));
 assert(b.raw_cut==0&&b.cut==1&&b.channel_plan==9&&b.crystal_cap==0x21);
 assert(b.ext_pa_2g&&b.ext_pa_5g&&b.ext_lna_2g&&b.ext_lna_5g&&b.rfe_option==3);
 assert(b.tx_streams==2&&b.rx_streams==2&&b.vht&&b.thermal_valid&&b.mac[0]==2&&b.btcoex&&b.share_ant);
 hints[1]=0x20;assert(!rtw_8812au_parse_board(0x04001000,p.data(),hints,&b));assert(b.tx_streams==1&&b.cut==2);
 hints[1]=0x40;hints[3]=8;assert(!rtw_8812au_parse_board(0x04000000,p.data(),hints,&b));assert(b.tx_streams==2&&!b.vht);
 p[0xca]=0xff;p[0xb9]=0xff;assert(!rtw_8812au_parse_board(0x04000000,p.data(),hints,&b));assert(b.rfe_option==0&&b.crystal_cap==0x20);
 {alignas(rtw_8812au_board) std::array<uint8_t,512> aliased=p;auto before=aliased;
  assert(rtw_8812au_parse_board(0x04000000,aliased.data(),hints,reinterpret_cast<rtw_8812au_board*>(aliased.data()))==EINVAL);
  assert(aliased==before);}
 p[0xd7]=1;assert(rtw_8812au_parse_board(0x04000000,p.data(),hints,&b)==EINVAL);assert(!b.mac[0]);
}
static void identify_and_power(){
 Bus b;encode(b,rom());auto c=b.context();
 rtw_8812au_boot_init(&c,&c.bus,&c.waiter);assert(c.bus.context==&b&&c.waiter.context==&b);
 assert(!rtw_8812au_identify_read_board(&c));
 assert(c.identified&&!c.powered&&!c.fw_running&&b.regs[0xcf]==0);
 assert((b.read(2,2)&0x1000)&&(b.read(8,2)&0x22)==0x22);
 assert(!rtw_8812au_power_on(&c));assert(c.powered&&!c.fw_running);
 assert(b.read(0x100,2)==0x63f&&!(b.read(0x100,2)&0xc0)&&b.regs[0x522]==0x3f);
 auto n=b.writes.size();assert(!rtw_8812au_power_on(&c)&&b.writes.size()==n);
 for(const auto&w:b.writes)assert(w.reg!=0x1f&&w.reg!=0xc90&&w.reg!=0xe90);
}
static void failures(){
 {Bus b;auto c=b.context();assert(rtw_8812au_power_on(&c)==EINVAL&&b.operations==0);}
 {Bus b;encode(b,rom());auto c=b.context();b.stuckOTP=true;assert(rtw_8812au_identify_read_board(&c)==ETIMEDOUT);assert(!c.identified&&b.regs[0xcf]==0);}
 {Bus b;encode(b,rom());auto c=b.context();assert(!rtw_8812au_identify_read_board(&c));b.regs[0x80]=0x80;auto n=b.writes.size();assert(rtw_8812au_power_on(&c)==EBUSY&&!c.powered&&b.writes.size()==n);}
 {Bus b;encode(b,rom());auto c=b.context();assert(!rtw_8812au_identify_read_board(&c));b.stuckPower=true;assert(rtw_8812au_power_on(&c)==ETIMEDOUT&&!c.powered);assert(b.waits<1100);}
 for(unsigned fail=1;fail<=35;fail++){Bus b;encode(b,rom());auto c=b.context();b.failAt=fail;assert(rtw_8812au_identify_read_board(&c)!=0&&!c.identified&&b.regs[0xcf]==0);}
}
static void failed_gate_closure(){
 Bus baseline;encode(baseline,rom());auto good=baseline.context();assert(!rtw_8812au_identify_read_board(&good));
 Bus failed;encode(failed,rom());auto bad=failed.context();failed.failAt=baseline.operations;
 assert(rtw_8812au_identify_read_board(&bad)==EIO&&!bad.identified);
 assert(failed.regs[0xcf]==0);
}
static void mailboxes(){
 Bus b;auto c=b.context();std::array<uint8_t,8> command{{0x40,0,1,2,3,4,5,6}};
 assert(rtw_8812au_h2c_mailbox(&c,command.data())==EINVAL&&b.operations==0);
 c.identified=c.powered=c.fw_running=1;
 for(unsigned i=0;i<5;i++){auto n=b.writes.size();assert(!rtw_8812au_h2c_mailbox(&c,command.data()));assert(b.writes[n].reg==0x1f0+(i%4)*4&&b.writes[n+1].reg==0x1d0+(i%4)*4);}
 assert(c.h2c_box==1);b.regs[0x1cc]=2;auto n=b.writes.size();assert(rtw_8812au_h2c_mailbox(&c,command.data())==ETIMEDOUT&&c.h2c_box==1&&b.writes.size()==n);assert(b.waits<=30);
}
static void firmware_preflight(){
 Bus b;auto c=b.context();rtw_boot_report r{};uint8_t image[33]{};image[0]=1;image[1]=0x95;
 assert(rtw_8812au_download_firmware(&c,image,sizeof(image),&r)==EINVAL&&b.operations==0);
 c.identified=c.powered=1;image[1]=0x21;assert(rtw_8812au_download_firmware(&c,image,sizeof(image),&r)==EINVAL&&b.operations==0&&!c.fw_running);
}
static void firmware_success_and_cleanup(){
 std::vector<uint8_t> image(32+8500,0x5a);image[0]=1;image[1]=0x95;image[4]=33;image[5]=0;image[6]=2;
 {Bus b;auto c=b.context();c.identified=c.powered=1;rtw_boot_report report{};
  assert(!rtw_8812au_download_firmware(&c,image.data(),image.size(),&report));
  assert(c.fw_running&&report.firmware_ready&&report.checksum_seen&&report.pages_written==3);
  assert(report.bytes_written==8500&&b.firmwareBytes==8500&&report.version==33&&report.subversion==2);
  assert(!(b.regs[0x80]&1));}
 {Bus b;auto c=b.context();c.identified=c.powered=1;b.checksumReady=false;rtw_boot_report report{};
  assert(rtw_8812au_download_firmware(&c,image.data(),image.size(),&report)==ETIMEDOUT&&!c.fw_running);
  assert(!(b.regs[0x80]&1)&&!report.firmware_ready&&b.waits<401);}
 {Bus b;auto c=b.context();c.identified=c.powered=1;b.initReady=false;rtw_boot_report report{};
  assert(rtw_8812au_download_firmware(&c,image.data(),image.size(),&report)==ETIMEDOUT&&!c.fw_running);
  assert(!(b.regs[0x80]&1)&&report.checksum_seen&&!report.firmware_ready);}
 for(unsigned n=12;n<30;n++) {Bus b;auto c=b.context();c.identified=c.powered=1;b.failAt=n;rtw_boot_report report{};
  assert(rtw_8812au_download_firmware(&c,image.data(),image.size(),&report)==EIO&&!c.fw_running);
  assert(!(b.regs[0x80]&1));}
}
int main(){parsed_board();identify_and_power();failures();failed_gate_closure();mailboxes();firmware_preflight();firmware_success_and_cleanup();puts("RTL8812AU boot: ROM/RFE/NSS,1024-byte OTP addressing,cold power,noRF,failures,mailboxes passed");}
