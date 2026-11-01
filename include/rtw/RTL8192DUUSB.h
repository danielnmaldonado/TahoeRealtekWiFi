/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_RTL8192DU_USB_H
#define RTW_RTL8192DU_USB_H
#include "USBRegisterIO.h"
#include "RTL8192DUBoot.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Per-MAC vendor-control aperture from lwfinger/rtl8192du hal/usb_ops_linux.c
 * at dc2c6951a9fd28fdb20549140052bbdac3490865. Mainline's shared USB transport
 * does not apply this MAC1 remap. Physical USB requests must apply it once,
 * beneath DU boot/MAC/PHY, including the cross-PHY pseudo-addresses.
 * This mapping grants no ownership: callers must exclusively own both USB
 * interfaces before any DU power, firmware or RF operation. */
typedef struct {
    rtw_usb_rw_transport raw,mapped;
    uint8_t mac_index;
} rtw_92du_usb_map;
rtw_usb_error rtw_92du_usb_address(uint8_t mac_index,uint16_t logical,uint16_t *physical);
rtw_usb_error rtw_92du_usb_map_init(rtw_92du_usb_map *,const rtw_usb_rw_transport *,uint8_t mac_index);
const rtw_usb_rw_transport *rtw_92du_usb_bus(const rtw_92du_usb_map *);
/* Select the descriptor-validated data interface for a verified PROM board.
 * The catalog identity stays anchored to interface0. DMDP MAC1 requires a
 * separate interface1; its endpoint count must never be borrowed from MAC0.
 * All interfaces must be vendor FF/FF/FF at alternate0, with disjoint bulk
 * addresses. No I/O, claims, ownership or station activation is implied. */
rtw_92du_result rtw_92du_usb_interface_endpoints(const rtw_backend_usb_plan *,
    const rtw_92du_board *,const uint8_t *configuration,size_t length,
    rtw_usb_endpoints *out);
#ifdef __cplusplus
}
#endif
#endif
