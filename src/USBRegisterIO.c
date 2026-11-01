#include "rtw/USBRegisterIO.h"

rtw_usb_error rtw_usb_rw_read(const rtw_usb_rw_transport* t,
                              uint16_t address, uint8_t width,
                              uint32_t* value) {
    rtw_usb_transport read_only;
    if (!t || !t->control_in) {
        if (value) *value = 0;
        return RTW_USB_INVALID_ARGUMENT;
    }
    read_only.context = t->context;
    read_only.control_in = t->control_in;
    return rtw_usb_read_register(&read_only, address, width, value);
}

rtw_usb_error rtw_usb_write_block(const rtw_usb_rw_transport* t,
                                  uint16_t address, const uint8_t* data,
                                  uint16_t count) {
    rtw_usb_setup setup;
    uint16_t actual = 0;
    rtw_usb_error err;
    if (!t || !t->control_out || !data || !count ||
        count > RTW_USB_MAX_VENDOR_PAYLOAD)
        return RTW_USB_INVALID_ARGUMENT;
    if ((uint32_t)address + count > 0x10000UL)
        return RTW_USB_INVALID_REGISTER;
    setup.bm_request_type = 0x40U;
    setup.b_request = 0x05U;
    setup.w_value = address;
    setup.w_index = 0;
    setup.w_length = count;
    setup.timeout_ms = 1000;
    err = t->control_out(t->context, &setup, data, &actual);
    if (err != RTW_USB_OK) return err;
    return actual == count ? RTW_USB_OK : RTW_USB_SHORT_TRANSFER;
}

rtw_usb_error rtw_usb_write_register(const rtw_usb_rw_transport* t,
                                     uint16_t address, uint8_t width,
                                     uint32_t value) {
    uint8_t bytes[4];
    unsigned i;
    if (width != 1U && width != 2U && width != 4U)
        return RTW_USB_INVALID_ARGUMENT;
    for (i = 0; i < width; ++i) bytes[i] = (uint8_t)(value >> (i * 8U));
    return rtw_usb_write_block(t, address, bytes, width);
}
