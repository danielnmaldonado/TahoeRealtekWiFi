#include "rtw/Chip.h"
#include <string.h>

int rtw_chip_decode_syscfg(uint32_t sys_cfg, rtw_chip_info* result) {
    if (!result) return 0;
    memset(result, 0, sizeof(*result));
    if (!sys_cfg || sys_cfg == UINT32_MAX) return 0;
    result->sys_cfg = sys_cfg;
    result->cut = (uint8_t)((sys_cfg >> 12U) & 0x0FU);
    result->is_test_chip = (uint8_t)((sys_cfg >> 23U) & 1U);
    result->is_umc = (uint8_t)((sys_cfg >> 19U) & 1U);
    result->is_usb = (uint8_t)((sys_cfg >> 26U) & 1U);
    return 1;
}
