/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef RTW_RTW88_USB_DATA_PATH_H
#define RTW_RTW88_USB_DATA_PATH_H
#include <stddef.h>
#include <stdint.h>
#include "rtw/USBWiFiProfiles.h"
#ifdef __cplusplus
extern "C" {
#endif
#define RTW88_USB_TX_DESC_BYTES 48U
/* Descriptor packing for RTL8821CU / RTL8822BU / RTL8822CU only. The caller
 * must have a powered, calibrated, authorized station before submitting data.
 * This allocation-free encoder does not open USB, transmit, enable a backend,
 * choose a regulatory channel, or implement firmware/RF initialization.
 * Security bits remain zero: callers must supply software-encrypted MPDUs. */
typedef struct {
    uint16_t sequence;       /* software 802.11 sequence, 0..4095 */
    uint16_t report_tag;     /* firmware report correlation, 0..4095 */
    uint8_t mac_id;
    uint8_t queue;          /* QoS TID0..15, beacon16, high17, management18, H2C19 */
    uint8_t rate_id;        /* firmware rate-table ID, 0..31 */
    uint8_t rate;           /* Linux rtw88 descriptor rate: legacy0..11, HT12+, VHT44+ */
    uint8_t bandwidth;      /* 0=20MHz, 1=40MHz, 2=80MHz */
    uint8_t band_5ghz;
    uint8_t multicast;
    uint8_t firmware_rate;
    uint8_t short_gi;
    uint8_t ldpc;
    uint8_t stbc;
    uint8_t ampdu;
    uint8_t ampdu_density;  /* 0..7 */
    uint8_t ampdu_factor;   /* maximum aggregation count, 0..31 */
    uint8_t request_report;
    uint8_t disable_fallback;
    uint8_t rts;
    uint8_t hardware_sequence;
    uint8_t disable_queue_sequence;
} rtw88_usb_tx_options;
typedef enum {
    RTW88_USB_TX_OK=0,
    RTW88_USB_TX_INVALID_ARGUMENT,
    RTW88_USB_TX_UNSUPPORTED_FAMILY,
    RTW88_USB_TX_UNSUPPORTED_RATE
} rtw88_usb_tx_result;
/* Writes exactly48 bytes on success; failure leaves caller output untouched.
 * The 8821C/8822B checksum covers32 bytes; 8822C covers all48 bytes.
 * Variable packet offset and USB aggregation are deliberately not emitted. */
rtw88_usb_tx_result rtw88_usb_make_tx_descriptor(rtw_wifi_family family,
    size_t mpdu_length,const rtw88_usb_tx_options *options,
    uint8_t *descriptor,size_t descriptor_length);
/* Exact USB control packets use different defaults from station descriptors.
 * Reserved firmware pages have offset48/LS1; H2C has offset0/LS0. Both leave
 * station rate/sequence flags zero. Reserved length1..4097,H2C length32.
 * No payload or USB operation is performed. */
typedef enum {RTW88_USB_CONTROL_RESERVED=0,RTW88_USB_CONTROL_H2C=1} rtw88_usb_control_kind;
rtw88_usb_tx_result rtw88_usb_make_control_descriptor(rtw_wifi_family,size_t,
    rtw88_usb_control_kind,uint8_t*,size_t);
#ifdef __cplusplus
}
#endif
#endif
