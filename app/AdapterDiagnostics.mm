// SPDX-License-Identifier: GPL-2.0-only
#import "AdapterDiagnostics.h"
#include "AdapterSupport.hpp"
#include "RTWControl.h"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>

namespace {
class RegistryObject {
public:
    explicit RegistryObject(io_object_t object = IO_OBJECT_NULL) : object_(object) {}
    ~RegistryObject() { if (object_) IOObjectRelease(object_); }
    RegistryObject(const RegistryObject&) = delete;
    RegistryObject& operator=(const RegistryObject&) = delete;
    RegistryObject(RegistryObject&& other) noexcept : object_(other.object_) {
        other.object_ = IO_OBJECT_NULL;
    }
    RegistryObject& operator=(RegistryObject&& other) noexcept {
        if (this != &other) {
            if (object_) IOObjectRelease(object_);
            object_ = other.object_;
            other.object_ = IO_OBJECT_NULL;
        }
        return *this;
    }
    io_object_t get() const { return object_; }
private:
    io_object_t object_;
};

int readUnsigned(io_registry_entry_t entry, CFStringRef key, int maximum) {
    CFTypeRef raw = IORegistryEntryCreateCFProperty(entry, key, kCFAllocatorDefault, 0);
    int64_t value = -1;
    const bool valid = raw && CFGetTypeID(raw) == CFNumberGetTypeID() &&
        CFNumberGetValue(static_cast<CFNumberRef>(raw), kCFNumberSInt64Type, &value) &&
        value >= 0 && value <= maximum;
    if (raw) CFRelease(raw);
    return valid ? static_cast<int>(value) : -1;
}

bool readFlag(io_registry_entry_t entry, CFStringRef key) {
    CFTypeRef raw=IORegistryEntryCreateCFProperty(entry,key,kCFAllocatorDefault,0);
    const bool value=raw&&CFGetTypeID(raw)==CFBooleanGetTypeID()&&CFBooleanGetValue(static_cast<CFBooleanRef>(raw));
    if(raw)CFRelease(raw);return value;
}

std::string readText(io_registry_entry_t entry, CFStringRef key) {
    CFTypeRef raw = IORegistryEntryCreateCFProperty(entry, key, kCFAllocatorDefault, 0);
    char text[128]{};
    CFIndex used=0;bool valid=false;
    if(raw&&CFGetTypeID(raw)==CFStringGetTypeID()){
        const auto string=static_cast<CFStringRef>(raw);const CFIndex length=CFStringGetLength(string);
        valid=CFStringGetBytes(string,CFRangeMake(0,length),kCFStringEncodingUTF8,0,false,
            reinterpret_cast<UInt8*>(text),sizeof(text),&used)==length&&used<static_cast<CFIndex>(sizeof(text))&&
            std::memchr(text,0,static_cast<size_t>(used))==nullptr;
    }
    if(raw)CFRelease(raw);
    return valid?std::string(text,static_cast<size_t>(used)):std::string{};
}

bool sameDriver(io_registry_entry_t entry) {
    return IOObjectConformsTo(entry, RTW_CONTROL_SERVICE_CLASS) &&
        readText(entry, CFSTR("CFBundleIdentifier")) == RTW_CONTROL_BUNDLE_ID;
}

// Inspect only the children of THIS interface. Never cross into another USB
// device/interface, so a sibling adapter cannot lend its driver to this one.
void inspectDriver(io_registry_entry_t iface, rtwui::USBInterfaceObservation& observation,
                   bool& partial) {
    io_iterator_t raw = IO_OBJECT_NULL;
    const kern_return_t error = IORegistryEntryGetChildIterator(iface, kIOServicePlane, &raw);
    RegistryObject iterator(raw);
    if (error != KERN_SUCCESS || !raw) {
        partial = true;
        return;
    }
    unsigned visited = 0, drivers = 0;
    io_object_t child;
    while ((child = IOIteratorNext(iterator.get()))) {
        RegistryObject held(child);
        if (++visited > 256) { partial = true; break; }
        if (IOObjectConformsTo(child, "IOUSBHostDevice") ||
            IOObjectConformsTo(child, "IOUSBHostInterface")) continue;
        if (sameDriver(child)) {
            ++drivers;
            observation.driverBound = true;
            observation.driverFamily = readText(child, CFSTR("RTWSiliconFamily"));
            observation.runtimeIdentityVerified=readFlag(child,CFSTR("RTWChipIdentityVerified"));
            observation.hardwareChipCode=readUnsigned(child,CFSTR("RTWHardwareChipCode"),255);
        }
    }
    if (drivers > 1) {
        observation.driverFamily = "AMBIGUOUS";
        partial = true;
    }
    if (!IOIteratorIsValid(iterator.get())) partial = true;
}

void inspectInterfaces(io_registry_entry_t device, rtwui::USBAdapterObservation& observation,
                       bool& partial) {
    io_iterator_t raw = IO_OBJECT_NULL;
    const kern_return_t error = IORegistryEntryGetChildIterator(device, kIOServicePlane, &raw);
    RegistryObject iterator(raw);
    if (error != KERN_SUCCESS || !raw) {
        partial = true;
        return;
    }
    unsigned visited = 0;
    io_object_t child;
    while ((child = IOIteratorNext(iterator.get()))) {
        RegistryObject held(child);
        if (++visited > 256) { partial = true; break; }
        if (!IOObjectConformsTo(child, "IOUSBHostInterface")) continue;
        rtwui::USBInterfaceObservation iface;
        iface.number = readUnsigned(child, CFSTR("bInterfaceNumber"), 255);
        iface.configuration = readUnsigned(child, CFSTR("bConfigurationValue"), 255);
        iface.interfaceClass = readUnsigned(child, CFSTR("bInterfaceClass"), 255);
        iface.interfaceSubclass = readUnsigned(child, CFSTR("bInterfaceSubClass"), 255);
        iface.interfaceProtocol = readUnsigned(child, CFSTR("bInterfaceProtocol"), 255);
        inspectDriver(child, iface, partial);
        observation.interfaces.push_back(std::move(iface));
    }
    if (!IOIteratorIsValid(iterator.get())) partial = true;
}

NSString* utf8(const char* value) {
    return value ? ([NSString stringWithUTF8String:value] ?: @"[Invalid text]") : @"";
}

NSString* interfaceConstraints(const rtw_usb_wifi_profile* p) {
    if (!p->interface_match_flags) return @"class/subclass/protocol: any";
    const uint8_t values[] = {p->interface_class, p->interface_subclass, p->interface_protocol};
    NSMutableArray<NSString*>* parts = [NSMutableArray arrayWithCapacity:3];
    for (unsigned n = 0; n < 3; ++n)
        [parts addObject:(p->interface_match_flags & (1u << n)) ?
            [NSString stringWithFormat:@"0x%02x", static_cast<unsigned>(values[n])] : @"any"];
    return [NSString stringWithFormat:@"class/subclass/protocol: %@", [parts componentsJoinedByString:@"/"]];
}

NSString* description(int value, bool hex) {
    if (value < 0) return @"unavailable";
    return hex ? [NSString stringWithFormat:@"0x%04x", static_cast<unsigned>(value)] :
                 [NSString stringWithFormat:@"%d", value];
}
NSString* radioBands(uint8_t mask) {
    if ((mask & (RTW_WIFI_BAND_2GHZ | RTW_WIFI_BAND_5GHZ)) ==
        (RTW_WIFI_BAND_2GHZ | RTW_WIFI_BAND_5GHZ)) return @"2.4 + 5 GHz";
    if (mask & RTW_WIFI_BAND_2GHZ) return @"2.4 GHz";
    if (mask & RTW_WIFI_BAND_5GHZ) return @"5 GHz";
    return @"none / unverified";
}

NSString* radioCapabilities(const rtw_usb_wifi_profile* p) {
    const auto c=rtw_wifi_radio_profile_capabilities(p);
    return [NSString stringWithFormat:
        @"Chip family reference: %@ · PHY ceiling %u Mbps · up to %u stream(s).\n"
         "Driver station: %@ · PHY ceiling %u Mbps · 2.4 GHz up to %u MHz · 5 GHz up to %u MHz.\n"
         "These are nominal silicon/implementation ceilings, not measured throughput or device certification.%@",
        radioBands(c.family_bands),static_cast<unsigned>(c.family_max_phy_mbps),
        static_cast<unsigned>(c.family_max_nss),radioBands(c.station_bands),
        static_cast<unsigned>(c.station_max_phy_mbps),
        static_cast<unsigned>(c.station_max_2g_width_mhz),
        static_cast<unsigned>(c.station_max_5g_width_mhz),
        c.identity_verified?@"":@" Exact USB chipset identity is unverified."];
}
} // namespace

