/* Read-only USB matching diagnostic. Never opens a USB device or loads code. */
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <set>
int main(int argc,char**argv) {
    if(argc!=2){std::fprintf(stderr,"Usage: rtw-usb-match <Info.plist>\n");return 1;}
    std::ifstream in(argv[1],std::ios::binary);
    std::string bytes((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
    if(bytes.empty()||bytes.size()>1024*1024){std::fputs("Invalid plist file\n",stderr);return 1;}
    auto data=CFDataCreate(nullptr,reinterpret_cast<const UInt8*>(bytes.data()),bytes.size());
    auto root=CFPropertyListCreateWithData(nullptr,data,kCFPropertyListImmutable,nullptr,nullptr);CFRelease(data);
    if(!root||CFGetTypeID(root)!=CFDictionaryGetTypeID()){if(root)CFRelease(root);return 1;}
    auto raw=CFDictionaryGetValue(static_cast<CFDictionaryRef>(root),CFSTR("IOKitPersonalities"));
    if(!raw||CFGetTypeID(raw)!=CFDictionaryGetTypeID()){CFRelease(root);return 1;}
    auto personalities=static_cast<CFDictionaryRef>(raw);auto n=CFDictionaryGetCount(personalities);
    if(n<1||n>1024){CFRelease(root);return 1;}
    std::vector<const void*>values(static_cast<size_t>(n));
    CFDictionaryGetKeysAndValues(personalities,nullptr,values.data());
    std::set<uint64_t> services;kern_return_t result=KERN_SUCCESS;
    for(auto value:values){
        if(!value||CFGetTypeID(value)!=CFDictionaryGetTypeID()){result=KERN_INVALID_ARGUMENT;break;}
        auto p=static_cast<CFDictionaryRef>(value);
        auto provider=CFDictionaryGetValue(p,CFSTR("IOProviderClass"));
        if(!provider||CFGetTypeID(provider)!=CFStringGetTypeID()||!CFEqual(provider,CFSTR("IOUSBHostInterface"))){result=KERN_INVALID_ARGUMENT;break;}
        auto match=IOServiceMatching("IOUSBHostInterface");
        if(!match){result=KERN_RESOURCE_SHORTAGE;break;}
        const CFStringRef keys[]={CFSTR("idVendor"),CFSTR("idProduct"),CFSTR("bConfigurationValue"),CFSTR("bInterfaceNumber"),CFSTR("bcdDevice")};
        bool valid=true;
        for(unsigned i=0;i<5;i++){
            auto v=CFDictionaryGetValue(p,keys[i]);
            if(!v&&i==4)continue;
            int number=0;
            if(!v||CFGetTypeID(v)!=CFNumberGetTypeID()||!CFNumberGetValue(static_cast<CFNumberRef>(v),kCFNumberIntType,&number)||number<0||number>((i==2||i==3)?255:65535)){valid=false;break;}
            CFDictionarySetValue(match,keys[i],v);
        }
        if(!valid){CFRelease(match);result=KERN_INVALID_ARGUMENT;break;}
        io_iterator_t it=0;result=IOServiceGetMatchingServices(kIOMainPortDefault,match,&it);
        if(result!=KERN_SUCCESS)break;
        io_service_t service;
        while((service=IOIteratorNext(it))){uint64_t id=0;if(IORegistryEntryGetRegistryEntryID(service,&id)==KERN_SUCCESS)services.insert(id);IOObjectRelease(service);}
        IOObjectRelease(it);
    }
    CFRelease(root);
    std::printf("IOUSBHostInterface: result=0x%x profiles=%ld matched=%zu\n",result,n,services.size());
    return result==KERN_SUCCESS&&!services.empty()?0:2;
}
