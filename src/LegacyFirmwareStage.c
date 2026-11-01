/* SPDX-License-Identifier: GPL-2.0-only */
/* RTL8XXXU 8051 download and startup protocol adapted from the Linux
   rtl8xxxu core (GPL-2.0-only). The live path is intentionally NOT
   connected to the macOS kext or to automatic device attachment. */
#include "rtw/LegacyFirmwareStage.h"
#include <string.h>

enum {
    REG_SYS_FUNC = 0x0002,
    REG_RSV_CTRL_HI = 0x001d,
    REG_MCUFWDL = 0x0080,
    REG_MCUFWDL_8710B = 0x0090,
    REG_FW_WINDOW = 0x1000,
    REG_FW_WINDOW_8192F = 0x4000,
    CPU_ENABLE = 0x0400,
    DOWNLOAD_ENABLE = 0x01,
    DOWNLOAD_READY = 0x02,
    CHECKSUM_REPORT = 0x04,
    INIT_READY = 0x40,
    RAM_PRESENT = 0x80
};

uint8_t rtw_legacy_fw_capability(rtw_wifi_family family) {
    switch (family) {
    case RTW_WIFI_RTL8723BU:
        /* Typed native8723 owns its actual530x NIC firmware and boot. */
        return 1U;
    case RTW_WIFI_RTL8192FU:
        /* Native FU owns its distinct reset/power/readiness sequence. */
        return 1U;
    case RTW_WIFI_RTL8710BU:
        return 1U;
    default:
        return 0U;
    }
}

rtw_legacy_fw_result rtw_legacy_fw_prepare(
    const rtw_backend_usb_plan *usb, const uint8_t *firmware,
    size_t length, rtw_legacy_fw_plan *out) {
    rtw_8192eu_fw_info image;
    rtw_legacy_fw_plan next;
    size_t i;
    int pointer_verified = 0;
    if (out) memset(out, 0, sizeof(*out));
    if (!out || !usb || !firmware || !usb->profile || !usb->backend)
        return RTW_LEGACY_FW_BAD_ARGUMENT;
    if (!usb->profile->primary_verified ||
        usb->profile->family != usb->backend->family ||
        usb->backend != rtw_backend_for_family(usb->profile->family) ||
        usb->station_backend_integrated || !usb->usb_preflight_only)
        return RTW_LEGACY_FW_IDENTITY_UNVERIFIED;
    for (i = 0U; i < rtw_usb_wifi_profile_count(); ++i) {
        if (rtw_usb_wifi_profile_at(i) == usb->profile) {
            pointer_verified = 1;
            break;
        }
    }
    if (!pointer_verified) return RTW_LEGACY_FW_IDENTITY_UNVERIFIED;
    if (usb->endpoints.num_bulk_in == 0U ||
        usb->endpoints.num_bulk_out == 0U ||
        usb->profile->vendor == 0U || usb->profile->product == 0U)
        return RTW_LEGACY_FW_IDENTITY_UNVERIFIED;

    memset(&next, 0, sizeof(next));
    next.family = usb->profile->family;
    switch (next.family) {
    case RTW_WIFI_RTL8192FU:
        next.mcu_download_register = REG_MCUFWDL;
        next.firmware_window_register = REG_FW_WINDOW_8192F;
        /* Offline layout only: generic reserved-control reset is not FU. */
        next.boot_experimental_available = 0U;
        break;
    case RTW_WIFI_RTL8723BU:
        next.mcu_download_register = REG_MCUFWDL;
        next.firmware_window_register = REG_FW_WINDOW;
        /* Offline layout only; the old230x image family is not8723BU. */
        next.boot_experimental_available = 0U;
        break;
    case RTW_WIFI_RTL8710BU:
        next.mcu_download_register = REG_MCUFWDL_8710B;
        next.firmware_window_register = REG_FW_WINDOW;
        /* 8710BU has a distinct CPU reset/power sequence. */
        break;
    default:
        return RTW_LEGACY_FW_UNSUPPORTED;
    }
    if (!rtw_usb_wifi_inspect_firmware(next.family, firmware, length, &image))
        return RTW_LEGACY_FW_BAD_IMAGE;
    if (image.payload_length > RTW_LEGACY_FW_PAGE_SIZE *
                              RTW_LEGACY_FW_MAX_PAGES)
        return RTW_LEGACY_FW_CAPACITY_EXCEEDED;
    next.firmware_signature = image.signature;
    next.firmware_version = image.major_version;
    next.payload = image.payload;
    next.payload_size = image.payload_length;
    next.pages = (uint8_t)((image.payload_length +
                    RTW_LEGACY_FW_PAGE_SIZE - 1U) / RTW_LEGACY_FW_PAGE_SIZE);
    *out = next;
    return RTW_LEGACY_FW_OK;
}

