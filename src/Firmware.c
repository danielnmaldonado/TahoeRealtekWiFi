#include "rtw/Firmware.h"
#include <string.h>

rtw_fw_error rtw_fw_inspect_8812au(
    const uint8_t* image, size_t length, rtw_fw_image* result) {
    size_t payload_len;
    uint16_t signature;
    if (result) memset(result,0,sizeof(*result));
    if (!result || !image) return RTW_FW_BAD_ARGUMENT;
    if (length < RTW_FW_HEADER_SIZE+1U) return RTW_FW_INVALID_IMAGE;
    signature = (uint16_t)((unsigned)image[0] | ((unsigned)image[1]<<8U));
    /* RTL8812AU image signature 0x95xx; do not accept 8821 or unknown. */
    if ((signature & 0xFFF0U) != 0x9500U) return RTW_FW_INVALID_IMAGE;
    payload_len = length - RTW_FW_HEADER_SIZE;
    if (payload_len > RTW_FW_PAGE_SIZE*RTW_FW_MAX_PAGES)
        return RTW_FW_TOO_LARGE;
    result->signature = signature;
    result->category = image[2];
    result->function = image[3];
    result->version = (uint16_t)((unsigned)image[4] | ((unsigned)image[5]<<8U));
    result->subversion = image[6];
    result->payload = image + RTW_FW_HEADER_SIZE;
    result->payload_length = payload_len;
    result->pages = (uint8_t)((payload_len + RTW_FW_PAGE_SIZE - 1U) / RTW_FW_PAGE_SIZE);
    return RTW_FW_OK;
}

rtw_fw_error rtw_fw_plan_chunks(
    const rtw_fw_image* image, rtw_fw_chunk_callback callback,
    void* context, size_t* chunk_count) {
    size_t done=0, chunks=0;
    if (chunk_count) *chunk_count=0;
    if (!image || !callback || !image->payload || !image->payload_length ||
        (image->signature & 0xFFF0U) != 0x9500U ||
        image->payload_length > RTW_FW_PAGE_SIZE*RTW_FW_MAX_PAGES ||
        image->pages != (uint8_t)((image->payload_length+RTW_FW_PAGE_SIZE-1U)/RTW_FW_PAGE_SIZE))
        return RTW_FW_BAD_ARGUMENT;
    while (done < image->payload_length) {
        size_t in_page = done % RTW_FW_PAGE_SIZE;
        size_t remain = image->payload_length - done;
        size_t page_left = RTW_FW_PAGE_SIZE - in_page;
        size_t count;
        uint8_t page = (uint8_t)(done / RTW_FW_PAGE_SIZE);
        /* Vendor USB writes use 254-byte blocks, then 8, then 1 for
           each page, rather than spilling a 254-byte block across pages. */
        if (remain > page_left) remain = page_left;
        if (remain >= 254U) count = 254U;
        else if (remain >= 8U) count = 8U;
        else count = 1U;
        if (!callback(context, page,
                      (uint16_t)(RTW_FW_PAGE_WINDOW + in_page),
                      image->payload + done, (uint16_t)count)) {
            if (chunk_count) *chunk_count=chunks;
            return RTW_FW_CALLBACK_FAILED;
        }
        done += count;
        ++chunks;
    }
    if (chunk_count) *chunk_count=chunks;
    return RTW_FW_OK;
}

const char* rtw_fw_error_string(rtw_fw_error error) {
    switch(error) {
        case RTW_FW_OK: return "ok";
        case RTW_FW_BAD_ARGUMENT: return "invalid argument";
        case RTW_FW_INVALID_IMAGE: return "invalid RTL8812AU firmware signature/header";
        case RTW_FW_TOO_LARGE: return "firmware exceeds page window";
        case RTW_FW_CALLBACK_FAILED: return "firmware chunk callback stopped";
        default: return "unknown";
    }
}
