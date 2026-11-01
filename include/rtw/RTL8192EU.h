/* SPDX-License-Identifier: GPL-2.0-only
 * RTL8192EU host-side protocol / identity helpers. Kernel-safe C11 subset.
 * Hardware register/efuse layouts cross-checked against Linux rtl8xxxu_8192e.c
 * and Realtek's rtl8192eu driver. Nothing here enables power, RF or a network.
 */
#ifndef RTW_RTL8192EU_H
#define RTW_RTL8192EU_H
#include "rtw/USBControl.h"
#include "rtw/USBPackets.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define RTW_8192EU_USB_VID 0x2357U
#define RTW_8192EU_USB_PID 0x0108U
#define RTW_8192EU_REG_SYS_CFG 0x00F0U
#define RTW_8192EU_REG_SYS_CFG2 0x00FCU
#define RTW_8192EU_REG_HPON_FSM 0x00ECU
#define RTW_8192EU_REG_MCU_FW_DL 0x0080U
#define RTW_8192EU_FW_HEADER_BYTES 32U
#define RTW_8192EU_FW_MAX_PAYLOAD 32768U
#define RTW_8192EU_EFUSE_BYTES 512U
#define RTW_8192EU_EFUSE_RTL_ID 0x8129U

typedef enum {
    RTW_8192EU_OK=0,
    RTW_8192EU_BAD_ARGUMENT,
    RTW_8192EU_WRONG_USB_ID,
    RTW_8192EU_USB_READ_FAILED,
    RTW_8192EU_IMPLAUSIBLE_REGISTER,
    RTW_8192EU_TEST_SILICON,
    RTW_8192EU_BAD_EFUSE,
    RTW_8192EU_BAD_FIRMWARE,
    RTW_8192EU_OVERSIZED_FIRMWARE
} rtw_8192eu_result;

typedef struct {
    uint16_t vid, pid;
    uint32_t sys_cfg;
    uint32_t sys_cfg2;
    uint32_t hpon_fsm;
    uint32_t mcu_fw_dl;
    uint8_t silicon_cut;     /* SYS_CFG bits 12..15. Not a conclusive chip ID. */
    uint8_t foundry_code;    /* SYS_CFG bits 18..19 (vendor extension). */
    uint8_t is_8191eu_bonding; /* HPON_FSM bits 22..23 == 01. */
    uint8_t inferred_tx_paths; /* 1 when 8191EU bonding, otherwise 2. */
} rtw_8192eu_identity;

/* Supports exactly one recorded USB ID, 2357:0108; do not match unrelated
 * products. Pure decoder: caller must already have obtained register values.
 * A plausible SYS_CFG is not a guarantee of a functioning chipset. */
rtw_8192eu_result rtw_8192eu_decode_identity(uint16_t vid, uint16_t pid,
    uint32_t sys_cfg, uint32_t sys_cfg2, uint32_t hpon_fsm,
    uint32_t mcu_fw_dl, rtw_8192eu_identity* out);

/* Performs FOUR bounded USB vendor READS, never opens/claims/resets interfaces,
 * sends OUT requests, loads firmware or activates RF. Transport is owned by
 * caller and must use the Realtek USB vendor request (0xC0/0x05).
 * Requires VID/PID as an explicit preflight before the first control request. */
rtw_8192eu_result rtw_8192eu_probe_registers(
    const rtw_usb_transport* usb, uint16_t vid, uint16_t pid,
    rtw_8192eu_identity* out);

typedef struct {
    uint8_t mac[6];
    uint8_t channel_plan;  /* 0xb8 (NOT the 8812AU offset 0xc8) */
    uint8_t crystal_cap;   /* 0xb9; low six bits */
    uint8_t thermal_meter; /* 0xba */
    uint8_t rf_board_option; /* 0xc1 */
    uint16_t rtl_id;      /* 0x8129 is required */
} rtw_8192eu_efuse_identity;

/* Takes an already-unpacked 512-byte logical efuse image. No OTP reads or
 * writes. An all-zero/all-FF/multicast MAC is not valid. USB VID/PID bytes
 * in this map are board metadata, NOT a safe substitute for USB enumeration. */
rtw_8192eu_result rtw_8192eu_parse_logical_efuse(
    const uint8_t* logical, size_t length, rtw_8192eu_efuse_identity* out);

/* Shares the packed-efuse decoder with the 8812 module, but applies 8192EU's
 * own signature and offsets. Does not access physical hardware. */
rtw_8192eu_result rtw_8192eu_parse_physical_efuse(
    const uint8_t* physical, size_t length, rtw_8192eu_efuse_identity* out);

typedef struct {
    uint16_t signature;    /* (signature & 0xFFF0) == 0x92E0 */
    uint8_t category, function;
    uint16_t major_version;
    uint8_t minor_version;
    uint16_t reported_ramcode_size; /* metadata; not blindly trusted */
    uint32_t svn_index;
    const uint8_t* payload;
    size_t payload_length;
} rtw_8192eu_fw_info;

/* Offline inspection. Rejects incorrect 8812AU firmware (0x950x) and other
 * chip families, as well as images larger than 32 KiB of firmware RAM.
 * Does not download or execute the supplied firmware. */
rtw_8192eu_result rtw_8192eu_inspect_firmware(
    const uint8_t* image, size_t length, rtw_8192eu_fw_info* out);

typedef struct {
    rtw_rx_packet payload;  /* borrowed, valid while input bulk buffer lives */
    uint8_t report_is_c2h; /* DW2 bit 28; must not be treated as 802.11 data */
    uint8_t security_code; /* DW0 bits 20..22 */
    uint8_t sw_decrypt_required;  /* DW0 bit 27: SWDEC set = frame requires software decrypt; NOT authorization */
    uint8_t qos;
    uint8_t fragmented;
    uint16_t sequence;
    uint8_t fragment_number;
} rtw_8192eu_rx_frame;

/* RTL8192EU uses the Realtek 24-byte RX descriptor format also implemented
 * by rtw_rx_next; adds the family-specific metadata. Does not accept frames
 * into a network stack or claim hardware decryption means RSN is verified. */
rtw_packet_error rtw_8192eu_rx_next(
    rtw_rx_iterator* iterator, rtw_8192eu_rx_frame* out);

const char* rtw_8192eu_result_string(rtw_8192eu_result error);
#ifdef __cplusplus
}
#endif
#endif
