/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/FamilyBackend.h"
#include "rtw/RTL8192CDataPath.h"
#include "rtw/RTL8192DUDataPath.h"
#include "rtw/RTL8710BUDataPath.h"
#include "rtw/RTL8192EU.h"
#include "rtw/RTL8814AUDataPath.h"
#include "rtw/RTL8812AUDataPath.h"
#include "rtw/FamilyRX24.h"
#include <string.h>

/* Reject result/input aliasing before clearing a result object. The source
 * belongs to the caller and may be a descriptor or a shared USB RX buffer. */
static int ranges_overlap(const void *out, size_t out_len,
                          const void *input, size_t input_len) {
    uintptr_t a, b;
    if (!out || !input || !out_len || !input_len) return 0;
    a = (uintptr_t)out;
    b = (uintptr_t)input;
    return a <= b ? b - a < out_len : a - b < input_len;
}

#define USB_ONLY RTW_BACKEND_CAP_USB_PREFLIGHT
#define STATION (USB_ONLY | RTW_BACKEND_CAP_RX_WIRE | \
    RTW_BACKEND_CAP_TX_WIRE | RTW_BACKEND_CAP_KEXT_STATION)
#define RX_DIAGNOSTIC (USB_ONLY | RTW_BACKEND_CAP_RX_WIRE)
#define FW_HEADER RTW_BACKEND_CAP_LEGACY_FIRMWARE_HEADER
#define CHANNEL_MODEL RTW_BACKEND_CAP_PASSIVE_CHANNEL_GEOMETRY
#define AC_DIAGNOSTIC (USB_ONLY | RTW_BACKEND_CAP_RX_WIRE | \
    RTW_BACKEND_CAP_TX_WIRE | RTW_BACKEND_CAP_OFFLINE_MGMT)

/* Capabilities describe integrated code, not physical board validation.
 * A verified USB identity still requires the typed backend's ROM, firmware,
 * endpoint and operating-profile checks. Unsupported HALs are never aliased. */
static const rtw_family_backend backends[] = {
    {RTW_WIFI_RTL8192CU, "FreeBSD rtwn / Linux rtl8xxxu",
     "Hardware validation per board revision", STATION | FW_HEADER | CHANNEL_MODEL,
     RTW_BACKEND_RX_LEGACY_CU, 2},
    {RTW_WIFI_RTL8188EU, "FreeBSD rtwn / Linux rtl8xxxu",
     "Hardware validation per board revision", STATION | FW_HEADER | CHANNEL_MODEL,
     RTW_BACKEND_RX_LEGACY_88EU, 1},
    {RTW_WIFI_RTL8192EU, "Linux rtl8xxxu / Realtek RTL8192EU",
     "Hardware validation per board revision", STATION | FW_HEADER | CHANNEL_MODEL,
     RTW_BACKEND_RX_8192EU, 2},
    {RTW_WIFI_RTL8192DU, "Linux rtlwifi rtl8192du",
     "Physical WPA2/IP/traffic validation per board revision",
     STATION | FW_HEADER | CHANNEL_MODEL | RTW_BACKEND_CAP_OFFLINE_MGMT, RTW_BACKEND_RX_8192DU, 2},
    {RTW_WIFI_RTL8192FU, "Linux rtl8xxxu",
     "Physical WPA2/IP/traffic validation per board revision",
     STATION | FW_HEADER | CHANNEL_MODEL, RTW_BACKEND_RX_RTL8XXXU, 2},
    {RTW_WIFI_RTL8710BU, "Linux rtl8xxxu",
     "Physical WPA2/IP/traffic validation per board revision",
     STATION | FW_HEADER | CHANNEL_MODEL, RTW_BACKEND_RX_8710BU, 1},
    {RTW_WIFI_RTL8723BU, "Linux rtl8xxxu",
     "Physical WPA2/IP/traffic validation per board revision",
     STATION | FW_HEADER | CHANNEL_MODEL, RTW_BACKEND_RX_RTL8XXXU, 1},
    {RTW_WIFI_RTL8812AU, "FreeBSD rtwn r12a / Linux rtw88",
     "Physical WPA2/IP/traffic validation per board revision",
     STATION | RTW_BACKEND_CAP_RADIO_DIAGNOSTICS | CHANNEL_MODEL | RTW_BACKEND_CAP_OFFLINE_MGMT, RTW_BACKEND_RX_8812AU, 2},
    {RTW_WIFI_RTL8814AU, "Linux rtw88 RTL8814AU",
     "Hardware validation per board revision", STATION |
     RTW_BACKEND_CAP_RADIO_DIAGNOSTICS | CHANNEL_MODEL |
     RTW_BACKEND_CAP_OFFLINE_MGMT, RTW_BACKEND_RX_8814AU, 2},
    {RTW_WIFI_RTL8814BU, "Dedicated RTL8814B USB HAL and PHY; station disabled",
     "Qualified NIC firmware/protocol, station integration and physical validation",
     USB_ONLY, RTW_BACKEND_RX_NONE, 0},
    {RTW_WIFI_RTL8821AU, "FreeBSD rtwn r12a / Linux rtw88",
     "Physical WPA2/IP/traffic validation per board revision",
     STATION | RTW_BACKEND_CAP_RADIO_DIAGNOSTICS | CHANNEL_MODEL | RTW_BACKEND_CAP_OFFLINE_MGMT, RTW_BACKEND_RX_8812AU, 1},
    {RTW_WIFI_RTL8821CU, "Linux rtw88 USB",
     "Physical WPA2/IP/traffic validation per board revision",
     STATION | CHANNEL_MODEL | RTW_BACKEND_CAP_OFFLINE_MGMT, RTW_BACKEND_RX_RTW88, 1},
    {RTW_WIFI_RTL8822BU, "Linux rtw88 USB",
     "Physical WPA2/IP/traffic validation per board revision",
     STATION | CHANNEL_MODEL | RTW_BACKEND_CAP_OFFLINE_MGMT, RTW_BACKEND_RX_RTW88, 2},
    {RTW_WIFI_RTL8822CU, "Linux rtw88 USB",
     "Physical WPA2/IP/traffic validation per board revision",
     STATION | CHANNEL_MODEL | RTW_BACKEND_CAP_OFFLINE_MGMT, RTW_BACKEND_RX_RTW88, 2}
};

