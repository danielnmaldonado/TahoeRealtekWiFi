/* SPDX-License-Identifier: GPL-2.0-only
 * RF readback sequencing follows the RTL8812AU Jaguar LSSI/HSSI reference
 * (xen-proc/rtl8812au-macos rtl8812au/phy.py), restricted to RF[0x18].
 */
#include "rtw/RFReadback.h"
#include <string.h>

enum {
    RTW_RF_CCA_SECONDARY = 0x0838U,
    RTW_RF_HSSI_READ = 0x08B0U,
    RTW_RF_PI_MODE_A = 0x0C00U,
    RTW_RF_PI_MODE_B = 0x0E00U,
    RTW_RF_PI_READ_A = 0x0D04U,
    RTW_RF_SI_READ_A = 0x0D08U,
    RTW_RF_PI_READ_B = 0x0D44U,
    RTW_RF_SI_READ_B = 0x0D48U,
    RTW_RF_CHANNEL_REGISTER = 0x18U
};

static int read32(const rtw_usb_rw_transport *bus,
                  uint16_t address, uint32_t *value) {
    return rtw_usb_rw_read(bus, address, 4U, value) == RTW_USB_OK;
}

static int write32(const rtw_usb_rw_transport *bus,
                   uint16_t address, uint32_t value) {
    return rtw_usb_write_register(bus, address, 4U, value) == RTW_USB_OK;
}

static int matches_channel6(uint32_t value) {
    /* RF18 channel field is bits 7:0. RF18 band selection resides in
       bits 18:16 and 9:8. The high 0x7xxx portion is board-specific. */
    return value != 0U && value != 0x000FFFFFU &&
           (value & 0xFFU) == 6U &&
           (value & 0x00070300U) == 0U;
}

static rtw_rf_readback_error rf18_read(
    const rtw_usb_rw_transport *bus, const rtw_waiter *waiter,
    uint32_t hssi_original, uint16_t pi_or_si_register, uint32_t *sample,
    size_t *completed) {
    uint32_t raw = 0U;
    const uint32_t selector =
        (hssi_original & ~0xFFU) | RTW_RF_CHANNEL_REGISTER;
    if (!write32(bus, RTW_RF_HSSI_READ, selector))
        return RTW_RF_READBACK_USB_ERROR;
    /* Bound the RF serial settling interval and propagate failure. */
    if (!waiter->wait_us(waiter->context, 10U))
        return RTW_RF_READBACK_DELAY_FAILED;
    if (!read32(bus, pi_or_si_register, &raw))
        return RTW_RF_READBACK_USB_ERROR;
    *sample = raw & 0x000FFFFFU;
    ++*completed;
    return RTW_RF_READBACK_OK;
}

