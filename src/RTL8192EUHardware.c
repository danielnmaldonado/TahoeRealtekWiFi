/* SPDX-License-Identifier: GPL-2.0-only
 * RTL8192EU (92E) USB 8051 bring-up, built from kernel-safe C.
 * Reference: lwfinger/rtl8xxxu rtl8xxxu_8192e.c and rtl8xxxu_core.c.
 * Independent implementation; NOT the RTL8812AU power / firmware flow.
 */
#include "rtw/RTL8192EUHardware.h"
#include <string.h>

#define REG_SYS_FUNC 0x0002U
#define REG_APS_FSMCO 0x0004U
#define REG_RSV_CTRL 0x001CU
#define REG_MCUFWDL 0x0080U
#define REG_FW_PAGE 0x0082U
#define FW_APERTURE 0x1000U
#define FW_PAGE_SIZE 4096U
#define FW_WRITE_BLOCK 128U
#define FW_RAM_SEL 0x80U
#define FW_DL_ENABLE 0x01U
#define FW_DL_READY 0x02U
#define FW_CSUM 0x04U
#define FW_WINT_READY 0x40U
#define SYS_FUNC_CPU_ENABLE (1U << 10U)
#define MAC_ENABLE (1U << 8U)
#define POWER_READY (1U << 17U)

