#ifndef RTW_NET80211_HOST_PLATFORM_HPP
#define RTW_NET80211_HOST_PLATFORM_HPP

/* Host-only platform substitutes. The cipher, hash, packet format and key
 * structures are the unmodified imported implementation. No production target
 * may add this directory to its include search path. */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include "../host_endian.hpp"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#ifndef __packed
#define __packed __attribute__((packed))
#endif
#define _KERNEL
#define hweight8(value) __builtin_popcount((unsigned char)(value))
#define DIV_ROUND_UP(value, divisor) (((value) + (divisor) - 1) / (divisor))
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#define explicit_bzero(address, size) bzero((address), (size))

int timingsafe_bcmp(const void *, const void *, size_t);
inline void *malloc(size_t size, int, int) { return calloc(1, size); }

struct RTWHostMbuf {
    uint8_t bytes[4096];
    int length;
    int packet_length;
    RTWHostMbuf *next;
};
typedef RTWHostMbuf *mbuf_t;
enum { MBUF_DONTWAIT = 0, MBUF_EXT = 1 };
#undef MCLBYTES
#define MCLBYTES 4096
#define mtod(m, type) ((type)((m)->bytes))
inline int mbuf_type(mbuf_t) { return 1; }
inline int mbuf_get(int, int, mbuf_t *m) {
    *m = (mbuf_t)calloc(1, sizeof(RTWHostMbuf));
    return *m == nullptr ? ENOMEM : 0;
}
inline int mbuf_get_mhlen() { return 256; }
inline int mbuf_get_mlen() { return 256; }
inline int mbuf_get_minclsize() { return 256; }
inline int mbuf_flags(mbuf_t) { return MBUF_EXT; }
inline int mbuf_mclget(int, int, mbuf_t *) { return 0; }
inline int mbuf_len(mbuf_t m) { return m->length; }
inline void mbuf_setlen(mbuf_t m, int size) { m->length = size; }
inline int mbuf_pkthdr_len(mbuf_t m) { return m->packet_length; }
inline void mbuf_pkthdr_setlen(mbuf_t m, int size) { m->packet_length = size; }
inline int m_dup_pkthdr(mbuf_t dst, mbuf_t src, int) {
    dst->packet_length = src->packet_length;
    return 0;
}
inline mbuf_t mbuf_next(mbuf_t m) { return m->next; }
inline void mbuf_setnext(mbuf_t m, mbuf_t next) { m->next = next; }
inline int mbuf_trailingspace(mbuf_t m) { return int(sizeof(m->bytes)) - m->length; }
inline void mbuf_freem(mbuf_t m) {
    while (m != nullptr) {
        mbuf_t next = m->next;
        free(m);
        m = next;
    }
}
inline void mbuf_copydata(mbuf_t m, int offset, int count, void *output) {
    auto *dst = (uint8_t *)output;
    while (m && offset >= m->length) { offset -= m->length; m = m->next; }
    while (m && count > 0) {
        int copied = min(m->length - offset, count);
        memcpy(dst, m->bytes + offset, copied);
        count -= copied;
        dst += copied;
        m = m->next;
        offset = 0;
    }
    if (count != 0) abort();
}

#endif
