#ifndef RTW_PAE_HOST_PLATFORM_HPP
#define RTW_PAE_HOST_PLATFORM_HPP
/* Host-only KPI model. Crypto and EAPOL processing remain upstream code. */
#include <vector>
#include <algorithm>
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
#include <stdarg.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#define _KERNEL
#define __packed __attribute__((packed))
#define hweight8(value) __builtin_popcount((unsigned char)(value))
#define DIV_ROUND_UP(a, b) (((a) + (b) - 1) / (b))
#define nitems(a) (sizeof(a) / sizeof((a)[0]))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define explicit_bzero(address, size) bzero((address), (size))
#define XYLog(...) ((void)0)
#define _KASSERT(condition) do { if (!(condition)) abort(); } while (0)
#define IEEE80211_ADDR_COPY(dst, src) memcpy((dst), (src), 6)
#define IEEE80211_ADDR_EQ(a, b) (memcmp((a), (b), 6) == 0)
#define IEEE80211_IS_MULTICAST(a) (((const uint8_t *)(a))[0] & 1)
#define LINK_STATE_UP 4
#define LINK_STATE_DOWN 2
class CTimeout {};
struct mbuf_queue {};
struct RTWPAEMbuf {
    uint8_t storage[4096];
    size_t offset, length, packet_length;
    RTWPAEMbuf *next;
};
typedef RTWPAEMbuf *mbuf_t;
extern unsigned rtw_pae_invalid_setdata;
extern unsigned rtw_pae_fail_allocations;
void rtw_pae_random(void *, size_t);
#define arc4random_buf rtw_pae_random
#define MBUF_DONTWAIT 0
#define MBUF_EXT 1
#define MT_DATA 1
#undef MCLBYTES
#define MCLBYTES 4096
#define mtod(m, type) ((type)mbuf_data(m))
inline void *mbuf_data(mbuf_t m) { return m->storage + m->offset; }
inline unsigned mbuf_len(mbuf_t m) { return unsigned(m->length); }
inline unsigned mbuf_pkthdr_len(mbuf_t m) { return unsigned(m->packet_length); }
inline void mbuf_setlen(mbuf_t m, size_t n) { if (n > sizeof(m->storage)-m->offset) abort(); m->length=n; }
inline void mbuf_pkthdr_setlen(mbuf_t m, size_t n) { m->packet_length=n; }
inline int mbuf_type(mbuf_t) { return MT_DATA; }
inline int mbuf_flags(mbuf_t) { return MBUF_EXT; }
inline unsigned mbuf_get_mhlen() { return 256; }
inline int mbuf_gethdr(int, int, mbuf_t *out) {
    if (rtw_pae_fail_allocations) { --rtw_pae_fail_allocations; *out=nullptr; return ENOMEM; }
    *out=(mbuf_t)calloc(1,sizeof(RTWPAEMbuf));
    return *out ? 0 : ENOMEM;
}
inline int mbuf_mclget(int, int, mbuf_t *) { return 0; }
inline mbuf_t mbuf_free(mbuf_t m) { mbuf_t next=m->next; free(m); return next; }
inline void mbuf_freem(mbuf_t m) { while(m) m=mbuf_free(m); }
inline int mbuf_setdata(mbuf_t m, void *data, size_t n) {
    auto *p=(uint8_t *)data;
    if (p<m->storage || p>m->storage+sizeof(m->storage) || n>size_t(m->storage+sizeof(m->storage)-p)) {
        ++rtw_pae_invalid_setdata; return EINVAL;
    }
    m->offset=size_t(p-m->storage); m->length=n; return 0;
}
inline void mbuf_copydata(mbuf_t m, size_t offset, size_t n, void *out) {
    auto *dst=(uint8_t *)out;
    while(m && offset>=m->length) { offset-=m->length; m=m->next; }
    while(m && n) {
        size_t copied=min(m->length-offset,n);
        memcpy(dst,(uint8_t *)mbuf_data(m)+offset,copied);
        dst+=copied; n-=copied; offset=0; m=m->next;
    }
    if(n) abort();
}
inline int mbuf_prepend(mbuf_t *packet,size_t n,int) {
    mbuf_t m=*packet;
    if(m->offset>=n) { m->offset-=n; m->length+=n; m->packet_length+=n; return 0; }
    mbuf_t head=nullptr;
    if(mbuf_gethdr(0,0,&head)) { mbuf_freem(m); *packet=nullptr; return ENOMEM; }
    head->length=n; head->packet_length=m->packet_length+n; head->next=m;
    *packet=head; return 0;
}
inline int mbuf_pullup(mbuf_t *packet,size_t n) {
    mbuf_t m=*packet;
    if(m->length>=n) return 0;
    if(n>m->packet_length) { mbuf_freem(m); *packet=nullptr; return ENOBUFS; }
    mbuf_t flat=nullptr;
    if(mbuf_gethdr(0,0,&flat)) return ENOMEM;
    flat->length=flat->packet_length=m->packet_length;
    mbuf_copydata(m,0,m->packet_length,flat->storage);
    mbuf_freem(m); *packet=flat; return 0;
}
inline void mbuf_adj(mbuf_t m,int n) {
    if(n<0 || size_t(n)>m->length) abort();
    m->offset+=n; m->length-=n; m->packet_length-=n;
}
struct _ifqueue { std::vector<mbuf_t> *packets; };
inline int ifq_enqueue(_ifqueue *queue,mbuf_t m) { queue->packets->push_back(m); return 0; }
struct _ifnet {
    unsigned if_flags;
    unsigned if_ibytes,if_imcasts;
    char if_xname[16];
    _ifqueue if_snd;
    void (*if_start)(_ifnet *);
};
void *rtw_pae_allocate(size_t);
void rtw_pae_deallocate(void *);
inline void *malloc(size_t n,int,int) { return rtw_pae_allocate(n); }
#define free rtw_pae_deallocate
int timingsafe_bcmp(const void *,const void *,size_t);
inline void panic(const char *,...) { abort(); }
inline char *ether_sprintf(const uint8_t *) { static char name[]="test"; return name; }
inline int splnet() { return 0; }
inline void splx(int) {}
inline int timeout_del(CTimeout **) { return 0; }
inline int timeout_free(CTimeout **) { return 0; }
inline int timeout_pending(CTimeout **) { return 0; }
#endif
