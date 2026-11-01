/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/USBWiFiFirmware.h"
#include <string.h>

/* The legacy 32-byte 8051 header, not the 64-byte rtw88 firmware ABI.
 * Sources: Linux rtl8xxxu/rtl8xxxu.h, rtl8xxxu/core.c; rtlwifi/rtl8192d/fw_common.h.
 * This inspect-only function does not authenticate firmware or load it. */
static uint16_t le16(const uint8_t *b) {
    return (uint16_t)((unsigned)b[0] | ((unsigned)b[1] << 8U));
}

int rtw_usb_wifi_inspect_firmware(rtw_wifi_family family, const uint8_t *b,
                                  size_t n, rtw_8192eu_fw_info *out) {
    uint16_t signature;
    uint16_t required = 0;
    size_t maximum = 32768U;
    int exact_signature = 0;

    if (out) memset(out, 0, sizeof(*out));
    if (!out || !b || n <= 32U) return 0;
    switch (family) {
    case RTW_WIFI_RTL8192CU: required = 0x88c0U; break;
    case RTW_WIFI_RTL8188EU: required = 0x88e0U; break;
    case RTW_WIFI_RTL8192EU: required = 0x92e0U; break;
    case RTW_WIFI_RTL8192DU:
        required = 0x92d0U; exact_signature = 1; break;
    case RTW_WIFI_RTL8192FU:
        required = 0x92f0U; maximum = 65536U; break;
    case RTW_WIFI_RTL8723BU:
        required = 0x5300U; maximum = 65536U; break;
    case RTW_WIFI_RTL8710BU:
        required = 0x10b0U; maximum = 65536U; break;
    default: return 0;
    }
    if (n - 32U > maximum) return 0;
    signature = le16(b);
    if (exact_signature) {
        if (signature < required || signature > 0x92d3U) return 0;
    } else if ((signature & 0xfff0U) != required) {
        return 0;
    }
    /* Only the two source-pinned 8710BU vendor firmware identities exist here.
     * A generic family mask cannot authorize an unknown vendor image. */
    if(family==RTW_WIFI_RTL8710BU && signature!=0x10b1U && signature!=0x10b2U)
        return 0;
    /* 0 means unspecified in some legacy images. A non-zero RAM code size
     * must fit in the supplied payload and the family memory bound. */
    if (le16(b + 12U) > maximum ||
        (le16(b + 12U) && (size_t)le16(b + 12U) > n - 32U))
        return 0;

    out->signature = signature;
    out->category = b[2];
    out->function = b[3];
    out->major_version = le16(b + 4U);
    out->minor_version = b[6];
    out->reported_ramcode_size = le16(b + 12U);
    out->svn_index = (uint32_t)b[16] | ((uint32_t)b[17] << 8U) |
        ((uint32_t)b[18] << 16U) | ((uint32_t)b[19] << 24U);
    out->payload = b + 32U;
    out->payload_length = n - 32U;
    return 1;
}
