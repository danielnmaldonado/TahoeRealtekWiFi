#ifndef RTW_RX_MONITOR_H
#define RTW_RX_MONITOR_H
#include "rtw/HAL8812.h"
#include "rtw/PHYProfile.h"
#include "rtw/USBPackets.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Passive RTL8812AU monitor laboratory configuration. Does NOT implement
 * association/RSN, delivery to macOS, or a live Ethernet interface.
 * All public operations are explicit: no writes occur during kext probe. */
typedef enum {
    RTW_MONITOR_OK=0, RTW_MONITOR_BAD_ARGUMENT, RTW_MONITOR_NOT_AUTHORIZED,
    RTW_MONITOR_UNSUPPORTED, RTW_MONITOR_PHY_FAILED,
    RTW_MONITOR_MAC_FAILED, RTW_MONITOR_USB_FAILED,
    RTW_MONITOR_REGISTER_MISMATCH
} rtw_monitor_error;
typedef struct {
    rtw_phy_profile_report phy;
    size_t mac_operations;
    size_t rx_register_operations;
    uint8_t monitor_registers_written; /* Not evidence of actual RF reception. */
    uint8_t ethernet_authorized; /* Must remain zero. */
} rtw_monitor_report;
/* Requires successful pre-RF run and RFE 3 B-cut/TSMC/USB. Caller must
 * be the explicit opt-in lab tool, never an automatic kext start callback. */
rtw_monitor_error rtw_monitor_prepare_channel6(
    const rtw_usb_rw_transport* bus,const rtw_waiter* waiter,
    const rtw_hal_report* boot, int allow_hardware_writes,
    rtw_monitor_report* report);
/* Read-only verification of the narrow RTL8812AU channel-6 lab profile.
 * Compares the silicon identity, RX filter, RX enable and selected baseband
 * gate bits after monitor setup. No writes or firmware operations occur.
 * Passing does NOT prove RF calibration, actual reception or permission to TX.
 * Exactly six bounded register reads on success. */
rtw_monitor_error rtw_monitor_verify_channel6(
    const rtw_usb_rw_transport* bus, const rtw_monitor_report* report);
/* Best-effort fail-closed MAC receive shutdown for the explicitly
 * configured monitor. Clears MACRXEN, RXDMA_EN, HCI_RXDMA_EN in CR (0x100),
 * preserving all other CR bits. Does not power down the RF or undo firmware.
 * Also accepts a report with partial RX configuration so failures on the
 * last MAC-enable control transfer do not leave a possibly active receiver.
 * The caller must report an I/O failure and recommend physical replug. */
rtw_monitor_error rtw_monitor_quiesce_rx_channel6(
    const rtw_usb_rw_transport *bus, const rtw_monitor_report *report);
/* Aggregate RX parsing: strict descriptor validation, drops CRC/ICV errors;
 * only FCS-valid beacons/probe responses are passed to the caller. Callback MUST copy
 * any frame it needs after the call, and MUST NOT assume authentication. */
typedef int (*rtw_monitor_beacon_cb)(void* context,const uint8_t* mpdu,
                                      size_t length,const rtw_rx_packet* packet);
typedef struct {
    size_t packets_seen, beacons, crc_dropped, icv_dropped,
           non_beacons, malformed, fcs_dropped;
} rtw_monitor_rx_stats;
rtw_packet_error rtw_monitor_process_bulk(const uint8_t* bulk,size_t length,
                                rtw_monitor_beacon_cb on_beacon,void* context,
                                rtw_monitor_rx_stats* stats);
const char* rtw_monitor_error_string(rtw_monitor_error err);
#ifdef __cplusplus
}
#endif
#endif
