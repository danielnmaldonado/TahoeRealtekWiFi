/* SPDX-License-Identifier: GPL-2.0-only
   RTL8812AU interoperability; implementation based on vendor sequences and
   GPL-2.0 reference xen-proc/rtl8812au-macos and aircrack-ng/rtl8812au.
   This does NOT attest to validation on the user's physical hardware. */
#include "rtw/HAL8812.h"
#include "rtw/Chip.h"
#include <string.h>

rtw_hal_error rtw_hal_boot_pre_rf_8812au(const rtw_hal_environment* env,
                                          rtw_hal_report* report) {
    rtw_hal_stage target=RTW_HAL_CHIP_IDENTIFIED;
    rtw_chip_info chip;
    uint32_t cfg=0;
    uint8_t physical[RTW_EFUSE_BYTES];
    uint8_t logical[RTW_EFUSE_BYTES];
    const rtw_boot_options opts={400,5000};
    const rtw_mac_options mac={env ? env->out_endpoints : 0,4,0};
    rtw_hal_error error=RTW_HAL_OK;
    if(report) memset(report,0,sizeof(*report));
    if(!report || !env || !env->bus || !env->bus->control_in ||
       !env->bus->control_out || !env->waiter || !env->waiter->wait_us ||
       !env->firmware_image || !env->firmware_bytes ||
       env->out_endpoints!=3)
        return RTW_HAL_BAD_ARGUMENT;
    if (!env->allow_hardware_writes) return RTW_HAL_NOT_AUTHORIZED;
    /* Completely verify supplied firmware before touching any register. */
    {
        rtw_fw_image inspected;
        if(rtw_fw_inspect_8812au(env->firmware_image,env->firmware_bytes,&inspected)!=RTW_FW_OK)
            return RTW_HAL_BAD_ARGUMENT;
    }
    if (rtw_usb_rw_read(env->bus,0x00F0,4,&cfg)!=RTW_USB_OK ||
        !rtw_chip_decode_syscfg(cfg,&chip) || chip.cut!=1 || !chip.is_usb ||
        chip.is_umc || chip.is_test_chip) {
        error=RTW_HAL_UNSUPPORTED_HARDWARE; goto fail;
    }
    report->sys_cfg=cfg;
    report->last_success=RTW_HAL_CHIP_IDENTIFIED;
    /* Cold chips may not allow OTP register access before power-on.
       Execute the B-cut-specific power flow first, then read OTP safely. */
    target=RTW_HAL_POWERED;
    if(rtw_power_on_8812au_bcut_tsmc_usb(env->bus,env->waiter,cfg,
                                         &report->power_steps)!=RTW_POWER_OK) {
        error=RTW_HAL_POWER_FAILURE; goto fail;
    }
    report->last_success=RTW_HAL_POWERED;
    target=RTW_HAL_EFUSE_READ;
    if(rtw_efuse_read_physical(env->bus,physical,sizeof(physical),64)!=RTW_EFUSE_OK ||
       rtw_efuse_decode(physical,sizeof(physical),logical,sizeof(logical),NULL)!=RTW_EFUSE_OK ||
       rtw_efuse_extract_identity(logical,sizeof(logical),&report->identity)!=RTW_EFUSE_OK) {
        error=RTW_HAL_EFUSE_FAILURE; goto fail;
    }
    /* One board/RFE profile only. Expanding IDs requires independently
       verifying power tables and RF calibration for each hardware variant. */
    if(report->identity.eeprom_vendor!=0x0BDA ||
       report->identity.eeprom_product!=0x8812 ||
       report->identity.rfe_type!=3) {
        error=RTW_HAL_UNSUPPORTED_HARDWARE; goto fail;
    }
    report->last_success=RTW_HAL_EFUSE_READ;
    target=RTW_HAL_LLT_READY;
    if(rtw_llt_initialize_8812au(env->bus,env->waiter,24,
                                 &report->llt_entries)!=RTW_LLT_OK) {
        error=RTW_HAL_LLT_FAILURE;goto fail;
    }
    report->last_success=RTW_HAL_LLT_READY;
    target=RTW_HAL_FIRMWARE_READY;
    if(rtw_fw_boot_8812au(env->bus,env->waiter,env->firmware_image,
                          env->firmware_bytes,&opts,&report->firmware)!=RTW_BOOT_OK) {
        error=RTW_HAL_FIRMWARE_FAILURE;goto fail;
    }
    report->last_success=RTW_HAL_FIRMWARE_READY;
    target=RTW_HAL_MAC_PREPARED;
    if(rtw_mac_dma_prepare_8812au(env->bus,&mac,
                                  &report->mac_registers)!=RTW_MAC_OK) {
        error=RTW_HAL_MAC_FAILURE;goto fail;
    }
    report->last_success=RTW_HAL_MAC_PREPARED;
    /* Deliberately never signal interface/radio readiness before real RF
       profiles, scanning, station association and WPA2/CCMP exist. */
    report->radio_ready=0;
    report->ethernet_authorized=0;
    return RTW_HAL_OK;
fail:
    report->failed_at=target;
    report->radio_ready=0;
    report->ethernet_authorized=0;
    return error;
}
const char* rtw_hal_error_string(rtw_hal_error error) {
    switch(error) {
        case RTW_HAL_OK:return "MAC and firmware prepared; RF NOT READY";
        case RTW_HAL_NOT_AUTHORIZED:return "hardware writes not authorized";
        case RTW_HAL_BAD_ARGUMENT:return "invalid HAL options or firmware";
        case RTW_HAL_UNSUPPORTED_HARDWARE:return "unsupported chipset / efuse profile";
        case RTW_HAL_EFUSE_FAILURE:return "physical efuse read/parse failed";
        case RTW_HAL_POWER_FAILURE:return "power-on sequence failed";
        case RTW_HAL_LLT_FAILURE:return "packet buffer LLT failed";
        case RTW_HAL_FIRMWARE_FAILURE:return "firmware download or boot failed";
        case RTW_HAL_MAC_FAILURE:return "MAC DMA preparation failed";
        default:return "unknown";
    }
}