size_t rtw_backend_family_count(void) {
    return sizeof(backends) / sizeof(backends[0]);
}

const rtw_family_backend *rtw_backend_family_at(size_t i) {
    return i < rtw_backend_family_count() ? &backends[i] : NULL;
}

const rtw_family_backend *rtw_backend_for_family(rtw_wifi_family family) {
    size_t i;
    for (i = 0; i < rtw_backend_family_count(); ++i)
        if (backends[i].family == family) return &backends[i];
    return NULL;
}

static int catalog_id_exists(uint16_t vendor, uint16_t product) {
    size_t i;
    for (i = 0; i < rtw_usb_wifi_profile_count(); ++i) {
        const rtw_usb_wifi_profile *p = rtw_usb_wifi_profile_at(i);
        if (p && p->vendor == vendor && p->product == product) return 1;
    }
    return 0;
}

/* The endpoint parser does not expose bInterfaceClass/SubClass/Protocol.
 * Obtain those fields from the same bounded configuration descriptor; never
 * supply a guessed 0xff to the catalog match. */
static int interface_identity(const uint8_t *b, size_t length,
    uint8_t number, uint8_t out[3]) {
    size_t at, end;
    unsigned matches = 0;
    if (!b || !out || length < 9U || b[0] != 9U || b[1] != 2U)
        return 0;
    end = (size_t)b[2] + ((size_t)b[3] << 8U);
    if (end < 9U || end > length || b[4] == 0U || b[5] == 0U) return 0;
    at = 9U;
    while (at < end) {
        uint8_t size, type;
        if (end - at < 2U) return 0;
        size = b[at]; type = b[at + 1U];
        if (size < 2U || (size_t)size > end - at) return 0;
        if (type == 4U) {
            if (size < 9U) return 0;
            if (b[at + 2U] == number && b[at + 3U] == 0U) {
                if (++matches != 1U) return 0;
                out[0] = b[at + 5U];
                out[1] = b[at + 6U];
                out[2] = b[at + 7U];
            }
        }
        at += size;
    }
    return matches == 1U;
}

