/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_USB_WIFI_PROFILES_H
#define RTW_USB_WIFI_PROFILES_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Backend families, not a claim that every family has a Tahoe implementation. */
typedef enum rtw_wifi_family {
    RTW_WIFI_UNKNOWN = 0,
    RTW_WIFI_RTL8192CU,
    RTW_WIFI_RTL8188EU,
    RTW_WIFI_RTL8192EU,
    RTW_WIFI_RTL8192DU,
    RTW_WIFI_RTL8192FU,
    RTW_WIFI_RTL8710BU,
    RTW_WIFI_RTL8723BU,
    RTW_WIFI_RTL8812AU,
    RTW_WIFI_RTL8814AU,
    RTW_WIFI_RTL8814BU,
    RTW_WIFI_RTL8821AU,
    RTW_WIFI_RTL8821CU,
    RTW_WIFI_RTL8822BU,
    RTW_WIFI_RTL8822CU
} rtw_wifi_family;
typedef struct rtw_usb_wifi_profile {
    uint16_t vendor, product, bcd_device;
    uint8_t match_bcd, configuration, interface_number;
    rtw_wifi_family family;
    uint8_t primary_verified;
    const char *name;
    /* A zero mask preserves the exact legacy matching contract. */
    uint8_t interface_match_flags, interface_class, interface_subclass, interface_protocol;
    /* Source-backed board limit: 0 retains the legacy family ceiling. */
    uint8_t band_restriction;
} rtw_usb_wifi_profile;
enum {
    RTW_USB_MATCH_CLASS = 1,
    RTW_USB_MATCH_SUBCLASS = 2,
    RTW_USB_MATCH_PROTOCOL = 4
};
/* The legacy API cannot confirm a class-constrained identity; it fails closed.
 * Runtime binding must use the full-descriptor API below. */
const rtw_usb_wifi_profile *rtw_usb_wifi_lookup(uint16_t vendor, uint16_t product,
    uint16_t bcd_device, uint8_t configuration, uint8_t interface_number);
const rtw_usb_wifi_profile *rtw_usb_wifi_lookup_descriptor(
    uint16_t vendor, uint16_t product, uint16_t bcd_device,
    uint8_t configuration, uint8_t interface_number,
    uint8_t interface_class, uint8_t interface_subclass, uint8_t interface_protocol);
size_t rtw_usb_wifi_profile_count(void);
const rtw_usb_wifi_profile *rtw_usb_wifi_profile_at(size_t index);
const char *rtw_usb_wifi_family_name(rtw_wifi_family family);
/* Source-validated family dispatch only. It does not mean hardware validation.
 * Only pointers returned by lookup/profile_at are accepted. */
rtw_wifi_family rtw_usb_wifi_station_backend(const rtw_usb_wifi_profile *profile);
/* Metadata only; normal immutable-profile authority checks remain required. */
uint8_t rtw_usb_wifi_profile_allowed_bands(const rtw_usb_wifi_profile *profile);

/* Family-level RF/PHY ceiling vs. modes IMPLEMENTED by this macOS driver.
 * These are ceilings for a reference silicon family, not measured link rates,
 * a board certification, transmit permission or a promise that an AP will
 * negotiate that mode. Actual antenna/USB/regulatory constraints may be lower.
 * Zero means UNKNOWN/NOT IMPLEMENTED, never "all bands". RTL8814BU identity
 * B814 is verified; its station remains gated pending qualified NIC firmware,
 * protocol and board profile, and carries no guessed capabilities. */
enum {
    RTW_WIFI_BAND_2GHZ = 1u,
    RTW_WIFI_BAND_5GHZ = 2u
};
typedef struct {
    uint8_t family_bands;          /* chipset reference capability */
    uint8_t station_bands;         /* actually implemented macOS station */
    uint8_t family_max_nss;        /* upper bound, not guaranteed for a USB ID */
    uint8_t station_max_nss;       /* driver ceiling, may vary with eFuse */
    uint8_t family_max_2g_width_mhz;
    uint8_t family_max_5g_width_mhz;
    uint8_t station_max_2g_width_mhz;
    uint8_t station_max_5g_width_mhz;
    uint16_t family_max_phy_mbps;  /* rounded reference PHY ceiling (SGI) */
    uint16_t station_max_phy_mbps; /* driver ceiling, NOT TCP throughput */
    uint8_t identity_verified;     /* only true for a real catalog profile */
} rtw_wifi_radio_capabilities;

rtw_wifi_radio_capabilities rtw_wifi_radio_family_capabilities(rtw_wifi_family family);
/* Reject fabricated profiles; never authorize a non-verified USB identity.
 * Family reference ceilings are still visible to aid offline triage, but the
 * station fields are zero unless the exact immutable profile is enabled. */
rtw_wifi_radio_capabilities rtw_wifi_radio_profile_capabilities(
    const rtw_usb_wifi_profile *profile);
#ifdef __cplusplus
}
#endif
#endif
