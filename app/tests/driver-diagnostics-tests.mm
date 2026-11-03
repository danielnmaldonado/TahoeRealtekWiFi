// SPDX-License-Identifier: GPL-2.0-only
#import <Foundation/Foundation.h>
#include <IOKit/IOKitLib.h>
#include <cstdio>
#include <cstring>
#if !__has_include("../DriverDiagnostics.mm")
int main(){std::fputs("FAIL: coherent on-demand RX report unavailable\n",stderr);return 1;}
#else
static CFMutableDictionaryRef fixture;
static bool absent=false,conforming=true,failRead=false,floatGeneration=false,boolGeneration=false,classChecked=false;
static uint64_t selectedID=0;
static int64_t before=12,after=12;
static unsigned reads=0,lookups=0,releases=0;
static io_service_t fakeLookup(mach_port_t,CFDictionaryRef d){++lookups;if(d)CFRelease(d);return absent?0:1;}
static boolean_t fakeConforms(io_object_t,const io_name_t name){classChecked=!std::strcmp(name,"RTWUSBController");return conforming&&classChecked;}
static CFMutableDictionaryRef fakeMatching(uint64_t id){selectedID=id;return CFDictionaryCreateMutable(nullptr,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);}
static kern_return_t fakeRelease(io_object_t){++releases;return KERN_SUCCESS;}
static kern_return_t fakeProperties(io_registry_entry_t,CFMutableDictionaryRef*d,CFAllocatorRef,IOOptionBits){if(failRead)return KERN_FAILURE;*d=CFDictionaryCreateMutableCopy(nullptr,0,fixture);return KERN_SUCCESS;}
static CFTypeRef fakeProperty(io_registry_entry_t,CFStringRef,CFAllocatorRef,IOOptionBits){int64_t n=reads++?after:before;if(boolGeneration)return CFRetain(kCFBooleanTrue);if(floatGeneration){double d=n;return CFNumberCreate(nullptr,kCFNumberDoubleType,&d);}return CFNumberCreate(nullptr,kCFNumberSInt64Type,&n);}
#define IORegistryEntryIDMatching fakeMatching
#define IOServiceOpen report_must_never_open_a_user_client
#define IOServiceGetMatchingService fakeLookup
#define IOObjectConformsTo fakeConforms
#define IOObjectRelease fakeRelease
#define IORegistryEntryCreateCFProperties fakeProperties
#define IORegistryEntryCreateCFProperty fakeProperty
#include "../DriverDiagnostics.mm"
#undef IORegistryEntryIDMatching
#undef IOServiceOpen
#undef IOServiceGetMatchingService
#undef IOObjectConformsTo
#undef IOObjectRelease
#undef IORegistryEntryCreateCFProperties
#undef IORegistryEntryCreateCFProperty
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL%d: %s\n",__LINE__,#x);return 1;}}while(0)
static void put(CFStringRef k,int64_t n){auto v=CFNumberCreate(nullptr,kCFNumberSInt64Type,&n);CFDictionarySetValue(fixture,k,v);CFRelease(v);}
static void reset(){CFDictionaryRemoveAllValues(fixture);absent=false;conforming=true;failRead=false;floatGeneration=boolGeneration=classChecked=false;selectedID=0;before=after=12;reads=lookups=releases=0;put(CFSTR("RTWDiagPublicationGeneration"),12);}
int main(){@autoreleasepool{
 fixture=CFDictionaryCreateMutable(nullptr,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);reset();
 put(CFSTR("RTWDiagDriverBuild"),64);put(CFSTR("RTWDiagRXErrors"),12);put(CFSTR("RTWDiagRXBlockAckFiltered"),89);put(CFSTR("RTWDiagRXCRCDrops"),2);put(CFSTR("RTWDiagRXRejectControlSubtype9"),3);put(CFSTR("RTWDiagRXRejectControlSubtype13"),1);CFDictionarySetValue(fixture,CFSTR("SecretSSID"),CFSTR("DO-NOT-EXPORT"));
 NSString*r=RTWDriverDiagnosticsReport(1);CHECK([r containsString:@"Driver build: 64"]);CHECK([r containsString:@"RX errors: 12"]);CHECK([r containsString:@"Validated BlockAck filtered: 89"]);CHECK([r containsString:@"CRC: 2"]);CHECK([r containsString:@"BlockAck: 3"]);CHECK([r containsString:@"ACK: 1"]);CHECK(![r containsString:@"DO-NOT-EXPORT"]);CHECK(reads==2&&lookups==1&&releases==1&&selectedID==1&&classChecked);
 reset();r=RTWDriverDiagnosticsReport(0);CHECK(lookups==0);CHECK([r containsString:@"No adapter selected"]);
 reset();absent=true;r=RTWDriverDiagnosticsReport(1);CHECK([r containsString:@"unavailable"]);CHECK(releases==0);
 reset();conforming=false;r=RTWDriverDiagnosticsReport(1);CHECK([r containsString:@"unavailable"]);CHECK(releases==1);
 reset();failRead=true;r=RTWDriverDiagnosticsReport(1);CHECK([r containsString:@"unavailable"]);CHECK(releases==1);
 for(unsigned m=0;m<4;++m){reset();if(m==0)before=11;if(m==1)after=14;if(m==2)put(CFSTR("RTWDiagPublicationGeneration"),10);if(m==3)before=after=0;put(CFSTR("RTWDiagRXErrors"),777);r=RTWDriverDiagnosticsReport(1);CHECK([r containsString:@"snapshot unavailable"]);CHECK(![r containsString:@"777"]);CHECK(releases==1);}
 reset();CFDictionarySetValue(fixture,CFSTR("RTWDiagRXErrors"),kCFBooleanTrue);put(CFSTR("RTWDiagRXCRCDrops"),-1);double d=1.5;auto f=CFNumberCreate(nullptr,kCFNumberDoubleType,&d);CFDictionarySetValue(fixture,CFSTR("RTWDiagRXICVDrops"),f);CFRelease(f);r=RTWDriverDiagnosticsReport(1);CHECK([r containsString:@"RX errors: not reported"]);CHECK([r containsString:@"CRC: not reported"]);CHECK([r containsString:@"ICV: not reported"]);CHECK([r containsString:@"Driver build: not reported"]);
 reset();put(CFSTR("RTWDiagRXRejectControlSubtype0"),0);put(CFSTR("RTWDiagRXAggregateMalformed"),4);put(CFSTR("RTWDiagRXInvalidFrameDrops"),5);put(CFSTR("RTWDiagRXHardwareDecryptDrops"),6);put(CFSTR("RTWDiagRXAllocationDrops"),7);put(CFSTR("RTWDiagRXCopyDrops"),8);put(CFSTR("RTWDiagRXNoNodeDrops"),9);put(CFSTR("RTWDiagRXRecoveryAttempts"),10);put(CFSTR("RTWDiagRXRecoveryCompleted"),11);put(CFSTR("RTWDiagRXRecoveryFailures"),12);put(CFSTR("RTWDiagRXRecoveryExhausted"),13);put(CFSTR("RTWDiagRXRejectControlSubtype16"),765);put(CFSTR("RTWDiagArbitraryUnknown"),555);CFDictionarySetValue(fixture,CFSTR("RTWDiagRXRejectLastReceiver"),CFSTR("DO-NOT-EXPORT-MAC"));
 r=RTWDriverDiagnosticsReport(1);CHECK([r containsString:@"Malformed aggregate: 4"]);CHECK([r containsString:@"Invalid frame: 5"]);CHECK([r containsString:@"Hardware decrypt: 6"]);CHECK([r containsString:@"Allocation: 7"]);CHECK([r containsString:@"Copy: 8"]);CHECK([r containsString:@"Missing node: 9"]);CHECK([r containsString:@"Recovery attempts: 10"]);CHECK([r containsString:@"Recovery completed: 11"]);CHECK([r containsString:@"Recovery failures: 12"]);CHECK([r containsString:@"Recovery exhausted: 13"]);CHECK(![r containsString:@"765"]&&![r containsString:@"555"]&&![r containsString:@"DO-NOT-EXPORT-MAC"]);CHECK(![r containsString:@"Control subtype 0: 0"]);
 for(unsigned m=0;m<6;++m){reset();if(m==0)before=-2;if(m==1)after=-2;if(m==2)floatGeneration=true;if(m==3)boolGeneration=true;if(m==4)CFDictionarySetValue(fixture,CFSTR("RTWDiagPublicationGeneration"),kCFBooleanTrue);if(m==5)after=11;put(CFSTR("RTWDiagRXErrors"),777);r=RTWDriverDiagnosticsReport(1);CHECK([r containsString:@"snapshot unavailable"]);CHECK(![r containsString:@"777"]);}
 CFRelease(fixture);std::printf("RX report: %u coherent read-only checks passed\n",checks);return 0;
}}
#endif
