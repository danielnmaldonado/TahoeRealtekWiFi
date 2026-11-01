/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_R12AU_RF_CALIBRATION_H
#define RTW_R12AU_RF_CALIBRATION_H

#include "rtw/RFReadback.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Observational thermal preflight, NOT an IQK/LCK/DPK calibration.
 *
 * Strictly limited to the explicitly authorized RTL8812AU B-cut/TSMC/USB,
 * RFE3, channel-6, 20MHz passive laboratory profile. Reads RF path A's
 * RF[0x42] temperature-meter field [15:10] via the Jaguar HSSI/PI/SI path.
 * This does NOT trigger a new temperature conversion; the RF reading may
 * be stale. Only BB CCA/selector registers are modified temporarily.
 * No RF write, transmit, firmware command, power tracking or IQK occurs.
 *
 * A successful preflight never authorizes TX or station association.
 * eFuse factory temperature and observed thermal codes are dimensionless
 * hardware codes, NOT calibrated degrees Celsius.
 */
typedef enum {
    RTW_RF_CAL_OK = 0,
    RTW_RF_CAL_BAD_ARGUMENT,
    RTW_RF_CAL_NOT_AUTHORIZED,
    RTW_RF_CAL_PROFILE_UNVERIFIED,
    RTW_RF_CAL_USB_ERROR,
    RTW_RF_CAL_DELAY_FAILED,
    RTW_RF_CAL_RESTORE_FAILED,
    RTW_RF_CAL_SENSOR_UNAVAILABLE,
    RTW_RF_CAL_SENSOR_UNSTABLE,
    RTW_RF_CAL_FACTORY_UNAVAILABLE
} rtw_rf_cal_error;

typedef struct {
    uint32_t rf42_first;
    uint32_t rf42_second;
    size_t samples_completed;
    uint8_t first_code;
    uint8_t second_code;
    uint8_t averaged_code;
    uint8_t factory_code;
    uint8_t baseline_valid;
    uint8_t stable;
    uint8_t drift_codes;
    uint8_t delta_exceeds_iqk_threshold; /* A recommendation only. */
    uint8_t bb_registers_restored;
    uint8_t iqk_performed;               /* Always zero. */
    uint8_t tx_authorized;               /* Always zero. */
} rtw_rf_cal_report;

/*
 * The passive monitor and path-A channel-6 RF readback must both have passed.
 * Refuses any I/O without explicit user authorization and exact chip profile.
 * On all exits after a selector write, tries to restore previous HSSI/CCA
 * state; a restore failure overrides other error results. Callers must
 * quiesce MAC RX and recommend physical unplug/replug on uncertain cleanup.
 */
rtw_rf_cal_error rtw_rf_thermal_preflight_8812au(
    const rtw_usb_rw_transport *bus,
    const rtw_waiter *waiter,
    const rtw_hal_report *boot,
    const rtw_monitor_report *monitor,
    const rtw_rf_readback_report *rf_channel,
    int allow_hardware_writes,
    rtw_rf_cal_report *report);

const char *rtw_rf_cal_error_string(rtw_rf_cal_error error);

#ifdef __cplusplus
}
#endif
#endif