NSString* RTWAdapterSupportReport(void) {
    @autoreleasepool {
        NSMutableString* report = [NSMutableString stringWithString:
            @"TAHOE WI-FI USB — ADAPTERS AND SUPPORT\n\n"];
        size_t enabled = 0;
        for (size_t i = 0; i < rtw_usb_wifi_profile_count(); ++i)
            enabled += rtw_usb_wifi_station_backend(rtw_usb_wifi_profile_at(i)) != RTW_WIFI_UNKNOWN;
        [report appendFormat:@"Catalog: %zu USB IDs; %zu profiles enabled in this build.\n",
            rtw_usb_wifi_profile_count(), enabled];
        [report appendString:
            @"USB detection does not imply a complete driver. A bound kernel extension does not confirm a connection.\n"
             "The enumeration below is read-only and does not require helper approval.\n"
             "Only catalog devices and Realtek vendor IDs are listed.\n\n"];
        auto matching = IOServiceMatching("IOUSBHostDevice");
        if (!matching) {
            [report appendString:@"Could not create the USB query.\n"];
            return report;
        }
        io_iterator_t raw = IO_OBJECT_NULL;
        const kern_return_t error = IOServiceGetMatchingServices(kIOMainPortDefault, matching, &raw);
        RegistryObject iterator(raw);
        if (error != KERN_SUCCESS || !raw) {
            [report appendFormat:@"USB enumeration failed or its iterator is unavailable: 0x%08x.\n",
                static_cast<unsigned>(error)];
            return report;
        }
        unsigned visited = 0, included = 0;
        bool partial = false;
        io_object_t device;
        while ((device = IOIteratorNext(iterator.get()))) {
            RegistryObject held(device);
            if (++visited > 4096) { partial = true; break; }
            const int vendor = readUnsigned(device, CFSTR("idVendor"), 65535);
            const int product = readUnsigned(device, CFSTR("idProduct"), 65535);
            if (vendor < 0 || product < 0) { partial = true; continue; }
            const auto* profile = rtwui::adapterCatalogEntry(
                static_cast<uint16_t>(vendor), static_cast<uint16_t>(product));
            if (!profile && vendor != 0x0bda) continue;
            if (included >= 64) { partial = true; break; }
            ++included;
            rtwui::USBAdapterObservation observation;
            observation.vendor = static_cast<uint16_t>(vendor);
            observation.product = static_cast<uint16_t>(product);
            if (IORegistryEntryGetRegistryEntryID(device, &observation.registryID) != KERN_SUCCESS)
                partial = true;
            observation.bcdDevice = readUnsigned(device, CFSTR("bcdDevice"), 65535);
            observation.configuration = readUnsigned(device, CFSTR("bConfigurationValue"), 255);
            bool adapterPartial=false;inspectInterfaces(device, observation, adapterPartial);
            observation.registryPartial=adapterPartial;partial=partial||adapterPartial;
            const auto status = rtwui::adapterSupport(observation);
            [report appendFormat:@"#%u — %04x:%04x · registry ID 0x%llx\n", included,
                static_cast<unsigned>(vendor), static_cast<unsigned>(product),
                static_cast<unsigned long long>(observation.registryID)];
            [report appendFormat:@"Revision: %@ · device configuration: %@\n",
                description(observation.bcdDevice, true), description(observation.configuration, false)];
            for (const auto& iface : observation.interfaces) {
                [report appendFormat:@"Interface: %@ · configuration: %@ · project kernel extension: %@\n",
                    description(iface.number, false), description(iface.configuration, false),
                    iface.driverBound ? @"bound" : @"not bound"];
                [report appendFormat:@"Observed class/subclass/protocol: %@/%@/%@\n",
                    description(iface.interfaceClass, true), description(iface.interfaceSubclass, true),
                    description(iface.interfaceProtocol, true)];
            }
            if (profile) {
                [report appendFormat:@"Catalog profile: %@ · %@: %@\n", utf8(profile->name),
                    profile->primary_verified?@"family":@"family candidate",utf8(rtw_usb_wifi_family_name(profile->family))];
                if(status.resolvedFamily!=RTW_WIFI_UNKNOWN)
                    [report appendFormat:@"Runtime family: %@\n",utf8(rtw_usb_wifi_family_name(status.resolvedFamily))];
                [report appendFormat:@"Expected: configuration %u · interface %u · revision %@\n",
                    static_cast<unsigned>(profile->configuration),
                    static_cast<unsigned>(profile->interface_number),
                    profile->match_bcd ? description(profile->bcd_device, true) : @"any"];
                [report appendFormat:@"Expected %@\n", interfaceConstraints(profile)];
            }
            [report appendFormat:@"State: %@\n", utf8(rtwui::adapterSupportDescription(status.state))];
            if (profile) {
                [report appendFormat:@"%@\n", utf8(rtwui::adapterCapabilityDescription(status.resolvedFamily!=RTW_WIFI_UNKNOWN?status.resolvedFamily:profile->family))];
                if(status.state==rtwui::AdapterSupportState::DriverRuntimeResolved)
                    [report appendString:@"Family selected from checked live silicon identity; the catalog remains conflicting.\n"];
                else [report appendFormat:@"%@\n", radioCapabilities(profile)];
            }
            if (status.state == rtwui::AdapterSupportState::EligibleNotBound)
                [report appendString:
                    @"Check the kernel extension version, loading and initialization errors. "
                     "Reinstalling the app alone does not install the kernel extension.\n"];
            [report appendString:@"\n"];
        }
        if (!IOIteratorIsValid(iterator.get())) partial = true;
        if (!included) [report appendString:@"No catalog device or Realtek vendor device was found.\n"];
        if (partial) [report appendString:
            @"WARNING: enumeration was partial, limited or changed during reading; repeat diagnostics.\n"];
        [report appendString:
            @"This report does not read passwords, Keychain, SSIDs, BSSIDs, MAC addresses, serial numbers or traffic.\n"
             "Use 'Driver details' for the current connection state. Open 'Adapter catalog' to inspect all profiles.\n"];
        return report;
    }
}

