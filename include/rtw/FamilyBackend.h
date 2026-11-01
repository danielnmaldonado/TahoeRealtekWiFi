/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_FAMILY_BACKEND_H
#define RTW_FAMILY_BACKEND_H

#include <stddef.h>
#include <stdint.h>
#include "rtw/USBWiFiProfiles.h"
#include "rtw/USBDescriptors.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Capability flags describe implementation in THIS source tree.
 * Discovery of USB endpoints is never proof that firmware, radio or station
 * authentication works. Hardware is never accessed by these APIs. */
enum {
    RTW_BACKEND_CAP_USB_PREFLIGHT = 1u << 0,
    RTW_BACKEND_CAP_RX_WIRE = 1u << 1,
    RTW_BACKEND_CAP_TX_WIRE = 1u << 2,
    RTW_BACKEND_CAP_OFFLINE_MGMT = 1u << 3,
    RTW_BACKEND_CAP_RADIO_DIAGNOSTICS = 1u << 4,
    RTW_BACKEND_CAP_KEXT_STATION = 1u << 5,
    RTW_BACKEND_CAP_LEGACY_FIRMWARE_HEADER = 1u << 6,
    RTW_BACKEND_CAP_PASSIVE_CHANNEL_GEOMETRY = 1u << 7
};

typedef enum {
    RTW_BACKEND_RX_NONE = 0,
    RTW_BACKEND_RX_LEGACY_CU,
    RTW_BACKEND_RX_LEGACY_88EU,
    RTW_BACKEND_RX_8192EU,
    RTW_BACKEND_RX_8814AU,
    RTW_BACKEND_RX_8812AU,
    RTW_BACKEND_RX_RTW88,
    RTW_BACKEND_RX_RTL8XXXU,
    RTW_BACKEND_RX_8192DU,
    RTW_BACKEND_RX_8710BU
} rtw_backend_rx_codec;

typedef struct {
    rtw_wifi_family family;
    const char *reference_driver;
    const char *remaining_requirement;
    uint32_t capabilities;
    rtw_backend_rx_codec rx_codec;
    uint8_t rx_chains;
} rtw_family_backend;

/* One immutable entry for each catalog family. Missing PHY, firmware,
 * association or security integrations are declared explicitly. */
const rtw_family_backend *rtw_backend_for_family(rtw_wifi_family family);
size_t rtw_backend_family_count(void);
const rtw_family_backend *rtw_backend_family_at(size_t index);

typedef enum {
    RTW_BACKEND_PREFLIGHT_STATION = 0,
    RTW_BACKEND_PREFLIGHT_DIAGNOSTICS,
    RTW_BACKEND_PREFLIGHT_NOT_CATALOGUED,
    RTW_BACKEND_PREFLIGHT_UNVERIFIED,
    RTW_BACKEND_PREFLIGHT_DESCRIPTOR_MISMATCH,
    RTW_BACKEND_PREFLIGHT_BAD_ENDPOINTS,
    RTW_BACKEND_PREFLIGHT_INVALID_ARGUMENT
} rtw_backend_preflight_status;

typedef struct {
    const rtw_usb_wifi_profile *profile;
    const rtw_family_backend *backend;
    rtw_usb_endpoints endpoints;
    uint8_t station_backend_integrated;
    uint8_t usb_preflight_only;
} rtw_backend_usb_plan;

/* Validate the actual configuration blob as well as VID/PID/revision.
 * Distinguish an unverified identity from a verified but unfinished backend.
 * Caller retains the descriptor bytes; no memory is allocated and no USB
 * requests are issued. A plan never authorizes DMA, firmware, or TX. */
rtw_backend_preflight_status rtw_backend_usb_preflight(
    uint16_t vendor, uint16_t product, uint16_t bcd_device,
    const uint8_t *config_descriptor, size_t config_length,
    uint8_t interface_number, rtw_backend_usb_plan *out);

typedef enum {
    RTW_BACKEND_RX_VALID = 0,
    RTW_BACKEND_RX_UNAVAILABLE,
    RTW_BACKEND_RX_INVALID_INPUT,
    RTW_BACKEND_RX_MALFORMED,
    RTW_BACKEND_RX_BAD_RADIO_FRAME
} rtw_backend_rx_status;

typedef struct {
    size_t frames;
    size_t management_or_data_frames;
    size_t firmware_events;
    size_t error_offset;
} rtw_backend_rx_report;

/* Frame-atomic offline RX aggregate preflight for the implemented codecs.
 * A malformed suffix invalidates the entire candidate aggregate; callers
 * must not publish any frame until the function returns VALID.
 * Unsupported families return UNAVAILABLE, never decode with a guessed ABI. */
rtw_backend_rx_status rtw_backend_rx_preflight(
    rtw_wifi_family family, const uint8_t *transfer, size_t length,
    rtw_backend_rx_report *out);

const char *rtw_backend_preflight_status_name(rtw_backend_preflight_status status);
const char *rtw_backend_rx_status_name(rtw_backend_rx_status status);

#ifdef __cplusplus
}
#endif
#endif
