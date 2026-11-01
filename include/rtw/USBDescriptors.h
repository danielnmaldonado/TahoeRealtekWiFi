#ifndef RTW_USB_DESCRIPTORS_H
#define RTW_USB_DESCRIPTORS_H

// Safe USB configuration-descriptor walking, no dynamic allocations.
// Does not access hardware. The caller owns input storage and all outputs.
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RTW_MAX_BULK_PIPES 8

typedef struct {
    uint8_t interface_number;
    uint8_t alternate_setting;
    uint8_t bulk_in[RTW_MAX_BULK_PIPES];
    uint8_t bulk_out[RTW_MAX_BULK_PIPES];
    uint8_t num_bulk_in;
    uint8_t num_bulk_out;
    uint8_t num_endpoints;
} rtw_usb_endpoints;

typedef enum {
    RTW_DESC_OK = 0,
    RTW_DESC_INVALID_ARGUMENT,
    RTW_DESC_MALFORMED,
    RTW_DESC_INTERFACE_NOT_FOUND,
    RTW_DESC_DUPLICATE_ENDPOINT,
    RTW_DESC_NO_BULK_PAIR,
    RTW_DESC_TOO_MANY_PIPES
} rtw_usb_desc_error;

rtw_usb_desc_error rtw_find_usb_bulk_endpoints(
    const uint8_t* configuration, size_t available_length,
    uint8_t interface_number, uint8_t alternate_setting,
    rtw_usb_endpoints* result);

const char* rtw_usb_desc_error_string(rtw_usb_desc_error error);

#ifdef __cplusplus
}
#endif
#endif
