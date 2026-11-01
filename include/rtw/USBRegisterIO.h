#ifndef RTW_USB_REGISTER_IO_H
#define RTW_USB_REGISTER_IO_H
#include "rtw/USBControl.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* USB control OUT to Realtek's vendor request 0x05.
   Cannot program irreversible efuse OTP using these helpers: caller decides
   which register addresses can be written. USB writes are NOT invoked by
   the research kext's current read-only probe. */
#define RTW_USB_MAX_VENDOR_PAYLOAD 254U

typedef rtw_usb_error (*rtw_usb_control_out)(
    void* context, const rtw_usb_setup* setup,
    const uint8_t* data, uint16_t* actual);

typedef struct {
    void* context;
    rtw_usb_control_in control_in;
    rtw_usb_control_out control_out;
} rtw_usb_rw_transport;

rtw_usb_error rtw_usb_rw_read(const rtw_usb_rw_transport* t,
                              uint16_t address, uint8_t width,
                              uint32_t* value);
rtw_usb_error rtw_usb_write_block(const rtw_usb_rw_transport* t,
                                  uint16_t address, const uint8_t* data,
                                  uint16_t count);
rtw_usb_error rtw_usb_write_register(const rtw_usb_rw_transport* t,
                                     uint16_t address, uint8_t width,
                                     uint32_t value);
#ifdef __cplusplus
}
#endif
#endif
