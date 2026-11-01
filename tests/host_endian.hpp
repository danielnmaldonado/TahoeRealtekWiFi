// SPDX-License-Identifier: GPL-2.0-only
#pragma once
// Host-test compatibility only. Production uses the real macOS kernel headers.
#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#else
#include <endian.h>
#include <stdint.h>
static inline uint16_t _OSSwapInt16(uint16_t value) { return __builtin_bswap16(value); }
static inline uint32_t _OSSwapInt32(uint32_t value) { return __builtin_bswap32(value); }
static inline uint64_t _OSSwapInt64(uint64_t value) { return __builtin_bswap64(value); }
#ifndef ETHERTYPE_PAE
#define ETHERTYPE_PAE 0x888e
#endif
static inline uint16_t OSSwapHostToLittleInt16(uint16_t value) { return htole16(value); }
static inline uint16_t OSSwapLittleToHostInt16(uint16_t value) { return le16toh(value); }
static inline uint32_t OSSwapHostToLittleInt32(uint32_t value) { return htole32(value); }
static inline uint32_t OSSwapLittleToHostInt32(uint32_t value) { return le32toh(value); }
static inline uint64_t OSSwapHostToLittleInt64(uint64_t value) { return htole64(value); }
static inline uint64_t OSSwapLittleToHostInt64(uint64_t value) { return le64toh(value); }
static inline uint16_t OSSwapHostToBigInt16(uint16_t value) { return htobe16(value); }
static inline uint16_t OSSwapBigToHostInt16(uint16_t value) { return be16toh(value); }
static inline uint32_t OSSwapHostToBigInt32(uint32_t value) { return htobe32(value); }
static inline uint32_t OSSwapBigToHostInt32(uint32_t value) { return be32toh(value); }
static inline uint64_t OSSwapHostToBigInt64(uint64_t value) { return htobe64(value); }
static inline uint64_t OSSwapBigToHostInt64(uint64_t value) { return be64toh(value); }
#endif
