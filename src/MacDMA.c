/* SPDX-License-Identifier: GPL-2.0-only
   RTL8812AU interoperability; implementation based on vendor sequences and
   GPL-2.0 reference xen-proc/rtl8812au-macos and aircrack-ng/rtl8812au.
   This does NOT attest to validation on the user's physical hardware. */
#include "rtw/MacDMA.h"
#define RTW_RX_FIFO_BOUNDARY 0x3E7FU
#define RTW_TX_PAGE_BOUNDARY 0xF7U
static int write_reg(const rtw_usb_rw_transport* t,uint16_t reg,uint8_t bytes,uint32_t value,size_t* count) {
    if(rtw_usb_write_register(t,reg,bytes,value)!=RTW_USB_OK) return 0;
    ++*count;return 1;
}
rtw_mac_error rtw_mac_dma_prepare_8812au(const rtw_usb_rw_transport* t,
                                           const rtw_mac_options* options,
                                           size_t* operations) {
    size_t n=0;
    uint32_t ctrl=0;
    if(operations) *operations=0;
    if(!t || !t->control_in || !t->control_out || !options ||
       options->out_endpoints != 3 ||
       options->drvinfo_words != 4 || options->enable_receive != 0)
        return RTW_MAC_BAD_ARGUMENT;
    /* Hardware page boundaries from captured Linux/vendor RTL8812AU setup. */
    if(!write_reg(t,0x0424,1,RTW_TX_PAGE_BOUNDARY,&n) ||
       !write_reg(t,0x0425,1,RTW_TX_PAGE_BOUNDARY,&n) ||
       !write_reg(t,0x045D,1,RTW_TX_PAGE_BOUNDARY,&n) ||
       !write_reg(t,0x0114,1,RTW_TX_PAGE_BOUNDARY,&n) ||
       !write_reg(t,0x0209,1,RTW_TX_PAGE_BOUNDARY,&n) ||
       !write_reg(t,0x0116,2,RTW_RX_FIFO_BOUNDARY,&n))
       goto usb_failed;
    if(rtw_usb_rw_read(t,0x010C,2,&ctrl)!=RTW_USB_OK) goto usb_failed;
    /* Queue map verified for 3 OUT endpoints only -> 0xF5B0, plus
       preserved low RX bits. Other endpoint profiles are rejected. */
    ctrl=(ctrl & 7U) | 0xF5B0U;
    if(!write_reg(t,0x010C,2,ctrl,&n) ||
       !write_reg(t,0x010C,1,(ctrl & 0xF8U)|0x04U,&n) ||
       !write_reg(t,0x0280,2,0x1A07,&n) ||
       !write_reg(t,0x0288,2,0x7400,&n) ||
       !write_reg(t,0x0289,1,0xF5,&n) ||
       !write_reg(t,0x0290,1,0x0E,&n) ||
       !write_reg(t,0x060F,1,options->drvinfo_words,&n))
       goto usb_failed;
    /* RQPN/TX allocations and RX enable must be run AFTER PHY/RF init;
       deliberately not written here to avoid invalid TX/monitor behaviour. */
    if(operations) *operations=n;
    return RTW_MAC_OK;
usb_failed:
    if(operations) *operations=n;
    return RTW_MAC_USB_ERROR;
}
const char* rtw_mac_error_string(rtw_mac_error e) {
    switch(e) {
        case RTW_MAC_OK:return "ok";
        case RTW_MAC_BAD_ARGUMENT:return "bad MAC DMA settings";
        case RTW_MAC_USB_ERROR:return "USB MAC register error";
        default:return "unknown";
    }
}
