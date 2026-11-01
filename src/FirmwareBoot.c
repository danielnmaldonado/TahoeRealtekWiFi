/* SPDX-License-Identifier: GPL-2.0-only
   RTL8812AU interoperability; implementation based on vendor sequences and
   GPL-2.0 reference xen-proc/rtl8812au-macos and aircrack-ng/rtl8812au.
   This does NOT attest to validation on the user's physical hardware. */
#include "rtw/FirmwareBoot.h"
#include <string.h>
#define RTW_REG_RSV_CTRL 0x001CU
#define RTW_REG_CPUEN 0x0003U
#define RTW_CPUEN_BIT 0x04U
#define RTW_REG_MCUFWDL 0x0080U
#define RTW_REG_MCUFWDL2 0x0082U
#define RTW_REG_MCUFWDL1 0x0081U
#define RTW_FW_DL_ENABLE 0x01U
#define RTW_FW_RDY 0x02U
#define RTW_FW_CHECKSUM 0x04U
#define RTW_FW_WINTINI 0x40U
#define RTW_FW_RAM_SEL 0x80U

static int readreg(const rtw_usb_rw_transport* t,uint16_t a,uint8_t size,uint32_t* out) {
    return rtw_usb_rw_read(t,a,size,out)==RTW_USB_OK;
}
static int writereg(const rtw_usb_rw_transport* t,uint16_t a,uint8_t size,uint32_t v) {
    return rtw_usb_write_register(t,a,size,v)==RTW_USB_OK;
}
static int masked(const rtw_usb_rw_transport* t,uint16_t a,uint8_t clear,uint8_t set) {
    uint32_t v=0;
    return readreg(t,a,1,&v) && writereg(t,a,1,(v & (uint32_t)~clear) | set);
}
/* Source reference: vendor 8051 reset protocol documented by
   xen-proc/rtl8812au-macos/rtl8812au/firmware.py (GPL-2.0). */
static int reset_8051(const rtw_usb_rw_transport* t) {
    uint32_t cpu=0;
    if (!masked(t,RTW_REG_RSV_CTRL,0x02U,0) ||
        !masked(t,RTW_REG_RSV_CTRL+1U,0x08U,0) ||
        !readreg(t,RTW_REG_CPUEN,1,&cpu) ||
        !writereg(t,RTW_REG_CPUEN,1,cpu & ~RTW_CPUEN_BIT) ||
        !masked(t,RTW_REG_RSV_CTRL,0x02U,0) ||
        !masked(t,RTW_REG_RSV_CTRL+1U,0,0x08U) ||
        !writereg(t,RTW_REG_CPUEN,1,cpu | RTW_CPUEN_BIT)) return 0;
    return 1;
}
static int enable_firmware_dl(const rtw_usb_rw_transport* t) {
    uint32_t status = 0U;
    if (!masked(t,RTW_REG_CPUEN,0,RTW_CPUEN_BIT) ||
        !masked(t,RTW_REG_MCUFWDL,0,RTW_FW_DL_ENABLE) ||
        !masked(t,RTW_REG_MCUFWDL2,0x08U,0))
        return 0;
    /*
     * MCUFWDL bit 2 is write-one-to-clear. Reset the checksum report
     * before writing the image: otherwise a previous boot's checksum can
     * make a failed second download appear successful. The Linux rtl8xxxu
     * firmware path resets this status before sending page data.
     */
    if (!masked(t,RTW_REG_MCUFWDL,0,RTW_FW_CHECKSUM) ||
        !readreg(t,RTW_REG_MCUFWDL,1,&status))
        return 0;
    return (status & RTW_FW_CHECKSUM) == 0U;
}
static int disable_firmware_dl(const rtw_usb_rw_transport* t) {
    /* Preserve all non-download control bits; reserved+1 cleared per vendor. */
    const int cleared=masked(t,RTW_REG_MCUFWDL,RTW_FW_DL_ENABLE,0);
    const int reserved=writereg(t,RTW_REG_MCUFWDL1,1,0);
    return cleared && reserved;
}
static rtw_boot_error wait_bit(const rtw_usb_rw_transport* t,const rtw_waiter* wait,
                               const rtw_boot_options* opt,uint32_t bit,
                               int initialising,rtw_boot_report* report) {
    uint32_t i, status=0;
    for(i=0;i<opt->polls;++i) {
        if(!readreg(t,RTW_REG_MCUFWDL,4,&status)) return RTW_BOOT_USB_ERROR;
        report->last_mcu_fwdl=status;
        if(status&bit) return RTW_BOOT_OK;
        if(i+1<opt->polls && !wait->wait_us(wait->context,opt->poll_delay_us))
            return RTW_BOOT_WAIT_ERROR;
    }
    return initialising ? RTW_BOOT_INIT_TIMEOUT : RTW_BOOT_CHECKSUM_TIMEOUT;
}

