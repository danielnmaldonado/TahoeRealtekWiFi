/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_USB_PROBE_H
#define RTW_USB_PROBE_H
#include "rtw/Chip.h"
#include "rtw/RTL8192EU.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RTW_USB_PROBE_FAMILY_NONE=0,
    RTW_USB_PROBE_FAMILY_RTL8812AU,
    RTW_USB_PROBE_FAMILY_RTL8192EU
} rtw_usb_probe_family;

typedef enum {
    RTW_USB_PROBE_OK=0,
    RTW_USB_PROBE_BAD_ARGUMENT,
    RTW_USB_PROBE_UNSUPPORTED_ID,
    RTW_USB_PROBE_READ_FAILED,
    RTW_USB_PROBE_UNSUPPORTED_PROFILE
} rtw_usb_probe_result;

typedef struct {
    rtw_usb_probe_family family;
    rtw_chip_info rtl8812au;
    rtw_8192eu_identity rtl8192eu;
} rtw_usb_probe_report;

/* Exactly 0bda:8812 and 2357:0108. Caller supplies actual enumerated VID/PID.
 * Unsupported IDs cause zero vendor requests. Supported IDs cause only bounded
 * register READs: one for 8812AU, four for 8192EU. No writes, firmware, RF,
 * network interfaces or authorizations. On failure the report is cleared.
 * A successful probe is only an identity/profile check, not driver readiness.
 */
rtw_usb_probe_result rtw_usb_probe_read_only(const rtw_usb_transport* transport,
    uint16_t vid,uint16_t pid,rtw_usb_probe_report* report);

#ifdef __cplusplus
}
#endif
#endif
