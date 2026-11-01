/* SPDX-License-Identifier: GPL-2.0-only
   RTL8812AU interoperability; implementation based on vendor sequences and
   GPL-2.0 reference xen-proc/rtl8812au-macos and aircrack-ng/rtl8812au.
   This does NOT attest to validation on the user's physical hardware. */
#include "rtw/LLT.h"

#define RTW_LLT_REG 0x01E0U
#define RTW_LLT_BUSY_WRITE 0x40000000UL

rtw_llt_error rtw_llt_initialize_8812au(const rtw_usb_rw_transport* t,
                                          const rtw_waiter* waiter,
                                          uint32_t polls_per_entry,
                                          size_t* entries_done) {
    uint32_t i;
    if (entries_done) *entries_done=0;
    if (!t || !t->control_in || !t->control_out ||
        !waiter || !waiter->wait_us || !polls_per_entry || polls_per_entry>1000U)
        return RTW_LLT_BAD_ARGUMENT;
    for(i=0;i<RTW_LLT_ENTRIES;++i) {
        uint32_t linked;
        uint32_t j, status=0;
        if (i < RTW_LLT_BOUNDARY_8812AU-1U) linked=i+1U;
        else if (i==RTW_LLT_BOUNDARY_8812AU-1U) linked=0xFFU;
        else if (i==RTW_LLT_ENTRIES-1U) linked=RTW_LLT_BOUNDARY_8812AU;
        else linked=i+1U;
        if (rtw_usb_write_register(t,RTW_LLT_REG,4,
                  RTW_LLT_BUSY_WRITE | (i<<8U) | linked)!=RTW_USB_OK)
            return RTW_LLT_USB_ERROR;
        for(j=0;j<polls_per_entry;++j) {
            if (rtw_usb_rw_read(t,RTW_LLT_REG,4,&status)!=RTW_USB_OK)
                return RTW_LLT_USB_ERROR;
            if (!(status & 0xC0000000UL)) break;
            if (j+1<polls_per_entry && !waiter->wait_us(waiter->context,1000U))
                return RTW_LLT_DELAY_FAILED;
        }
        if (j==polls_per_entry) return RTW_LLT_TIMEOUT;
        if (entries_done) *entries_done=i+1U;
    }
    return RTW_LLT_OK;
}
const char* rtw_llt_error_string(rtw_llt_error e) {
    switch(e) {
        case RTW_LLT_OK:return "ok";
        case RTW_LLT_BAD_ARGUMENT:return "invalid argument";
        case RTW_LLT_USB_ERROR:return "USB register I/O failed";
        case RTW_LLT_TIMEOUT:return "LLT operation timed out";
        case RTW_LLT_DELAY_FAILED:return "delay failed";
        default:return "unknown";
    }
}
