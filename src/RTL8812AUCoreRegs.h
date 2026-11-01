/* SPDX-License-Identifier: BSD-3-Clause */
/* Private AU register vocabulary; pinned Linuxrtw88 definitions. */
#ifndef RTW_R12AU_CORE_REGS_H
#define RTW_R12AU_CORE_REGS_H
#include <stdint.h>
#define BIT(n) (UINT32_C(1)<<(n))
#define GENMASK(h,l) ((UINT32_MAX<<(l))&(UINT32_MAX>>(31U-(h))))
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#include "RTL8812AURegisters.inc"
#endif
