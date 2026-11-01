/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_USB_RX_CAPACITY_H
#define RTW_USB_RX_CAPACITY_H
#include "USBWiFiProfiles.h"

/* RTL8814A DMA threshold 0x2005 is20KiB, larger than the legacy16KiB read.
 * A read must hold the aggregate; the RX codec starts at a descriptor boundary.
 * Linux rtw88 uses32768-byte USB RX buffers with this same DMA threshold.
 * Primary pins:
 * aircrack-ng/rtl8814au 892641430c24308e7ae193d746b25b600ef227d8,
 * hal/rtl8814a/usb/usb_halinit.c:148 ("dmc agg th 20K").
 * torvalds/linux 0e135910deec43ffe97eca0b6af1f8f847285b59,
 * drivers/net/wireless/realtek/rtw88/usb.h:33,usb.c:884-902.
 * No USB timing, DMA register or retry-budget change is implied. */
static inline uint32_t rtw_usb_rx_capacity(rtw_wifi_family family)
{
    return family==RTW_WIFI_RTL8814AU?32768U:16384U;
}
#endif
