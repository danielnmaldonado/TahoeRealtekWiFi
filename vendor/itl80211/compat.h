/*
 * Copyright (C) 2020 钟先耀
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
/* Derived from itlwm/itl80211/compat.h. The PCI/DMA HAL is intentionally absent;
 * only the common device container needed by the USB rtwn port is retained. */
#ifndef RTW_NET80211_COMPAT_H
#define RTW_NET80211_COMPAT_H

#include "RTWNet80211Prefix.hpp"
#include <IOKit/IOService.h>
#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_amrr.h>

struct device {
    IOService *provider;
    void *_data;
    char dv_xname[16];
};

#endif