static int reg_read(const rtw_usb_rw_transport *t,uint16_t addr,uint8_t width,uint32_t *out) {
    return rtw_usb_rw_read(t,addr,width,out) == RTW_USB_OK;
}
static int reg_write(const rtw_usb_rw_transport *t,uint16_t addr,uint8_t width,uint32_t val) {
    return rtw_usb_write_register(t,addr,width,val) == RTW_USB_OK;
}
static int mask8(const rtw_usb_rw_transport *t,uint16_t addr,uint8_t clear,uint8_t set) {
    uint32_t val=0;
    return reg_read(t,addr,1U,&val) &&
           reg_write(t,addr,1U,((val & (~(uint32_t)clear & 0xFFU)) | set));
}
static int mask16(const rtw_usb_rw_transport *t,uint16_t addr,uint16_t clear,uint16_t set) {
    uint32_t val=0;
    return reg_read(t,addr,2U,&val) &&
           reg_write(t,addr,2U,((val & (~(uint32_t)clear & 0xFFFFU)) | set));
}
/* 8192eu reset: RSV_CTRL+1 bit0, SYS_FUNC bit10. */
static int reset_mcu(const rtw_usb_rw_transport *t) {
    return mask8(t,REG_RSV_CTRL+1U,0x01U,0) &&
           mask16(t,REG_SYS_FUNC,SYS_FUNC_CPU_ENABLE,0) &&
           mask8(t,REG_RSV_CTRL+1U,0,0x01U) &&
           mask16(t,REG_SYS_FUNC,0,SYS_FUNC_CPU_ENABLE);
}
static int cleanup_fwdl(const rtw_usb_rw_transport *t) {
    return mask16(t,REG_MCUFWDL,FW_DL_ENABLE,0);
}
static rtw_92e_boot_result poll_reg(const rtw_usb_rw_transport *t,
   const rtw_waiter *wait,const rtw_92e_boot_config *config,
   uint16_t reg,uint32_t mask,int want_set,rtw_92e_boot_result exhausted,
   uint32_t *out) {
    uint32_t val=0;
    for(uint32_t i=0;i<config->max_polls;++i) {
        if(!reg_read(t,reg,4U,&val)) return RTW_92E_BOOT_USB_FAILED;
        if ((!!(val&mask)) == (!!want_set)) { if(out) *out=val; return RTW_92E_BOOT_OK; }
        if(i+1U<config->max_polls &&
           !wait->wait_us(wait->context,config->sleep_us))
            return RTW_92E_BOOT_SLEEP_FAILED;
    }
    if(out) *out=val;
    return exhausted;
}
static rtw_92e_boot_result power_on(const rtw_usb_rw_transport *t,
    const rtw_waiter *wait,const rtw_92e_boot_config *config) {
    rtw_92e_boot_result err;
    uint32_t reg=0;
    /* RTL8192e disabled->EMU and EMU->active transitions (register 0x0005). */
    if(!mask8(t,REG_APS_FSMCO+1U,0x9CU,0)) return RTW_92E_BOOT_USB_FAILED;
    err=poll_reg(t,wait,config,REG_APS_FSMCO,POWER_READY,1,
                 RTW_92E_BOOT_POWER_TIMEOUT,NULL);
    if(err!=RTW_92E_BOOT_OK) return err;
    if(!mask8(t,REG_APS_FSMCO+2U,0,0x01U)) return RTW_92E_BOOT_USB_FAILED;
    if(!reg_read(t,REG_APS_FSMCO,4U,&reg) ||
       !reg_write(t,REG_APS_FSMCO,4U,reg|MAC_ENABLE))
        return RTW_92E_BOOT_USB_FAILED;
    return poll_reg(t,wait,config,REG_APS_FSMCO,MAC_ENABLE,0,
                    RTW_92E_BOOT_MAC_TIMEOUT,NULL);
}
static rtw_92e_boot_result download_firmware(const rtw_usb_rw_transport *t,
    const rtw_waiter *wait,const rtw_92e_boot_config *config,
    const rtw_8192eu_fw_info *image, rtw_92e_boot_report *report) {
    uint32_t value=0;
    rtw_92e_boot_result err=RTW_92E_BOOT_OK;
    int download_enabled=0;

    /* Enable MCU clock before download. */
    if(!mask8(t,REG_SYS_FUNC+1U,0,0x04U) ||
       !mask16(t,REG_SYS_FUNC,0,SYS_FUNC_CPU_ENABLE) ||
       !reg_read(t,REG_MCUFWDL,1U,&value))
        return RTW_92E_BOOT_USB_FAILED;
    if(value&FW_RAM_SEL) {
        if(!reg_write(t,REG_MCUFWDL,1U,0) || !reset_mcu(t))
            return RTW_92E_BOOT_USB_FAILED;
    }
    if(!mask8(t,REG_MCUFWDL,0,FW_DL_ENABLE)) {
        (void)cleanup_fwdl(t);
        return RTW_92E_BOOT_USB_FAILED;
    }
    download_enabled=1;
    /* Reset 8051 before block upload and clear the old checksum latch.
       Bit 19 lives in 0x0082 bit3; leave the page select intact. */
    if(!reg_read(t,REG_MCUFWDL,4U,&value) ||
       !reg_write(t,REG_MCUFWDL,4U,value & ~(1U<<19U)) ||
       !mask8(t,REG_MCUFWDL,0,FW_CSUM)) {
        err=RTW_92E_BOOT_USB_FAILED; goto cleanup;
    }
    for(size_t offset=0;offset<image->payload_length;) {
        size_t page=offset/FW_PAGE_SIZE;
        size_t in_page=offset%FW_PAGE_SIZE;
        size_t amount=image->payload_length-offset;
        if(amount>FW_WRITE_BLOCK) amount=FW_WRITE_BLOCK;
        if(amount>FW_PAGE_SIZE-in_page) amount=FW_PAGE_SIZE-in_page;
        if(in_page==0U) {
            uint32_t current=0;
            if(page>7U || !reg_read(t,REG_FW_PAGE,1U,&current) ||
               !reg_write(t,REG_FW_PAGE,1U,(current & 0xF8U)|(uint32_t)page)) {
                err=RTW_92E_BOOT_USB_FAILED;goto cleanup;
            }
            report->pages=(uint8_t)(page+1U);
        }
        if(rtw_usb_write_block(t,(uint16_t)(FW_APERTURE+in_page),
                               image->payload+offset,(uint16_t)amount)!=RTW_USB_OK) {
            err=RTW_92E_BOOT_USB_FAILED;goto cleanup;
        }
        offset+=amount;
        report->usb_firmware_writes++;
        report->firmware_bytes+=amount;
    }
    report->last_stage=RTW_92E_STAGE_FW_DOWNLOAD;
cleanup:
    /* Disable FWDL on success and on all errors after enabling it. */
    if(download_enabled && !cleanup_fwdl(t)) err=RTW_92E_BOOT_USB_FAILED;
    if(err!=RTW_92E_BOOT_OK) return err;

    err=poll_reg(t,wait,config,REG_MCUFWDL,FW_CSUM,1,
                 RTW_92E_BOOT_CHECKSUM_TIMEOUT,&report->last_mcu_status);
    if(err!=RTW_92E_BOOT_OK) return err;
    report->last_stage=RTW_92E_STAGE_FW_CHECKSUM;
    if(!reg_read(t,REG_MCUFWDL,4U,&value) ||
       !reg_write(t,REG_MCUFWDL,4U,(value|FW_DL_READY)&~FW_WINT_READY) ||
       !reset_mcu(t)) return RTW_92E_BOOT_USB_FAILED;
    err=poll_reg(t,wait,config,REG_MCUFWDL,FW_WINT_READY,1,
                 RTW_92E_BOOT_FIRMWARE_TIMEOUT,&report->last_mcu_status);
    if(err!=RTW_92E_BOOT_OK) return err;
    report->firmware_ready=1U;
    report->last_stage=RTW_92E_STAGE_FW_RUNNING;
    return RTW_92E_BOOT_OK;
}