rtw_backend_preflight_status rtw_backend_usb_preflight(
    uint16_t vendor, uint16_t product, uint16_t bcd_device,
    const uint8_t *config, size_t length, uint8_t interface_number,
    rtw_backend_usb_plan *out) {
    uint8_t iface[3] = {0};
    const rtw_usb_wifi_profile *profile;
    const rtw_family_backend *backend;
    rtw_usb_desc_error error;
    rtw_usb_endpoints endpoints;
    rtw_backend_usb_plan next;

    if (out && ranges_overlap(out, sizeof(*out), config, length))
        return RTW_BACKEND_PREFLIGHT_INVALID_ARGUMENT;
    if (out) memset(out, 0, sizeof(*out));
    if (!out || !config || length < 9U || length > 65535U)
        return RTW_BACKEND_PREFLIGHT_INVALID_ARGUMENT;
    if (!catalog_id_exists(vendor, product))
        return RTW_BACKEND_PREFLIGHT_NOT_CATALOGUED;
    if (!interface_identity(config, length, interface_number, iface))
        return RTW_BACKEND_PREFLIGHT_DESCRIPTOR_MISMATCH;
    profile = rtw_usb_wifi_lookup_descriptor(vendor, product, bcd_device,
        config[5], interface_number, iface[0], iface[1], iface[2]);
    if (!profile) return RTW_BACKEND_PREFLIGHT_DESCRIPTOR_MISMATCH;
    if (!profile->primary_verified)
        return RTW_BACKEND_PREFLIGHT_UNVERIFIED;
    backend = rtw_backend_for_family(profile->family);
    if (!backend || (backend->capabilities & USB_ONLY) == 0U)
        return RTW_BACKEND_PREFLIGHT_INVALID_ARGUMENT;
    error = rtw_find_usb_bulk_endpoints(config, length, interface_number,
        0U, &endpoints);
    if (error != RTW_DESC_OK) return RTW_BACKEND_PREFLIGHT_BAD_ENDPOINTS;

    memset(&next, 0, sizeof(next));
    next.profile = profile;
    next.backend = backend;
    next.endpoints = endpoints;
    next.station_backend_integrated =
        (uint8_t)(rtw_usb_wifi_station_backend(profile) == profile->family &&
                  (backend->capabilities & RTW_BACKEND_CAP_KEXT_STATION) != 0U);
    next.usb_preflight_only = (uint8_t)!next.station_backend_integrated;
    *out = next;
    return next.station_backend_integrated ?
        RTW_BACKEND_PREFLIGHT_STATION :
        RTW_BACKEND_PREFLIGHT_DIAGNOSTICS;
}

