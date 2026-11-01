#ifndef RTW_FIRMWARE_BOOT_H
#define RTW_FIRMWARE_BOOT_H
#include "rtw/Firmware.h"
#include "rtw/PowerSequence.h"
#include "rtw/USBRegisterIO.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RTW_BOOT_OK=0, RTW_BOOT_BAD_ARGUMENT, RTW_BOOT_BAD_FIRMWARE,
    RTW_BOOT_USB_ERROR, RTW_BOOT_CHECKSUM_TIMEOUT,
    RTW_BOOT_INIT_TIMEOUT, RTW_BOOT_WAIT_ERROR
} rtw_boot_error;
typedef struct {
    uint32_t polls;         /* 1..2000; verified bit with one USB read/poll */
    uint32_t poll_delay_us; /* 1..10000 */
} rtw_boot_options;
typedef struct {
    uint16_t version;
    uint8_t subversion;
    uint8_t pages_written;
    size_t bytes_written;
    size_t vendor_writes;
    uint32_t last_mcu_fwdl;
    uint8_t checksum_seen;
    uint8_t firmware_ready;
} rtw_boot_report;
/* Firmware supplied by caller, NEVER bundled or fetched by the kext.
   Performs real 8051 USB register writes with bounded polls. No RF setup.
   On any failure after enabling download, attempts to disable the\n   download-enable bit and returns an error. Clears stale checksum status\n   before downloading; never treats an earlier checksum as a new result.
   Calling this twice without cold unplug/required chip reset may fail. */
rtw_boot_error rtw_fw_boot_8812au(const rtw_usb_rw_transport* transport,
                                   const rtw_waiter* waiter,
                                   const uint8_t* firmware, size_t length,
                                   const rtw_boot_options* options,
                                   rtw_boot_report* report);
const char* rtw_boot_error_string(rtw_boot_error err);
#ifdef __cplusplus
}
#endif
#endif