NSString* RTWAdapterCatalogReport(void) {
    @autoreleasepool {
        const size_t count=rtw_usb_wifi_profile_count();
        size_t enabled=0;
        for(size_t i=0;i<count;++i)
            if(rtw_usb_wifi_station_backend(rtw_usb_wifi_profile_at(i))!=RTW_WIFI_UNKNOWN)++enabled;
        NSMutableString* report=[NSMutableString stringWithFormat:
            @"Tahoe Wi-Fi USB adapter catalog\n%zu profiles · %zu enabled · %zu not enabled\n\n"
             "Enabled means verified identity plus an implemented backend, not a hardware certification.\n"
             "Exact USB configuration, interface and revision restrictions still apply.\n"
             "Adding a USB ID cannot implement a missing chipset backend.\n\n",count,enabled,count-enabled];
        for(size_t i=0;i<count;++i) {
            const auto* profile=rtw_usb_wifi_profile_at(i);
            if(!profile)continue;
            const BOOL ready=rtw_usb_wifi_station_backend(profile)!=RTW_WIFI_UNKNOWN;
            const BOOL formatOnly=profile->family==RTW_WIFI_RTL8812AU ||
                                  profile->family==RTW_WIFI_RTL8821AU;
            NSString* state=ready?@"ENABLED":
                !profile->primary_verified?@"NOT ENABLED — exact chipset identity is unverified":
                formatOnly?@"NOT ENABLED — wire-format module only; station and RF missing":
                @"NOT ENABLED — station backend missing";
            [report appendFormat:@"%04x:%04x · %@ · %@\n%@\nConfiguration %u · interface %u · revision %@\n%@\n%@\n%@\n\n",
                static_cast<unsigned>(profile->vendor),static_cast<unsigned>(profile->product),
                utf8(profile->name),utf8(rtw_usb_wifi_family_name(profile->family)),state,
                static_cast<unsigned>(profile->configuration),static_cast<unsigned>(profile->interface_number),
                profile->match_bcd?description(profile->bcd_device,true):@"any", interfaceConstraints(profile),
                utf8(rtwui::adapterCapabilityDescription(profile->family)),radioCapabilities(profile)];
        }
        return report;
    }
}
