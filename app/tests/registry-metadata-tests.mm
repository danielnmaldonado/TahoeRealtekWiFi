// SPDX-License-Identifier: GPL-2.0-only
// Exercise production registry readers; the fixture never accesses IOKit.
#import <Foundation/Foundation.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <cstdio>
#include <cstring>
#include <vector>
static CFMutableDictionaryRef props;
static CFTypeRef fakeProperty(io_registry_entry_t, CFStringRef key, CFAllocatorRef, IOOptionBits){auto v=CFDictionaryGetValue(props,key);return v?CFRetain(v):nullptr;}
static boolean_t fakeConforms(io_object_t,const char*){return true;}
#define IORegistryEntryCreateCFProperty fakeProperty
#define IOObjectConformsTo fakeConforms
#include "../AdapterDiagnostics.mm"
#undef IORegistryEntryCreateCFProperty
#undef IOObjectConformsTo
static rtwui::USBAdapterObservation valid(){
 rtwui::USBAdapterObservation o;o.vendor=0x13b1;o.product=0x0043;o.configuration=1;o.bcdDevice=0x200;
 o.interfaces.push_back({0,1,true,"RTL8822BU",255,255,255,true,0x0a});return o;
}
static CFStringRef nulSuffix(const char*s){std::vector<UniChar> u;for(;*s;s++)u.push_back((unsigned char)*s);u.push_back(0);u.push_back('X');return CFStringCreateWithCharacters(nullptr,u.data(),u.size());}
int main(){@autoreleasepool{
 using S=rtwui::AdapterSupportState;props=CFDictionaryCreateMutable(nullptr,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
 unsigned falseClaims=0;
 for(unsigned mode=0;mode<3;mode++){
  auto o=valid();auto duplicate=o.interfaces[0];if(mode==0)duplicate.interfaceProtocol=-1;if(mode==1)duplicate.interfaceClass=3;if(mode==2)duplicate.configuration=-2;o.interfaces.push_back(duplicate);
  auto result=rtwui::adapterSupport(o);bool claim=result.state==S::DriverRuntimeResolved;
  std::printf("duplicate mode%u: resolved=%d complete=%d family=%s\n",mode,claim,result.completeIdentityMatches,rtw_usb_wifi_family_name(result.resolvedFamily));falseClaims+=claim;
 }
 CFStringRef family=nulSuffix("RTL8822BU");CFDictionarySetValue(props,CFSTR("RTWSiliconFamily"),family);
 CFStringRef bundle=nulSuffix(RTW_CONTROL_BUNDLE_ID);CFDictionarySetValue(props,CFSTR("CFBundleIdentifier"),bundle);
 CFDictionarySetValue(props,CFSTR("RTWChipIdentityVerified"),kCFBooleanTrue);int chip=0x0a;auto number=CFNumberCreate(nullptr,kCFNumberIntType,&chip);CFDictionarySetValue(props,CFSTR("RTWHardwareChipCode"),number);
 auto o=valid();o.interfaces[0].driverBound=sameDriver(1);o.interfaces[0].driverFamily=readText(1,CFSTR("RTWSiliconFamily"));o.interfaces[0].runtimeIdentityVerified=readFlag(1,CFSTR("RTWChipIdentityVerified"));o.interfaces[0].hardwareChipCode=readUnsigned(1,CFSTR("RTWHardwareChipCode"),255);
 auto result=rtwui::adapterSupport(o);std::printf("embeddedNUL: fullFamilyLength=%ld parsedFamily=%s sameDriver=%d resolved=%d\n",(long)CFStringGetLength(family),o.interfaces[0].driverFamily.c_str(),o.interfaces[0].driverBound,result.state==S::DriverRuntimeResolved);falseClaims+=result.state==S::DriverRuntimeResolved;
 CFDictionarySetValue(props,CFSTR("RTWChipIdentityVerified"),number);std::printf("CFNumber verification flag rejected=%d\n",!readFlag(1,CFSTR("RTWChipIdentityVerified")));
 CFRelease(family);CFRelease(bundle);CFRelease(number);CFRelease(props);std::printf("strict identity false claims=%u\n",falseClaims);return falseClaims?1:0;
}}
