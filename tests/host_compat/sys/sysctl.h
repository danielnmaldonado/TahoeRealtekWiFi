// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#ifdef __APPLE__
#include_next <sys/sysctl.h>
#else
// The selected host crypto units do not call sysctl or expose kernel tunables.
#include <sys/types.h>
#endif