rtw_rf_readback_error rtw_rf_readback_channel6_8812au(
    const rtw_usb_rw_transport *bus, const rtw_waiter *waiter,
    const rtw_monitor_report *monitor, int allow_hardware_writes,
    rtw_rf_readback_report *report) {
    rtw_rf_readback_report current;
    rtw_rf_readback_error status = RTW_RF_READBACK_OK;
    uint32_t cca_original = 0U;
    uint32_t hssi_original = 0U;
    uint32_t mode_a = 0U;
    uint32_t mode_b = 0U;
    uint16_t data_a;
    uint16_t data_b;
    int cca_touched = 0;
    int hssi_touched = 0;
    int restored = 1;

    memset(&current, 0, sizeof(current));
    if (report) memset(report, 0, sizeof(*report));
    if (!report || !bus || !bus->control_in || !bus->control_out ||
        !waiter || !waiter->wait_us || !monitor)
        return RTW_RF_READBACK_BAD_ARGUMENT;
    if (!allow_hardware_writes)
        return RTW_RF_READBACK_NOT_AUTHORIZED;
    if (!monitor->monitor_registers_written ||
        monitor->ethernet_authorized)
        return RTW_RF_READBACK_MONITOR_UNVERIFIED;
    /* Prevent an RF selector write unless MAC/BB still match the narrow,
       explicitly authorized monitor configuration. */
    if (rtw_monitor_verify_channel6(bus, monitor) != RTW_MONITOR_OK)
        return RTW_RF_READBACK_MONITOR_UNVERIFIED;

    /* Snapshot all BB registers BEFORE any modification. */
    if (!read32(bus, RTW_RF_CCA_SECONDARY, &cca_original) ||
        !read32(bus, RTW_RF_HSSI_READ, &hssi_original) ||
        !read32(bus, RTW_RF_PI_MODE_A, &mode_a) ||
        !read32(bus, RTW_RF_PI_MODE_B, &mode_b))
        return RTW_RF_READBACK_USB_ERROR;
    current.pi_mode_mask = (uint8_t)(
        ((mode_a & 4U) ? 1U : 0U) | ((mode_b & 4U) ? 2U : 0U));
    data_a = (uint16_t)((mode_a & 4U) ?
        RTW_RF_PI_READ_A : RTW_RF_SI_READ_A);
    data_b = (uint16_t)((mode_b & 4U) ?
        RTW_RF_PI_READ_B : RTW_RF_SI_READ_B);

    /* The reference driver guards the secondary CCA while RF[0x18] is read.
       Preserve the original bit (including an already enabled guard). */
    if (!(cca_original & 8U)) {
        cca_touched = 1;
        if (!write32(bus, RTW_RF_CCA_SECONDARY, cca_original | 8U)) {
            status = RTW_RF_READBACK_USB_ERROR;
            goto cleanup;
        }
    }

    /* Mark the selector as touched BEFORE its first write; a reported USB
       failure might still have reached hardware and must be cleaned up. */
    hssi_touched = 1;
    status = rf18_read(bus, waiter, hssi_original, data_a,
                       &current.rf18_a_first, &current.successful_samples);
    if (status != RTW_RF_READBACK_OK) goto cleanup;
    status = rf18_read(bus, waiter, hssi_original, data_b,
                       &current.rf18_b_first, &current.successful_samples);
    if (status != RTW_RF_READBACK_OK) goto cleanup;
    status = rf18_read(bus, waiter, hssi_original, data_a,
                       &current.rf18_a_second, &current.successful_samples);
    if (status != RTW_RF_READBACK_OK) goto cleanup;
    status = rf18_read(bus, waiter, hssi_original, data_b,
                       &current.rf18_b_second, &current.successful_samples);

cleanup:
    /* Never trust success or authorize RX until selector/CCA cleanup succeeds.
       Both restorations are attempted even if the first fails. */
    if (hssi_touched && !write32(bus, RTW_RF_HSSI_READ, hssi_original))
        restored = 0;
    if (cca_touched && !write32(bus, RTW_RF_CCA_SECONDARY, cca_original))
        restored = 0;
    if (restored && hssi_touched) {
        uint32_t restored_value = 0U;
        if (!read32(bus, RTW_RF_HSSI_READ, &restored_value) ||
            restored_value != hssi_original)
            restored = 0;
    }
    if (restored && cca_touched) {
        uint32_t restored_value = 0U;
        if (!read32(bus, RTW_RF_CCA_SECONDARY, &restored_value) ||
            restored_value != cca_original)
            restored = 0;
    }
    current.bb_registers_restored = (uint8_t)restored;
    current.path_a_confirmed = (uint8_t)(
        restored && status == RTW_RF_READBACK_OK &&
        current.successful_samples >= 3U &&
        matches_channel6(current.rf18_a_first) &&
        matches_channel6(current.rf18_a_second));
    current.path_b_confirmed = (uint8_t)(
        restored && status == RTW_RF_READBACK_OK &&
        current.successful_samples == 4U &&
        matches_channel6(current.rf18_b_first) &&
        matches_channel6(current.rf18_b_second));
    current.path_b_inconclusive = (uint8_t)(
        current.successful_samples >= 2U && !current.path_b_confirmed);
    *report = current;
    if (!restored) return RTW_RF_READBACK_RESTORE_FAILED;
    if (status != RTW_RF_READBACK_OK) return status;
    if (!current.path_a_confirmed)
        return RTW_RF_READBACK_CHANNEL_MISMATCH;
    return RTW_RF_READBACK_OK;
}

const char *rtw_rf_readback_error_string(rtw_rf_readback_error error) {
    switch (error) {
    case RTW_RF_READBACK_OK:
        return "channel 6 observed on RF path A; TX not authorized";
    case RTW_RF_READBACK_BAD_ARGUMENT:
        return "invalid RF readback arguments";
    case RTW_RF_READBACK_NOT_AUTHORIZED:
        return "RF selector writes not authorized";
    case RTW_RF_READBACK_MONITOR_UNVERIFIED:
        return "MAC/BB channel 6 monitor verification required";
    case RTW_RF_READBACK_USB_ERROR:
        return "RF readback USB register access failed";
    case RTW_RF_READBACK_DELAY_FAILED:
        return "RF serial settling delay failed";
    case RTW_RF_READBACK_RESTORE_FAILED:
        return "cannot restore RF readback BB selector/CCA registers";
    case RTW_RF_READBACK_CHANNEL_MISMATCH:
        return "RF path A is not confirmed on channel 6";
    default:
        return "unknown RF readback error";
    }
}
