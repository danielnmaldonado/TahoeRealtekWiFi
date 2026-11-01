#include "rtw/Efuse.h"
#include <string.h>

#define RTW_EFUSE_CTRL 0x0030U
#define RTW_EFUSE_ACCESS 0x00CFU

static uint16_t le16(const uint8_t* p) {
    return (uint16_t)((unsigned)p[0] | ((unsigned)p[1] << 8U));
}

static int valid_unicast_mac(const uint8_t mac[6]) {
    unsigned i;
    unsigned nonzero = 0U;
    unsigned not_ff = 0U;
    if (mac[0] & 1U) return 0;
    for (i=0; i<6; ++i) {
        nonzero |= mac[i];
        not_ff |= (unsigned)(mac[i] ^ 0xFFU);
    }
    return nonzero != 0 && not_ff != 0;
}

rtw_efuse_error rtw_efuse_decode(const uint8_t* phys, size_t len,
                                 uint8_t* logical, size_t logical_len,
                                 size_t* decoded_words) {
    size_t pos=0, words=0;
    if (decoded_words) *decoded_words=0;
    if (!phys || !logical || logical_len < RTW_EFUSE_BYTES ||
        len == 0 || len > RTW_EFUSE_BYTES)
        return RTW_EFUSE_BAD_ARGUMENT;
    memset(logical, 0xFF, logical_len);
    while (pos < len) {
        unsigned block, mask, word;
        const uint8_t header = phys[pos++];
        if (header == 0xFFU) {
            if (decoded_words) *decoded_words = words;
            return RTW_EFUSE_OK;
        }
        if ((header & 0x1FU) == 0x0FU) {
            uint8_t ext;
            if (pos >= len) goto truncated;
            ext = phys[pos++];
            block = (unsigned)((ext & 0xF0U) >> 1U) | ((unsigned)header >> 5U);
            mask = (unsigned)(ext & 0x0FU);
        } else {
            block = (unsigned)(header >> 4U);
            mask = (unsigned)(header & 0x0FU);
        }
        for (word=0; word<4U; ++word) {
            size_t off;
            if (mask & (1U << word)) continue;
            off = (size_t)block * 8U + (size_t)word * 2U;
            if (pos > len || len - pos < 2U) goto truncated;
            if (off > logical_len || logical_len - off < 2U) {
                memset(logical, 0xFF, logical_len);
                return RTW_EFUSE_OUT_OF_RANGE;
            }
            logical[off] = phys[pos++];
            logical[off+1U] = phys[pos++];
            ++words;
        }
    }
    if (decoded_words) *decoded_words = words;
    return RTW_EFUSE_OK;
truncated:
    memset(logical, 0xFF, logical_len);
    return RTW_EFUSE_TRUNCATED;
}

rtw_efuse_error rtw_efuse_extract_identity(
    const uint8_t* map, size_t length, rtw_efuse_identity* identity) {
    uint8_t rfe;
    if (identity) memset(identity, 0, sizeof(*identity));
    if (!identity || !map || length < RTW_EFUSE_BYTES)
        return RTW_EFUSE_BAD_ARGUMENT;
    if (!valid_unicast_mac(map + RTW_EFUSE_MAC_OFFSET))
        return RTW_EFUSE_INVALID_IDENTITY;
    memcpy(identity->mac, map + RTW_EFUSE_MAC_OFFSET, 6U);
    identity->eeprom_vendor = le16(map + 0xD0U);
    identity->eeprom_product = le16(map + 0xD2U);
    identity->rf_board_option = map[0xC1U];
    identity->channel_plan = map[0xC8U];
    identity->thermal_meter = map[0xBAU];
    identity->thermal_meter_valid = (uint8_t)(
        /* 0x3F saturates the six-bit thermal code; not a reliable baseline. */
        identity->thermal_meter > 0U && identity->thermal_meter < 63U);
    rfe = map[0xCAU];
    identity->rfe_type = (rfe == 0xFFU || (rfe & 0x80U)) ? 0U : (uint8_t)(rfe & 0x1FU);
    return RTW_EFUSE_OK;
}

