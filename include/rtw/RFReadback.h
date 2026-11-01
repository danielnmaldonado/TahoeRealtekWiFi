/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_R12AU_RF_READBACK_H
#define RTW_R12AU_RF_READBACK_H

#include "rtw/RXMonitor.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Invasive, opt-in RF register diagnostic for the exact RTL8812AU
 * B-cut/TSMC/USB/RFE3/channel-6 receive-only laboratory configuration.
 * RF[0x18] is read through the Jaguar HSSI/LSSI path, not USB register space.
 *
 * Reading RF requires temporary BB register writes (0x0838 and 0x08B0).
 * The previous complete register values are restored even after failures,
 * insofar as the device still accepts USB control transfers.
 * Path B is informational: the reference hardware sometimes returns zero
 * for otherwise active path B. A verified path A is required for success.
 *
 * A successful result does NOT establish a calibrated radio, a working
 * second chain, regulatory authorization, TX readiness, or STA support.
 */
typedef enum {
    RTW_RF_READBACK_OK = 0,
    RTW_RF_READBACK_BAD_ARGUMENT,
    RTW_RF_READBACK_NOT_AUTHORIZED,
    RTW_RF_READBACK_MONITOR_UNVERIFIED,
    RTW_RF_READBACK_USB_ERROR,
    RTW_RF_READBACK_DELAY_FAILED,
    RTW_RF_READBACK_RESTORE_FAILED,
    RTW_RF_READBACK_CHANNEL_MISMATCH
} rtw_rf_readback_error;

typedef struct {
    uint32_t rf18_a_first;
    uint32_t rf18_b_first;
    uint32_t rf18_a_second;
    uint32_t rf18_b_second;
    size_t successful_samples;
    uint8_t path_a_confirmed;
    uint8_t path_b_confirmed;
    uint8_t path_b_inconclusive;
    uint8_t pi_mode_mask;
    uint8_t bb_registers_restored;
} rtw_rf_readback_report;

/*
 * Requires explicit consent, an already configured monitor report, and
 * a successful read-only 6-register MAC/BB verification of channel 6.
 * After a successful check, only RF path A is sufficiently verified to
 * gate a passive capture. The channel 6 RF path B value is reported but
 * not treated as proof of path B reception.
 */
rtw_rf_readback_error rtw_rf_readback_channel6_8812au(
    const rtw_usb_rw_transport *bus, const rtw_waiter *waiter,
    const rtw_monitor_report *monitor, int allow_hardware_writes,
    rtw_rf_readback_report *report);
const char *rtw_rf_readback_error_string(rtw_rf_readback_error error);

#ifdef __cplusplus
}
#endif
#endif
