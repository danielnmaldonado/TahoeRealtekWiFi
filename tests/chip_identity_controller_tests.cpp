/* SPDX-License-Identifier: GPL-2.0-only
 * Actual Controller discovery/request/guard methods, fake USB host only. */
#include "rtw/ChipIdentity.h"
#include "rtw/DFSStation.h"
#include "rtw/RTW88Boot.h"
#include "rtw/FamilyBackend.h"
#include "rtw/RTW88PHYOperating.h"
#include "rtw/RTW88Station.h"
#include "rtw/RTW88Coex.h"
#include "RTL8821CUFirmware.inc"
#include "RTL8822BUFirmware.inc"
#include "RTL8822CUFirmware.inc"
#include <array>
#include <vector>
#include <string>
#include <cstring>
#include <cassert>
#include <cerrno>
#include <cstdio>
/* ACTUAL_BOOT_BUS_MODEL */
extern "C" void rtw88_phy_radio_invalidate(rtw88_phy_context*p){if(p)*p={};}
extern "C" int rtw88_coex_ready(const rtw88_coex*,const rtw88_phy_context*){assert(false);return 0;}
using IOReturn=int;
static constexpr int kIOReturnSuccess=0,kIOReturnUnderrun=1;
static uint16_t USBToHost16(uint16_t x){return x;}
static uint16_t OSSwapHostToLittleInt16(uint16_t x){return x;}
namespace StandardUSB {struct DeviceRequest{uint8_t bmRequestType=0,bRequest=0;uint16_t wValue=0,wIndex=0,wLength=0;};}
struct __attribute__((packed)) DeviceDescriptor{uint16_t idVendor=0x13b1,idProduct=0x43,bcdDevice=0x200;};
struct __attribute__((packed)) ConfigDescriptor{uint8_t bLength,bDescriptorType;uint16_t wTotalLength;uint8_t bNumInterfaces,bConfigurationValue,iConfiguration,bmAttributes,bMaxPower;};
struct __attribute__((packed)) InterfaceDescriptor{uint8_t bLength,bDescriptorType,bInterfaceNumber,bAlternateSetting,bNumEndpoints,bInterfaceClass,bInterfaceSubClass,bInterfaceProtocol,iInterface;};
struct __attribute__((packed)) EndpointDescriptor{uint8_t bLength,bDescriptorType,bEndpointAddress,bmAttributes;uint16_t wMaxPacketSize;uint8_t bInterval;};
struct IOUSBHostPipe{EndpointDescriptor descriptor{};const EndpointDescriptor*getEndpointDescriptor(){return &descriptor;}};
struct IOUSBHostDevice{DeviceDescriptor descriptor{};const DeviceDescriptor*getDeviceDescriptor(){return &descriptor;}};
struct IOUSBHostInterface {
 bool missing_device=false,missing_interface=false,missing_configuration=false;std::array<unsigned,9>chip_positions{};IOUSBHostDevice device;std::array<uint8_t,39> config{};IOUSBHostPipe pipes[3]{};Bus bus;
 unsigned requests=0,reads=0,writes=0,bulks=0,fail_request=0,chip_reads=0,change_at=0,change_to=0,change_writes=0;bool short_read=false,requested=false;unsigned hook_at=0,hook_writes=0;void(*hook)(void*)=nullptr;void*hook_cookie=nullptr;
 IOUSBHostInterface(unsigned code):bus(code==0x13?RTW_WIFI_RTL8822CU:RTW_WIFI_RTL8822BU){const uint8_t bytes[]={9,2,39,0,1,1,0,0x80,50,9,4,0,0,3,255,255,255,0,7,5,0x81,2,0,2,0,7,5,1,2,0,2,0,7,5,2,2,0,2,0};std::memcpy(config.data(),bytes,sizeof(bytes));for(unsigned i=0;i<3;i++)std::memcpy(&pipes[i].descriptor,bytes+18+7*i,7);put(bus.reg,0xfc,4,code);bus.reg[3]=4;bus.reg[0x1d]=1;bus.reg[9]=0x40;bus.reg[0x1082]=1;}
 IOUSBHostDevice*getDevice(){return missing_device?nullptr:&device;}const InterfaceDescriptor*getInterfaceDescriptor(){if(missing_interface)return nullptr;return reinterpret_cast<const InterfaceDescriptor*>(config.data()+9);}const ConfigDescriptor*getConfigurationDescriptor(){if(missing_configuration)return nullptr;return reinterpret_cast<const ConfigDescriptor*>(config.data());}
 IOUSBHostPipe*copyPipe(uint8_t address){for(auto&pipe:pipes)if(pipe.descriptor.bEndpointAddress==address)return &pipe;return nullptr;}
 int deviceRequest(const StandardUSB::DeviceRequest&r,uint8_t*b,uint32_t&actual,unsigned){
  ++requests;assert(r.bRequest==5&&!r.wIndex&&r.wLength&&r.wLength<=4);if(fail_request==requests)return 17;actual=r.wLength;
  if(r.bmRequestType==0xc0){++reads;if(r.wValue==0xfc){++chip_reads;assert(chip_reads<chip_positions.size());chip_positions[chip_reads]=requests;if(change_at==chip_reads){bus.reg[0xfc]=(uint8_t)change_to;change_writes=writes+bulks;}}uint32_t value=0;int e=Bus::read(&bus,r.wValue,r.wLength,&value);if(e)return e;for(unsigned i=0;i<r.wLength;i++)b[i]=(uint8_t)(value>>(8*i));if(short_read)actual=0;if(r.wValue==0xfc&&chip_reads==hook_at&&hook){hook_writes=writes+bulks;hook(hook_cookie);}
  }else{
   assert(r.bmRequestType==0x40);++writes;uint32_t v=0;for(unsigned i=0;i<r.wLength;i++)v|=uint32_t(b[i])<<(8*i);
   if(r.wValue==0x1a0&&r.wLength==1&&v==0xfd)requested=true;
   if(r.wValue==0x1208){v&=~0x80000000U;if(v&0x02000000U)v&=~0x08000000U;}
   int e=Bus::write(&bus,r.wValue,r.wLength,v);if(e)return e;
   if(r.wValue==3&&(v&4U)&&(bus.reg[0x81]&0x40U)){bus.reg[0x80]=0x78;bus.reg[0x81]=0xc0;if(requested){bus.reg[0x1a0]=0x19;put(bus.reg,0x1a6,4,(7U<<16)|(2U<<19)|(1U<<21)|(3U<<26));}}
  }return 0;
 }
 int bulk(uint8_t q,const uint8_t*b,size_t n){++requests;if(fail_request==requests)return EIO;assert(q==16&&b&&n>=49&&n<=4145&&b[2]==48);++bulks;bus.reg[0x205]|=0x80;return 0;}
};
enum {EDCA_AC_BE=0,EDCA_AC_BK,EDCA_AC_VI,EDCA_AC_VO};
static constexpr unsigned LINK_STATE_DOWN=0,kIONetworkLinkValid=1;
struct FakeSuper{static void setLinkStatus(unsigned){}};
class RTWUSBController{public:
 bool dfsTXHold_=false;void resetDFSStation(bool=false){}
 using super=FakeSuper;bool radioReady_=false,attached_=false;
 IOUSBHostInterface*usb_=nullptr;IOUSBHostPipe*rxPipe_=nullptr,*txPipes_[4]{};uint16_t txMPS_[4]{};bool present_=true,usbOpened_=true,sleeping_=false;IOReturn usbError_=0;
 const rtw_usb_wifi_profile*profile_=nullptr;rtw_wifi_family family_=RTW_WIFI_UNKNOWN;rtw_usb_endpoints endpoints_{};struct{unsigned ac2idx[4]{},ntxchains=0;struct{struct{unsigned if_link_state=0;}ic_if;}sc_ic;}core_;
 const rtw_usb_wifi_profile*r88DiscoveryProfile_=nullptr;const void*r88DiscoveryProvider_=nullptr;
 alignas(uint64_t) uint8_t r88IdentityStorage_[RTW_CHIP_IDENTITY_STORAGE_BYTES]{};rtw_chip_identity*r88ResolvedIdentity_=nullptr;rtw88_boot_context r88Core_{};
 static int r88Read(void*,uint16_t,unsigned,uint32_t*);
 static int r88Write(void*,uint16_t,unsigned,uint32_t);
 static int r88Bulk(void*p,uint8_t q,const uint8_t*b,size_t n){auto*s=owner(p);if(!s->permits88Transport()){s->clear88Discovery(true);return ENXIO;}int e=s->usb_->bulk(q,b,n);if(e)s->fault(e);return e;}
 bool read88IdentityDescriptor(rtw_chip_identity_descriptor*)const;bool matches88Discovery()const;bool permits88Transport(bool=false)const;bool owns88Discovery()const;void clear88Discovery(bool);int prepare88Identity(const rtw88_fw_bus&);int recheck88Identity();int publish88Identity();
 rtw88_mac_context r88MAC_{};rtw88_phy_context r88PHY_{};rtw88_station_context r88Station_{};rtw88_coex*r88Coex_=nullptr;
 rtw88_fw_bus r88AuthorityBus_{},r88AuthorityRaw_{};rtw88_board r88AuthorityBoard_{};uint32_t r88AuthorityEpoch_=0;bool r88AuthorityBound_=false,r88AuthorityComplete_=false;
 std::vector<uint64_t> storage=std::vector<uint64_t>((RTW88_PHY_RADIO_SCRATCH_BYTES+RTW88_FW_SCRATCH_BYTES+RTW88_EFUSE_SCRATCH_BYTES+7)/8);void*r88Storage_=storage.data();bool r88BoardKnown_=false;uint8_t r88MACAddress_[6]{};uint16_t r88PacketSequence_=0;
 int bootstrap88(bool);void remember88Authority(bool);bool owns88Authority(bool)const;void coex88Stop(){}
 bool verified=false;unsigned hardwareCode=0;std::string publishedFamily;
 static RTWUSBController*owner(void*p){return static_cast<RTWUSBController*>(p);}static int ioError(void*p){return owner(p)->usbError_==kIOReturnSuccess?0:EIO;}static void delayUS(void*,uint32_t){}
 void fault(IOReturn);
 bool is8812Family()const{return family_==RTW_WIFI_RTL8812AU||family_==RTW_WIFI_RTL8821AU;}bool is88Family()const{return family_==RTW_WIFI_RTL8821CU||family_==RTW_WIFI_RTL8822BU||family_==RTW_WIFI_RTL8822CU;}
 void setProperty(const char*name,bool v){if(!std::strcmp(name,"RTWChipIdentityVerified"))verified=v;}
 void setProperty(const char*name,unsigned long long v,unsigned bits){if(!std::strcmp(name,"RTWHardwareChipCode")){assert(bits==8);hardwareCode=(unsigned)v;}}
 void setProperty(const char*name,const char*v){if(!std::strcmp(name,"RTWSiliconFamily"))publishedFamily=v;}
 void removeProperty(const char*name){if(!std::strcmp(name,"RTWChipIdentityVerified"))verified=false;if(!std::strcmp(name,"RTWHardwareChipCode"))hardwareCode=0;}
 int vendorRequest(bool,uint16_t,uint8_t*,size_t);uint32_t readRegister(uint16_t,uint8_t);int writeRegister(uint16_t,uint8_t,uint32_t);bool discoverUSB();
};
/* ACTUAL_REGISTRY */
/* ACTUAL_CONTROLLER_METHODS */
struct ProviderSwap{RTWUSBController*c;IOUSBHostInterface*other;};
static void swapProvider(void*p){auto*s=static_cast<ProviderSwap*>(p);s->c->usb_=s->other;}
static void changeDescriptor(void*p){static_cast<RTWUSBController*>(p)->usb_->config[14]=0;}
int main(){
 unsigned cases=0,faults=0;
 // Ordinary immutable B/C profiles perform no added control request at match.
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}){
  IOUSBHostInterface usb(0x0a);RTWUSBController c;c.usb_=&usb;const auto*p=profile(family);assert(p);
  usb.device.descriptor.idVendor=p->vendor;usb.device.descriptor.idProduct=p->product;
  usb.device.descriptor.bcdDevice=p->bcd_device;usb.config[5]=p->configuration;usb.config[11]=p->interface_number;
  assert(c.discoverUSB()&&c.family_==family&&!c.r88ResolvedIdentity_&&!usb.requests);++cases;
 }
 unsigned accepted=0;
 for(unsigned i=0;i<256;i++){
  unsigned code=(i+0x0aU)&255U;IOUSBHostInterface usb(code);RTWUSBController c;c.usb_=&usb;
  bool result=c.discoverUSB();assert(result==(code==0x0a||code==0x13));assert(!usb.writes&&!usb.bulks&&usb.reads==1);
  if(result){++accepted;assert(c.profile_->vendor==0x13b1&&c.profile_->product==0x43&&!c.profile_->primary_verified);assert(c.family_==(code==0x0a?RTW_WIFI_RTL8822BU:RTW_WIFI_RTL8822CU));}
  assert(!c.verified&&!c.hardwareCode);++cases;
 }
 assert(accepted==2);
 for(unsigned variant=0;variant<11;variant++){
  IOUSBHostInterface usb(0x0a);RTWUSBController c;c.usb_=&usb;
  switch(variant){case 0:usb.device.descriptor.idVendor=0xffff;break;case 1:usb.device.descriptor.idProduct=0xffff;break;
   case 2:usb.config[5]=2;break;case 3:usb.config[11]=1;break;case 4:usb.config[14]=0;break;case 5:usb.config[15]=0;break;case 6:usb.config[16]=0;break;
   case 7:usb.missing_device=true;break;case 8:usb.missing_interface=true;break;case 9:usb.missing_configuration=true;break;
   case 10:usb.device.descriptor.idVendor=0x0bda;usb.device.descriptor.idProduct=0x8192;break;}
  assert(!c.discoverUSB()&&!usb.requests&&!c.verified);++cases;
 }
 for(bool shortened:{false,true}){IOUSBHostInterface usb(0x0a);RTWUSBController c;c.usb_=&usb;usb.short_read=shortened;usb.fail_request=shortened?0:1;assert(!c.discoverUSB()&&!usb.writes&&!usb.bulks&&!c.verified);++cases;}
 // A copied Controller cannot consume or revoke the original object's proof.
 {IOUSBHostInterface usb(0x0a);RTWUSBController c;c.usb_=&usb;assert(c.discoverUSB());RTWUSBController clone=c;unsigned before=usb.requests;assert(clone.bootstrap88(false)==ENODEV);assert(usb.requests==before);assert(rtw_chip_identity_profile(c.r88ResolvedIdentity_)==c.profile_);++cases;}
 for(unsigned code:{0x0aU,0x13U}){
  IOUSBHostInterface usb(code);RTWUSBController c;c.usb_=&usb;assert(c.discoverUSB()&&!c.bootstrap88(false));
  assert(c.verified&&c.hardwareCode==code&&c.owns88Authority(false));assert(c.publishedFamily==rtw_usb_wifi_family_name(c.family_));assert(usb.chip_reads==6);
  unsigned before=usb.requests;for(unsigned i=0;i<10000;i++)assert(c.owns88Authority(false));assert(usb.requests==before);
  // Raw cookie, provider, copied profile and copied Core never authorize I/O.
  IOUSBHostInterface other(code);auto*original=c.usb_;c.usb_=&other;assert(!c.owns88Authority(false)&&!other.requests);c.usb_=original;
  void*cookie=c.r88Core_.raw_bus.cookie;c.r88Core_.raw_bus.cookie=&other;assert(!c.owns88Authority(false)&&!other.requests);c.r88Core_.raw_bus.cookie=cookie;
  auto copy=*c.profile_;const auto*original_profile=c.profile_;c.profile_=&copy;assert(!c.owns88Authority(false));c.profile_=original_profile;
  RTWUSBController cloned=c;assert(!cloned.owns88Authority(false)&&usb.requests==before);++cases;
  // Every actual control/bulk transport fault must lose published authority.
  const unsigned operations=usb.requests;
  for(unsigned at=1;at<=operations;at++){
   IOUSBHostInterface bad(code);RTWUSBController failed;failed.usb_=&bad;bad.fail_request=at;
   bool discovered=failed.discoverUSB();int e=discovered?failed.bootstrap88(false):EIO;
   assert(e&&!failed.verified&&!failed.hardwareCode&&!failed.owns88Authority(false));++faults;
  }
  // All256 values at each subsequent silicon check; nothing writes after loss.
  for(unsigned stage=2;stage<=6;stage++)for(unsigned next=0;next<256;next++){
   IOUSBHostInterface changed(code);RTWUSBController checked;checked.usb_=&changed;changed.change_at=stage;changed.change_to=next;
   assert(checked.discoverUSB());int e=checked.bootstrap88(false);
   if(next==code)assert(!e&&checked.verified);
   else{assert(e&&!checked.verified&&!checked.hardwareCode&&!checked.owns88Authority(false));assert(changed.writes+changed.bulks==changed.change_writes);if(stage<=4)assert(!changed.writes&&!changed.bulks);}
   ++cases;
  }
  // Changing provider or descriptors inside any identity callback fails closed.
  for(unsigned stage=1;stage<=6;stage++)for(bool descriptor:{false,true}){
   IOUSBHostInterface changed(code),foreign(code);RTWUSBController checked;checked.usb_=&changed;ProviderSwap swap{&checked,&foreign};
   changed.hook_at=stage;changed.hook=descriptor?changeDescriptor:swapProvider;changed.hook_cookie=descriptor?static_cast<void*>(&checked):static_cast<void*>(&swap);
   bool discovered=checked.discoverUSB();int e=discovered?checked.bootstrap88(false):ENODEV;
   assert(e&&!checked.verified&&!checked.hardwareCode&&!checked.owns88Authority(false)&&!foreign.requests);
   assert(changed.writes+changed.bulks==changed.hook_writes);++cases;
  }
  // Every captured descriptor field must remain bound before a restart.
  for(unsigned field=0;field<9;field++){
   IOUSBHostInterface changed(code);RTWUSBController checked;checked.usb_=&changed;assert(checked.discoverUSB());
   if(field<3){auto&d=changed.device.descriptor;if(field==0)d.idVendor^=1;if(field==1)d.idProduct^=1;if(field==2)d.bcdDevice^=1;}
   else changed.config[field==3?5:field==4?11:field==5?12:field==6?14:field==7?15:16]^=1;
   unsigned calls=changed.requests;assert(checked.bootstrap88(false)==ENODEV&&changed.requests==calls&&!changed.writes&&!changed.bulks&&!checked.verified);++cases;
  }
  // Actual stop power sequence checks silicon again before its first write.
  for(unsigned next=0;next<256;next++)if(next!=code){
   IOUSBHostInterface changed(code);RTWUSBController checked;checked.usb_=&changed;assert(checked.discoverUSB()&&!checked.bootstrap88(false));changed.bus.reg[0xfc]=(uint8_t)next;
   unsigned writes=changed.writes+changed.bulks;assert(rtw88_boot_stop(&checked.r88Core_)==ENODEV);assert(changed.writes+changed.bulks==writes&&!checked.verified&&!checked.owns88Authority(false));++cases;
  }
  c.clear88Discovery(false);assert(!c.verified&&!c.hardwareCode);assert(!rtw88_boot_stop(&c.r88Core_));++cases;
 }
 std::printf("Actual Controller discovery: %u descriptor/code/clone/provider/stop cases,%u transport faults;20,000 ownership checks0USB;real B/C Boot/FW/OTP/features passed\n",cases,faults);
}
