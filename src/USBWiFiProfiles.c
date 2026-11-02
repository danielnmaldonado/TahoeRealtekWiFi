/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/USBWiFiProfiles.h"

static const rtw_usb_wifi_profile profiles[] = {
#include "USBWiFiProfiles.inc"
};

size_t rtw_usb_wifi_profile_count(void) {
    return sizeof(profiles) / sizeof(profiles[0]);
}

uint8_t rtw_usb_wifi_profile_allowed_bands(const rtw_usb_wifi_profile *profile) {
    return profile ? (profile->band_restriction ? profile->band_restriction : 3u) : 0u;
}

const rtw_usb_wifi_profile *rtw_usb_wifi_profile_at(size_t index) {
    return index < rtw_usb_wifi_profile_count() ? &profiles[index] : NULL;
}

/* The generated table is sorted by the numeric USB key. A bounded binary
 * search avoids repeated linear scans in app diagnostics and kernel probing. */
static const rtw_usb_wifi_profile *find_identity(uint16_t vendor, uint16_t product) {
    size_t first = 0, end = rtw_usb_wifi_profile_count();
    const uint32_t key = ((uint32_t)vendor << 16) | product;
    while (first < end) {
        const size_t middle = first + (end - first) / 2;
        const rtw_usb_wifi_profile *p = &profiles[middle];
        const uint32_t candidate = ((uint32_t)p->vendor << 16) | p->product;
        if (candidate < key) first = middle + 1;
        else if (candidate > key) end = middle;
        else return p;
    }
    return NULL;
}

static int identity_matches(const rtw_usb_wifi_profile *p, uint16_t bcd,
                            uint8_t configuration, uint8_t number) {
    return p && configuration != 0 && p->configuration == configuration &&
           p->interface_number == number && (!p->match_bcd || p->bcd_device == bcd);
}

const rtw_usb_wifi_profile *rtw_usb_wifi_lookup(uint16_t vendor, uint16_t product,
    uint16_t bcd_device, uint8_t configuration, uint8_t interface_number) {
    const rtw_usb_wifi_profile *p = find_identity(vendor, product);
    return identity_matches(p, bcd_device, configuration, interface_number) &&
           p->interface_match_flags == 0 ? p : NULL;
}

const rtw_usb_wifi_profile *rtw_usb_wifi_lookup_descriptor(
    uint16_t vendor, uint16_t product, uint16_t bcd_device,
    uint8_t configuration, uint8_t interface_number,
    uint8_t interface_class, uint8_t interface_subclass, uint8_t interface_protocol) {
    const rtw_usb_wifi_profile *p = find_identity(vendor, product);
    if (!identity_matches(p, bcd_device, configuration, interface_number)) return NULL;
    if (p->interface_match_flags & ~(RTW_USB_MATCH_CLASS | RTW_USB_MATCH_SUBCLASS |
                                    RTW_USB_MATCH_PROTOCOL)) return NULL;
    if ((p->interface_match_flags & RTW_USB_MATCH_CLASS) &&
        p->interface_class != interface_class) return NULL;
    if ((p->interface_match_flags & RTW_USB_MATCH_SUBCLASS) &&
        p->interface_subclass != interface_subclass) return NULL;
    if ((p->interface_match_flags & RTW_USB_MATCH_PROTOCOL) &&
        p->interface_protocol != interface_protocol) return NULL;
    return p;
}

const char *rtw_usb_wifi_family_name(rtw_wifi_family family) {
    switch (family) {
    case RTW_WIFI_RTL8192CU: return "RTL8192CU";
    case RTW_WIFI_RTL8188EU: return "RTL8188EU";
    case RTW_WIFI_RTL8192EU: return "RTL8192EU";
    case RTW_WIFI_RTL8192DU: return "RTL8192DU";
    case RTW_WIFI_RTL8192FU: return "RTL8192FU";
    case RTW_WIFI_RTL8710BU: return "RTL8710BU";
    case RTW_WIFI_RTL8723BU: return "RTL8723BU";
    case RTW_WIFI_RTL8812AU: return "RTL8812AU";
    case RTW_WIFI_RTL8814AU: return "RTL8814AU";
    case RTW_WIFI_RTL8814BU: return "RTL8814BU";
    case RTW_WIFI_RTL8821AU: return "RTL8821AU";
    case RTW_WIFI_RTL8821CU: return "RTL8821CU";
    case RTW_WIFI_RTL8822BU: return "RTL8822BU";
    case RTW_WIFI_RTL8822CU: return "RTL8822CU";
    default: return "UNKNOWN";
    }
}

