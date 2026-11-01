/* SPDX-License-Identifier: GPL-2.0-only
 * Bounded IOKit diagnostic for the connected TP-Link2357:0106 only.
 * USB vendor IN reads and cached descriptors. No OUT, interface claim, configuration,
 * reset, firmware, eFuse triggers, power or radio operations. */
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <cstdio>
#include <cstdint>
#include <initializer_list>
static void number(CFMutableDictionaryRef d,CFStringRef key,int value){auto n=CFNumberCreate(nullptr,kCFNumberIntType,&value);CFDictionarySetValue(d,key,n);CFRelease(n);}
int main(){
    auto match=IOServiceMatching(kIOUSBDeviceClassName);if(!match)return 1;
    number(match,CFSTR("idVendor"),0x2357);number(match,CFSTR("idProduct"),0x0106);
    io_iterator_t iterator=0;auto result=IOServiceGetMatchingServices(kIOMainPortDefault,match,&iterator);
    if(result!=KERN_SUCCESS)return 2;
    io_service_t service=IOIteratorNext(iterator);
    auto second=IOIteratorNext(iterator);IOObjectRelease(iterator);
    if(!service||second){if(service)IOObjectRelease(service);if(second)IOObjectRelease(second);std::fputs("Expected exactly one2357:0106 device\n",stderr);return 2;}
    IOCFPlugInInterface**plugin=nullptr;SInt32 score=0;
    result=IOCreatePlugInInterfaceForService(service,kIOUSBDeviceUserClientTypeID,kIOCFPlugInInterfaceID,&plugin,&score);IOObjectRelease(service);
    if(result!=KERN_SUCCESS||!plugin){std::printf("PLUGIN_ERROR=0x%x\n",result);return 3;}
    IOUSBDeviceInterface500**device=nullptr;
    auto query=(*plugin)->QueryInterface(plugin,CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID500),reinterpret_cast<LPVOID*>(&device));
    (*plugin)->Release(plugin);
    if(query||!device){std::printf("QUERY_ERROR=0x%x\n",static_cast<unsigned>(query));return 3;}
    UInt16 vendor=0,product=0;
    result=(*device)->GetDeviceVendor(device,&vendor);
    if(result==kIOReturnSuccess)result=(*device)->GetDeviceProduct(device,&product);
    if(result!=kIOReturnSuccess||vendor!=0x2357||product!=0x0106){(*device)->Release(device);return 3;}
    result=(*device)->USBDeviceOpen(device);
    if(result!=kIOReturnSuccess){std::printf("OPEN_ERROR=0x%x\n",result);(*device)->Release(device);return 4;}
    std::printf("USB=%04x:%04x READ_ONLY=1\n",vendor,product);
    UInt8 configuration=0;result=(*device)->GetConfiguration(device,&configuration);
    std::printf("CONFIGURATION=%u RESULT=0x%x\n",configuration,result);
    IOUSBConfigurationDescriptorPtr config=nullptr;
    if((*device)->GetConfigurationDescriptorPtr(device,0,&config)==kIOReturnSuccess&&config){
        const auto *b=reinterpret_cast<const uint8_t*>(config);size_t length=size_t(b[2])|(size_t(b[3])<<8);
        if(length>=9&&length<=4096){
            for(size_t pos=0;pos+2<=length;){
                unsigned n=b[pos];if(n<2||n>length-pos)break;
                if(b[pos+1]==4&&n>=9)std::printf("INTERFACE=%u ALT=%u ENDPOINTS=%u\n",b[pos+2],b[pos+3],b[pos+4]);
                if(b[pos+1]==5&&n>=7)std::printf("ENDPOINT=0x%02x TYPE=%u MPS=%u\n",b[pos+2],b[pos+3]&3,unsigned(b[pos+4])|(unsigned(b[pos+5])<<8));
                pos+=n;
            }
        }
    }
    int exitCode=0;
    for(UInt16 address:{UInt16(0x00f0),UInt16(0x00fc),UInt16(0x00ec)}){
        uint8_t bytes[4]{};IOUSBDevRequestTO request{};
        request.bmRequestType=0xc0;request.bRequest=5;request.wValue=address;request.wIndex=0;
        request.wLength=4;request.pData=bytes;request.noDataTimeout=500;request.completionTimeout=1000;
        result=(*device)->DeviceRequestTO(device,&request);
        if(result!=kIOReturnSuccess||request.wLenDone!=4){std::printf("READ_ERROR ADDRESS=0x%04x RESULT=0x%x BYTES=%u\n",address,result,unsigned(request.wLenDone));exitCode=5;break;}
        uint32_t value=uint32_t(bytes[0])|(uint32_t(bytes[1])<<8)|(uint32_t(bytes[2])<<16)|(uint32_t(bytes[3])<<24);
        std::printf("REGISTER=0x%04x VALUE=0x%08x\n",address,value);
        if(address==0xf0)std::printf("CUT=%u TEST_SILICON=%u VENDOR_FIELD=%u\n",(value>>12)&15,(value>>23)&1,(value>>16)&15);
    }
    (*device)->USBDeviceClose(device);(*device)->Release(device);return exitCode;
}
