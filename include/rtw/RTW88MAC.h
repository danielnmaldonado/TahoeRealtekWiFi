/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef RTW_RTW88_MAC_H
#define RTW_RTW88_MAC_H
#include "rtw/RTW88Firmware.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {RTW88_USB_FULL_SPEED=1,RTW88_USB_HIGH_SPEED=2,RTW88_USB_SUPER_SPEED=3} rtw88_usb_speed;
typedef struct {
 rtw_wifi_family family;
 uint16_t tx_pages,reserved_pages,reserved_boundary,public_pages;
 uint16_t driver_pages,csi_pages,firmware_tx_page,h2c_page,h2c_static_page,h2c_info_page;
 uint16_t high_pages,low_pages,normal_pages,extra_pages,queue_mapping;
 uint32_t rx_fifo_bytes;
 uint8_t bulk_out_count;
 uint8_t queue_endpoint[20]; /* zero-based descriptor endpoint slot;255=unavailable */
} rtw88_mac_plan;
typedef struct {
 rtw88_fw_context *device;
 rtw88_mac_plan fifo;
 uint8_t mac_ready;
} rtw88_mac_context;
/* Pure chip/FIFO geometry and USB DMA mapping. No register or RF operations. */
int rtw88_mac_prepare(rtw_wifi_family,unsigned bulk_out_count,rtw88_mac_plan*);
int rtw88_mac_queue_endpoint(const rtw88_mac_plan*,unsigned queue,uint8_t*);
/* Actual MAC/LLT/H2C/USB setup, requiring identified,powered,running firmware.
 * A failure clears mac_ready and disables CR. No PHY state is promoted here.
 * Chip-specific power-on,eFuse board validation and RF initialization belong
 * to the board HAL and must not be replaced with success flags. */
int rtw88_mac_initialize(rtw88_mac_context*,rtw88_fw_context*,unsigned bulk_out_count,rtw88_usb_speed);
#ifdef __cplusplus
}
#endif
#endif