typedef struct {
    const rtw_usb_rw_transport* bus;
    uint8_t active_page;
    rtw_boot_report* report;
} chunk_sink;
static int do_chunk(void* context, uint8_t page, uint16_t aperture,
                    const uint8_t* bytes, uint16_t count) {
    chunk_sink* c=(chunk_sink*)context;
    if (page!=c->active_page) {
        uint32_t old=0;
        if(!readreg(c->bus,RTW_REG_MCUFWDL2,1,&old) ||
           !writereg(c->bus,RTW_REG_MCUFWDL2,1,(old&0xF8U)|(page&7U))) return 0;
        c->active_page=page;
        c->report->pages_written=(uint8_t)(page+1U);
    }
    if(rtw_usb_write_block(c->bus,aperture,bytes,count)!=RTW_USB_OK) return 0;
    c->report->vendor_writes++;
    c->report->bytes_written+=count;
    return 1;
}
rtw_boot_error rtw_fw_boot_8812au(const rtw_usb_rw_transport* t,
                                   const rtw_waiter* waiter,
                                   const uint8_t* firmware, size_t length,
                                   const rtw_boot_options* options,
                                   rtw_boot_report* report) {
    rtw_fw_image image;
    rtw_boot_error result=RTW_BOOT_OK;
    chunk_sink sink;
    uint32_t status=0;
    int cleanup_ok;
    if(report) memset(report,0,sizeof(*report));
    if(!t || !t->control_in || !t->control_out || !waiter || !waiter->wait_us ||
       !options || !report || !firmware || !options->polls || options->polls>2000U ||
       !options->poll_delay_us || options->poll_delay_us>10000U)
        return RTW_BOOT_BAD_ARGUMENT;
    if (rtw_fw_inspect_8812au(firmware,length,&image)!=RTW_FW_OK)
        return RTW_BOOT_BAD_FIRMWARE;
    report->version=image.version;
    report->subversion=image.subversion;
    /* Before touching RAM, reset any existing resident 8051 firmware. */
    if(!readreg(t,RTW_REG_MCUFWDL,1,&status)) return RTW_BOOT_USB_ERROR;
    if(status&RTW_FW_RAM_SEL) {
        if(!writereg(t,RTW_REG_MCUFWDL,1,0) || !reset_8051(t))
            return RTW_BOOT_USB_ERROR;
    }
    if(!enable_firmware_dl(t)) {
        (void)disable_firmware_dl(t);
        return RTW_BOOT_USB_ERROR;
    }
    sink.bus=t; sink.active_page=0xFFU; sink.report=report;
    if(rtw_fw_plan_chunks(&image,&do_chunk,&sink,NULL)!=RTW_FW_OK)
        result=RTW_BOOT_USB_ERROR;
    /* Never proceed to boot while download-enabled, even on failure. */
    cleanup_ok=disable_firmware_dl(t);
    if(!cleanup_ok) result=RTW_BOOT_USB_ERROR;
    if(result!=RTW_BOOT_OK) return result;
    result=wait_bit(t,waiter,options,RTW_FW_CHECKSUM,0,report);
    if(result!=RTW_BOOT_OK) return result;
    report->checksum_seen=1;
    if(!readreg(t,RTW_REG_MCUFWDL,4,&status)) return RTW_BOOT_USB_ERROR;
    status=(status|RTW_FW_RDY)&~RTW_FW_WINTINI;
    if(!writereg(t,RTW_REG_MCUFWDL,4,status) || !reset_8051(t))
        return RTW_BOOT_USB_ERROR;
    result=wait_bit(t,waiter,options,RTW_FW_WINTINI,1,report);
    if(result!=RTW_BOOT_OK) return result;
    report->firmware_ready=1;
    return RTW_BOOT_OK;
}
const char* rtw_boot_error_string(rtw_boot_error e) {
    switch(e) {
        case RTW_BOOT_OK:return "ok";
        case RTW_BOOT_BAD_ARGUMENT:return "invalid firmware boot argument";
        case RTW_BOOT_BAD_FIRMWARE:return "invalid firmware image";
        case RTW_BOOT_USB_ERROR:return "USB register transfer failed";
        case RTW_BOOT_CHECKSUM_TIMEOUT:return "firmware checksum timeout";
        case RTW_BOOT_INIT_TIMEOUT:return "8051 did not signal ready";
        case RTW_BOOT_WAIT_ERROR:return "firmware poll delay failed";
        default:return "unknown";
    }
}