rtw_wifi_family rtw_usb_wifi_station_backend(const rtw_usb_wifi_profile *profile) {
    size_t i;
    if (!profile)
        return RTW_WIFI_UNKNOWN;
    /* Only immutable catalog entries carry verified provenance. A copied or
     * fabricated profile cannot upgrade an unverified identity by changing a flag. */
    for (i = 0; i < rtw_usb_wifi_profile_count(); ++i)
        if (profile == &profiles[i])
            break;
    if (i == rtw_usb_wifi_profile_count() || !profile->primary_verified)
        return RTW_WIFI_UNKNOWN;
    /* Keep synchronized with READY_FAMILIES in scripts/usb_wifi_catalog.py.
     * These implementations require normal runtime silicon/endpoint checks. */
    switch (profile->family) {
    case RTW_WIFI_RTL8192CU:
    case RTW_WIFI_RTL8188EU:
    case RTW_WIFI_RTL8192EU:
    case RTW_WIFI_RTL8814AU:
    case RTW_WIFI_RTL8192DU:
    case RTW_WIFI_RTL8192FU:
    case RTW_WIFI_RTL8710BU:
    case RTW_WIFI_RTL8723BU:
    case RTW_WIFI_RTL8812AU:
    case RTW_WIFI_RTL8821AU:
    case RTW_WIFI_RTL8821CU:
    case RTW_WIFI_RTL8822BU:
    case RTW_WIFI_RTL8822CU:
        return profile->family;
    default:
        return RTW_WIFI_UNKNOWN;
    }
}

