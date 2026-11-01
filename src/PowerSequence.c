/* SPDX-License-Identifier: GPL-2.0-only
   RTL8812AU interoperability; implementation based on vendor sequences and
   GPL-2.0 reference xen-proc/rtl8812au-macos and aircrack-ng/rtl8812au.
   This does NOT attest to validation on the user's physical hardware. */
#include "rtw/PowerSequence.h"
#include "rtw/Chip.h"

/* Preflight all steps before making the first hardware write. */
static int valid_sequence(const rtw_power_step* steps, size_t count) {
    size_t i;
    if (!steps || count < 2 || count > 128 ||
        steps[count-1].opcode != RTW_PWR_END) return 0;
    for (i=0;i<count;++i) {
        const rtw_power_step* s=&steps[i];
        if (s->opcode == RTW_PWR_END) return i==count-1;
        if (s->opcode == RTW_PWR_DELAY_US) {
            if (!s->address_or_delay) return 0;
            continue;
        }
        if (s->opcode != RTW_PWR_WRITE8 && s->opcode != RTW_PWR_POLL8) return 0;
        if (!s->mask || s->address_or_delay >= 0xF000U) return 0;
    }
    return 0;
}
rtw_power_error rtw_power_execute(const rtw_usb_rw_transport* t,
                                  const rtw_power_step* steps, size_t count,
                                  uint32_t max_polls, const rtw_waiter* waiter,
                                  size_t* executed) {
    size_t i;
    if (executed) *executed=0;
    if (!t || !t->control_in || !t->control_out || !waiter || !waiter->wait_us ||
        !max_polls || max_polls>5000U || !valid_sequence(steps,count))
        return RTW_POWER_BAD_ARGUMENT;
    for (i=0;i<count-1;++i) {
        const rtw_power_step* s=steps+i;
        uint32_t old=0;
        if (s->opcode==RTW_PWR_DELAY_US) {
            if (!waiter->wait_us(waiter->context,s->address_or_delay))
                return RTW_POWER_DELAY_FAILED;
        } else if (s->opcode==RTW_PWR_WRITE8) {
            if (rtw_usb_rw_read(t,s->address_or_delay,1,&old)!=RTW_USB_OK ||
                rtw_usb_write_register(t,s->address_or_delay,1,
                    (old & ~(uint32_t)s->mask) | ((uint32_t)s->value & s->mask))!=RTW_USB_OK)
                return RTW_POWER_USB_ERROR;
        } else {
            uint32_t j;
            for(j=0;j<max_polls;++j) {
                if (rtw_usb_rw_read(t,s->address_or_delay,1,&old)!=RTW_USB_OK)
                    return RTW_POWER_USB_ERROR;
                if ((old & s->mask)==(s->value & s->mask)) break;
                if (j+1<max_polls && !waiter->wait_us(waiter->context,1000U))
                    return RTW_POWER_DELAY_FAILED;
            }
            if (j==max_polls) return RTW_POWER_POLL_TIMEOUT;
        }
        if (executed) *executed=i+1U;
    }
    return RTW_POWER_OK;
}

/* rtl8812_power_on_flow for cut B, TSMC, USB; originated from vendor flow
   as documented by xen-proc/rtl8812au-macos (GPL-2.0). */
static const rtw_power_step kPowerOnB[] = {
    {0x0005,RTW_PWR_WRITE8,0x04,0x00},
    {0x0006,RTW_PWR_POLL8,0x02,0x02},
    {0x0005,RTW_PWR_WRITE8,0x08,0x00},
    {0x0005,RTW_PWR_WRITE8,0x01,0x01},
    {0x0005,RTW_PWR_POLL8,0x01,0x00},
    {0x0024,RTW_PWR_WRITE8,0x02,0x00},
    {0x0028,RTW_PWR_WRITE8,0x08,0x00},
    {0,RTW_PWR_END,0,0}
};
rtw_power_error rtw_power_on_8812au_bcut_tsmc_usb(
    const rtw_usb_rw_transport* t, const rtw_waiter* waiter,
    uint32_t sys_cfg, size_t* executed) {
    rtw_chip_info chip;
    if (executed) *executed=0;
    if (!rtw_chip_decode_syscfg(sys_cfg,&chip) || chip.cut!=1 ||
        chip.is_umc || chip.is_test_chip || !chip.is_usb)
        return RTW_POWER_UNSUPPORTED;
    return rtw_power_execute(t,kPowerOnB,sizeof(kPowerOnB)/sizeof(kPowerOnB[0]),
                             1000,waiter,executed);
}
const char* rtw_power_error_string(rtw_power_error e) {
    switch(e) {
        case RTW_POWER_OK:return "ok";
        case RTW_POWER_BAD_ARGUMENT:return "invalid power sequence";
        case RTW_POWER_UNSUPPORTED:return "unsupported cut/foundry/USB profile";
        case RTW_POWER_USB_ERROR:return "USB register I/O failed";
        case RTW_POWER_POLL_TIMEOUT:return "power polling timed out";
        case RTW_POWER_DELAY_FAILED:return "power delay failed";
        default:return "unknown";
    }
}
