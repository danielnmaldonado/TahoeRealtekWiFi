/* Exercise the actual imported Ethernet handoff with an external KPI model. */
#include "net80211_input_host/platform.hpp"

static IOCommandGate gate;
IOCommandGate *_fCommandGate = &gate;
static unsigned checks;
static void require(bool value, const char *description) {
    ++checks;
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", description); std::exit(1); }
}
static void multiple_packets_transfer_as_isolated_mbufs(bool consume_immediately) {
    IONetworkInterface iface;
    iface.consume_immediately = consume_immediately;
    IONetworkStats stats;
    _ifnet ifp { &iface, &stats };
    mbuf_list list = MBUF_LIST_INITIALIZER();
    ml_enqueue(&list, new RTWInputMbuf { 11, 60, nullptr });
    ml_enqueue(&list, new RTWInputMbuf { 22, 1514, nullptr });
    ml_enqueue(&list, new RTWInputMbuf { 33, 98, nullptr });
    require(if_input(&ifp, &list) == kIOReturnSuccess, "input action succeeds");
    require(iface.chain_violations == 0, "inputPacket receives no linked sibling");
    require(iface.received == std::vector<unsigned>({11, 22, 33}),
            "all aggregate packets transfer once in order");
    require(iface.lengths == std::vector<unsigned>({60, 1514, 98}),
            "packet lengths survive the handoff");
    require(list.ml_head == nullptr && list.ml_tail == nullptr && list.ml_len == 0,
            "consumed source list owns no packets");
    require(stats.inputPackets == 3, "all Ethernet packets are counted");
    require(iface.flushes == 1, "one aggregate needs one input queue flush");
    require(iface.bsd_packets == 3 && iface.bsd_bytes == 1672,
            "native queue alone accounts each packet and complete Ethernet byte once");
    require(iface.freed == 3, "native interface owns and frees each transferred packet once");
    require(ifp.if_input_diag.offeredPackets == 3 && ifp.if_input_diag.offeredBytes == 1672 &&
            ifp.if_input_diag.immediateReturns == 0 && ifp.if_input_diag.flushPackets == 3 &&
            ifp.if_input_diag.flushCalls == 1 && ifp.if_input_diag.submissionMismatches == 0,
            "normal boundary observations agree with one queue flush");
}
static void empty_input_does_not_flush() {
    IONetworkInterface iface;
    IONetworkStats stats;
    _ifnet ifp { &iface, &stats };
    mbuf_list list = MBUF_LIST_INITIALIZER();
    require(if_input(&ifp, &list) == kIOReturnSuccess, "empty input succeeds");
    require(iface.received.empty() && iface.flushes == 0 && stats.inputPackets == 0,
            "empty input changes no delivery state");
    require(ifp.if_input_diag.calls == 1 && ifp.if_input_diag.offeredPackets == 0 &&
            ifp.if_input_diag.flushCalls == 0 && ifp.if_input_diag.noIfnetCalls == 0,
            "empty input records only the call and does not inspect a native ifnet");
}
static void delivery_does_not_require_stats() {
    IONetworkInterface iface;
    _ifnet ifp { &iface, nullptr };
    mbuf_list list = MBUF_LIST_INITIALIZER();
    ml_enqueue(&list, new RTWInputMbuf { 44, 74, nullptr });
    require(if_input(&ifp, &list) == kIOReturnSuccess, "input without stats succeeds");
    require(iface.received == std::vector<unsigned>({44}) && iface.flushes == 1,
            "missing optional statistics do not discard a packet");
}
static void flush_shortfall_is_reported_after_ownership_transfer() {
    IONetworkInterface iface;
    iface.flush_shortfall = 1;
    IONetworkStats stats;
    _ifnet ifp { &iface, &stats };
    mbuf_list list = MBUF_LIST_INITIALIZER();
    ml_enqueue(&list, new RTWInputMbuf { 55, 100, nullptr });
    require(if_input(&ifp, &list) == kIOReturnIOError,
            "flush count shortfall cannot report successful handoff");
    require(iface.freed == 1 && list.ml_len == 0,
            "shortfall never reclaims an already transferred packet");
    require(ifp.if_input_diag.submissionMismatches == 1 &&
            ifp.if_input_diag.offeredPackets == 1 && ifp.if_input_diag.flushPackets == 0,
            "shortfall preserves the offered and returned boundary counts");
}
static void detached_interface_reports_unavailable_without_retry() {
    IONetworkInterface iface;
    iface.attached = false;
    IONetworkStats stats;
    _ifnet ifp { &iface, &stats };
    mbuf_list list = MBUF_LIST_INITIALIZER();
    ml_enqueue(&list, new RTWInputMbuf { 66, 200, nullptr });
    ml_enqueue(&list, new RTWInputMbuf { 77, 300, nullptr });
    require(if_input(&ifp, &list) == kIOReturnNotReady,
            "detached native interface does not report successful handoff");
    require(iface.freed == 2 && list.ml_len == 0 && iface.bsd_packets == 0 && iface.bsd_bytes == 0,
            "native detached drop consumes ownership without BSD accounting or retry");
    require(ifp.if_input_diag.noIfnetCalls == 1 && ifp.if_input_diag.immediateReturns == 2 &&
            ifp.if_input_diag.flushPackets == 0 && ifp.if_input_diag.submissionMismatches == 1,
            "native immediate frees and absent ifnet remain observable separately");
}
static void adapter_observations_are_isolated() {
    IONetworkInterface iface1, iface2;
    IONetworkStats stats1, stats2;
    _ifnet ifp1 { &iface1, &stats1 }, ifp2 { &iface2, &stats2 };
    mbuf_list list1 = MBUF_LIST_INITIALIZER(), list2 = MBUF_LIST_INITIALIZER();
    ml_enqueue(&list1, new RTWInputMbuf { 88, 64, nullptr });
    ml_enqueue(&list2, new RTWInputMbuf { 99, 1514, nullptr });
    require(if_input(&ifp1, &list1) == kIOReturnSuccess &&
            if_input(&ifp2, &list2) == kIOReturnSuccess, "two adapters submit independently");
    require(ifp1.if_input_diag.offeredBytes == 64 && ifp2.if_input_diag.offeredBytes == 1514 &&
            ifp1.if_input_diag.flushPackets == 1 && ifp2.if_input_diag.flushPackets == 1,
            "boundary byte and packet counters belong to each adapter");
    require(iface1.bsd_packets == 1 && iface1.bsd_bytes == 64 &&
            iface2.bsd_packets == 1 && iface2.bsd_bytes == 1514,
            "BSD accounting is neither shared nor doubled by observer counters");
}
static void queue_return_is_not_an_ifnet_acceptance_result() {
    IONetworkInterface iface;
    /* Apple ignores ifnet_input_extended's errno and returns the queue count.
     * Boundary observations must not pretend to repair BSD statistics. */
    iface.reject_in_ifnet = true;
    IONetworkStats stats;
    _ifnet ifp { &iface, &stats };
    mbuf_list list = MBUF_LIST_INITIALIZER();
    ml_enqueue(&list, new RTWInputMbuf { 100, 128, nullptr });
    require(if_input(&ifp, &list) == kIOReturnSuccess,
            "queue submission success does not expose downstream native errno");
    require(iface.bsd_packets == 0 && iface.bsd_bytes == 0 && iface.freed == 1 &&
            ifp.if_input_diag.offeredPackets == 1 && ifp.if_input_diag.offeredBytes == 128 &&
            ifp.if_input_diag.flushPackets == 1 && ifp.if_input_diag.submissionMismatches == 0,
            "observer preserves downstream-zero symptom without manual BSD double counting");
}
int main() {
    multiple_packets_transfer_as_isolated_mbufs(false);
    multiple_packets_transfer_as_isolated_mbufs(true);
    empty_input_does_not_flush();
    delivery_does_not_require_stats();
    flush_shortfall_is_reported_after_ownership_transfer();
    detached_interface_reports_unavailable_without_retry();
    adapter_observations_are_isolated();
    queue_return_is_not_an_ifnet_acceptance_result();
    std::printf("PASS imported Ethernet input accounting and ownership (%u assertions)\n", checks);
}