static rtw_usb_error read8(const rtw_usb_rw_transport* t,
                            uint16_t address, uint32_t* v) {
    return rtw_usb_rw_read(t, address, 1, v);
}
static rtw_usb_error write8(const rtw_usb_rw_transport* t,
                             uint16_t address, uint8_t v) {
    return rtw_usb_write_register(t, address, 1, v);
}
static rtw_efuse_error physical_byte(const rtw_usb_rw_transport* t,
                                      unsigned address, uint32_t polls,
                                      uint8_t* out) {
    uint32_t value=0, i;
    if (write8(t, RTW_EFUSE_CTRL+1U, (uint8_t)address) != RTW_USB_OK ||
        read8(t, RTW_EFUSE_CTRL+2U, &value) != RTW_USB_OK ||
        write8(t, RTW_EFUSE_CTRL+2U,
            (uint8_t)((value & 0xFCU) | ((address >> 8U) & 3U))) != RTW_USB_OK ||
        read8(t, RTW_EFUSE_CTRL+3U, &value) != RTW_USB_OK ||
        write8(t, RTW_EFUSE_CTRL+3U, (uint8_t)(value & 0x7FU)) != RTW_USB_OK)
        return RTW_EFUSE_USB_ERROR;
    for (i=0; i<polls; ++i) {
        if (rtw_usb_rw_read(t, RTW_EFUSE_CTRL, 4, &value) != RTW_USB_OK)
            return RTW_EFUSE_USB_ERROR;
        if (value & 0x80000000UL) {
            *out = (uint8_t)value;
            return RTW_EFUSE_OK;
        }
    }
    return RTW_EFUSE_TIMEOUT;
}

rtw_efuse_error rtw_efuse_read_physical(
    const rtw_usb_rw_transport* t, uint8_t* physical, size_t cap,
    uint32_t poll_limit) {
    size_t i;
    rtw_efuse_error outcome=RTW_EFUSE_OK;
    if (!physical || cap < RTW_EFUSE_BYTES || !t ||
        !t->control_in || !t->control_out || !poll_limit || poll_limit > 128U)
        return RTW_EFUSE_BAD_ARGUMENT;
    memset(physical, 0, RTW_EFUSE_BYTES);
    if (write8(t, RTW_EFUSE_ACCESS, 0x69U) != RTW_USB_OK) {
        /* On ambiguous USB failure, attempt to close the access gate. */
        (void)write8(t, RTW_EFUSE_ACCESS, 0);
        return RTW_EFUSE_USB_ERROR;
    }
    for (i=0; i<RTW_EFUSE_BYTES; ++i) {
        outcome = physical_byte(t, (unsigned)i, poll_limit, physical+i);
        if (outcome != RTW_EFUSE_OK) break;
        /* Do not stop at the first 0xFF: 0xFF is legal payload data
           inside a packed efuse word. Decode headers only after reading. */
    }
    if (write8(t, RTW_EFUSE_ACCESS, 0) != RTW_USB_OK && outcome == RTW_EFUSE_OK)
        outcome = RTW_EFUSE_USB_ERROR;
    if (outcome != RTW_EFUSE_OK) memset(physical, 0, RTW_EFUSE_BYTES);
    return outcome;
}

const char* rtw_efuse_error_string(rtw_efuse_error e) {
    switch(e) {
        case RTW_EFUSE_OK: return "ok";
        case RTW_EFUSE_BAD_ARGUMENT: return "invalid argument";
        case RTW_EFUSE_TRUNCATED: return "truncated efuse record";
        case RTW_EFUSE_OUT_OF_RANGE: return "logical efuse offset out of range";
        case RTW_EFUSE_INVALID_IDENTITY: return "invalid station MAC in efuse";
        case RTW_EFUSE_USB_ERROR: return "USB I/O failure";
        case RTW_EFUSE_TIMEOUT: return "efuse read timeout";
        default: return "unknown";
    }
}
