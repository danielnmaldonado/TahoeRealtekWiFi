// Exercise the real diagnostic with controlled IOKit enumeration; no USB opens.
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <unistd.h>
static unsigned fixtureCount=0;
static io_service_t nextService=0;
static kern_return_t fakeGet(mach_port_t,CFDictionaryRef match,io_iterator_t* iterator) {
    int product=0;
    auto value=static_cast<CFNumberRef>(CFDictionaryGetValue(match,CFSTR("idProduct")));
    if(value)CFNumberGetValue(value,kCFNumberIntType,&product);
    nextService=product>=1&&unsigned(product)<=fixtureCount?unsigned(product)+100:0;
    CFRelease(match);*iterator=99;return KERN_SUCCESS;
}
static io_object_t fakeNext(io_iterator_t) {auto service=nextService;nextService=0;return service;}
static kern_return_t fakeRelease(io_object_t) {return KERN_SUCCESS;}
static kern_return_t fakeID(io_registry_entry_t service,uint64_t* id) {*id=service;return KERN_SUCCESS;}
#define IOServiceGetMatchingServices fakeGet
#define IOIteratorNext fakeNext
#define IOObjectRelease fakeRelease
#define IORegistryEntryGetRegistryEntryID fakeID
#define main diagnostic_main
#include "../tools/rtw-usb-match.cpp"
#undef main
int main() {
    char filename[]="/private/tmp/rtw-usb-match-test.XXXXXX";
    int fd=mkstemp(filename);assert(fd>=0);close(fd);
    std::ofstream file(filename);
    file<<"<?xml version=\"1.0\"?><plist version=\"1.0\"><dict><key>IOKitPersonalities</key><dict>";
    // Include a duplicate profile: it must not count the same service twice.
    for(unsigned product:{1u,2u,1u}) {
        static unsigned row=0;
        file<<"<key>"<<++row<<"</key><dict><key>IOProviderClass</key><string>IOUSBHostInterface</string>"
            "<key>idVendor</key><integer>9047</integer><key>idProduct</key><integer>"<<product<<
            "</integer><key>bConfigurationValue</key><integer>1</integer>"
            "<key>bInterfaceNumber</key><integer>0</integer></dict>";
    }
    file<<"</dict></dict></plist>";file.close();
    char command[]="rtw-usb-match";char* argv[]={command,filename,nullptr};
    fixtureCount=0;assert(diagnostic_main(2,argv)==2);
    fixtureCount=1;assert(diagnostic_main(2,argv)==0);
    fixtureCount=2;int result=diagnostic_main(2,argv);unlink(filename);
    if(result!=0){std::fprintf(stderr,"Two compatible USB interfaces must be accepted (got %d)\n",result);return 1;}
    return 0;
}
