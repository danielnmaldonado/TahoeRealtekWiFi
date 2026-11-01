// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#ifdef __APPLE__
#include_next <sys/sockio.h>
#else
#include <sys/ioctl.h>
#endif
