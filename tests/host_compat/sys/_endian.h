// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#ifdef __APPLE__
#include_next <sys/_endian.h>
#else
#include <endian.h>
#include <sys/endian.h>
#endif
