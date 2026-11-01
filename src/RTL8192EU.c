/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/RTL8192EU.h"
#include "rtw/Efuse.h"
#include <string.h>

static uint16_t le16(const uint8_t* b) {
    return (uint16_t)((unsigned)b[0] | ((unsigned)b[1] << 8U));
}
static uint32_t le32(const uint8_t* b) {
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8U) |
           ((uint32_t)b[2] << 16U) | ((uint32_t)b[3] << 24U);
}
static int mac_unicast(const uint8_t* mac) {
    unsigned any=0U, any_non_ff=0U;
    size_t i;
    if (mac[0] & 1U) return 0;
    for (i=0; i<6U; ++i) {
        any |= mac[i];
        any_non_ff |= (unsigned)(mac[i] ^ 0xFFU);
    }
    return any != 0U && any_non_ff != 0U;
}

rtw_8192eu_result rtw_8192eu_decode_identity(uint16_t vid, uint16_t pid,
    uint32_t sys_cfg, uint32_t sys_cfg2, uint32_t hpon_fsm,
    uint32_t mcu_fw_dl, rtw_8192eu_identity* out) {
    if (!out) return RTW_8192EU_BAD_ARGUMENT;
    memset(out, 0, sizeof(*out));
    if (vid != RTW_8192EU_USB_VID || pid != RTW_8192EU_USB_PID)
        return RTW_8192EU_WRONG_USB_ID;
    if (sys_cfg == 0U || sys_cfg == UINT32_MAX ||
        hpon_fsm == UINT32_MAX || sys_cfg2 == UINT32_MAX ||
        mcu_fw_dl == UINT32_MAX)
        return RTW_8192EU_IMPLAUSIBLE_REGISTER;
    /* The Linux RTL8192EU driver explicitly refuses SYS_CFG bit 23. */
    if (sys_cfg & (1U << 23U)) return RTW_8192EU_TEST_SILICON;
    out->vid=vid;
    out->pid=pid;
    out->sys_cfg=sys_cfg;
    out->sys_cfg2=sys_cfg2;
    out->hpon_fsm=hpon_fsm;
    out->mcu_fw_dl=mcu_fw_dl;
    out->silicon_cut=(uint8_t)((sys_cfg >> 12U) & 15U);
    out->foundry_code=(uint8_t)((sys_cfg >> 18U) & 3U);
    out->is_8191eu_bonding=(uint8_t)((hpon_fsm & 0x00C00000U) == 0x00400000U);
    out->inferred_tx_paths=out->is_8191eu_bonding ? 1U : 2U;
    return RTW_8192EU_OK;
}

rtw_8192eu_result rtw_8192eu_probe_registers(
    const rtw_usb_transport* usb, uint16_t vid, uint16_t pid,
    rtw_8192eu_identity* out) {
    uint32_t cfg=0, cfg2=0, fsm=0, fw=0;
    if (out) memset(out, 0, sizeof(*out));
    if (!usb || !usb->control_in || !out) return RTW_8192EU_BAD_ARGUMENT;
    if (vid != RTW_8192EU_USB_VID || pid != RTW_8192EU_USB_PID)
        return RTW_8192EU_WRONG_USB_ID;
    if (rtw_usb_read_register(usb, RTW_8192EU_REG_SYS_CFG, 4U, &cfg) != RTW_USB_OK ||
        rtw_usb_read_register(usb, RTW_8192EU_REG_HPON_FSM, 4U, &fsm) != RTW_USB_OK ||
        rtw_usb_read_register(usb, RTW_8192EU_REG_SYS_CFG2, 4U, &cfg2) != RTW_USB_OK ||
        rtw_usb_read_register(usb, RTW_8192EU_REG_MCU_FW_DL, 4U, &fw) != RTW_USB_OK)
        return RTW_8192EU_USB_READ_FAILED;
    return rtw_8192eu_decode_identity(vid,pid,cfg,cfg2,fsm,fw,out);
}

rtw_8192eu_result rtw_8192eu_parse_logical_efuse(
    const uint8_t* logical, size_t len, rtw_8192eu_efuse_identity* out) {
    if (out) memset(out, 0, sizeof(*out));
    if (!out || !logical || len < RTW_8192EU_EFUSE_BYTES)
        return RTW_8192EU_BAD_ARGUMENT;
    if (le16(logical) != RTW_8192EU_EFUSE_RTL_ID ||
        !mac_unicast(logical + 0xD7U)) return RTW_8192EU_BAD_EFUSE;
    out->rtl_id=le16(logical);
    memcpy(out->mac,logical+0xD7U,6U);
    out->channel_plan=logical[0xB8U];
    out->crystal_cap=(uint8_t)(logical[0xB9U] & 0x3FU);
    out->thermal_meter=logical[0xBAU];
    out->rf_board_option=logical[0xC1U];
    return RTW_8192EU_OK;
}
rtw_8192eu_result rtw_8192eu_parse_physical_efuse(
    const uint8_t* physical, size_t len, rtw_8192eu_efuse_identity* out) {
    uint8_t logical[RTW_8192EU_EFUSE_BYTES];
    rtw_8192eu_result status;
    if (out) memset(out,0,sizeof(*out));
    if (!out || !physical || !len || len>RTW_8192EU_EFUSE_BYTES)
        return RTW_8192EU_BAD_ARGUMENT;
    if (rtw_efuse_decode(physical,len,logical,sizeof(logical),NULL)!=RTW_EFUSE_OK)
        return RTW_8192EU_BAD_EFUSE;
    status=rtw_8192eu_parse_logical_efuse(logical,sizeof(logical),out);
    memset(logical,0,sizeof(logical));
    return status;
}

