/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_FAMILY_RX24_H
#define RTW_FAMILY_RX24_H

#include <stddef.h>
#include <stdint.h>
#include "rtw/USBWiFiProfiles.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Offline-only 24-byte RX descriptors.
 * rtw88: RTL8821CU/8822BU/8822CU (Linux rtw88 rx.h, usb.c).
 * rtl8xxxu: RTL8192FU/8723BU (Linux rtl8xxxu.h, core.c).
 * This is not a firmware loader, USB transport or macOS station backend.
 */
typedef struct {
    const uint8_t *bytes;
    size_t length;
    size_t cursor;
    unsigned frames;
} rtw_rx24_iterator;

typedef struct {
    const uint8_t *payload;
    size_t payload_length;
    const uint8_t *driver_info;
    size_t driver_info_length;
    uint32_t timestamp;
    uint8_t is_firmware_event;
    uint8_t crc_error;
    uint8_t icv_error;
    uint8_t rate;
    uint8_t bandwidth_code;
    uint8_t rf_chains;
} rtw_rx24_frame;

typedef enum {
    RTW_RX24_FRAME = 0,
    RTW_RX24_END,
    RTW_RX24_UNSUPPORTED,
    RTW_RX24_INVALID,
    RTW_RX24_MALFORMED
} rtw_rx24_status;

void rtw_rx24_iterator_init(rtw_rx24_iterator *iterator,
                            const uint8_t *bytes, size_t length);
rtw_rx24_status rtw_rx24_next(rtw_rx24_iterator *iterator,
                              rtw_wifi_family family, rtw_rx24_frame *out);

#ifdef __cplusplus
}
#endif
#endif