rtw_wifi_radio_capabilities rtw_wifi_radio_family_capabilities(rtw_wifi_family family) {
    rtw_wifi_radio_capabilities c = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint8_t g2 = RTW_WIFI_BAND_2GHZ;
    const uint8_t dual = RTW_WIFI_BAND_2GHZ | RTW_WIFI_BAND_5GHZ;
    /* Rounded theoretical PHY maxima assume the chipset reference design,
     * best supported width, SGI, MCS/NSS and a suitable peer. The app/CLI
     * must NEVER present these values as observed throughput. */
    switch (family) {
    case RTW_WIFI_RTL8192CU:
        c.family_bands=g2; c.family_max_nss=2;
        c.family_max_2g_width_mhz=40; c.family_max_phy_mbps=300;
        c.station_bands=g2; c.station_max_nss=2;
        c.station_max_2g_width_mhz=20; c.station_max_phy_mbps=144;
        break;
    case RTW_WIFI_RTL8188EU:
        c.family_bands=g2; c.family_max_nss=1;
        c.family_max_2g_width_mhz=40; c.family_max_phy_mbps=150;
        c.station_bands=g2; c.station_max_nss=1;
        c.station_max_2g_width_mhz=20; c.station_max_phy_mbps=72;
        break;
    case RTW_WIFI_RTL8192EU:
        c.family_bands=g2; c.family_max_nss=2;
        c.family_max_2g_width_mhz=40; c.family_max_phy_mbps=300;
        c.station_bands=g2; c.station_max_nss=2;
        c.station_max_2g_width_mhz=40; c.station_max_phy_mbps=300;
        break;
    case RTW_WIFI_RTL8192DU:
        c.family_bands=dual; c.family_max_nss=2;
        c.family_max_2g_width_mhz=40; c.family_max_5g_width_mhz=40;
        c.family_max_phy_mbps=300;
        // SMSP reaches 2SS; the DMDP runtime is restricted to 1SS per MAC.
        c.station_bands=dual; c.station_max_nss=2;
        c.station_max_2g_width_mhz=20; c.station_max_5g_width_mhz=20;
        c.station_max_phy_mbps=144;
        break;
    case RTW_WIFI_RTL8192FU:
        c.family_bands=g2; c.family_max_nss=2;
        c.family_max_2g_width_mhz=40; c.family_max_phy_mbps=300;
        c.station_bands=g2; c.station_max_nss=2;
        c.station_max_2g_width_mhz=40; c.station_max_phy_mbps=300;
        break;
    case RTW_WIFI_RTL8710BU:
    case RTW_WIFI_RTL8723BU:
        c.family_bands=g2; c.family_max_nss=1;
        c.family_max_2g_width_mhz=40; c.family_max_phy_mbps=150;
        c.station_bands=g2; c.station_max_nss=1;
        c.station_max_2g_width_mhz=(family==RTW_WIFI_RTL8710BU?40:20);
        c.station_max_phy_mbps=(family==RTW_WIFI_RTL8710BU?150:72);
        break;
    case RTW_WIFI_RTL8812AU:
    case RTW_WIFI_RTL8822BU:
    case RTW_WIFI_RTL8822CU:
        c.family_bands=dual; c.family_max_nss=2;
        c.family_max_2g_width_mhz=40; c.family_max_5g_width_mhz=80;
        c.family_max_phy_mbps=867;
        c.station_bands=dual; c.station_max_nss=2;
        c.station_max_2g_width_mhz=(family==RTW_WIFI_RTL8812AU?40:20);
        c.station_max_5g_width_mhz=80;
        c.station_max_phy_mbps=867;
        break;
    case RTW_WIFI_RTL8814AU:
        c.family_bands=dual; c.family_max_nss=3;
        c.family_max_2g_width_mhz=40; c.family_max_5g_width_mhz=80;
        c.family_max_phy_mbps=1300;
        // Each low/upper 5GHz channel is separately checked against board ROM.
        c.station_bands=dual; c.station_max_nss=2;
        c.station_max_5g_width_mhz=20; c.station_max_2g_width_mhz=20;
        c.station_max_phy_mbps=144;
        break;
    case RTW_WIFI_RTL8821AU:
    case RTW_WIFI_RTL8821CU:
        c.family_bands=dual; c.family_max_nss=1;
        c.family_max_2g_width_mhz=40; c.family_max_5g_width_mhz=80;
        c.family_max_phy_mbps=433;
        c.station_bands=dual; c.station_max_nss=1;
        c.station_max_2g_width_mhz=(family==RTW_WIFI_RTL8821AU?40:20);
        c.station_max_5g_width_mhz=80;
        c.station_max_phy_mbps=433;
        break;
    case RTW_WIFI_RTL8814BU:
    default:
        /* B814 identity and HAL/PHY components are present. Its station still
         * needs qualified NIC firmware/protocol, board profile and remaining
         * tracking functions. Never borrow RTL8814AU capabilities. */
        break;
    }
    return c;
}

rtw_wifi_radio_capabilities rtw_wifi_radio_profile_capabilities(
    const rtw_usb_wifi_profile *profile) {
    rtw_wifi_radio_capabilities c = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    size_t i;
    /* Check pointer identity BEFORE dereferencing it. */
    for (i=0; i<rtw_usb_wifi_profile_count(); ++i)
        if (profile == &profiles[i]) break;
    if (!profile || i == rtw_usb_wifi_profile_count()) return c;
    c = rtw_wifi_radio_family_capabilities(profile->family);
    c.family_bands &= rtw_usb_wifi_profile_allowed_bands(profile);
    c.station_bands &= rtw_usb_wifi_profile_allowed_bands(profile);
    if (!(c.family_bands & RTW_WIFI_BAND_2GHZ)) c.family_max_2g_width_mhz=0;
    if (!(c.family_bands & RTW_WIFI_BAND_5GHZ)) c.family_max_5g_width_mhz=0;
    if (!(c.station_bands & RTW_WIFI_BAND_2GHZ)) c.station_max_2g_width_mhz=0;
    if (!(c.station_bands & RTW_WIFI_BAND_5GHZ)) c.station_max_5g_width_mhz=0;
    if (!c.family_bands) c.family_max_nss=c.family_max_phy_mbps=0;
    if (!c.station_bands) c.station_max_nss=c.station_max_phy_mbps=0;
    c.identity_verified = profile->primary_verified != 0;
    if (rtw_usb_wifi_station_backend(profile) == RTW_WIFI_UNKNOWN) {
        c.station_bands = 0;
        c.station_max_nss = 0;
        c.station_max_2g_width_mhz = 0;
        c.station_max_5g_width_mhz = 0;
        c.station_max_phy_mbps = 0;
    }
    return c;
}
