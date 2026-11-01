/* SPDX-License-Identifier: BSD-3-Clause */
/* Compile the native PHY unchanged. These read-only test accessors expose
 * its private thermal inputs without adding an API to production code. */
#include "../src/RTL8814AUPHY.c"

unsigned rtw_test_8814_thermal_rate(const struct rtw_8814au *ctx)
{
    return ctx && ctx->phy ? ctx->phy->tx_rate : 255U;
}

int rtw_test_8814_thermal_delta(const struct rtw_8814au *ctx, unsigned path)
{
    return ctx && ctx->phy && path < 4U ?
        ctx->phy->dm_info.delta_power_index[path] : 127;
}
