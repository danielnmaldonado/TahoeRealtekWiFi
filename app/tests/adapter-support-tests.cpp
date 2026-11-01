// SPDX-License-Identifier: GPL-2.0-only
#include "AdapterSupport.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace {
unsigned checks = 0;
void check(bool value, const char* message) {
    ++checks;
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
rtwui::USBAdapterObservation observation(const rtw_usb_wifi_profile* p) {
    rtwui::USBAdapterObservation result;
    result.vendor = p->vendor; result.product = p->product;
    result.bcdDevice = p->match_bcd ? p->bcd_device : 0x1234;
    result.configuration = p->configuration;
    result.interfaces.push_back({p->interface_number, p->configuration, false, "",
                                 p->interface_class, p->interface_subclass, p->interface_protocol});
    return result;
}
}
int main() {
    using S = rtwui::AdapterSupportState;
    using rtwui::adapterSupport;
    auto runtime=observation(rtwui::adapterCatalogEntry(0x13b1,0x0043));
    auto& iface=runtime.interfaces[0];iface.interfaceClass=iface.interfaceSubclass=iface.interfaceProtocol=255;
    check(adapterSupport(runtime).state==S::RuntimeDiscoveryPending,"conflicting ID waits for silicon proof");
    iface.driverBound=true;iface.runtimeIdentityVerified=true;
    for(int code=0;code<256;++code){iface.hardwareChipCode=code;iface.driverFamily=code==0x13?"RTL8822CU":"RTL8822BU";
        auto result=adapterSupport(runtime);check((result.state==S::DriverRuntimeResolved)==(code==0x0a||code==0x13),"only two checked silicon codes resolve");}
    iface.hardwareChipCode=0x0a;iface.driverFamily="RTL8822CU";check(adapterSupport(runtime).state==S::DriverFamilyMismatch,"family must match chip code");
    iface.driverFamily="RTL8822BU";iface.runtimeIdentityVerified=false;check(adapterSupport(runtime).state==S::DriverMetadataUnavailable,"bound family alone is not proof");
    iface.runtimeIdentityVerified=true;runtime.interfaces.push_back(iface);check(adapterSupport(runtime).state==S::AmbiguousInterfaces,"duplicate interfaces do not resolve");
    runtime.interfaces.resize(1);
    for(int mode=0;mode<3;++mode){auto duplicate=runtime.interfaces[0];if(mode==0)duplicate.interfaceProtocol=-1;if(mode==1)duplicate.interfaceClass=3;if(mode==2)duplicate.configuration=-2;
        runtime.interfaces.push_back(duplicate);check(adapterSupport(runtime).state==S::AmbiguousInterfaces,"incomplete or divergent duplicate cannot resolve");runtime.interfaces.pop_back();}
    runtime.registryPartial=true;check(adapterSupport(runtime).state==S::DescriptorsUnavailable,"partial registry cannot establish unique binding");
    size_t implemented = 0, missing = 0, unverified = 0;
    check(rtw_usb_wifi_profile_count() == 270, "retain all catalog identities");
    for (size_t i = 0; i < rtw_usb_wifi_profile_count(); ++i) {
        const auto* p = rtw_usb_wifi_profile_at(i);
        auto observed = observation(p);
        auto result = adapterSupport(observed);
        check(result.profile == p, "diagnostic uses the exact immutable kernel catalog entry");
        if (!p->primary_verified) {
            ++unverified;
            check(result.state == S::IdentityUnverified, "unverified/conflicting ID is not ready");
        } else if (rtw_usb_wifi_station_backend(p) == RTW_WIFI_UNKNOWN) {
            ++missing;
            check(result.state == S::BackendMissing, "numeric evidence does not invent a backend");
        } else {
            ++implemented;
            check(result.state == S::EligibleNotBound, "matching identity is not driver binding");
            check(result.completeIdentityMatches && !result.driverSeen, "identity and live binding separate");
            for (int config = 0; config <= 255; ++config) {
                auto candidate = observation(p);
                candidate.configuration = config;
                candidate.interfaces[0].configuration = config;
                check(adapterSupport(candidate).completeIdentityMatches == (config == p->configuration),
                      "all 256 configuration values obey kernel constraints");
            }
            for (int iface = 0; iface <= 255; ++iface) {
                auto candidate = observation(p);
                candidate.interfaces[0].number = iface;
                check(adapterSupport(candidate).completeIdentityMatches == (iface == p->interface_number),
                      "all 256 interface numbers obey kernel constraints");
            }
            for (int revision : {-2, -1, 0, 1, 0x0200, 0x0201, 0xffff, 0x10000}) {
                auto candidate = observation(p); candidate.bcdDevice = revision;
                check(adapterSupport(candidate).completeIdentityMatches ==
                    (!p->match_bcd || revision == p->bcd_device), "revision wildcard is profile-specific");
            }
            for (unsigned field=0; field<3; ++field) {
                if (!(p->interface_match_flags & (1u << field))) continue;
                const int expected[] = {p->interface_class,p->interface_subclass,p->interface_protocol};
                for (int value=-2; value<=256; ++value) {
                    auto candidate=observation(p);
                    auto& iface=candidate.interfaces[0];
                    int* descriptors[] = {&iface.interfaceClass,&iface.interfaceSubclass,&iface.interfaceProtocol};
                    *descriptors[field]=value;
                    const auto classified=adapterSupport(candidate);
                    check(classified.completeIdentityMatches==(value==expected[field]),
                          "class constraints reject mismatched, missing or overflowing descriptors");
                    if(value<0||value>255)
                        check(classified.state==S::DescriptorsUnavailable,"unknown class is not assumed vendor-specific");
                }
            }
            observed.interfaces[0].driverBound = true;
            observed.interfaces[0].driverFamily = rtw_usb_wifi_family_name(p->family);
            check(adapterSupport(observed).state == S::DriverBound, "binding is observed on the matching interface");
            observed.interfaces[0].driverFamily = p->family==RTW_WIFI_RTL8192EU ? "RTL8814AU" : "RTL8192EU";
            check(adapterSupport(observed).state == S::DriverFamilyMismatch, "stale/wrong family is not claimed ready");
            observed.interfaces[0].driverFamily.clear();
            check(adapterSupport(observed).state == S::DriverMetadataUnavailable, "missing published family stays explicit");
            observed = observation(p); observed.interfaces.clear();
            check(adapterSupport(observed).state == S::DescriptorsUnavailable, "no interface is not complete matching");
            observed = observation(p); observed.configuration = -1;
            check(adapterSupport(observed).completeIdentityMatches, "interface-local configuration suffices");
            observed = observation(p); observed.interfaces[0].configuration = -1;
            check(adapterSupport(observed).completeIdentityMatches, "explicit device configuration can fill a missing field");
            observed.configuration = -1;
            check(adapterSupport(observed).state == S::DescriptorsUnavailable, "do not assume configuration one");
            observed = observation(p); observed.interfaces[0].configuration = 256;
            check(!adapterSupport(observed).completeIdentityMatches, "invalid configuration is not truncated to uint8");
            observed = observation(p); observed.interfaces[0].number = 256;
            check(!adapterSupport(observed).completeIdentityMatches, "invalid interface is not truncated to uint8");
            observed = observation(p); observed.interfaces.push_back(observed.interfaces[0]);
            check(adapterSupport(observed).state == S::AmbiguousInterfaces, "duplicate expected interfaces do not pass");
            check(!adapterSupport(observed).completeIdentityMatches, "ambiguous identity not confirmed");
            observed.interfaces.resize(257);
            check(adapterSupport(observed).state == S::AmbiguousInterfaces, "observation bound is enforced");
        }
        check(rtwui::adapterCapabilityDescription(p->family)[0] != '\0', "every family has an honest description");
        check(rtwui::adapterSupportDescription(result.state)[0] != '\0', "every support result is printable");
    }
    check(std::strstr(rtwui::adapterCapabilityDescription(RTW_WIFI_RTL8812AU),
                      "VHT20/40/80") != nullptr, "8812AU reports implemented wide modes");
    check(std::strstr(rtwui::adapterCapabilityDescription(RTW_WIFI_RTL8821AU),
                      "VHT20/40/80") != nullptr, "8821AU reports implemented one-stream modes");
    for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}) {
        const char* description=rtwui::adapterCapabilityDescription(family);
        check(std::strstr(description,"HT20") && !std::strstr(description,"VHT"),
              "B/C does not advertise unimplemented wide modes");
    }
    check(implemented == 262 && missing == 1 && unverified == 7, "complete activation accounting");
    rtwui::USBAdapterObservation unknown;
    unknown.vendor = 0xffff; unknown.product = 0xffff;
    check(adapterSupport(unknown).state == S::NotCatalogued, "unknown ID never borrows another adapter");
    const auto* composite = rtwui::adapterCatalogEntry(0x0bda, 0x818c);
    check(composite && composite->interface_number == 2, "composite device expected interface is two");
    auto multi = observation(composite);
    multi.interfaces[0].number = 0;
    multi.interfaces[0].driverBound = true;
    multi.interfaces[0].driverFamily = "RTL8192EU";
    check(adapterSupport(multi).state == S::IdentityMismatch, "a sibling interface cannot lend its driver");
    multi.interfaces.push_back({2, 1, false, ""});
    check(adapterSupport(multi).state == S::EligibleNotBound, "correct interface stays unbound despite sibling");
    auto first = observation(rtwui::adapterCatalogEntry(0x2357, 0x0108));
    auto second = observation(rtwui::adapterCatalogEntry(0x0bda, 0x8176));
    first.interfaces[0].driverBound = true; first.interfaces[0].driverFamily = "RTL8192EU";
    check(adapterSupport(first).state == S::DriverBound, "first device bound");
    check(adapterSupport(second).state == S::EligibleNotBound, "second device remains independent");
    check(adapterSupport(first).state == S::DriverBound, "classification has no cross-device global state");
    for (const auto pair : {std::pair<uint16_t,uint16_t>{0x0bda,0x0a8a}, {0xcdab,0x8011}}) {
        const auto* p = rtwui::adapterCatalogEntry(pair.first,pair.second);
        check(p && p->primary_verified && p->family == RTW_WIFI_RTL8192CU, "new CTV exact-source evidence");
        check(p->match_bcd && p->bcd_device == 0x0200, "CTV revision not broadened");
        check(rtw_usb_wifi_station_backend(p) == RTW_WIFI_RTL8192CU, "CTV routes to actual C-generation backend");
    }
    std::printf("Adapter support: %u checks passed; 270 catalogued, 262 eligible, 1 backend missing, 7 unverified/conflict.\n", checks);
}