rtw_backend_rx_status rtw_backend_rx_preflight(
    rtw_wifi_family family, const uint8_t *transfer, size_t length,
    rtw_backend_rx_report *out) {
    const rtw_family_backend *backend = rtw_backend_for_family(family);
    rtw_backend_rx_report next;
    rtw_rx_iterator generic;
    rtw_r12au_rx_iterator jaguar;
    rtw_rx24_iterator rx24;
    rtw_r10bu_rx_iterator bu;
    unsigned is_jaguar, is_rx24;
    if (out && ranges_overlap(out, sizeof(*out), transfer, length))
        return RTW_BACKEND_RX_INVALID_INPUT;
    if (out) memset(out, 0, sizeof(*out));
    if (!out || (!transfer && length) || length > 65536U)
        return RTW_BACKEND_RX_INVALID_INPUT;
    if (!backend || !(backend->capabilities & RTW_BACKEND_CAP_RX_WIRE) ||
        backend->rx_codec == RTW_BACKEND_RX_NONE)
        return RTW_BACKEND_RX_UNAVAILABLE;
    memset(&next, 0, sizeof(next));
    rtw_rx_iterator_init(&generic, transfer, length);
    memset(&bu,0,sizeof(bu));bu.buffer=transfer;bu.length=length;
    memset(&jaguar, 0, sizeof(jaguar));
    jaguar.buffer = transfer;
    jaguar.length = length;
    is_jaguar = backend->rx_codec == RTW_BACKEND_RX_8812AU;
    is_rx24 = backend->rx_codec == RTW_BACKEND_RX_RTW88 ||
              backend->rx_codec == RTW_BACKEND_RX_RTL8XXXU;
    rtw_rx24_iterator_init(&rx24, transfer, length);

    while (is_rx24 ? rx24.cursor < rx24.length :
           (is_jaguar ? jaguar.cursor < jaguar.length :
                        generic.cursor < generic.length)) {
        size_t before = is_rx24 ? rx24.cursor :
                        (is_jaguar ? jaguar.cursor : generic.cursor);
        unsigned firmware_event = 0U, crc = 0U, icv = 0U;
        int frame = 0, end = 0;
        if (next.frames == 64U) {
            out->error_offset = before;
            return RTW_BACKEND_RX_MALFORMED;
        }
        switch (backend->rx_codec) {
        case RTW_BACKEND_RX_LEGACY_CU:
        case RTW_BACKEND_RX_LEGACY_88EU: {
            rtw_92c_rx_frame pkt;
            rtw_packet_error e = rtw_92c_rx_next(&generic,
                backend->rx_codec == RTW_BACKEND_RX_LEGACY_88EU, &pkt);
            frame = e == RTW_PACKETS_OK; end = e == RTW_PACKETS_DONE;
            if (frame) {
                firmware_event = pkt.report_type != 0U;
                crc = pkt.packet.crc_error; icv = pkt.packet.icv_error;
            }
            break;
        }
        case RTW_BACKEND_RX_8710BU: {
            rtw_r10bu_rx_frame pkt;
            rtw_r10bu_result e=rtw_r10bu_rx_next(&bu,backend->rx_chains,&pkt);
            frame=e==RTW_R10BU_FRAME;end=e==RTW_R10BU_END;
            if(frame) {
                firmware_event=pkt.is_c2h;
                crc=pkt.crc_error;icv=pkt.icv_error;
                generic.cursor=bu.cursor;
            }
            break;
        }
        case RTW_BACKEND_RX_8192DU: {
            rtw_92du_rx_frame pkt;
            rtw_packet_error e=rtw_92du_parse_rx(transfer,length,&pkt);
            frame=e==RTW_PACKETS_OK;
            if(frame) {
                crc=pkt.packet.crc_error || !pkt.fcs_valid;
                icv=pkt.packet.icv_error;
                generic.cursor=length; // DU is a single USB MPDU, including FCS.
            }
            break;
        }
        case RTW_BACKEND_RX_8192EU: {
            rtw_8192eu_rx_frame pkt;
            rtw_packet_error e = rtw_8192eu_rx_next(&generic, &pkt);
            frame = e == RTW_PACKETS_OK; end = e == RTW_PACKETS_DONE;
            if (frame) {
                firmware_event = pkt.report_is_c2h != 0U;
                crc = pkt.payload.crc_error; icv = pkt.payload.icv_error;
            }
            break;
        }
        case RTW_BACKEND_RX_8814AU: {
            rtw_8814au_rx_frame pkt;
            rtw_packet_error e = rtw_8814au_rx_next(&generic, &pkt);
            frame = e == RTW_PACKETS_OK; end = e == RTW_PACKETS_DONE;
            if (frame) {
                firmware_event = pkt.is_c2h != 0U;
                crc = pkt.packet.crc_error; icv = pkt.packet.icv_error;
            }
            break;
        }
        case RTW_BACKEND_RX_8812AU: {
            rtw_r12au_rx_frame pkt;
            rtw_r12au_result e = rtw_r12au_rx_next(&jaguar,
                backend->rx_chains, &pkt);
            frame = e == RTW_R12AU_FRAME; end = e == RTW_R12AU_END;
            if (frame) {
                firmware_event = pkt.is_c2h != 0U;
                crc = pkt.crc_error; icv = pkt.icv_error;
            }
            break;
        }
        case RTW_BACKEND_RX_RTW88:
        case RTW_BACKEND_RX_RTL8XXXU: {
            rtw_rx24_frame pkt;
            rtw_rx24_status e = rtw_rx24_next(&rx24, family, &pkt);
            frame = e == RTW_RX24_FRAME; end = e == RTW_RX24_END;
            if (frame) {
                firmware_event = pkt.is_firmware_event;
                crc = pkt.crc_error; icv = pkt.icv_error;
            }
            break;
        }
        default:
            return RTW_BACKEND_RX_UNAVAILABLE;
        }
        if (end) break;
        if (!frame || (is_rx24 ? rx24.cursor :
            (is_jaguar ? jaguar.cursor : generic.cursor)) <= before) {
            out->error_offset = before;
            return RTW_BACKEND_RX_MALFORMED;
        }
        if (crc || icv) {
            out->error_offset = before;
            return RTW_BACKEND_RX_BAD_RADIO_FRAME;
        }
        ++next.frames;
        if (firmware_event) ++next.firmware_events;
        else ++next.management_or_data_frames;
    }
    *out = next;
    return RTW_BACKEND_RX_VALID;
}

const char *rtw_backend_preflight_status_name(rtw_backend_preflight_status s) {
    switch (s) {
    case RTW_BACKEND_PREFLIGHT_STATION: return "station backend integrated (hardware unverified)";
    case RTW_BACKEND_PREFLIGHT_DIAGNOSTICS: return "USB descriptor validated; station backend pending";
    case RTW_BACKEND_PREFLIGHT_NOT_CATALOGUED: return "unknown USB ID";
    case RTW_BACKEND_PREFLIGHT_UNVERIFIED: return "conflicting or unverified USB identity";
    case RTW_BACKEND_PREFLIGHT_DESCRIPTOR_MISMATCH: return "USB identity or interface mismatch";
    case RTW_BACKEND_PREFLIGHT_BAD_ENDPOINTS: return "invalid USB bulk endpoint configuration";
    default: return "invalid argument";
    }
}

const char *rtw_backend_rx_status_name(rtw_backend_rx_status s) {
    switch (s) {
    case RTW_BACKEND_RX_VALID: return "validated RX aggregate";
    case RTW_BACKEND_RX_UNAVAILABLE: return "chipset-specific RX decoder not implemented";
    case RTW_BACKEND_RX_INVALID_INPUT: return "invalid input";
    case RTW_BACKEND_RX_MALFORMED: return "malformed RX aggregate";
    default: return "CRC/ICV error in RX aggregate";
    }
}
