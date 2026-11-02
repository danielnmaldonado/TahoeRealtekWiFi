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
    iface1.native_index=4; iface2.native_index=8;
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
    require(ifp1.if_input_diag.nativeAfter.index==4 &&
            ifp2.if_input_diag.nativeAfter.index==8 &&
            ifp1.if_input_diag.nativeAfter.bytesIn==64 &&
            ifp2.if_input_diag.nativeAfter.bytesIn==1514,
            "native handoff observations retain each adapter's numeric identity and counters");
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
static void native_boundary_sampling_observes_the_submitting_interface() {
    IONetworkInterface iface;
    iface.bsd_packets=20; iface.bsd_bytes=3000; iface.bsd_errors=2;
    iface.bsd_tx_packets=13; iface.bsd_tx_bytes=2000; iface.bsd_tx_errors=1;
    iface.native_index=9; iface.native_supported=0x402; iface.native_enabled=0x400;
    IONetworkStats stats;
    _ifnet ifp { &iface, &stats };
    mbuf_list list = MBUF_LIST_INITIALIZER();
    ml_enqueue(&list, new RTWInputMbuf { 101, 128, nullptr });
    require(if_input(&ifp, &list) == kIOReturnSuccess, "sampled queue submission succeeds");
    require(iface.native_reads == 2,
            "first nonempty batch observes native statistics before handoff and after flush");
    const auto& d=ifp.if_input_diag;
    require(d.nativeSampleCount==1 && d.nativeSampleBatch==1 &&
            d.nativeSampleOfferedPackets==1 && d.nativeSampleOfferedBytes==128 &&
            d.nativeSampleImmediateReturns==0 && d.nativeSampleFlushPackets==1 &&
            d.nativeSampleIfnetStable==1,
            "snapshot pair identifies its batch and exact offered and returned totals");
    require(d.nativeBefore.statsError==0 && d.nativeAfter.statsError==0 &&
            d.nativeBefore.index==9 && d.nativeAfter.index==9 &&
            d.nativeBefore.supportedCapabilities==0x402 &&
            d.nativeAfter.enabledCapabilities==0x400,
            "snapshot identity and supported/enabled masks describe submitting ifnet");
    require(d.nativeBefore.packetsIn==20 && d.nativeBefore.bytesIn==3000 &&
            d.nativeAfter.packetsIn==21 && d.nativeAfter.bytesIn==3128 &&
            d.nativeBefore.errorsIn==2 && d.nativeAfter.errorsIn==2,
            "native RX observations surround the queue handoff without adjusting counters");
    require(d.nativeBefore.packetsOut==13 && d.nativeBefore.bytesOut==2000 &&
            d.nativeBefore.errorsOut==1 && d.nativeAfter.packetsOut==13 &&
            d.nativeAfter.bytesOut==2000 && d.nativeAfter.errorsOut==1,
            "native TX observations remain distinct from offered receive traffic");
}
static void native_sampling_is_bounded_and_skips_empty_input() {
    IONetworkInterface iface;
    _ifnet ifp { &iface, nullptr };
    bool submitted=true;
    for (unsigned i=0;i<1024;++i) {
        mbuf_list list=MBUF_LIST_INITIALIZER();
        ml_enqueue(&list,new RTWInputMbuf {i,60,nullptr});
        submitted=if_input(&ifp,&list)==kIOReturnSuccess && submitted;
    }
    require(submitted,"bounded observation preserves all 1024 batch submissions");
    require(iface.native_reads==2 && ifp.if_input_diag.nativeSampleCount==1 &&
            ifp.if_input_diag.nativeSampleBatch==1,
            "1024 receive batches need only one before/after native snapshot pair");
    mbuf_list empty=MBUF_LIST_INITIALIZER();
    require(if_input(&ifp,&empty)==kIOReturnSuccess && iface.native_reads==2,
            "empty input neither resamples nor advances the sampling batch");
    ml_enqueue(&empty,new RTWInputMbuf {1025,70,nullptr});
    require(if_input(&ifp,&empty)==kIOReturnSuccess && iface.native_reads==4 &&
            ifp.if_input_diag.nativeSampleCount==2 && ifp.if_input_diag.nativeSampleBatch==1025 &&
            ifp.if_input_diag.nativeSampleOfferedPackets==1 &&
            ifp.if_input_diag.nativeSampleOfferedBytes==70 &&
            ifp.if_input_diag.nativeBefore.packetsIn==1024 &&
            ifp.if_input_diag.nativeAfter.packetsIn==1025,
            "next bounded sample describes its own batch rather than cumulative offers");
}
static void successful_delivery_with_zero_native_counters_remains_visible() {
    IONetworkInterface iface;
    iface.omit_native_accounting=true;
    _ifnet ifp { &iface, nullptr };
    mbuf_list list=MBUF_LIST_INITIALIZER();
    ml_enqueue(&list,new RTWInputMbuf {1,1514,nullptr});
    require(if_input(&ifp,&list)==kIOReturnSuccess && iface.delivered_packets==1 &&
            iface.delivered_bytes==1514,"modeled delivery succeeds despite omitted native accounting");
    const auto& d=ifp.if_input_diag;
    require(d.nativeBefore.statsError==0 && d.nativeAfter.statsError==0 &&
            d.nativeBefore.packetsIn==0 && d.nativeAfter.packetsIn==0 &&
            d.nativeAfter.bytesIn==0 && d.nativeSampleOfferedBytes==1514 &&
            d.nativeSampleFlushPackets==1 && d.submissionMismatches==0 &&
            iface.bsd_packets==0 && iface.bsd_bytes==0,
            "observation exposes working-delivery/native-zero discrepancy without masking it");
}
static void delayed_native_accounting_is_not_reported_as_handoff_failure() {
    IONetworkInterface iface;
    iface.defer_native_accounting=true;
    _ifnet ifp { &iface, nullptr };
    mbuf_list list=MBUF_LIST_INITIALIZER();
    ml_enqueue(&list,new RTWInputMbuf {1,96,nullptr});
    require(if_input(&ifp,&list)==kIOReturnSuccess &&
            ifp.if_input_diag.nativeAfter.packetsIn==0 &&
            ifp.if_input_diag.submissionMismatches==0,
            "immediate zero native RX does not turn an accepted queue into a delivery error");
    iface.synchronizeNativeStats();
    ifnet_stats_param later{};
    require(ifnet_stat(iface.getIfnet(),&later)==0 && later.packets_in==1 && later.bytes_in==96 &&
            ifp.if_input_diag.nativeAfter.packetsIn==0,
            "a later native read can distinguish deferred accounting from the retained handoff sample");
}
static void native_read_failure_invalidates_the_whole_snapshot() {
    IONetworkInterface iface;
    iface.bsd_packets=100; iface.bsd_bytes=10000; iface.bsd_tx_packets=10;
    _ifnet ifp { &iface, nullptr };
    mbuf_list list=MBUF_LIST_INITIALIZER();
    ml_enqueue(&list,new RTWInputMbuf {1,60,nullptr});
    require(if_input(&ifp,&list)==kIOReturnSuccess &&
            ifp.if_input_diag.nativeBefore.statsError==0 &&
            ifp.if_input_diag.nativeBefore.packetsIn==100,
            "initial successful native observation retains its real counter value");
    iface.native_error=EIO;
    bool submitted=true;
    for(unsigned i=0;i<1024;++i) {
        ml_enqueue(&list,new RTWInputMbuf {i+2,60,nullptr});
        submitted=if_input(&ifp,&list)==kIOReturnSuccess && submitted;
    }
    require(submitted && iface.delivered_packets==1025,
            "diagnostic native read failure cannot block queue delivery");
    require(ifp.if_input_diag.nativeBefore.statsError==EIO &&
            ifp.if_input_diag.nativeAfter.statsError==EIO &&
            ifp.if_input_diag.nativeBefore.index==7 &&
            ifp.if_input_diag.nativeAfter.packetsIn==0 &&
            ifp.if_input_diag.nativeBefore.packetsOut==0,
            "failed native read retains identity and error but never exposes invalid counter contents");
    IONetworkInterface missing;
    missing.attached=false;
    _ifnet other { &missing, nullptr };
    ml_enqueue(&list,new RTWInputMbuf {2,60,nullptr});
    require(if_input(&other,&list)==kIOReturnNotReady && missing.native_reads==0 &&
            other.if_input_diag.nativeBefore.statsError==ENXIO &&
            other.if_input_diag.nativeAfter.statsError==ENXIO &&
            other.if_input_diag.nativeBefore.index==0 &&
            other.if_input_diag.nativeSampleIfnetStable==0,
            "absent native ifnet is unavailable without a diagnostic read or changed detached-drop behavior");
}
static void native_detach_and_counter_reset_are_preserved() {
    IONetworkInterface iface;
    iface.bsd_packets=100; iface.bsd_bytes=10000; iface.reset_native_on_flush=true;
    _ifnet ifp { &iface, nullptr };
    mbuf_list list=MBUF_LIST_INITIALIZER();
    ml_enqueue(&list,new RTWInputMbuf {1,60,nullptr});
    require(if_input(&ifp,&list)==kIOReturnSuccess &&
            ifp.if_input_diag.nativeBefore.packetsIn==100 &&
            ifp.if_input_diag.nativeAfter.packetsIn==0,
            "native counter reset remains raw observations rather than an underflowed delta");
    IONetworkInterface detached;
    detached.detach_on_flush=true;
    _ifnet other { &detached, nullptr };
    ml_enqueue(&list,new RTWInputMbuf {2,60,nullptr});
    require(if_input(&other,&list)==kIOReturnSuccess && detached.delivered_packets==1 &&
            other.if_input_diag.nativeBefore.index==7 &&
            other.if_input_diag.nativeAfter.index==0 &&
            other.if_input_diag.nativeAfter.statsError==ENXIO &&
            other.if_input_diag.nativeSampleIfnetStable==0,
            "detach during flush invalidates after identity without changing completed submission");
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
    native_boundary_sampling_observes_the_submitting_interface();
    native_sampling_is_bounded_and_skips_empty_input();
    successful_delivery_with_zero_native_counters_remains_visible();
    delayed_native_accounting_is_not_reported_as_handoff_failure();
    native_read_failure_invalidates_the_whole_snapshot();
    native_detach_and_counter_reset_are_preserved();
    std::printf("PASS imported Ethernet input accounting and ownership (%u assertions)\n", checks);
}
