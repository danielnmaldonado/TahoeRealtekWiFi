/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_USB_RX_CAPACITY_H
#define RTW_USB_RX_CAPACITY_H
#include "USBWiFiProfiles.h"

/* RTL8192EU USB aggregation threshold 6 is 24 KiB (6*4 KiB).
 * A 32 KiB read also holds the final currently admitted 4096-byte MPDU plus
 * descriptor 24 + driver info 120 + shift 3 + alignment 7: 28826 bytes total.
 * Mange/rtl8192eu-linux-driver c629cc972a0d878c930d19c4a9309c27c1e61f92,
 * hal/rtl8192e/usb/usb_halinit.c:148-155,445-460 (USB aggregation units),
 * include/rtl8192e_recv.h:18-40 (ordinary USB MAX_RECVBUF_SZ=32768).
 * RTL8812AU/8821AU use rtw88 v2: threshold5 (20KiB) at full/high speed,
 * threshold6 (24KiB) at super speed. Their final admitted4096-byte unit
 * plus descriptor24/info32/shift3/pad7 yields28738 bytes at that threshold.
 * RTL8814AU/8821CU/8822BU/8822CU use rtw88 v1: threshold5 (20KiB).
 * The largest accepted11454-byte wire payload plus descriptor24/info32/
 * shift3/pad7 yields32000 bytes, still inside32768. rtw88 usb.c:884-940
 * selects v1/v2 by chip; usb.h:33 specifies the32768-byte receive buffer.
 * The earlier16KiB reads could split a complete USB aggregate.
 * A read must hold the aggregate; the RX codec starts at a descriptor boundary.
 * Linux rtw88 uses32768-byte USB RX buffers with this same DMA threshold.
 * Primary pins:
 * aircrack-ng/rtl8814au 892641430c24308e7ae193d746b25b600ef227d8,
 * hal/rtl8814a/usb/usb_halinit.c:148 ("dmc agg th 20K").
 * torvalds/linux 0e135910deec43ffe97eca0b6af1f8f847285b59,
 * drivers/net/wireless/realtek/rtw88/usb.h:33,usb.c:884-902.
 * The HAL selects aggregation mode; this helper supplies read capacity. */
static inline uint32_t rtw_usb_rx_capacity(rtw_wifi_family family)
{
    switch(family){
    case RTW_WIFI_RTL8192EU:
    case RTW_WIFI_RTL8812AU:
    case RTW_WIFI_RTL8821AU:
    case RTW_WIFI_RTL8814AU:
    case RTW_WIFI_RTL8821CU:
    case RTW_WIFI_RTL8822BU:
    case RTW_WIFI_RTL8822CU:return 32768U;
    default:return 16384U;
    }
}
#endif
