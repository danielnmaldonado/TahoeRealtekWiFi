/* SPDX-License-Identifier: GPL-2.0-only
 * MaLd0n RTL8814B portable HALMAC boundary. The enclosing owner serializes all
 * calls; vendor mutex callbacks are therefore local serialization assertions.
 * USB only, little endian, exact B14 generation1. No Linux/router OS headers.
 */
#ifndef RTW_B14_PLATFORM_H
#define RTW_B14_PLATFORM_H
#include <stdint.h>
#include <stddef.h>
#define _HALMAC_2_PLATFORM_H_
#define __HALMAC__HW_CFG_H__
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef uint64_t u64;
typedef int8_t s8;typedef int16_t s16;typedef int32_t s32;typedef int64_t s64;
typedef u8* pu8;typedef u16* pu16;typedef u32* pu32;typedef s8* ps8;typedef s16* ps16;typedef s32* ps32;
typedef u16 __le16;typedef u32 __le32;typedef u16 __be16;typedef u32 __be32;
typedef unsigned HALMAC_MUTEX;
#define _TRUE 1
#define _FALSE 0
#define HALMAC_INLINE __inline
#define HALMAC_PLATFORM_LITTLE_ENDIAN 1
#define HALMAC_PLATFORM_BIG_ENDIAN 0
#define HALMAC_SYSTEM_ENDIAN 1
#define HALMAC_PLATFORM_WINDOWS 0
#define HALMAC_PLATFORM_LINUX 0
#define HALMAC_PLATFORM_AP 1
#define HALMAC_PLATFORM_TESTPROGRAM 0
#define HALMAC_DBG_MSG_ENABLE 0
#define HALMAC_USB_SUPPORT 1
#define HALMAC_PCIE_SUPPORT 0
#define HALMAC_SDIO_SUPPORT 0
#define HALMAC_8814B_SUPPORT 1
#define HALMAC_8822B_SUPPORT 0
#define HALMAC_8822C_SUPPORT 0
#define HALMAC_8821C_SUPPORT 0
#define HALMAC_8812F_SUPPORT 0
#define HALMAC_8197F_SUPPORT 0
/* Field masks are hardware unsigned32 values. The upstream default shifts
 * signed integer literals into bit31; keep its exact bits without C UB. */
#define SET_MEM_OP(dw,v,m,s) (((u32)(dw)&~((u32)(m)<<(s)))|(((u32)(v)&(u32)(m))<<(s)))
#define HALMAC_SET_DESC_FIELD_NO_CLR(dw,v,m,s) ((dw)|=rtk_cpu_to_le32(((u32)(v)&(u32)(m))<<(s)))
#ifndef BIT
#define BIT(n) (UINT32_C(1) << (n))
#endif
#ifndef __packed
#define __packed __attribute__((packed))
#endif
#ifndef __maybe_unused
#define __maybe_unused __attribute__((unused))
#endif
#endif