static int plan_valid(const rtw_legacy_fw_plan *plan) {
    if (!plan || !plan->payload || !plan->payload_size ||
        plan->payload_size > RTW_LEGACY_FW_PAGE_SIZE * RTW_LEGACY_FW_MAX_PAGES ||
        plan->pages != (uint8_t)((plan->payload_size +
                     RTW_LEGACY_FW_PAGE_SIZE - 1U) / RTW_LEGACY_FW_PAGE_SIZE))
        return 0;
    /* Confirm the immutable geometry before using any caller supplied plan. */
    switch (plan->family) {
    case RTW_WIFI_RTL8192FU:
        if (plan->mcu_download_register != REG_MCUFWDL ||
            plan->firmware_window_register != REG_FW_WINDOW_8192F ||
            plan->boot_experimental_available) return 0;
        break;
    case RTW_WIFI_RTL8723BU:
        if (plan->mcu_download_register != REG_MCUFWDL ||
            plan->firmware_window_register != REG_FW_WINDOW ||
            plan->boot_experimental_available) return 0;
        break;
    case RTW_WIFI_RTL8710BU:
        if (plan->mcu_download_register != REG_MCUFWDL_8710B ||
            plan->firmware_window_register != REG_FW_WINDOW ||
            plan->boot_experimental_available) return 0;
        break;
    default: return 0;
    }
    if (plan->family == RTW_WIFI_RTL8192FU &&
        (plan->firmware_signature & 0xfff0U) != 0x92f0U) return 0;
    if (plan->family == RTW_WIFI_RTL8723BU &&
        (plan->firmware_signature & 0xfff0U) != 0x5300U) return 0;
    if (plan->family == RTW_WIFI_RTL8710BU &&
        (plan->firmware_signature & 0xfff0U) != 0x10b0U) return 0;
    return 1;
}

rtw_legacy_fw_result rtw_legacy_fw_walk(
    const rtw_legacy_fw_plan *plan, rtw_legacy_fw_chunk callback,
    void *context, size_t *count) {
    size_t at = 0U, n = 0U;
    if (count) *count = 0U;
    if (!callback || !plan_valid(plan))
        return RTW_LEGACY_FW_BAD_ARGUMENT;
    while (at < plan->payload_size) {
        size_t in_page = at % RTW_LEGACY_FW_PAGE_SIZE;
        size_t available = plan->payload_size - at;
        size_t left_on_page = RTW_LEGACY_FW_PAGE_SIZE - in_page;
        uint16_t chunk;
        uint8_t page = (uint8_t)(at / RTW_LEGACY_FW_PAGE_SIZE);
        if (available > left_on_page) available = left_on_page;
        if (available > RTW_USB_MAX_VENDOR_PAYLOAD)
            available = RTW_USB_MAX_VENDOR_PAYLOAD;
        chunk = (uint16_t)available;
        if (!callback(context, page,
              (uint16_t)(plan->firmware_window_register + in_page),
              plan->payload + at, chunk)) {
            if (count) *count = n;
            return RTW_LEGACY_FW_CANCELLED;
        }
        at += chunk;
        ++n;
    }
    if (count) *count = n;
    return RTW_LEGACY_FW_OK;
}

