#ifndef RTW_INPUT_HOST_PLATFORM_HPP
#define RTW_INPUT_HOST_PLATFORM_HPP

/* Host substitutes for the external mbuf/IOKit boundary. The real imported
 * _mbuf.cpp is compiled into the test executable, never loaded into a kernel. */
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "../../vendor/itl80211/openbsd/sys/_input_diagnostics.h"

using IOReturn = int;
constexpr IOReturn kIOReturnSuccess = 0;
constexpr IOReturn kIOReturnIOError = -1;
constexpr IOReturn kIOReturnNotReady = -2;
class OSObject {};
struct RTWInputMbuf {
    unsigned id;
    unsigned packet_length;
    RTWInputMbuf *nextpkt;
};
using mbuf_t = RTWInputMbuf *;
inline mbuf_t mbuf_nextpkt(mbuf_t m) { return m->nextpkt; }
inline void mbuf_setnextpkt(mbuf_t m, mbuf_t next) { m->nextpkt = next; }
inline unsigned mbuf_pkthdr_len(mbuf_t m) { return m->packet_length; }
inline unsigned mbuf_len(mbuf_t m) { return m->packet_length; }
struct mbuf_list { mbuf_t ml_head, ml_tail; unsigned ml_len; };
#define MBUF_LIST_INITIALIZER() { nullptr, nullptr, 0 }
#define MBUF_LIST_FOREACH(ml, m) \
    for ((m) = (ml)->ml_head; (m); (m) = mbuf_nextpkt(m))
inline void ml_enqueue(mbuf_list *ml, mbuf_t m) {
    if (ml->ml_tail) mbuf_setnextpkt(ml->ml_tail, m);
    else ml->ml_head = m;
    ml->ml_tail = m;
    mbuf_setnextpkt(m, nullptr);
    ++ml->ml_len;
}
inline mbuf_t ml_dequeue(mbuf_list *ml) {
    mbuf_t m = ml->ml_head;
    if (m) {
        ml->ml_head = mbuf_nextpkt(m);
        if (!ml->ml_head) ml->ml_tail = nullptr;
        mbuf_setnextpkt(m, nullptr);
        --ml->ml_len;
    }
    return m;
}
struct IONetworkStats { unsigned inputPackets = 0; };
class IONetworkInterface {
public:
    enum { kInputOptionQueuePacket = 1 };
    std::vector<unsigned> received, lengths;
    std::vector<mbuf_t> queue;
    unsigned chain_violations = 0, flushes = 0;
    uint64_t bsd_packets = 0, bsd_bytes = 0;
    unsigned pending_packets = 0, pending_bytes = 0, freed = 0;
    unsigned flush_shortfall = 0;
    bool attached = true;
    bool reject_in_ifnet = false;
    bool consume_immediately = false;
    void *getIfnet() const { return attached ? (void *)this : nullptr; }
    unsigned inputPacket(mbuf_t m, unsigned length, unsigned options) {
        // Apple's implementation asserts nextpkt == 0 before accepting a
        // packet. Ownership transfers here, including when it frees a packet
        // because the BSD interface has already detached.
        if (mbuf_nextpkt(m)) ++chain_violations;
        if (length != 0 || options != kInputOptionQueuePacket) std::abort();
        received.push_back(m->id);
        lengths.push_back(m->packet_length);
        mbuf_setnextpkt(m, nullptr);
        if (!attached) { ++freed; delete m; return 1; }
        ++pending_packets;
        pending_bytes += m->packet_length;
        if (consume_immediately) { ++freed; delete m; }
        else queue.push_back(m);
        return 0;
    }
    unsigned flushInputQueue() {
        ++flushes;
        unsigned count = pending_packets;
        if (!reject_in_ifnet) {
            bsd_packets += count;
            bsd_bytes += pending_bytes;
        }
        pending_packets = pending_bytes = 0;
        for (auto *m : queue) { ++freed; delete m; }
        queue.clear();
        return count > flush_shortfall ? count - flush_shortfall : 0;
    }
};
struct _ifnet {
    IONetworkInterface *iface;
    IONetworkStats *netStat;
    rtw_if_input_diagnostics if_input_diag{};
};
class IOCommandGate {
public:
    using Action = IOReturn (*)(OSObject *, void *, void *, void *, void *);
    IOReturn runAction(Action action, void *a0, void *a1) {
        return action(nullptr, a0, a1, nullptr, nullptr);
    }
};
#define XYLog(...) ((void)0)
#define panic(...) std::abort()
int if_input(_ifnet *, mbuf_list *);

#endif
