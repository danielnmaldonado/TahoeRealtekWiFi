// Local buffer/allocator boundary only; no network receive or frame parser.
#pragma once
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <sys/types.h>

void *iecache_allocate(size_t, int, int);
void iecache_release(void *);
int ieee80211_save_ie_tlv(const uint8_t *, uint8_t **, uint32_t *, uint32_t);

struct iecache_node {
    uint8_t *ni_rsnie_tlv = nullptr;
    uint32_t ni_rsnie_tlv_len = 0;
};

// These stand in only for the arithmetic operands in the extracted cache
// statements. The tests never interpret the bytes as a wireless frame.
struct iecache_prefix { uint8_t bytes[24]; };
struct iecache_buffer { const uint8_t *data; size_t length; };
inline size_t mbuf_len(const iecache_buffer *m) { return m->length; }
#define mtod(m, type) ((type)((m)->data))
void actual_iecache_update(const uint8_t *, size_t, iecache_node *);
