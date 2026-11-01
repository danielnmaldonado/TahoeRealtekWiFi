/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_USB_WIFI_FIRMWARE_H
#define RTW_USB_WIFI_FIRMWARE_H
#include "USBWiFiProfiles.h"
#include "RTL8192EU.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Validate the family-specific 8051 header before copying/loading any bytes.
 * This is a format check, not authentication or proof of correct board cut. */
int rtw_usb_wifi_inspect_firmware(rtw_wifi_family, const uint8_t *, size_t,
    rtw_8192eu_fw_info *);
#ifdef __cplusplus
}
#endif
#endif
