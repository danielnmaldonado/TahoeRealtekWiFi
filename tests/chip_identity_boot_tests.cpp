/* SPDX-License-Identifier: GPL-2.0-only. Actual Boot constructor and guards. */
#include "rtw/RTW88Boot.h"
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <initializer_list>
struct Bus{unsigned code,reads=0,writes=0;int fail=0;static int read(void*p,uint16_t a,unsigned w,uint32_t*v){auto&b=*(Bus*)p;++b.reads;*v=a==0xfc?b.code:a==0xf0?0x08000000:0;if(b.fail==int(b.reads))return EIO;assert((a==0xfc&&(w==1||w==4))||(a==0xf0&&w==4)||(a==4&&w==4));return 0;}static int write(void*p,uint16_t,unsigned,uint32_t){++((Bus*)p)->writes;assert(0);return EIO;}static void delay(void*,uint32_t){assert(0);}rtw88_fw_bus ops(){return {this,read,write,nullptr,delay,nullptr,nullptr};}};
int main(){
 const auto*origin=rtw_usb_wifi_lookup(0x13b1,0x43,0,1,0);assert(origin&&!origin->primary_verified);
 const rtw_chip_identity_descriptor d{0x13b1,0x43,0,1,0,255,255,255,0};unsigned cases=0;
 for(unsigned positive:{0x0aU,0x13U})for(unsigned next=0;next<256;++next){
  alignas(max_align_t)uint8_t storage[RTW_CHIP_IDENTITY_STORAGE_BYTES]{};Bus b{positive};auto bus=b.ops();rtw_chip_identity*proof=nullptr;rtw88_boot_context c{};
  assert(!rtw_chip_identity_init(storage,sizeof(storage),origin,&d,&bus,&proof));const auto*p=rtw_chip_identity_profile(proof);
  assert(rtw88_boot_init(&c,p,&bus)==ENODEV);auto before=b.reads;assert(!rtw88_boot_init_resolved(&c,proof,&bus)&&b.reads==before);assert(c.identity==p&&c.identity->vendor==0x13b1&&c.identity->product==0x43&&!c.identity->primary_verified);
  {auto alias=c;alias.firmware.bus.cookie=&alias;auto before=b.reads;assert(rtw88_boot_identify(&alias)==EINVAL&&b.reads==before);}
  assert(!rtw88_boot_identify(&c)&&c.firmware.identified);
  b.code=next;
  if(next==positive){assert(!rtw88_boot_revalidate_identity(&c));}
  else{assert(rtw88_boot_power_on(&c)==ENODEV);assert(!c.firmware.identified&&!c.firmware.powered&&!c.firmware.firmware_running&&!rtw_chip_identity_profile(proof));assert(rtw88_boot_stop(&c)!=0);}
  assert(!b.writes);++cases;
 }
 for(unsigned where=1;where<=5;++where){alignas(max_align_t)uint8_t storage[RTW_CHIP_IDENTITY_STORAGE_BYTES]{};Bus b{0x0a};auto bus=b.ops();rtw_chip_identity*proof=nullptr;rtw88_boot_context c{};assert(!rtw_chip_identity_init(storage,sizeof(storage),origin,&d,&bus,&proof));assert(!rtw88_boot_init_resolved(&c,proof,&bus));b.fail=int(b.reads+where);int e=rtw88_boot_identify(&c);if(!e)e=rtw88_boot_revalidate_identity(&c);if(!e)e=rtw88_boot_revalidate_identity(&c);assert(e==EIO&&!b.writes&&!c.firmware.identified);++cases;}
 std::printf("Chip identity actual Boot: %u constructor/code-reread/read-error cases;zero power/FW writes on negative discovery passed\n",cases);
}
