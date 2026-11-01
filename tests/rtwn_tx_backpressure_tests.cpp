// Reuse established boundary fixtures; the real complete gated method, wrapper,
// and wake method are extracted by test-rtwn-tx-backpressure.py.
#define main existing_wpa_recovery_main
#include "rtwn_wpa_recovery_tests.cpp"
#undef main

IOCommandGate *RTWUSBController::copyGate(IOWorkLoop **loop) {
    *loop = hostNoGate_ ? nullptr : &hostLoop_;
    hostGate_.target = this;
    return hostNoGate_ ? nullptr : &hostGate_;
}
static void configure_authorized(RTWUSBController &c, ieee80211_node &node) {
    run(c, node); node.ni_port_valid = true;
    c.pendingQueue_.capacity = 2;
    c.core_.sc_ops.is_oactive = [](void *) { return 1; };
}
static std::vector<mbuf_t> transmitted;
// Returning dropped or freeing on retry corrupts the queue's retained mbuf.
static void full_queue_retries_same_packet_without_error_or_duplicate() {
    RTWUSBController c; ieee80211_node node; configure_authorized(c, node);
    Packet a, b, retry;
    a.node=b.node=retry.node=&node;
    CHECK(c.outputPacket(&a, nullptr) == kIOReturnOutputSuccess);
    CHECK(c.outputPacket(&b, nullptr) == kIOReturnOutputSuccess);
    CHECK(c.outputPacket(&retry, nullptr) == kIOReturnOutputStall);
    CHECK(retry.frees == 0 && a.frees == 0 && b.frees == 0);
    CHECK(c.pendingQueue_.packets == std::deque<mbuf_t>({&a, &b}));
    CHECK(c.txErrors_ == 0 && c.txQueueDrops_ == 0 && c.statsValue_.outputErrors == 0);
    CHECK(c.outputPacket(&retry, nullptr) == kIOReturnOutputStall);
    CHECK(c.pendingQueue_.lockDequeue() == &a); mbuf_freem(&a);
    CHECK(c.outputPacket(&retry, nullptr) == kIOReturnOutputSuccess);
    CHECK(c.pendingQueue_.packets == std::deque<mbuf_t>({&b, &retry}));
    CHECK(c.hostGate_.releases == 5 && c.hostLoop_.releases == 5);
    // Actual imported dequeue must transmit the retained packets once in order.
    transmitted.clear(); node.refs=2;
    c.core_.sc_ic.ic_if.if_snd.ifq_oactive=0;
    c.core_.sc_ops.is_oactive=[](void*){return 0;};
    c.core_.sc_ops.tx=[](void*,mbuf_t packet,ieee80211_node *n){
        transmitted.push_back(packet);mbuf_freem(packet);--n->refs;return 0;
    };
    rtwn_start(&c.core_.sc_ic.ic_if);
    CHECK(transmitted==std::vector<mbuf_t>({&b,&retry}));
    CHECK(c.pendingQueue_.packets.empty() && node.refs==0);
    c.pendingQueue_.lockFlush(); CHECK(b.frees == 1 && retry.frees == 1);
}
// Losing authorization while stalled transfers ownership only on the retry.
static void stalled_packet_is_released_once_on_stop_or_disconnect() {
    for (unsigned reason = 0; reason < 7; ++reason) {
        RTWUSBController c; ieee80211_node node; configure_authorized(c, node);
        c.pendingQueue_.capacity = 0; Packet packet;
        CHECK(c.outputPacket(&packet, nullptr) == kIOReturnOutputStall);
        if (reason == 0) node.ni_port_valid = false;
        if (reason == 1) c.closing_ = true;
        if (reason == 2) c.present_ = false;
        if (reason == 3) c.sleeping_ = true;
        if (reason == 4) c.usbError_ = kIOReturnIOError;
        if (reason == 5) c.running_ = false;
        if (reason == 6) c.stopping_ = true;
        CHECK(c.outputPacket(&packet, nullptr) == kIOReturnOutputDropped);
        CHECK(packet.frees == 1 && c.txQueueDrops_ == 0 && c.txErrors_ == 0);
    }
}
// A failed gate invocation must not leak a packet or return retry for it.
static void unavailable_gate_owns_and_frees_the_packet_once() {
    for (bool missing : {false, true}) {
        RTWUSBController c; Packet packet;
        c.hostNoGate_ = missing; c.hostGate_.reject = !missing;
        CHECK(c.outputPacket(&packet, nullptr) == kIOReturnOutputDropped);
        CHECK(packet.frees == 1);
    }
}
// Real quiesce must flush both ownership domains, then allow a fresh association.
static void reset_flushes_stalled_and_inner_packets_once() {
    RTWUSBController c;ieee80211_node node;configure_authorized(c,node);
    Packet a,b,retry,fresh;
    CHECK(c.outputPacket(&a,nullptr)==kIOReturnOutputSuccess);
    CHECK(c.outputPacket(&b,nullptr)==kIOReturnOutputSuccess);
    CHECK(c.outputPacket(&retry,nullptr)==kIOReturnOutputStall);
    CHECK(c.outputQueue_.lockEnqueue(&retry)); // IOKit's retry/prepend ownership.
    c.quiesce(false);
    CHECK(a.frees==1&&b.frees==1&&retry.frees==1);
    CHECK(c.pendingQueue_.packets.empty()&&c.outputQueue_.packets.empty());
    CHECK(c.txQueueDrops_==0&&c.txErrors_==0);
    c.serviceTXOutputQueue();CHECK(c.outputQueue_.services==0);
    CHECK(c.enableStation()==kIOReturnSuccess);run(c,node);node.ni_port_valid=true;
    CHECK(c.outputPacket(&fresh,nullptr)==kIOReturnOutputSuccess);
    c.pendingQueue_.lockFlush();CHECK(fresh.frees==1);
}
// Completion wakes must be deferred, authorized, and specific to their adapter.
static void wake_waits_for_capacity_and_does_not_restart_unusable_link() {
    RTWUSBController a, b; ieee80211_node na, nb;
    configure_authorized(a, na); configure_authorized(b, nb);
    a.pendingQueue_.capacity = 0;
    a.serviceTXOutputQueue(); CHECK(a.outputQueue_.services == 0);
    a.pendingQueue_.capacity = 2;
    a.serviceTXOutputQueue();
    CHECK(a.outputQueue_.services == 1 && a.outputQueue_.serviceOptions == IOBasicOutputQueue::kServiceAsync);
    CHECK(b.outputQueue_.services == 0);
    for (unsigned reason = 0; reason < 6; ++reason) {
        RTWUSBController c; ieee80211_node node; configure_authorized(c, node);
        if (reason == 0) c.closing_ = true;
        if (reason == 1) c.stopping_ = true;
        if (reason == 2) node.ni_port_valid = false;
        if (reason == 3) c.usbError_ = kIOReturnIOError;
        if (reason == 4) c.running_ = false;
        if (reason == 5) c.core_.sc_ic.ic_if.if_snd.queue = nullptr;
        c.serviceTXOutputQueue(); CHECK(c.outputQueue_.services == 0);
    }
}
int main(int argc, char **argv) {
    if(argc==1 || std::strcmp(argv[1],"admission")==0){
        full_queue_retries_same_packet_without_error_or_duplicate();
        stalled_packet_is_released_once_on_stop_or_disconnect();
        unavailable_gate_owns_and_frees_the_packet_once();
        reset_flushes_stalled_and_inner_packets_once();
    }
    if(argc==1 || std::strcmp(argv[1],"wake")==0)
        wake_waits_for_capacity_and_does_not_restart_unusable_link();
    std::printf("TX backpressure ownership/wake: %u checks passed\n", checks);
}
