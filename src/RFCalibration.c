/* SPDX-License-Identifier: GPL-2.0-only
 * RTL8812AU Jaguar thermal preflight.
 * Thermal RF[0x42] [15:10]: Realtek HalPhyRf_8812A and rtl8821ae/dm.c.
 * Factory thermal code: Realtek EEPROM_THERMAL_METER_8812 at eFuse 0xBA.
 * Thermal IQK drift threshold: Realtek ODM thermal tracking (8 code units).
 *
 * This module does not execute an RF calibration or temperature trigger.
 */
#include "rtw/RFCalibration.h"
#include <string.h>

enum {
    RTW_CAL_CCA_REG = 0x0838U,
    RTW_CAL_HSSI_REG = 0x08B0U,
    RTW_CAL_PI_MODE_A = 0x0C00U,
    RTW_CAL_PI_READ_A = 0x0D04U,
    RTW_CAL_SI_READ_A = 0x0D08U,
    RTW_CAL_THERMAL_RF = 0x42U,
    RTW_CAL_IQK_DRIFT_THRESHOLD = 8U,
    RTW_CAL_MAX_SAMPLE_DIFFERENCE = 2U
};

static int read32(const rtw_usb_rw_transport *bus, uint16_t reg,
                  uint32_t *value) {
    return rtw_usb_rw_read(bus, reg, 4U, value) == RTW_USB_OK;
}

static int write32(const rtw_usb_rw_transport *bus, uint16_t reg,
                   uint32_t value) {
    return rtw_usb_write_register(bus, reg, 4U, value) == RTW_USB_OK;
}

