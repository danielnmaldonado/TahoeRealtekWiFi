// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#import <Foundation/Foundation.h>
#include "Devices.hpp"
#include <cmath>

namespace rtwui {
inline bool preferenceNumber(id value,uint64_t maximum,bool nonzero=false) {
    if(![value isKindOfClass:NSNumber.class]||CFGetTypeID((__bridge CFTypeRef)value)==CFBooleanGetTypeID())return false;
    const double number=[value doubleValue];
    return std::isfinite(number)&&number>=0&&number<=maximum&&number==[value unsignedLongLongValue]&&(!nonzero||number>0);
}
inline NSDictionary* adapterIdentity(id raw) {
    if(![raw isKindOfClass:NSDictionary.class]||
       !preferenceNumber(raw[@"vendor"],65535,true)||!preferenceNumber(raw[@"product"],65535))return nil;
    NSMutableDictionary* value=[@{@"vendor":raw[@"vendor"],@"product":raw[@"product"]} mutableCopy];
    id serial=raw[@"serial"],mac=raw[@"mac"],location=raw[@"location"];
    if(serial) {
        if(![serial isKindOfClass:NSString.class]||![serial length]||
           [serial lengthOfBytesUsingEncoding:NSUTF8StringEncoding]>128||
           [serial rangeOfCharacterFromSet:NSCharacterSet.controlCharacterSet].location!=NSNotFound)return nil;
        value[@"serial"]=serial;
    }
    if(mac) {
        if(![mac isKindOfClass:NSData.class]||[mac length]!=6)return nil;
        const auto* bytes=static_cast<const uint8_t*>([mac bytes]);
        if(bytes[0]&1U||!(bytes[0]|bytes[1]|bytes[2]|bytes[3]|bytes[4]|bytes[5]))return nil;
        value[@"mac"]=mac;
    }
    if(location) {
        if(!preferenceNumber(location,UINT32_MAX,true))return nil;
        value[@"location"]=location;
    }
    return serial||mac||location?[value copy]:nil;
}
// Rank a durable physical identifier above a USB port. A changed serial or
// known MAC is a different device, even if it occupies the previous port.
inline unsigned adapterIdentityMatch(NSDictionary* preferred,NSDictionary* candidate) {
    if(!preferred||!candidate||![preferred[@"vendor"] isEqual:candidate[@"vendor"]]||
       ![preferred[@"product"] isEqual:candidate[@"product"]])return 0;
    id serial=preferred[@"serial"],mac=preferred[@"mac"],location=preferred[@"location"];
    if(serial&&![serial isEqual:candidate[@"serial"]])return 0;
    if(mac&&![mac isEqual:candidate[@"mac"]])return 0;
    unsigned score=0;
    if(mac&&[mac isEqual:candidate[@"mac"]])score=8;
    if(serial&&[serial isEqual:candidate[@"serial"]])score+=4;
    // A port cannot substitute for an identifier that has not enumerated yet.
    if(serial||mac)return score;
    if(location&&[location isEqual:candidate[@"location"]])score=1;
    return score;
}
inline id registryPreferenceProperty(io_registry_entry_t entry,CFStringRef key) {
    return CFBridgingRelease(IORegistryEntryCreateCFProperty(entry,key,kCFAllocatorDefault,0));
}
inline NSDictionary* readAdapterIdentity(uint64_t registryID) {
    auto matching=IORegistryEntryIDMatching(registryID);if(!matching)return nil;
    io_service_t service=IOServiceGetMatchingService(kIOMainPortDefault,matching);
    if(!service)return nil;
    if(!correctDevice(service)){IOObjectRelease(service);return nil;}
    NSMutableDictionary* value=[NSMutableDictionary dictionary];
    // Search only this controller's descendants; a sibling USB cannot lend MAC.
    id mac=CFBridgingRelease(IORegistryEntrySearchCFProperty(service,kIOServicePlane,CFSTR("IOMACAddress"),
        kCFAllocatorDefault,kIORegistryIterateRecursively));
    if([mac isKindOfClass:NSData.class]&&[mac length]==6)value[@"mac"]=mac;
    io_registry_entry_t entry=service;
    for(unsigned depth=0;entry&&depth<8;++depth) {
        if(IOObjectConformsTo(entry,"IOUSBHostDevice")||IOObjectConformsTo(entry,"IOUSBDevice")) {
            id vendor=registryPreferenceProperty(entry,CFSTR("idVendor"));
            id product=registryPreferenceProperty(entry,CFSTR("idProduct"));
            id serial=registryPreferenceProperty(entry,CFSTR("USB Serial Number"));
            id location=registryPreferenceProperty(entry,CFSTR("locationID"));
            if(vendor)value[@"vendor"]=vendor;if(product)value[@"product"]=product;
            if([serial isKindOfClass:NSString.class]&&[serial length])value[@"serial"]=serial;
            if(preferenceNumber(location,UINT32_MAX,true))value[@"location"]=location;
            IOObjectRelease(entry);entry=IO_OBJECT_NULL;break;
        }
        io_registry_entry_t parent=IO_OBJECT_NULL;
        IORegistryEntryGetParentEntry(entry,kIOServicePlane,&parent);
        IOObjectRelease(entry);entry=parent;
    }
    if(entry)IOObjectRelease(entry);
    return adapterIdentity(value);
}
inline NSDictionary<NSNumber*,NSDictionary*>* readAdapterIdentities(const DeviceList& list) {
    NSMutableDictionary* result=[NSMutableDictionary dictionary];
    if(validDevices(list))for(unsigned i=0;i<list.count;++i) {
        const auto id=list.devices[i].registryID;NSDictionary* identity=readAdapterIdentity(id);
        if(identity)result[@(id)]=identity;
    }
    return result;
}
}