static int readreg(const rtw_usb_rw_transport *t,
                   uint16_t address, uint8_t width, uint32_t *value) {
    return rtw_usb_rw_read(t, address, width, value) == RTW_USB_OK;
}
static int writereg(const rtw_usb_rw_transport *t,
                    uint16_t address, uint8_t width, uint32_t value) {
    return rtw_usb_write_register(t, address, width, value) == RTW_USB_OK;
}
static int update(const rtw_usb_rw_transport *t, uint16_t address,
                  uint8_t width, uint32_t clear, uint32_t set) {
    uint32_t old = 0U;
    return readreg(t, address, width, &old) &&
           writereg(t, address, width, (old & ~clear) | set);
}
static int reset_8051(const rtw_usb_rw_transport *t) {
    /* Linux rtl8xxxu_reset_8051: reserved-control gate, then SYS_FUNC CPU. */
    return update(t, REG_RSV_CTRL_HI, 1U, 1U, 0U) &&
           update(t, REG_SYS_FUNC, 2U, CPU_ENABLE, 0U) &&
           update(t, REG_RSV_CTRL_HI, 1U, 0U, 1U) &&
           update(t, REG_SYS_FUNC, 2U, 0U, CPU_ENABLE);
}

typedef struct {
    const rtw_legacy_fw_plan *plan;
    const rtw_usb_rw_transport *bus;
    rtw_legacy_fw_report *report;
    uint8_t active_page;
} upload_sink;

static int write_chunk(void *context, uint8_t page, uint16_t address,
                       const uint8_t *bytes, uint16_t count) {
    upload_sink *sink = (upload_sink *)context;
    const uint16_t selector = (uint16_t)(sink->plan->mcu_download_register + 2U);
    if (page != sink->active_page) {
        if (!update(sink->bus, selector, 1U, 7U, page)) return 0;
        sink->active_page = page;
        sink->report->pages_written = (uint8_t)(page + 1U);
    }
    if (rtw_usb_write_block(sink->bus, address, bytes, count) != RTW_USB_OK)
        return 0;
    sink->report->bytes_written += count;
    ++sink->report->vendor_writes;
    return 1;
}
static rtw_legacy_fw_result poll_bit(
    const rtw_usb_rw_transport *t, uint16_t register_address,
    uint32_t mask, const rtw_waiter *waiter,
    const rtw_legacy_fw_options *options, rtw_legacy_fw_report *report,
    rtw_legacy_fw_result timeout) {
    uint16_t i;
    for (i = 0U; i < options->max_polls; ++i) {
        uint32_t status = 0U;
        if (!readreg(t, register_address, 4U, &status))
            return RTW_LEGACY_FW_IO_ERROR;
        report->last_mcu_status = status;
        if ((status & mask) != 0U) return RTW_LEGACY_FW_OK;
        if (i + 1U < options->max_polls &&
            !waiter->wait_us(waiter->context, options->delay_us))
            return RTW_LEGACY_FW_WAIT_FAILED;
    }
    return timeout;
}

