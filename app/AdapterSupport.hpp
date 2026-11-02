// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "rtw/USBWiFiProfiles.h"
#include <cstdint>
#include <string>
#include <vector>

namespace rtwui {
// A read-only observation. -1 means the registry did not supply that descriptor.
// This never authorizes a connection: only the kernel's complete lookup can do so.
struct USBInterfaceObservation {
    int number = -1;
    int configuration = -1;
    bool driverBound = false;
    std::string driverFamily;
    int interfaceClass = -1, interfaceSubclass = -1, interfaceProtocol = -1;
    bool runtimeIdentityVerified = false;
    int hardwareChipCode = -1;
};
struct USBAdapterObservation {
    uint16_t vendor = 0, product = 0;
    uint64_t registryID = 0;
    int bcdDevice = -1, configuration = -1;
    std::vector<USBInterfaceObservation> interfaces;
    bool registryPartial = false;
};
enum class AdapterSupportState {
    NotCatalogued,
    IdentityUnverified,
    RuntimeDiscoveryPending,
    DriverRuntimeResolved,
    BackendMissing,
    DescriptorsUnavailable,
    IdentityMismatch,
    EligibleNotBound,
    DriverBound,
    DriverFamilyMismatch,
    DriverMetadataUnavailable,
    AmbiguousInterfaces
};
struct AdapterSupportResult {
    AdapterSupportState state = AdapterSupportState::NotCatalogued;
    const rtw_usb_wifi_profile* profile = nullptr;
    rtw_wifi_family resolvedFamily = RTW_WIFI_UNKNOWN;
    bool completeIdentityMatches = false;
    bool driverSeen = false;
};
inline bool unsignedDescriptor(int value, int maximum) {
    return value >= 0 && value <= maximum;
}
inline const rtw_usb_wifi_profile* adapterCatalogEntry(uint16_t vendor, uint16_t product) {
    for (size_t i = 0; i < rtw_usb_wifi_profile_count(); ++i) {
        const auto* p = rtw_usb_wifi_profile_at(i);
        if (p && p->vendor == vendor && p->product == product) return p;
    }
    return nullptr;
}
// Exact conflict only: registry evidence is diagnostic and never authorizes RF.
inline AdapterSupportResult runtimeConflictSupport(const USBAdapterObservation& observed,
                                                   const rtw_usb_wifi_profile* p) {
    AdapterSupportResult result;result.profile=p;result.state=AdapterSupportState::IdentityUnverified;
    if(observed.interfaces.size()>256){result.state=AdapterSupportState::AmbiguousInterfaces;return result;}
    size_t candidates=0;bool incomplete=false;
    for(const auto& iface:observed.interfaces){
        if(iface.number==0)++candidates;
        if(iface.number<0||iface.number>255||iface.configuration < -1||iface.configuration>255)incomplete=true;
    }
    if(candidates>1){result.state=AdapterSupportState::AmbiguousInterfaces;return result;}
    if(incomplete){result.state=AdapterSupportState::DescriptorsUnavailable;return result;}
    size_t matches=0;
    for(const auto& iface:observed.interfaces){
        const int config=iface.configuration==-1?observed.configuration:iface.configuration;
        if(iface.number!=0||config!=1||(observed.configuration!=-1&&observed.configuration!=1)||
           iface.interfaceClass!=255||iface.interfaceSubclass!=255||iface.interfaceProtocol!=255)continue;
        ++matches;result.completeIdentityMatches=true;result.driverSeen=iface.driverBound;
        result.resolvedFamily=RTW_WIFI_UNKNOWN;
        if(!iface.driverBound){result.state=AdapterSupportState::RuntimeDiscoveryPending;continue;}
        if(!iface.runtimeIdentityVerified||(iface.hardwareChipCode!=0x0a&&iface.hardwareChipCode!=0x13)){
            result.state=AdapterSupportState::DriverMetadataUnavailable;continue;
        }
        const auto family=iface.hardwareChipCode==0x0a?RTW_WIFI_RTL8822BU:RTW_WIFI_RTL8822CU;
        if(iface.driverFamily!=rtw_usb_wifi_family_name(family)){result.state=AdapterSupportState::DriverFamilyMismatch;continue;}
        result.resolvedFamily=family;result.state=AdapterSupportState::DriverRuntimeResolved;
    }
    if(matches>1){result.state=AdapterSupportState::AmbiguousInterfaces;result.completeIdentityMatches=result.driverSeen=false;result.resolvedFamily=RTW_WIFI_UNKNOWN;}
    return result;
}
inline AdapterSupportResult adapterSupport(const USBAdapterObservation& observed) {
    AdapterSupportResult result;
    const auto* p = adapterCatalogEntry(observed.vendor, observed.product);
    result.profile = p;
    if (!p) return result;
    if(observed.registryPartial){result.state=AdapterSupportState::DescriptorsUnavailable;return result;}
    if (!p->primary_verified && observed.vendor==0x13b1 && observed.product==0x0043)
        return runtimeConflictSupport(observed,p);
    if (!p->primary_verified) {
        result.state = AdapterSupportState::IdentityUnverified;
        return result;
    }
    if (rtw_usb_wifi_station_backend(p) == RTW_WIFI_UNKNOWN) {
        result.state = AdapterSupportState::BackendMissing;
        return result;
    }
    if (p->match_bcd && !unsignedDescriptor(observed.bcdDevice, 65535)) {
        result.state = AdapterSupportState::DescriptorsUnavailable;
        return result;
    }
    if (p->match_bcd && observed.bcdDevice != p->bcd_device) {
        result.state = AdapterSupportState::IdentityMismatch;
        return result;
    }
    // Unbounded or duplicate observations must not produce a success claim.
    if (observed.interfaces.size() > 256) {
        result.state = AdapterSupportState::AmbiguousInterfaces;
        return result;
    }
    bool missing = observed.interfaces.empty();
    size_t matches = 0;
    for (const auto& iface : observed.interfaces) {
        if (!unsignedDescriptor(iface.number, 255)) { missing = true; continue; }
        if (iface.number != p->interface_number) continue;
        const int config = iface.configuration == -1 ? observed.configuration : iface.configuration;
        if (!unsignedDescriptor(config, 255)) { missing = true; continue; }
        if (unsignedDescriptor(observed.configuration, 255) &&
            unsignedDescriptor(iface.configuration, 255) &&
            observed.configuration != iface.configuration) continue;
        if (config != p->configuration) continue;
        const uint16_t revision = p->match_bcd ? static_cast<uint16_t>(observed.bcdDevice) : 0;
        const int descriptors[] = {iface.interfaceClass, iface.interfaceSubclass, iface.interfaceProtocol};
        bool incomplete = false;
        for (unsigned index = 0; index < 3; ++index)
            if ((p->interface_match_flags & (1u << index)) &&
                !unsignedDescriptor(descriptors[index], 255)) incomplete = true;
        if (incomplete) { missing = true; continue; }
        // Unconstrained absent fields may be zero; constrained missing fields
        // were rejected above and can never masquerade as a vendor interface.
        if (rtw_usb_wifi_lookup_descriptor(observed.vendor, observed.product, revision,
                static_cast<uint8_t>(config), static_cast<uint8_t>(iface.number),
                static_cast<uint8_t>(unsignedDescriptor(iface.interfaceClass,255) ? iface.interfaceClass : 0),
                static_cast<uint8_t>(unsignedDescriptor(iface.interfaceSubclass,255) ? iface.interfaceSubclass : 0),
                static_cast<uint8_t>(unsignedDescriptor(iface.interfaceProtocol,255) ? iface.interfaceProtocol : 0)) != p)
            continue;
        ++matches;
        result.completeIdentityMatches = true;
        result.driverSeen = iface.driverBound;
        if (!iface.driverBound) result.state = AdapterSupportState::EligibleNotBound;
        else if (iface.driverFamily.empty()) result.state = AdapterSupportState::DriverMetadataUnavailable;
        else if (iface.driverFamily != rtw_usb_wifi_family_name(p->family))
            result.state = AdapterSupportState::DriverFamilyMismatch;
        else result.state = AdapterSupportState::DriverBound;
    }
    if (matches > 1) {
        result.state = AdapterSupportState::AmbiguousInterfaces;
        result.completeIdentityMatches = false;
        result.driverSeen = false;
    } else if (!matches) {
        result.state = missing ? AdapterSupportState::DescriptorsUnavailable :
                                 AdapterSupportState::IdentityMismatch;
    }
    return result;
}
inline const char* adapterSupportDescription(AdapterSupportState state) {
    switch (state) {
    case AdapterSupportState::NotCatalogued:
        return "This ID is not in the project catalog.";
    case AdapterSupportState::RuntimeDiscoveryPending:
        return "Chip identification is required before selecting the station backend.";
    case AdapterSupportState::DriverRuntimeResolved:
        return "The bound driver verified the silicon identity. Link and IP status require separate checks.";
    case AdapterSupportState::IdentityUnverified:
        return "Identity is unverified or conflicting; not enabled.";
    case AdapterSupportState::BackendMissing:
        return "Family identified, but this project has no complete station backend for it.";
    case AdapterSupportState::DescriptorsUnavailable:
        return "USB registry descriptors are incomplete; the exact match could not be confirmed.";
    case AdapterSupportState::IdentityMismatch:
        return "USB revision, configuration, interface number or interface class does not match the permitted profile.";
    case AdapterSupportState::EligibleNotBound:
        return "Profile enabled, but this project's kernel extension is not bound to the expected interface.";
    case AdapterSupportState::DriverBound:
        return "Kernel extension bound with the expected family. This does not prove radio, WPA2 or internet operation.";
    case AdapterSupportState::DriverFamilyMismatch:
        return "Kernel extension bound with a different family. Check the loaded version.";
    case AdapterSupportState::DriverMetadataUnavailable:
        return "Kernel extension bound, but family metadata is missing; initialization may be incomplete or outdated.";
    case AdapterSupportState::AmbiguousInterfaces:
        return "Interface registry is ambiguous or oversized; no match is confirmed.";
    }
    return "Unknown support state.";
}
inline const char* adapterCapabilityDescription(rtw_wifi_family family) {
    switch (family) {
    case RTW_WIFI_RTL8192CU:
        return "C/CU backend: 2.4 GHz, HT20, up to 2 streams, firmware rate control, WPA2-Personal/CCMP; no HT40/5 GHz.";
    case RTW_WIFI_RTL8188EU:
        return "8188EU backend: 2.4 GHz, HT20, one stream, host rate control, WPA2-Personal/CCMP; no HT40/5 GHz; I-cut not supported.";
    case RTW_WIFI_RTL8192EU:
        return "8192EU backend: 2.4 GHz, negotiated HT20/HT40, WPA2-Personal/CCMP; no 5 GHz.";
    case RTW_WIFI_RTL8814AU:
        return "8814AU backend: HT20, 2T2R AB, RFE 0/1, WPA2-Personal/CCMP. "
               "Low 5 GHz (36/40/44/48) and upper 5 GHz (149/153/157/161/165) each require the running driver's explicit station capability; "
               "the catalog alone does not authorize 5 GHz. No VHT.";
    case RTW_WIFI_RTL8812AU:
        return "8812AU: 2.4/5 GHz, HT20/40, VHT20/40/80, up to 2 streams, WPA2-Personal/CCMP. "
               "The running driver must confirm the board and each 5 GHz channel group.";
    case RTW_WIFI_RTL8821AU:
        return "8821AU: 2.4/5 GHz, HT20/40, VHT20/40/80, one stream, WPA2-Personal/CCMP. "
               "The running driver must confirm the board and each 5 GHz channel group.";
    case RTW_WIFI_RTL8192DU:
        return "8192DU: 2.4/5 GHz, HT20, WPA2-Personal/CCMP. "
               "SMSP supports 2 streams; DMDP uses one stream per MAC with one stable logical address.";
    case RTW_WIFI_RTL8192FU:
        return "8192FU: 2.4 GHz, HT20/40, up to 2 streams, WPA2-Personal/CCMP; board/ROM checks apply.";
    case RTW_WIFI_RTL8710BU:
        return "8710BU: 2.4 GHz, HT20/40, one stream, WPA2-Personal/CCMP; supported SMIC/UMC board required.";
    case RTW_WIFI_RTL8723BU:
        return "8723BU: 2.4 GHz, HT20, one stream, WPA2-Personal/CCMP with Bluetooth coexistence controls.";
    case RTW_WIFI_RTL8814BU:
        return "8814BU: dedicated USB HAL and PHY source is present; connection stays disabled pending qualified NIC firmware/protocol and station validation.";
    case RTW_WIFI_RTL8821CU:
        return "8821CU: HT20 in 2.4 GHz/DFS; HT40 and VHT20/40/80 in non-DFS 5 GHz, one stream. Channel165: 20 MHz. WPA2-Personal/CCMP; running board/channel capability required.";
    case RTW_WIFI_RTL8822BU:
        return "8822BU: HT20 in 2.4 GHz/DFS; HT40 and VHT20/40/80 in non-DFS 5 GHz, up to 2 streams. Channel165: 20 MHz. WPA2-Personal/CCMP; running board/channel capability required.";
    case RTW_WIFI_RTL8822CU:
        return "8822CU: HT20 in 2.4 GHz/DFS; HT40 and VHT20/40/80 in non-DFS 5 GHz, up to 2 streams. Channel165: 20 MHz. WPA2-Personal/CCMP; running board/channel capability required.";

    default:
        return "Connection unavailable: this family has no integrated station backend.";
    }
}
} // namespace rtwui