rtw_rf_cal_error rtw_rf_thermal_preflight_8812au(
    const rtw_usb_rw_transport *bus,
    const rtw_waiter *waiter,
    const rtw_hal_report *boot,
    const rtw_monitor_report *monitor,
    const rtw_rf_readback_report *rf_channel,
    int allow_hardware_writes,
    rtw_rf_cal_report *report) {
    rtw_rf_cal_report measured;
    rtw_rf_cal_error status = RTW_RF_CAL_OK;
    uint32_t cca = 0U, selector = 0U, pi_mode = 0U;
    uint32_t observed = 0U, restored_value = 0U;
    uint16_t data_reg = RTW_CAL_SI_READ_A;
    unsigned index;
    int cca_touched = 0, selector_touched = 0, restored = 1;

    memset(&measured, 0, sizeof(measured));
    if (report) memset(report, 0, sizeof(*report));
    if (!report || !bus || !bus->control_in || !bus->control_out ||
        !waiter || !waiter->wait_us || !boot || !monitor || !rf_channel)
        return RTW_RF_CAL_BAD_ARGUMENT;
    if (!allow_hardware_writes)
        return RTW_RF_CAL_NOT_AUTHORIZED;
    if (boot->last_success != RTW_HAL_MAC_PREPARED ||
        boot->sys_cfg != 0x04411137U ||
        boot->identity.eeprom_vendor != 0x0BDAU ||
        boot->identity.eeprom_product != 0x8812U ||
        boot->identity.rfe_type != 3U ||
        boot->radio_ready || boot->ethernet_authorized ||
        !monitor->monitor_registers_written || monitor->ethernet_authorized ||
        !rf_channel->path_a_confirmed ||
        !rf_channel->bb_registers_restored)
        return RTW_RF_CAL_PROFILE_UNVERIFIED;
    /* Read-only MAC/BB verification before any selector writes. */
    if (rtw_monitor_verify_channel6(bus, monitor) != RTW_MONITOR_OK)
        return RTW_RF_CAL_PROFILE_UNVERIFIED;

    measured.factory_code = boot->identity.thermal_meter;
    measured.baseline_valid = boot->identity.thermal_meter_valid &&
        measured.factory_code > 0U && measured.factory_code < 63U;

    /* Complete snapshot prior to the first write. */
    if (!read32(bus, RTW_CAL_CCA_REG, &cca) ||
        !read32(bus, RTW_CAL_HSSI_REG, &selector) ||
        !read32(bus, RTW_CAL_PI_MODE_A, &pi_mode))
        return RTW_RF_CAL_USB_ERROR;
    if (pi_mode & 4U) data_reg = RTW_CAL_PI_READ_A;

    /* Preserve existing CCA state. For 8812AU B-cut, bit 3 must be set
     * during Jaguar RF serial reads (see vendor phy_RFSerialRead). */
    if (!(cca & 8U)) {
        cca_touched = 1; /* An ambiguous USB error may have applied a write. */
        if (!write32(bus, RTW_CAL_CCA_REG, cca | 8U)) {
            status = RTW_RF_CAL_USB_ERROR;
            goto cleanup;
        }
    }

    selector_touched = 1;
    for (index = 0U; index < 2U; ++index) {
        if (!write32(bus, RTW_CAL_HSSI_REG,
                     (selector & ~0xFFU) | RTW_CAL_THERMAL_RF)) {
            status = RTW_RF_CAL_USB_ERROR;
            break;
        }
        if (!waiter->wait_us(waiter->context, 10U)) {
            status = RTW_RF_CAL_DELAY_FAILED;
            break;
        }
        if (!read32(bus, data_reg, &observed)) {
            status = RTW_RF_CAL_USB_ERROR;
            break;
        }
        observed &= 0x000FFFFFU;
        if (index == 0U) measured.rf42_first = observed;
        else measured.rf42_second = observed;
        ++measured.samples_completed;
    }

cleanup:
    /* Restoration failures override all successful or partial observations.
     * Attempt both writes even if an earlier operation failed. */
    if (selector_touched && !write32(bus, RTW_CAL_HSSI_REG, selector))
        restored = 0;
    if (cca_touched && !write32(bus, RTW_CAL_CCA_REG, cca))
        restored = 0;

    if (selector_touched) {
        if (!read32(bus, RTW_CAL_HSSI_REG, &restored_value) ||
            restored_value != selector)
            restored = 0;
    }
    if (cca_touched) {
        if (!read32(bus, RTW_CAL_CCA_REG, &restored_value) ||
            restored_value != cca)
            restored = 0;
    }
    measured.bb_registers_restored = (uint8_t)restored;
    if (measured.samples_completed >= 1U)
        measured.first_code = (uint8_t)(
            (measured.rf42_first & 0xFC00U) >> 10U);
    if (measured.samples_completed == 2U)
        measured.second_code = (uint8_t)(
            (measured.rf42_second & 0xFC00U) >> 10U);
    *report = measured;

    if (!restored) return RTW_RF_CAL_RESTORE_FAILED;
    if (status != RTW_RF_CAL_OK) return status;
    if (measured.samples_completed != 2U ||
        measured.rf42_first == 0U || measured.rf42_first == 0xFFFFFU ||
        measured.rf42_second == 0U || measured.rf42_second == 0xFFFFFU ||
        measured.first_code == 0U || measured.first_code == 63U ||
        measured.second_code == 0U || measured.second_code == 63U)
        return RTW_RF_CAL_SENSOR_UNAVAILABLE;

    {
        const unsigned low = measured.first_code < measured.second_code
            ? measured.first_code : measured.second_code;
        const unsigned high = measured.first_code > measured.second_code
            ? measured.first_code : measured.second_code;
        if (high - low > RTW_CAL_MAX_SAMPLE_DIFFERENCE)
            return RTW_RF_CAL_SENSOR_UNSTABLE;
    }

    measured.stable = 1U;
    measured.averaged_code = (uint8_t)(
        ((unsigned)measured.first_code +
         (unsigned)measured.second_code + 1U) / 2U);
    if (measured.baseline_valid) {
        const unsigned observed_code = measured.averaged_code;
        const unsigned factory_code = measured.factory_code;
        const unsigned delta = observed_code > factory_code
            ? observed_code - factory_code : factory_code - observed_code;
        measured.drift_codes = (uint8_t)delta;
        measured.delta_exceeds_iqk_threshold = (uint8_t)(
            delta >= RTW_CAL_IQK_DRIFT_THRESHOLD);
    }
    /* A baseline comparison is NOT proof that IQK has ever been run.
     * Never authorize transmit, station association or apply RF corrections. */
    *report = measured;
    return measured.baseline_valid ? RTW_RF_CAL_OK :
        RTW_RF_CAL_FACTORY_UNAVAILABLE;
}

const char *rtw_rf_cal_error_string(rtw_rf_cal_error error) {
    switch (error) {
    case RTW_RF_CAL_OK: return "thermal codes observed; IQK NOT performed";
    case RTW_RF_CAL_BAD_ARGUMENT: return "invalid RF calibration preflight arguments";
    case RTW_RF_CAL_NOT_AUTHORIZED: return "thermal RF selector writes not authorized";
    case RTW_RF_CAL_PROFILE_UNVERIFIED: return "exact monitor and RF profile not verified";
    case RTW_RF_CAL_USB_ERROR: return "thermal sensor RF register access failed";
    case RTW_RF_CAL_DELAY_FAILED: return "thermal RF serial settling delay failed";
    case RTW_RF_CAL_RESTORE_FAILED: return "RF thermal BB selector/CCA restore failed";
    case RTW_RF_CAL_SENSOR_UNAVAILABLE: return "RF[0x42] temperature sample unavailable";
    case RTW_RF_CAL_SENSOR_UNSTABLE: return "inconsistent RF[0x42] temperature samples";
    case RTW_RF_CAL_FACTORY_UNAVAILABLE: return "factory eFuse thermal baseline unavailable";
    default: return "unknown RF thermal preflight error";
    }
}
