#ifndef RTW_HAL_8812_H
#define RTW_HAL_8812_H
#include "rtw/Efuse.h"
#include "rtw/FirmwareBoot.h"
#include "rtw/MacDMA.h"
#include "rtw/LLT.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RTW_HAL_NOT_STARTED=0, RTW_HAL_CHIP_IDENTIFIED,
    RTW_HAL_POWERED, RTW_HAL_EFUSE_READ, RTW_HAL_LLT_READY,
    RTW_HAL_FIRMWARE_READY, RTW_HAL_MAC_PREPARED,
    RTW_HAL_FAILED
} rtw_hal_stage;
typedef enum {
    RTW_HAL_OK=0, RTW_HAL_NOT_AUTHORIZED, RTW_HAL_BAD_ARGUMENT,
    RTW_HAL_UNSUPPORTED_HARDWARE, RTW_HAL_EFUSE_FAILURE,
    RTW_HAL_POWER_FAILURE, RTW_HAL_LLT_FAILURE,
    RTW_HAL_FIRMWARE_FAILURE, RTW_HAL_MAC_FAILURE
} rtw_hal_error;
typedef struct {
    rtw_hal_stage last_success;
    rtw_hal_stage failed_at;
    uint32_t sys_cfg;
    rtw_efuse_identity identity;
    rtw_boot_report firmware;
    size_t power_steps;
    size_t llt_entries;
    size_t mac_registers;
    /* Crucial: always 0; no RF calibration or authentication here. */
    uint8_t radio_ready;
    uint8_t ethernet_authorized;
} rtw_hal_report;
typedef struct {
    const rtw_usb_rw_transport* bus;
    const rtw_waiter* waiter;
    const uint8_t* firmware_image;
    size_t firmware_bytes;
    uint8_t out_endpoints;
    uint8_t allow_hardware_writes;
} rtw_hal_environment;
/* Single approved test profile: VID:PID=0BDA:8812 verified by outer USB
   transport, SYS_CFG B-cut/TSMC/USB and efuse 0BDA:8812 with RFE type 3.
   All hardware writes are opt-in; called ONLY by explicitly authorized tool.
   Stops before radio power calibration, channel tune, TX or Ethernet attach. */
rtw_hal_error rtw_hal_boot_pre_rf_8812au(
    const rtw_hal_environment* environment, rtw_hal_report* report);
const char* rtw_hal_error_string(rtw_hal_error error);
#ifdef __cplusplus
}
#endif
#endif