rtw_8192eu_result rtw_8192eu_inspect_firmware(
    const uint8_t* image, size_t len, rtw_8192eu_fw_info* out) {
    size_t size;
    uint16_t ram_size;
    if (out) memset(out,0,sizeof(*out));
    if (!out || !image) return RTW_8192EU_BAD_ARGUMENT;
    if (len <= RTW_8192EU_FW_HEADER_BYTES)
        return RTW_8192EU_BAD_FIRMWARE;
    if ((le16(image) & 0xFFF0U) != 0x92E0U)
        return RTW_8192EU_BAD_FIRMWARE;
    size=len-RTW_8192EU_FW_HEADER_BYTES;
    if (size>RTW_8192EU_FW_MAX_PAYLOAD)
        return RTW_8192EU_OVERSIZED_FIRMWARE;
    ram_size=le16(image+12U);
    /* Ram-code size field is metadata in the vendor format, and is not
     * necessarily equal to file length. Reject impossible large declarations.
     * Zero is tolerated for firmware builds which don't populate the field. */
    if ((size_t)ram_size>RTW_8192EU_FW_MAX_PAYLOAD)
        return RTW_8192EU_BAD_FIRMWARE;
    out->signature=le16(image);
    out->category=image[2];
    out->function=image[3];
    out->major_version=le16(image+4U);
    out->minor_version=image[6];
    out->reported_ramcode_size=ram_size;
    out->svn_index=le32(image+16U);
    out->payload=image+RTW_8192EU_FW_HEADER_BYTES;
    out->payload_length=size;
    return RTW_8192EU_OK;
}

rtw_packet_error rtw_8192eu_rx_next(
    rtw_rx_iterator* iterator, rtw_8192eu_rx_frame* out) {
    const uint8_t* base;
    uint32_t w0, w1, w2;
    rtw_packet_error status;
    if (out) memset(out,0,sizeof(*out));
    if (!iterator || !out || (!iterator->buffer && iterator->length) ||
        iterator->cursor>iterator->length) return RTW_PACKETS_BAD_ARGUMENT;
    if (iterator->cursor==iterator->length) return RTW_PACKETS_DONE;
    if (iterator->length-iterator->cursor<RTW_RX_DESC_BYTES)
        return rtw_rx_next(iterator,&out->payload);
    base=iterator->buffer+iterator->cursor;
    w0=le32(base);
    w1=le32(base+4U);
    w2=le32(base+8U);
    status=rtw_rx_next(iterator,&out->payload);
    if (status!=RTW_PACKETS_OK) return status;
    out->report_is_c2h=(uint8_t)((w2>>28U)&1U);
    out->security_code=(uint8_t)((w0>>20U)&7U);
    out->sw_decrypt_required=(uint8_t)((w0>>27U)&1U);
    out->qos=(uint8_t)((w0>>23U)&1U);
    out->sequence=(uint16_t)(w2&0x0FFFU);
    out->fragment_number=(uint8_t)((w2>>12U)&15U);
    out->fragmented=(uint8_t)(((w1>>27U)&1U) || out->fragment_number!=0U);
    return RTW_PACKETS_OK;
}

const char* rtw_8192eu_result_string(rtw_8192eu_result result) {
    switch(result) {
    case RTW_8192EU_OK:return "ok";
    case RTW_8192EU_BAD_ARGUMENT:return "invalid input";
    case RTW_8192EU_WRONG_USB_ID:return "not 2357:0108";
    case RTW_8192EU_USB_READ_FAILED:return "USB read failed";
    case RTW_8192EU_IMPLAUSIBLE_REGISTER:return "implausible chip register values";
    case RTW_8192EU_TEST_SILICON:return "unsupported RTL8192E test silicon";
    case RTW_8192EU_BAD_EFUSE:return "invalid or truncated RTL8192EU efuse identity";
    case RTW_8192EU_BAD_FIRMWARE:return "wrong/invalid RTL8192EU firmware header";
    case RTW_8192EU_OVERSIZED_FIRMWARE:return "RTL8192EU firmware exceeds 32 KiB limit";
    default:return "unknown RTL8192EU error";
    }
}
