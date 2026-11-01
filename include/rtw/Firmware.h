#ifndef RTW_FIRMWARE_H
#define RTW_FIRMWARE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* RTL8812AU 8051 firmware: 32-byte header, max 8 x 4KiB paged aperture.
   This code validates images and plans USB transfers; it DOES NOT perform
   power-on, firmware reset, checksum handshake or radio initialization. */
#define RTW_FW_HEADER_SIZE 32U
#define RTW_FW_PAGE_SIZE 4096U
#define RTW_FW_MAX_PAGES 8U
#define RTW_FW_PAGE_WINDOW 0x1000U

typedef enum {
    RTW_FW_OK=0,
    RTW_FW_BAD_ARGUMENT,
    RTW_FW_INVALID_IMAGE,
    RTW_FW_TOO_LARGE,
    RTW_FW_CALLBACK_FAILED
} rtw_fw_error;

typedef struct {
    uint16_t signature;
    uint8_t category;
    uint8_t function;
    uint16_t version;
    uint8_t subversion;
    const uint8_t* payload;
    size_t payload_length;
    uint8_t pages;
} rtw_fw_image;

/* Rejects bare/unknown images instead of writing firmware of another chip. */
rtw_fw_error rtw_fw_inspect_8812au(
    const uint8_t* image, size_t length, rtw_fw_image* result);

/* Each callback receives the 4KiB page index, control-register aperture
   address, data pointer and length <=254. Page selection register writes are
   intentionally NOT performed. Return false to abort immediately. */
typedef int (*rtw_fw_chunk_callback)(
    void* context, uint8_t page, uint16_t aperture_address,
    const uint8_t* bytes, uint16_t count);
rtw_fw_error rtw_fw_plan_chunks(
    const rtw_fw_image* image, rtw_fw_chunk_callback callback,
    void* context, size_t* chunk_count);
const char* rtw_fw_error_string(rtw_fw_error error);
#ifdef __cplusplus
}
#endif
#endif
