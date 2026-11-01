/* SPDX-License-Identifier: BSD-2-Clause */
#include "rtw/RTL8812AURadio.h"
#include <stddef.h>
#include <string.h>

/* Standards-defined 5-GHz channel groups. DFS groups are passive-only. */
static const uint8_t groups[][4] = {
    {36, 40, 44, 48},
    {52, 56, 60, 64},
    {100, 104, 108, 112},
    {116, 120, 124, 128},
    {132, 136, 140, 144},
    {149, 153, 157, 161}
};

static int dfs_channel(unsigned ch) {
    return ch >= 52U && ch <= 144U;
}
static int is_5g_channel(unsigned ch) {
    size_t i, j;
    if (ch == 165U) return 1;
    for (i = 0; i < sizeof(groups) / sizeof(groups[0]); ++i)
        for (j = 0; j < 4U; ++j)
            if (groups[i][j] == ch) return 1;
    return 0;
}
rtw_r12au_channel_error rtw_r12au_channel_prepare(
    const rtw_r12au_channel_request *r, rtw_r12au_channel_plan *out) {
    rtw_r12au_channel_plan next;
    unsigned center = 0U;
    size_t group_index, item_index;
    int found = 0;
    if (out) memset(out, 0, sizeof(*out));
    if (!r || !out) return RTW_R12AU_CHANNEL_INVALID_ARGUMENT;
    if (r->chip != RTW_R12AU_CHIP_8812AU &&
        r->chip != RTW_R12AU_CHIP_8821AU)
        return RTW_R12AU_CHANNEL_UNSUPPORTED;
    if (r->width_mhz != 20U && r->width_mhz != 40U &&
        r->width_mhz != 80U)
        return RTW_R12AU_CHANNEL_UNSUPPORTED;
    memset(&next, 0, sizeof(next));
    if (r->primary_channel >= 1U && r->primary_channel <= 11U) {
        if (r->width_mhz != 20U ||
            r->center_channel != r->primary_channel)
            return RTW_R12AU_CHANNEL_UNSUPPORTED;
        next.primary_frequency_mhz =
            (uint16_t)(2407U + 5U * r->primary_channel);
        next.center_frequency_mhz = next.primary_frequency_mhz;
    } else {
        if (!is_5g_channel(r->primary_channel))
            return RTW_R12AU_CHANNEL_UNSUPPORTED;
        for (group_index = 0U;
             group_index < sizeof(groups) / sizeof(groups[0]);
             ++group_index) {
            for (item_index = 0U; item_index < 4U; ++item_index) {
                if (groups[group_index][item_index] == r->primary_channel) {
                    found = 1;
                    if (r->width_mhz == 20U)
                        center = r->primary_channel;
                    else if (r->width_mhz == 40U)
                        center = (unsigned)groups[group_index]
                            [(item_index / 2U) * 2U] + 2U;
                    else
                        center = (unsigned)groups[group_index][0] + 6U;
                    break;
                }
            }
            if (found) break;
        }
        if (r->primary_channel == 165U) {
            if (r->width_mhz != 20U)
                return RTW_R12AU_CHANNEL_UNSUPPORTED;
            center = 165U;
        }
        if (center != r->center_channel)
            return RTW_R12AU_CHANNEL_UNSUPPORTED;
        next.primary_frequency_mhz =
            (uint16_t)(5000U + 5U * r->primary_channel);
        next.center_frequency_mhz = (uint16_t)(5000U + 5U * center);
        next.is_5ghz = 1U;
        next.dfs_required = (uint8_t)dfs_channel(r->primary_channel);
    }
    next.primary_channel = r->primary_channel;
    next.center_channel = r->center_channel;
    next.width_mhz = r->width_mhz;
    next.rf_chains = (uint8_t)(r->chip == RTW_R12AU_CHIP_8812AU ? 2U : 1U);
    next.passive_only = 1U;
    *out = next;
    return RTW_R12AU_CHANNEL_OK;
}