rtw_92e_boot_result rtw_8192eu_boot_8051(
    const rtw_usb_rw_transport *t, const rtw_waiter *wait,
    uint16_t vid,uint16_t pid,const uint8_t *firmware,size_t length,
    const rtw_92e_boot_config *config,rtw_92e_boot_report *report) {
    rtw_8192eu_fw_info fw={0};
    rtw_8192eu_identity identity={0};
    rtw_usb_transport probe;
    rtw_92e_boot_result err;
    if(report) memset(report,0,sizeof(*report));
    if(!report || !t || !t->control_in || !t->control_out ||
       !wait || !wait->wait_us || !config || !firmware ||
       config->max_polls==0 || config->max_polls>10000U ||
       config->sleep_us==0 || config->sleep_us>100000U)
        return RTW_92E_BOOT_INVALID_ARGUMENT;
    if(vid!=RTW_8192EU_USB_VID || pid!=RTW_8192EU_USB_PID)
        return RTW_92E_BOOT_WRONG_DEVICE;
    if(rtw_8192eu_inspect_firmware(firmware,length,&fw)!=RTW_8192EU_OK)
        return RTW_92E_BOOT_BAD_FIRMWARE;
    probe.context=t->context;
    probe.control_in=t->control_in;
    if(rtw_8192eu_probe_registers(&probe,vid,pid,&identity)!=RTW_8192EU_OK)
        return RTW_92E_BOOT_WRONG_DEVICE;
    report->last_stage=RTW_92E_STAGE_IDENTITY;
    report->sys_cfg=identity.sys_cfg;
    report->firmware_version=fw.major_version;
    report->firmware_subversion=fw.minor_version;
    err=power_on(t,wait,config);
    if(err!=RTW_92E_BOOT_OK) return err;
    report->last_stage=RTW_92E_STAGE_POWER;
    report->last_stage=RTW_92E_STAGE_MAC;
    return download_firmware(t,wait,config,&fw,report);
}
const char *rtw_8192eu_boot_result_string(rtw_92e_boot_result e) {
    switch(e) {
    case RTW_92E_BOOT_OK: return "8051 firmware ready (MAC/BB/RF still not configured)";
    case RTW_92E_BOOT_INVALID_ARGUMENT: return "invalid boot parameter";
    case RTW_92E_BOOT_WRONG_DEVICE: return "USB ID or silicon preflight failed";
    case RTW_92E_BOOT_BAD_FIRMWARE: return "incorrect RTL8192EU firmware";
    case RTW_92E_BOOT_USB_FAILED: return "USB transfer failed";
    case RTW_92E_BOOT_POWER_TIMEOUT: return "power-on poll timed out";
    case RTW_92E_BOOT_MAC_TIMEOUT: return "MAC enable timed out";
    case RTW_92E_BOOT_CHECKSUM_TIMEOUT: return "8051 checksum report timed out";
    case RTW_92E_BOOT_FIRMWARE_TIMEOUT: return "8051 did not signal firmware ready";
    case RTW_92E_BOOT_SLEEP_FAILED: return "poll sleep failed";
    default:return "unknown 92E boot failure";
    }
}
