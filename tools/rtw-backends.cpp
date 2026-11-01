// SPDX-License-Identifier: GPL-2.0-only
#include "rtw/FamilyBackend.h"
#include "rtw/LegacyFirmwareStage.h"
#include <cstdio>
#include <cstring>

int main(int argc, char **argv) {
    const bool json = argc == 2 && std::strcmp(argv[1], "--json") == 0;
    const bool adapters_json = argc == 2 && std::strcmp(argv[1], "--adapters-json") == 0;
    if (argc > 2 || (argc == 2 && !json && !adapters_json)) {
        std::fprintf(stderr, "Usage: rtw-backends [--json|--adapters-json]\n");
        return 2;
    }
    if (adapters_json) {
        /* Report EACH immutable catalog ID, without presenting an unverified
         * chipset guess as a certified USB board or an unfinished PHY as a
         * working macOS station. No USB hardware access occurs here. */
        std::puts("{\"adapters\":[");
        unsigned station = 0, verified = 0;
        for (size_t i=0; i<rtw_usb_wifi_profile_count(); ++i) {
            const auto* p = rtw_usb_wifi_profile_at(i);
            const auto c = rtw_wifi_radio_profile_capabilities(p);
            const bool enabled = rtw_usb_wifi_station_backend(p) != RTW_WIFI_UNKNOWN;
            verified += c.identity_verified != 0;
            station += enabled;
            std::printf(
                "%s{\"vid_pid\":\"%04x:%04x\",\"family\":\"%s\","
                "\"identity_verified\":%s,\"station_enabled\":%s,"
                "\"chip_family_2ghz\":%s,\"chip_family_5ghz\":%s,"
                "\"station_2ghz\":%s,\"station_5ghz\":%s,"
                "\"family_nss_ceiling\":%u,\"station_nss_ceiling\":%u,"
                "\"family_2g_width_mhz\":%u,\"family_5g_width_mhz\":%u,"
                "\"station_2g_width_mhz\":%u,\"station_5g_width_mhz\":%u,"
                "\"family_phy_ceiling_mbps\":%u,\"station_phy_ceiling_mbps\":%u}",
                i ? ",\n" : "", p->vendor, p->product,
                rtw_usb_wifi_family_name(p->family),
                c.identity_verified ? "true" : "false", enabled ? "true" : "false",
                (c.family_bands & RTW_WIFI_BAND_2GHZ) ? "true" : "false",
                (c.family_bands & RTW_WIFI_BAND_5GHZ) ? "true" : "false",
                (c.station_bands & RTW_WIFI_BAND_2GHZ) ? "true" : "false",
                (c.station_bands & RTW_WIFI_BAND_5GHZ) ? "true" : "false",
                c.family_max_nss, c.station_max_nss,
                c.family_max_2g_width_mhz, c.family_max_5g_width_mhz,
                c.station_max_2g_width_mhz, c.station_max_5g_width_mhz,
                c.family_max_phy_mbps, c.station_max_phy_mbps);
        }
        std::printf("\n],\"catalog_profiles\":%zu,\"verified_profiles\":%u,"
                    "\"station_profiles\":%u,"
                    "\"reference_only_not_certified\":true}\n",
                    rtw_usb_wifi_profile_count(), verified, station);
        return 0;
    }
    if (json) std::puts("{\"firmware_diagnostics_scope\":\"rtw-legacy-fwdiag only; typed kext bootstrap is separate\",\"families\":[");
    else std::puts("Family       Profiles Verified KextStation WireRX Firmware/RF next step");
    unsigned n_supported = 0, n_verified = 0;
    for (size_t i = 0; i < rtw_backend_family_count(); ++i) {
        const rtw_family_backend *backend = rtw_backend_family_at(i);
        const auto radio = rtw_wifi_radio_family_capabilities(backend->family);
        unsigned profiles = 0, verified = 0, station = 0;
        for (size_t j = 0; j < rtw_usb_wifi_profile_count(); ++j) {
            const rtw_usb_wifi_profile *p = rtw_usb_wifi_profile_at(j);
            if (!p || p->family != backend->family) continue;
            ++profiles;
            if (p->primary_verified) ++verified;
            if (rtw_usb_wifi_station_backend(p) == backend->family) ++station;
        }
        n_supported += station;
        n_verified += verified;
        if (json) {
            std::printf(
                "%s{\"name\":\"%s\",\"catalog\":%u,\"verified\":%u,"
                "\"station_profiles\":%u,\"usb_preflight\":true,"
                "\"rx_wire_codec\":%s,\"firmware_header_inspector\":%s,"
                "\"passive_radio_geometry\":%s,"
                "\"firmware_download_plan\":%s,"
                "\"experimental_usb_firmware_boot\":%s,"
                "\"kext_station_integrated\":%s,"
                "\"chip_family_2ghz\":%s,\"chip_family_5ghz\":%s,"
                "\"station_2ghz\":%s,\"station_5ghz\":%s,"
                "\"family_phy_ceiling_mbps\":%u,"
                "\"station_phy_ceiling_mbps\":%u}",
                i ? ",\n" : "", rtw_usb_wifi_family_name(backend->family),
                profiles, verified, station,
                backend->capabilities & RTW_BACKEND_CAP_RX_WIRE ? "true" : "false",
                backend->capabilities & RTW_BACKEND_CAP_LEGACY_FIRMWARE_HEADER ? "true" : "false",
                backend->capabilities & RTW_BACKEND_CAP_PASSIVE_CHANNEL_GEOMETRY ? "true" : "false",
                rtw_legacy_fw_capability(backend->family) > 0 ? "true" : "false",
                rtw_legacy_fw_capability(backend->family) > 1 ? "true" : "false",
                backend->capabilities & RTW_BACKEND_CAP_KEXT_STATION ? "true" : "false",
                radio.family_bands & RTW_WIFI_BAND_2GHZ ? "true" : "false",
                radio.family_bands & RTW_WIFI_BAND_5GHZ ? "true" : "false",
                radio.station_bands & RTW_WIFI_BAND_2GHZ ? "true" : "false",
                radio.station_bands & RTW_WIFI_BAND_5GHZ ? "true" : "false",
                radio.family_max_phy_mbps, radio.station_max_phy_mbps);
        } else {
            std::printf("%-13s %-8u %-8u %-11u %-6s %s\n",
                rtw_usb_wifi_family_name(backend->family), profiles, verified,
                station,
                backend->capabilities & RTW_BACKEND_CAP_RX_WIRE ? "yes" : "no",
                backend->remaining_requirement);
        }
    }
    if (json) std::printf("\n],\"catalog_profiles\":%zu,\"verified_profiles\":%u,"
                          "\"station_profiles\":%u}\n",
                          rtw_usb_wifi_profile_count(), n_verified, n_supported);
    else std::printf("TOTAL: %zu catalog / %u identity-verified / "
                     "%u KEXT-STATION PROFILES (not hardware-certified)\n",
                     rtw_usb_wifi_profile_count(), n_verified, n_supported);
    return 0;
}
