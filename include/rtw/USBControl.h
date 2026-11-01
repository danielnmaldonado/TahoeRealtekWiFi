#ifndef RTW_USB_CONTROL_H
#define RTW_USB_CONTROL_H

// RTL8812AU USB vendor-control READ protocol.
// C / allocation-free so it can be used from a kernel HAL and host tests.
// No reset, register write, firmware upload, or interface takeover occurs here.
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RTW_USB_OK = 0,
    RTW_USB_INVALID_ARGUMENT,
    RTW_USB_INVALID_REGISTER,
    RTW_USB_IO_ERROR,
    RTW_USB_SHORT_TRANSFER,
    RTW_USB_IMPLAUSIBLE_CHIP
} rtw_usb_error;

typedef struct {
    uint8_t bm_request_type;
    uint8_t b_request;
    uint16_t w_value;
    uint16_t w_index;
    uint16_t w_length;
    uint32_t timeout_ms;
} rtw_usb_setup;

// On success, actual MUST be initialized by the adapter to bytes delivered.
// The transfer adapter must write no more than setup->w_length bytes.
// The callback is responsible for enforcing this bound on the actual bus API.
typedef rtw_usb_error (*rtw_usb_control_in)(
    void* context, const rtw_usb_setup* setup,
    uint8_t* data, uint16_t* actual);

typedef struct {
    void* context;
    rtw_usb_control_in control_in;
} rtw_usb_transport;

// Read one little-endian RTL8812AU register, width 1, 2 or 4 bytes.
// Only control endpoint IN, vendor bRequest 0x05, wIndex 0.
rtw_usb_error rtw_usb_read_register(const rtw_usb_transport* transport,
                                   uint16_t address, uint8_t width,
                                   uint32_t* value);

// Non-destructive chip-responsive check, reads only REG_SYS_CFG at 0x00F0.
rtw_usb_error rtw_usb_probe_syscfg(const rtw_usb_transport* transport,
                                 uint32_t* syscfg);

const char* rtw_usb_error_string(rtw_usb_error error);

#ifdef __cplusplus
}
#endif
#endif
