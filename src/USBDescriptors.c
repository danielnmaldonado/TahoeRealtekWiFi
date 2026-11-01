#include "rtw/USBDescriptors.h"
#include <string.h>

rtw_usb_desc_error rtw_find_usb_bulk_endpoints(
    const uint8_t* config, size_t available,
    uint8_t interface_number, uint8_t alternate_setting,
    rtw_usb_endpoints* result) {
    size_t pos, end;
    int active = 0, seen_interface = 0;
    uint8_t expected = 0, encountered = 0;
    uint8_t seen[32] = {0};

    if (result) memset(result, 0, sizeof(*result));
    if (!config || !result) return RTW_DESC_INVALID_ARGUMENT;
    if (available < 9 || config[0] < 9 || config[1] != 2)
        return RTW_DESC_MALFORMED;
    end = (size_t)(config[2] | ((unsigned)config[3] << 8));
    if (end > available || end < config[0]) return RTW_DESC_MALFORMED;
    result->interface_number = interface_number;
    result->alternate_setting = alternate_setting;

    pos = config[0];
    while (pos < end) {
        uint8_t size, type;
        if (end-pos < 2) return RTW_DESC_MALFORMED;
        size = config[pos];
        type = config[pos+1];
        if (size < 2 || (size_t)size > end-pos)
            return RTW_DESC_MALFORMED;

        if (type == 4) { // interface descriptor
            if (size < 9) return RTW_DESC_MALFORMED;
            if (active && encountered != expected) return RTW_DESC_MALFORMED;
            active = (config[pos+2] == interface_number &&
                      config[pos+3] == alternate_setting);
            if (active) {
                if (seen_interface) return RTW_DESC_MALFORMED;
                seen_interface = 1;
                expected = config[pos+4];
                encountered = 0;
            }
        } else if (type == 5 && active) { // endpoint descriptor
            uint8_t addr, index;
            if (size < 7) return RTW_DESC_MALFORMED;
            if (encountered == UINT8_MAX) return RTW_DESC_MALFORMED;
            ++encountered;
            addr = config[pos+2];
            // Endpoint 0 is exclusively for device control.
            if ((addr & 0x0fU) == 0 || (addr & 0x70U) != 0)
                return RTW_DESC_MALFORMED;
            index = (uint8_t)((addr & 0x0fU) | ((addr & 0x80U) ? 16U : 0U));
            if (seen[index]) return RTW_DESC_DUPLICATE_ENDPOINT;
            seen[index] = 1;
            if ((config[pos+3] & 3U) == 2U) {
                if (addr & 0x80U) {
                    if (result->num_bulk_in >= RTW_MAX_BULK_PIPES)
                        return RTW_DESC_TOO_MANY_PIPES;
                    result->bulk_in[result->num_bulk_in++] = addr;
                } else {
                    if (result->num_bulk_out >= RTW_MAX_BULK_PIPES)
                        return RTW_DESC_TOO_MANY_PIPES;
                    result->bulk_out[result->num_bulk_out++] = addr;
                }
            }
        }
        pos += size;
    }
    if (!seen_interface) return RTW_DESC_INTERFACE_NOT_FOUND;
    if (encountered != expected) return RTW_DESC_MALFORMED;
    result->num_endpoints = encountered;
    if (!result->num_bulk_in || !result->num_bulk_out)
        return RTW_DESC_NO_BULK_PAIR;
    return RTW_DESC_OK;
}

const char* rtw_usb_desc_error_string(rtw_usb_desc_error error) {
    switch (error) {
        case RTW_DESC_OK: return "ok";
        case RTW_DESC_INVALID_ARGUMENT: return "invalid argument";
        case RTW_DESC_MALFORMED: return "malformed descriptor";
        case RTW_DESC_INTERFACE_NOT_FOUND: return "interface not found";
        case RTW_DESC_DUPLICATE_ENDPOINT: return "duplicate endpoint address";
        case RTW_DESC_NO_BULK_PAIR: return "no bulk RX/TX endpoint pair";
        case RTW_DESC_TOO_MANY_PIPES: return "too many USB bulk pipes";
        default: return "unknown";
    }
}
