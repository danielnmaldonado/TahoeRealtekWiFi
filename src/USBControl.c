#include "rtw/USBControl.h"

rtw_usb_error rtw_usb_read_register(const rtw_usb_transport* transport,
                                   uint16_t address, uint8_t width,
                                   uint32_t* value) {
    rtw_usb_setup setup;
    rtw_usb_error result;
    uint8_t bytes[4] = {0, 0, 0, 0};
    uint16_t received = 0;
    uint32_t decoded = 0;
    unsigned i;
    if (value) *value = 0;
    if (!transport || !transport->control_in || !value)
        return RTW_USB_INVALID_ARGUMENT;
    if (width != 1U && width != 2U && width != 4U)
        return RTW_USB_INVALID_ARGUMENT;
    if ((uint32_t)address + width > 0x10000U)
        return RTW_USB_INVALID_REGISTER;

    setup.bm_request_type = 0xC0U; // device -> host, vendor, recipient device
    setup.b_request = 0x05U;
    setup.w_value = address;
    setup.w_index = 0;
    setup.w_length = width;
    setup.timeout_ms = 1000;
    result = transport->control_in(transport->context, &setup, bytes, &received);
    if (result != RTW_USB_OK) return result;
    if (received != width) return RTW_USB_SHORT_TRANSFER;
    for (i = 0; i < width; ++i)
        decoded |= (uint32_t)bytes[i] << (8U*i);
    *value = decoded;
    return RTW_USB_OK;
}

rtw_usb_error rtw_usb_probe_syscfg(const rtw_usb_transport* transport,
                                 uint32_t* syscfg) {
    rtw_usb_error result = rtw_usb_read_register(transport, 0x00F0U, 4, syscfg);
    if (result != RTW_USB_OK) return result;
    if (*syscfg == 0 || *syscfg == UINT32_MAX) {
        *syscfg = 0;
        return RTW_USB_IMPLAUSIBLE_CHIP;
    }
    return RTW_USB_OK;
}

const char* rtw_usb_error_string(rtw_usb_error error) {
    switch (error) {
        case RTW_USB_OK: return "ok";
        case RTW_USB_INVALID_ARGUMENT: return "invalid argument";
        case RTW_USB_INVALID_REGISTER: return "register address overflow";
        case RTW_USB_IO_ERROR: return "USB transfer failed";
        case RTW_USB_SHORT_TRANSFER: return "USB transfer truncated";
        case RTW_USB_IMPLAUSIBLE_CHIP: return "invalid chip register value";
        default: return "unknown";
    }
}
