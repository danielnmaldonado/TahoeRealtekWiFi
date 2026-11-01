/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/USBProbe.h"
#include <string.h>

rtw_usb_probe_result rtw_usb_probe_read_only(const rtw_usb_transport* transport,
    uint16_t vid,uint16_t pid,rtw_usb_probe_report* report) {
    const int is_8812au=vid==0x0bdaU && pid==0x8812U;
    const int is_8192eu=vid==RTW_8192EU_USB_VID && pid==RTW_8192EU_USB_PID;
    if (report) memset(report,0,sizeof(*report));
    if (!report) return RTW_USB_PROBE_BAD_ARGUMENT;
    if (!is_8812au && !is_8192eu) return RTW_USB_PROBE_UNSUPPORTED_ID;
    if (!transport || !transport->control_in) return RTW_USB_PROBE_BAD_ARGUMENT;

    if (is_8192eu) {
        rtw_8192eu_identity identity;
        const rtw_8192eu_result status=rtw_8192eu_probe_registers(
            transport,vid,pid,&identity);
        if (status==RTW_8192EU_USB_READ_FAILED) return RTW_USB_PROBE_READ_FAILED;
        if (status!=RTW_8192EU_OK) return RTW_USB_PROBE_UNSUPPORTED_PROFILE;
        report->rtl8192eu=identity;
        report->family=RTW_USB_PROBE_FAMILY_RTL8192EU;
    } else {
        uint32_t cfg=0;
        rtw_chip_info identity;
        const rtw_usb_error status=rtw_usb_probe_syscfg(transport,&cfg);
        if (status==RTW_USB_IMPLAUSIBLE_CHIP) return RTW_USB_PROBE_UNSUPPORTED_PROFILE;
        if (status!=RTW_USB_OK) return RTW_USB_PROBE_READ_FAILED;
        if (!rtw_chip_decode_syscfg(cfg,&identity) || !identity.is_usb ||
            identity.is_test_chip || identity.cut!=1 || identity.is_umc)
            return RTW_USB_PROBE_UNSUPPORTED_PROFILE;
        report->rtl8812au=identity;
        report->family=RTW_USB_PROBE_FAMILY_RTL8812AU;
    }
    return RTW_USB_PROBE_OK;
}
