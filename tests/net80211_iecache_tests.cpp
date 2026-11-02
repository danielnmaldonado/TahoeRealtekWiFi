#include "net80211_iecache_host.hpp"
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>

static std::map<void *, size_t> allocations;
static bool fail_allocation;
static unsigned allocation_attempts;
static unsigned release_count;

static void require(bool condition, const char *message)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

void *iecache_allocate(size_t length, int, int)
{
    ++allocation_attempts;
    if (fail_allocation)
        return nullptr;
    void *p = std::malloc(length ? length : 1);
    require(p != nullptr, "host allocator failed");
    allocations.emplace(p, length);
    return p;
}

void iecache_release(void *p)
{
    require(allocations.erase(p) == 1, "cache releases each owned allocation once");
    ++release_count;
    std::free(p);
}

static void clear(iecache_node &node)
{
    require(ieee80211_save_ie_tlv(nullptr, &node.ni_rsnie_tlv,
        &node.ni_rsnie_tlv_len, 0) == 0, "empty cache update succeeds");
    require(node.ni_rsnie_tlv == nullptr && node.ni_rsnie_tlv_len == 0,
        "empty update leaves a null pointer and zero length");
}

static void check_bytes(const iecache_node &node, const char *bytes, uint32_t length)
{
    require(node.ni_rsnie_tlv_len == length, "cache length equals the accepted span");
    require(node.ni_rsnie_tlv != nullptr, "nonempty cache owns storage");
    require(std::memcmp(node.ni_rsnie_tlv, bytes, length) == 0,
        "cache retains the exact accepted bytes");
}

static void empty_update_does_not_allocate()
{
    iecache_node node;
    unsigned before = allocation_attempts;
    fail_allocation = true;
    clear(node);
    clear(node);
    fail_allocation = false;
    require(allocation_attempts == before, "empty cache never asks the allocator for zero bytes");
}

static void replacement_is_transactional()
{
    iecache_node node;
    require(ieee80211_save_ie_tlv((const uint8_t *)"old", &node.ni_rsnie_tlv,
        &node.ni_rsnie_tlv_len, 3) == 0, "initial cache succeeds");
    uint8_t *old = node.ni_rsnie_tlv;
    unsigned released = release_count;
    fail_allocation = true;
    require(ieee80211_save_ie_tlv((const uint8_t *)"replacement", &node.ni_rsnie_tlv,
        &node.ni_rsnie_tlv_len, 11) == ENOMEM, "replacement reports allocator failure");
    fail_allocation = false;
    require(node.ni_rsnie_tlv == old && release_count == released,
        "failed replacement preserves the previous owner");
    check_bytes(node, "old", 3);
    require(ieee80211_save_ie_tlv((const uint8_t *)"replacement", &node.ni_rsnie_tlv,
        &node.ni_rsnie_tlv_len, 11) == 0, "replacement retry succeeds");
    require(release_count == released + 1, "successful replacement releases the old cache");
    check_bytes(node, "replacement", 11);
    clear(node);
}

static void equal_length_update_reuses_storage()
{
    iecache_node node;
    require(ieee80211_save_ie_tlv((const uint8_t *)"one", &node.ni_rsnie_tlv,
        &node.ni_rsnie_tlv_len, 3) == 0, "initial equal-length cache succeeds");
    uint8_t *old = node.ni_rsnie_tlv;
    unsigned attempts = allocation_attempts;
    fail_allocation = true;
    require(ieee80211_save_ie_tlv((const uint8_t *)"two", &node.ni_rsnie_tlv,
        &node.ni_rsnie_tlv_len, 3) == 0, "equal-length refresh needs no allocation");
    fail_allocation = false;
    require(node.ni_rsnie_tlv == old && allocation_attempts == attempts,
        "equal-length refresh keeps its allocation");
    check_bytes(node, "two", 3);
    clear(node);
}

static void initial_failure_remains_empty()
{
    iecache_node node;
    fail_allocation = true;
    require(ieee80211_save_ie_tlv((const uint8_t *)"data", &node.ni_rsnie_tlv,
        &node.ni_rsnie_tlv_len, 4) == ENOMEM, "initial failure is reported");
    fail_allocation = false;
    require(node.ni_rsnie_tlv == nullptr && node.ni_rsnie_tlv_len == 0,
        "initial failure preserves the empty cache");
}

static void caller_uses_the_exact_valid_span()
{
    // A generic byte store with a 36-byte metadata prefix and five accepted
    // data bytes. Extra padding is allocated but excluded from valid_length,
    // so a wrong span fails by length without reading outside local storage.
    const uint8_t storage[] = "012345678901234567890123456789012345CACHEpadding";
    iecache_node node;
    actual_iecache_update(storage, 41, &node);
    check_bytes(node, "CACHE", 5);
    clear(node);
}

static void caller_discards_only_optional_cache_on_failure()
{
    const uint8_t storage[] = "012345678901234567890123456789012345CACHEpadding";
    iecache_node node;
    require(ieee80211_save_ie_tlv((const uint8_t *)"old", &node.ni_rsnie_tlv,
        &node.ni_rsnie_tlv_len, 3) == 0, "caller has a prior cache");
    fail_allocation = true;
    actual_iecache_update(storage, 41, &node);
    fail_allocation = false;
    require(node.ni_rsnie_tlv == nullptr && node.ni_rsnie_tlv_len == 0,
        "caller clears stale optional bytes after cache failure");
}

int main(int argc, char **argv)
{
    const std::string selected = argc > 1 ? argv[1] : "all";
    unsigned count = 0;
    const auto run = [&](const char *name, void (*test)()) {
        if (selected == "all" || selected == name) {
            test();
            require(allocations.empty(), "test releases all local cache allocations");
            ++count;
        }
    };
    run("empty", empty_update_does_not_allocate);
    run("replace", replacement_is_transactional);
    run("reuse", equal_length_update_reuses_storage);
    run("initial-failure", initial_failure_remains_empty);
    run("span", caller_uses_the_exact_valid_span);
    run("caller-failure", caller_discards_only_optional_cache_on_failure);
    require(count != 0, "at least one named memory test ran");
    std::printf("%u local IE-cache memory tests passed; no receive or device operations.\n", count);
}
