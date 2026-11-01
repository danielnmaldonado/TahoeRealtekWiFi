#ifndef RTW_EFUSE_H
#define RTW_EFUSE_H
#include "rtw/USBRegisterIO.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* RTL8812AU: physical OTP bytes and the decoded logical EEPROM map. */
#define RTW_EFUSE_BYTES 512U
#define RTW_EFUSE_MAC_OFFSET 0xD7U

typedef enum {
    RTW_EFUSE_OK = 0,
    RTW_EFUSE_BAD_ARGUMENT,
    RTW_EFUSE_TRUNCATED,
    RTW_EFUSE_OUT_OF_RANGE,
    RTW_EFUSE_INVALID_IDENTITY,
    RTW_EFUSE_USB_ERROR,
    RTW_EFUSE_TIMEOUT
} rtw_efuse_error;

typedef struct {
    uint8_t mac[6];
    uint16_t eeprom_vendor;
    uint16_t eeprom_product;
    uint8_t rfe_type;
    uint8_t channel_plan;
    uint8_t rf_board_option;
    /* RTL8812A eFuse 0xBA: reference thermal-meter code, not Celsius. */
    uint8_t thermal_meter;
    uint8_t thermal_meter_valid;
} rtw_efuse_identity;

/* Decodes packed physical records; zero-length, partial records and out-of-bounds
   logical offsets return an error, not a silently accepted partial identity.
   Unused logical bytes are 0xFF. */
rtw_efuse_error rtw_efuse_decode(const uint8_t* physical, size_t physical_len,
                                 uint8_t* logical, size_t logical_len,
                                 size_t* decoded_words);
rtw_efuse_error rtw_efuse_extract_identity(
    const uint8_t* logical, size_t logical_len, rtw_efuse_identity* identity);

/* Explicit hardware access: EFUSE_ACCESS=0x69, register-triggered read and
   bounded polling. Writes ONLY to efuse access/control (not OTP programming).
   This API is NOT called by the kext's read-only probe. On any error, the
   access gate is closed and the partially read output is zeroed. */
rtw_efuse_error rtw_efuse_read_physical(
    const rtw_usb_rw_transport* transport,
    uint8_t* physical, size_t physical_capacity,
    uint32_t poll_limit);
const char* rtw_efuse_error_string(rtw_efuse_error error);
#ifdef __cplusplus
}
#endif
#endif
