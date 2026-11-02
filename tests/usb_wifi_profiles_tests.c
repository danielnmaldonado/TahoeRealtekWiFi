/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/USBWiFiProfiles.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n", __LINE__, #x); return 1; } } while (0)
int main(void) {
    const rtw_usb_wifi_profile *p;
    rtw_usb_wifi_profile forged;
    CHECK(rtw_usb_wifi_profile_count() == 270);
    CHECK(rtw_usb_wifi_profile_at(270) == NULL);
    CHECK(rtw_usb_wifi_station_backend(NULL) == RTW_WIFI_UNKNOWN);
    CHECK(rtw_usb_wifi_profile_allowed_bands(NULL) == 0);
    CHECK(rtw_usb_wifi_lookup(0xffff,0xffff,0x0200,1,0) == NULL);
    /* Edimax WLAN requires its Linux FF/FF/FF tuple; Bluetooth is E0/01/01. */
    CHECK(rtw_usb_wifi_lookup(0x7392,0xa611,0x0200,1,2)==NULL);
    p=rtw_usb_wifi_lookup_descriptor(0x7392,0xa611,0x0200,1,2,255,255,255);
    CHECK(p && rtw_usb_wifi_station_backend(p)==RTW_WIFI_RTL8723BU);
    CHECK(rtw_usb_wifi_lookup_descriptor(0x7392,0xa611,0x0200,1,0,224,1,1)==NULL);
    CHECK(rtw_usb_wifi_lookup_descriptor(0x7392,0xa611,0x0200,1,1,224,1,1)==NULL);
    CHECK(rtw_usb_wifi_lookup_descriptor(0x7392,0xa611,0x0200,1,2,224,1,1)==NULL);
    CHECK(rtw_usb_wifi_lookup_descriptor(0x7392,0xa611,0x0200,1,2,255,255,1)==NULL);

    p=rtw_usb_wifi_lookup(0x2357,0x0108,0x0000,1,0);
    CHECK(p && p->family == RTW_WIFI_RTL8192EU && p->primary_verified);
    CHECK(rtw_usb_wifi_station_backend(p) == RTW_WIFI_RTL8192EU);
    CHECK(rtw_usb_wifi_profile_allowed_bands(p) == 3);
    p=rtw_usb_wifi_lookup(0x1668,0x8108,0,1,0);
    CHECK(p && p->primary_verified && p->family==RTW_WIFI_RTL8821AU);
    CHECK(rtw_usb_wifi_station_backend(p)==RTW_WIFI_RTL8821AU);
    CHECK(rtw_usb_wifi_profile_allowed_bands(p)==3 && p->band_restriction==0);
    {
        rtw_wifi_radio_capabilities caps=rtw_wifi_radio_profile_capabilities(p);
        CHECK(caps.identity_verified && caps.family_bands==3 && caps.station_bands==3);
    }
    CHECK(rtw_usb_wifi_lookup(0x1668,0x8108,0,2,0)==NULL);
    CHECK(rtw_usb_wifi_lookup(0x1668,0x8108,0,1,2)==NULL);
    p=rtw_usb_wifi_lookup(0x1668,0x8105,0,1,0);
    CHECK(p && !p->primary_verified && rtw_usb_wifi_station_backend(p)==RTW_WIFI_UNKNOWN);
    CHECK(rtw_usb_wifi_lookup(0x2357,0x0106,0,1,0) == NULL);
    p=rtw_usb_wifi_lookup_descriptor(0x2357,0x0106,0,1,0,0xff,0xff,0xff);
    CHECK(p && p->primary_verified && rtw_usb_wifi_station_backend(p)==RTW_WIFI_RTL8814AU);
    p=rtw_usb_wifi_lookup(0x2001,0x3320,0,1,0);
    CHECK(p && p->primary_verified && rtw_usb_wifi_station_backend(p)==RTW_WIFI_RTL8814AU);
    CHECK(rtw_usb_wifi_lookup(0x2357,0x0108,0x0000,0,0) == NULL);
    CHECK(rtw_usb_wifi_lookup(0x2357,0x0108,0x0000,1,2) == NULL);
    CHECK(rtw_usb_wifi_lookup(0x0bda,0x818c,0x1234,1,0) == NULL);
    p=rtw_usb_wifi_lookup(0x0bda,0x818c,0x1234,1,2);
    CHECK(p && rtw_usb_wifi_station_backend(p) == RTW_WIFI_RTL8192EU);
    p=rtw_usb_wifi_lookup(0x0bda,0x8176,0x0200,1,0);
    CHECK(p && p->match_bcd && p->bcd_device == 0x0200);
    CHECK(rtw_usb_wifi_station_backend(p) == RTW_WIFI_RTL8192CU);
    p=rtw_usb_wifi_lookup(0x0b05,0x17c0,0x0200,1,0);
    CHECK(p && p->primary_verified && rtw_usb_wifi_station_backend(p) == RTW_WIFI_RTL8192CU);
    CHECK(rtw_usb_wifi_lookup(0x0bda,0x8176,0x0201,1,0) == NULL);
    CHECK(rtw_usb_wifi_lookup(0x0bda,0x8176,0,1,0) == NULL);
    p=rtw_usb_wifi_lookup(0x2357,0x010c,0x0201,1,0);
    CHECK(p && rtw_usb_wifi_station_backend(p) == RTW_WIFI_RTL8188EU);
    p=rtw_usb_wifi_lookup(0x07b8,0x818b,0,1,0);
    CHECK(p && p->family == RTW_WIFI_RTL8192EU && p->primary_verified);
    CHECK(rtw_usb_wifi_station_backend(p) == RTW_WIFI_RTL8192EU);
    forged = *p; forged.primary_verified = 1;
    CHECK(rtw_usb_wifi_station_backend(&forged) == RTW_WIFI_UNKNOWN);
    p=rtw_usb_wifi_lookup(0x13b1,0x0043,0,1,0);
    CHECK(p && !p->primary_verified && rtw_usb_wifi_station_backend(p) == RTW_WIFI_UNKNOWN);
    p=rtw_usb_wifi_lookup(0x0bda,0x2102,0,1,0);
    CHECK(!p); /* Actualff/ff/ff interface identity is required. */
    p=rtw_usb_wifi_lookup_descriptor(0x0bda,0x2102,0,1,0,255,255,255);
    CHECK(p && p->primary_verified && p->family==RTW_WIFI_RTL8822BU);
    CHECK(rtw_usb_wifi_station_backend(p)==RTW_WIFI_RTL8822BU);
    p=rtw_usb_wifi_lookup(0x0bda,0x8812,0,1,0);
    CHECK(p && p->primary_verified && rtw_usb_wifi_station_backend(p) == RTW_WIFI_RTL8812AU);
    p=rtw_usb_wifi_lookup(0x0bda,0xb814,0,1,0);
    CHECK(!p);
    p=rtw_usb_wifi_lookup_descriptor(0x0bda,0xb814,0,1,0,255,255,255);
    CHECK(p && p->family == RTW_WIFI_RTL8814BU && p->primary_verified);
    CHECK(rtw_usb_wifi_station_backend(p)==RTW_WIFI_UNKNOWN);
    CHECK(strcmp(rtw_usb_wifi_family_name(RTW_WIFI_RTL8710BU),"RTL8710BU") == 0);
    CHECK(strcmp(rtw_usb_wifi_family_name((rtw_wifi_family)255),"UNKNOWN") == 0);
    {
        const uint16_t ids[][2] = {{0x04bb,0x0959},{0x04bb,0x095a},{0x0bda,0x2003}};
        size_t i;
        for (i=0;i<sizeof(ids)/sizeof(ids[0]);++i) {
            rtw_wifi_radio_capabilities caps;
            p=rtw_usb_wifi_lookup(ids[i][0],ids[i][1],0,1,0);
            CHECK(p && p->primary_verified && p->band_restriction==2);
            CHECK(rtw_usb_wifi_profile_allowed_bands(p)==RTW_WIFI_BAND_5GHZ);
            caps=rtw_wifi_radio_profile_capabilities(p);
            CHECK(caps.identity_verified && caps.family_bands==RTW_WIFI_BAND_5GHZ);
            CHECK(caps.station_bands==RTW_WIFI_BAND_5GHZ);
            CHECK(caps.family_max_2g_width_mhz==0 && caps.station_max_2g_width_mhz==0);
            CHECK(caps.family_max_5g_width_mhz && caps.station_max_5g_width_mhz);
            forged=*p; forged.band_restriction=3;
            caps=rtw_wifi_radio_profile_capabilities(&forged);
            CHECK(!caps.identity_verified && !caps.station_bands && !caps.family_bands);
        }
    }
    {
        const uint16_t ids[][2] = {{0x0bda,0x0a8a},{0xcdab,0x8011}};
        size_t i;
        for (i=0;i<sizeof(ids)/sizeof(ids[0]);++i) {
            p=rtw_usb_wifi_lookup(ids[i][0],ids[i][1],0x0200,1,0);
            CHECK(p && p->primary_verified);
            CHECK(rtw_usb_wifi_station_backend(p)==RTW_WIFI_RTL8192CU);
            CHECK(rtw_usb_wifi_lookup(ids[i][0],ids[i][1],0x0201,1,0)==NULL);
            CHECK(rtw_usb_wifi_lookup(ids[i][0],ids[i][1],0x0200,2,0)==NULL);
            CHECK(rtw_usb_wifi_lookup(ids[i][0],ids[i][1],0x0200,1,2)==NULL);
        }
    }
    puts("USB Wi-Fi profile boundary checks passed");
    return 0;
}
