// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "Protocol.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <algorithm>
#include <cstdio>
namespace rtwui {
inline bool correctDevice(io_service_t service) {
    if(!IOObjectConformsTo(service,RTW_CONTROL_SERVICE_CLASS))return false;
    CFTypeRef bundle=IORegistryEntryCreateCFProperty(service,CFSTR("CFBundleIdentifier"),kCFAllocatorDefault,0);
    bool correct=bundle&&CFGetTypeID(bundle)==CFStringGetTypeID()&&CFEqual(bundle,CFSTR(RTW_CONTROL_BUNDLE_ID));
    if(bundle)CFRelease(bundle);return correct;
}
inline uint32_t listDevices(DeviceList& output) {
    output={};output.version=ipcVersion;
    auto matching=IOServiceMatching(RTW_CONTROL_SERVICE_CLASS);if(!matching)return kIOReturnNoMemory;
    io_iterator_t iterator=IO_OBJECT_NULL;
    auto error=IOServiceGetMatchingServices(kIOMainPortDefault,matching,&iterator);if(error)return error;
    bool excess=false;io_service_t service;
    while((service=IOIteratorNext(iterator))){
        uint64_t id=0;
        if(correctDevice(service)&&IORegistryEntryGetRegistryEntryID(service,&id)==KERN_SUCCESS&&id){
            if(output.count==maxDevices)excess=true;
            else {
                auto& record=output.devices[output.count++];record.registryID=id;
                std::snprintf(record.family,sizeof(record.family),"Realtek USB Wi-Fi");
                CFTypeRef family=IORegistryEntryCreateCFProperty(service,CFSTR("RTWSiliconFamily"),kCFAllocatorDefault,0);
                if(family&&CFGetTypeID(family)==CFStringGetTypeID()){
                    char label[sizeof(record.family)]{};
                    if(CFStringGetCString(static_cast<CFStringRef>(family),label,sizeof(label),kCFStringEncodingUTF8)&&label[0])
                        std::memcpy(record.family,label,sizeof(record.family));
                }
                if(family)CFRelease(family);
            }
        }
        IOObjectRelease(service);
    }
    IOObjectRelease(iterator);
    if(excess){output={};return kIOReturnNoResources;}
    std::sort(output.devices,output.devices+output.count,[](const auto&a,const auto&b){return a.registryID<b.registryID;});
    return 0;
}
inline uint32_t openDevice(uint64_t registryID,io_connect_t& handle) {
    handle=IO_OBJECT_NULL;if(!registryID)return kIOReturnBadArgument;
    auto matching=IORegistryEntryIDMatching(registryID);if(!matching)return kIOReturnNoMemory;
    auto service=IOServiceGetMatchingService(kIOMainPortDefault,matching);
    if(!service)return kIOReturnNotFound;
    uint64_t actual=0;
    if(!correctDevice(service)||IORegistryEntryGetRegistryEntryID(service,&actual)!=KERN_SUCCESS||actual!=registryID){
        IOObjectRelease(service);return kIOReturnNotFound;
    }
    auto error=IOServiceOpen(service,mach_task_self(),RTW_CONTROL_USER_CLIENT_TYPE,&handle);
    IOObjectRelease(service);if(error)handle=IO_OBJECT_NULL;
    return error;
}
}