rtw_legacy_fw_result rtw_legacy_fw_boot_experimental(
    const rtw_legacy_fw_plan *plan, const rtw_usb_rw_transport *transport,
    const rtw_waiter *waiter, const rtw_legacy_fw_options *options,
    rtw_legacy_fw_report *report) {
    rtw_legacy_fw_result result = RTW_LEGACY_FW_OK;
    upload_sink sink;
    uint32_t status = 0U;
    uint16_t mcu;
    int enabled = 0;
    if (report) memset(report, 0, sizeof(*report));
    if (!plan_valid(plan) || !transport || !transport->control_in ||
        !transport->control_out || !waiter || !waiter->wait_us ||
        !options || !report || !options->max_polls ||
        options->max_polls > 2000U || !options->delay_us ||
        options->delay_us > 10000U)
        return RTW_LEGACY_FW_BAD_ARGUMENT;
    if (!plan->boot_experimental_available)
        return RTW_LEGACY_FW_UNSUPPORTED;
    if (!options->confirm_unstable_hardware ||
        !options->exclusive_interface_claimed)
        return RTW_LEGACY_FW_PERMISSION_REQUIRED;
    mcu = plan->mcu_download_register;
    /* Refuse to overwrite an active 8051. Recovery requires unplug/replug. */
    if (!readreg(transport, mcu, 1U, &status))
        return RTW_LEGACY_FW_IO_ERROR;
    if (status & RAM_PRESENT) return RTW_LEGACY_FW_BUSY;
    /* Requires caller to have already completed board-specific power-on. */
    if (!update(transport, REG_SYS_FUNC + 1U, 1U, 0U, 4U) ||
        !update(transport, REG_SYS_FUNC, 2U, 0U, CPU_ENABLE))
        return RTW_LEGACY_FW_IO_ERROR;
    /* Select firmware RAM mode and reset bit19 before writing pages. */
    if (!update(transport, mcu, 1U, 0U, DOWNLOAD_ENABLE)) {
        result = RTW_LEGACY_FW_IO_ERROR;
        goto cleanup;
    }
    enabled = 1;
    if (!update(transport, mcu, 4U, 1U << 19U, 0U) ||
        !update(transport, mcu, 1U, 0U, CHECKSUM_REPORT) ||
        !readreg(transport, mcu, 1U, &status)) {
        result = RTW_LEGACY_FW_IO_ERROR;
        goto cleanup;
    }
    if (status & CHECKSUM_REPORT) {
        result = RTW_LEGACY_FW_BUSY;
        goto cleanup;
    }
    sink.plan = plan;
    sink.bus = transport;
    sink.report = report;
    sink.active_page = 0xffU;
    if (rtw_legacy_fw_walk(plan, write_chunk, &sink, NULL) !=
        RTW_LEGACY_FW_OK) {
        result = RTW_LEGACY_FW_IO_ERROR;
    }
cleanup:
    if (enabled) {
        if (!update(transport, mcu, 2U, DOWNLOAD_ENABLE, 0U))
            result = RTW_LEGACY_FW_IO_ERROR;
        else report->download_disabled = 1U;
    } else {
        /* An interrupted enable may still have modified the hardware. */
        (void)update(transport, mcu, 2U, DOWNLOAD_ENABLE, 0U);
    }
    if (result != RTW_LEGACY_FW_OK) return result;
    result = poll_bit(transport, mcu, CHECKSUM_REPORT, waiter, options,
                      report, RTW_LEGACY_FW_CHECKSUM_TIMEOUT);
    if (result != RTW_LEGACY_FW_OK) return result;
    report->checksum_seen = 1U;
    if (!update(transport, mcu, 4U, INIT_READY, DOWNLOAD_READY) ||
        !reset_8051(transport))
        return RTW_LEGACY_FW_IO_ERROR;
    result = poll_bit(transport, mcu, INIT_READY, waiter, options,
                      report, RTW_LEGACY_FW_READY_TIMEOUT);
    if (result == RTW_LEGACY_FW_OK) report->firmware_ready = 1U;
    return result;
}

const char *rtw_legacy_fw_status_name(rtw_legacy_fw_result s) {
    switch (s) {
    case RTW_LEGACY_FW_OK: return "ok";
    case RTW_LEGACY_FW_BAD_ARGUMENT: return "invalid argument or plan";
    case RTW_LEGACY_FW_UNSUPPORTED: return "firmware boot not implemented for chipset";
    case RTW_LEGACY_FW_BAD_IMAGE: return "firmware signature or payload invalid";
    case RTW_LEGACY_FW_IDENTITY_UNVERIFIED: return "USB identity not verified";
    case RTW_LEGACY_FW_CAPACITY_EXCEEDED: return "firmware exceeds 8-page window";
    case RTW_LEGACY_FW_CANCELLED: return "transfer callback cancelled";
    case RTW_LEGACY_FW_PERMISSION_REQUIRED: return "experimental mode and exclusive USB claim required";
    case RTW_LEGACY_FW_BUSY: return "8051 already running or checksum not reset";
    case RTW_LEGACY_FW_IO_ERROR: return "USB register or firmware write failed";
    case RTW_LEGACY_FW_CHECKSUM_TIMEOUT: return "firmware checksum timeout";
    case RTW_LEGACY_FW_READY_TIMEOUT: return "8051 firmware startup timeout";
    default: return "firmware poll wait failed";
    }
}
