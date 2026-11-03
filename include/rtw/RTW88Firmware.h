/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef RTW_RTW88_FIRMWARE_H
#define RTW_RTW88_FIRMWARE_H
#include <stddef.h>
#include <stdint.h>
#include "rtw/USBWiFiProfiles.h"
#ifdef __cplusplus
extern "C" {
#endif
#define RTW88_FW_SCRATCH_BYTES (48U+4096U+1U)
typedef struct {size_t offset,length;uint32_t address;} rtw88_fw_segment;
typedef struct {uint16_t signature,version;uint8_t subversion;rtw88_fw_segment dmem,imem,emem;} rtw88_fw_image;
typedef struct {
    void* cookie;
    int (*read)(void*,uint16_t,unsigned,uint32_t*);
    int (*write)(void*,uint16_t,unsigned,uint32_t);
    /* Bounded synchronous USB write of full TX48 descriptor + payload.
     * This is a reserved-page transfer, never a station frame submission. */
    int (*bulk_write)(void*,uint8_t,const uint8_t*,size_t);
    void (*delay_us)(void*,uint32_t);
    /* The board HAL implements its chip-specific LTE coexistence register
     * handshake. Register0x38 is preserved across firmware boot. */
    int (*coex_read)(void*,uint16_t,uint32_t*);
    int (*coex_write)(void*,uint16_t,uint32_t);
} rtw88_fw_bus;
typedef struct {
    rtw_wifi_family family;
    rtw88_fw_bus bus;
    uint8_t identified,powered,firmware_running,h2c_box;
    int last_error;
} rtw88_fw_context;
/* No bus operations. Header parsing is not a cryptographic authenticity check.
 * IMEM origin differs: RTL8822B=0, RTL8821C/8822C=0x30000. Limits are deliberately
 * conservative port bounds, not claims of the silicon's maximum memory. */
int rtw88_fw_inspect(rtw_wifi_family,const uint8_t*,size_t,rtw88_fw_image*);
/* Chip-specific board identity and power-on must already have succeeded.
 * No power, RF tables, calibration, station attach or WPA is performed here.
 * Caller supplies nonoverlapping scratch storage to avoid a4KB kernel stack.
 * The function uses actual rtw88 reserved-page/IDDMA/startup register flows. */
int rtw88_fw_download(rtw88_fw_context*,const uint8_t*,size_t,uint8_t*,size_t);
/* Shared mailbox cursor for station, RFK and coexistence commands. Requires
 * caller serialization, running firmware, and the exact typed B/C context.
 * Checks HMETFR at most31 times with a3ms explicit delay budget. Bus
 * callbacks enforce their own request timeouts. Writes EX before MAIN and
 * advances only on success. */
int rtw88_fw_h2c(rtw88_fw_context*,const uint8_t command[8]);
/* Advisory RSSI0x42 only: mailbox contention is unlatched EBUSY; actual
 * register/USB failures, including ETIMEDOUT/EBUSY, remain sticky. */
int rtw88_fw_h2c_rssi_monitor(rtw88_fw_context*,const uint8_t command[8]);
#ifdef __cplusplus
}
#endif
#endif
