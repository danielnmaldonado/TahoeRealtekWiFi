#include "rtwn_rx_order_host.hpp"
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); std::exit(1); } } while (0)
static void start_rx(RTWUSBController &c) {
    for (unsigned i = 0; i < c.RXSlots; ++i) CHECK(c.submit(i, c.rxPipe_, c.RXCapacity) == 0);
}
// A numeric slot loop would deliver recycled 0 before older submissions 1..3.
static void recycled_slot_waits_for_older_reads() {
    RTWUSBController c; start_rx(c);
    c.finish(0, 0); c.drainCompletions(true);
    CHECK(c.delivered == std::vector<unsigned>({0}));
    c.finish(1, 1); c.finish(2, 2); c.finish(3, 3); c.finish(0, 4);
    c.drainCompletions(true);
    CHECK(c.delivered == std::vector<unsigned>({0, 1, 2, 3, 4}));
    CHECK(c.references == 5);
}
// A later completion cannot pass a missing callback; TX remains independent.
static void delayed_oldest_blocks_rx_only() {
    RTWUSBController c; start_rx(c);
    CHECK(c.submit(4, c.rxPipe_, 12) == 0);
    c.finish(2, 2); c.finish(4, 4);
    const auto signals = c.event_.signals;
    c.drainCompletions(true);
    CHECK(c.delivered.empty()); CHECK(c.txPackets_ == 1);
    CHECK(c.transfers_[2].storage.completions == 0);
    CHECK(c.event_.signals == signals); // A later-only completion cannot self-wake.
    c.finish(0, 0); c.drainCompletions(true);
    CHECK(c.delivered == std::vector<unsigned>({0}));
    c.finish(1, 1); c.drainCompletions(true);
    CHECK(c.delivered == std::vector<unsigned>({0, 1, 2}));
}
static void pending_tx_does_not_block_rx() {
    RTWUSBController c;
    CHECK(c.submit(4, c.rxPipe_, 12) == 0); start_rx(c);
    c.finish(0, 0); c.drainCompletions(true);
    CHECK(c.delivered == std::vector<unsigned>({0}));
    CHECK(c.txPackets_ == 0); CHECK(c.ledger_.pending & (1U << 4));
}
// Cleanup must reclaim later completions even when an earlier callback is absent.
static void stopped_cleanup_does_not_wait_for_order() {
    RTWUSBController c; start_rx(c);
    c.running_ = false; c.stopping_ = true; rtw_transfer_stop(&c.ledger_);
    c.finish(2, 2); c.drainCompletions(false);
    CHECK(c.delivered.empty()); CHECK(c.transfers_[2].storage.completions == 1);
    CHECK(c.references == 4); CHECK(!rtw_transfer_restart(&c.ledger_));
    c.finish(0, 0); c.finish(1, 1); c.finish(3, 3); c.drainCompletions(false);
    CHECK(c.references == 1); CHECK(rtw_transfer_drained(&c.ledger_));
    CHECK(rtw_transfer_restart(&c.ledger_));
}
// Real generation checking must ignore stale and duplicate callbacks.
static void duplicate_and_stale_callbacks_are_ignored() {
    RTWUSBController c; start_rx(c);
    c.finish(0, 0); const auto old = c.transfers_[0].generation;
    c.recordCompletion(&c.transfers_[0], kIOReturnIOError, 0, old);
    c.drainCompletions(true);
    CHECK(c.delivered == std::vector<unsigned>({0})); CHECK(c.usbError_ == 0);
    c.recordCompletion(&c.transfers_[0], kIOReturnIOError, 0, old);
    c.drainCompletions(true);
    CHECK(c.delivered == std::vector<unsigned>({0})); CHECK(c.usbError_ == 0);
}
// Immediate completions on resubmission must not make one drain run forever.
static void drain_budget_rearms_ready_oldest() {
    RTWUSBController c; start_rx(c);
    for (unsigned i = 0; i < c.RXSlots; ++i) c.finish(i, static_cast<uint8_t>(i));
    c.completeOnResubmit = true;
    c.drainCompletions(true);
    CHECK(c.delivered == std::vector<unsigned>({0, 1, 2, 3}));
    CHECK(c.event_.signals == 9); // Four initial, four resubmitted, one explicit rearm.
    c.drainCompletions(true);
    CHECK(c.delivered == std::vector<unsigned>({0, 1, 2, 3, 4, 5, 6, 7}));
    CHECK(c.event_.signals == 14);
}
static void failed_rx_is_reclaimed_before_quiesce() {
    RTWUSBController c; start_rx(c);
    c.finish(1, 1, kIOReturnIOError); c.drainCompletions(true);
    CHECK(c.quiesces == 0); CHECK(c.delivered.empty());
    c.finish(0, 0); c.drainCompletions(true);
    CHECK(c.delivered == std::vector<unsigned>({0}));
    CHECK(c.rxErrors_ == 1); CHECK(c.quiesces == 1);
    CHECK(c.transfers_[1].storage.completions == 1);
    CHECK(!(c.ledger_.pending & (1U << 1)));
}
static void successful_tx_preserves_error_counters_with_optional_stats() {
    for (bool haveStats : {false, true}) {
        RTWUSBController c;
        if (!haveStats)
            c.stats_ = nullptr;
        CHECK(c.submit(c.RXSlots, c.rxPipe_, 12) == 0);
        c.finish(c.RXSlots, 0);
        c.drainCompletions(true);
        CHECK(c.txPackets_ == 1);
        CHECK(c.statsValue_.outputPackets == (haveStats ? 1U : 0U));
        CHECK(c.txErrors_ == 0);
        CHECK(c.txUSBTransferErrors_ == 0);
        CHECK(c.statsValue_.outputErrors == 0);
        CHECK(c.usbError_ == kIOReturnSuccess);
        CHECK(c.quiesces == 0);
        CHECK(c.core_.sc_tx_timer == 0);
        CHECK(c.transfers_[c.RXSlots].storage.completions == 1);
        CHECK(rtw_transfer_drained(&c.ledger_));
        CHECK(c.references == 1);
        c.drainCompletions(true);
        CHECK(c.txPackets_ == 1);
        CHECK(c.transfers_[c.RXSlots].storage.completions == 1);
        CHECK(c.references == 1);
    }
}
static void failed_tx_updates_counters_once_with_optional_stats() {
    struct Completion {
        IOReturn result;
        uint32_t actual;
        IOReturn expectedError;
    };
    const Completion cases[] = {
        {kIOReturnIOError, 12, kIOReturnIOError},
        {kIOReturnSuccess, 11, kIOReturnUnderrun},
        {kIOReturnSuccess, 13, kIOReturnUnderrun},
        {kIOReturnSuccess, 0, kIOReturnUnderrun}
    };
    for (const auto &completion : cases) {
        for (bool haveStats : {false, true}) {
            RTWUSBController c;
            if (!haveStats)
                c.stats_ = nullptr;
            CHECK(c.submit(c.RXSlots, c.rxPipe_, 12) == 0);
            auto &t = c.transfers_[c.RXSlots];
            c.recordCompletion(&t, completion.result, completion.actual, t.generation);
            c.drainCompletions(true);
            CHECK(c.txPackets_ == 0);
            CHECK(c.statsValue_.outputPackets == 0);
            CHECK(c.txErrors_ == 1);
            CHECK(c.txUSBTransferErrors_ == 1);
            CHECK(c.statsValue_.outputErrors == (haveStats ? 1U : 0U));
            CHECK(c.usbError_ == completion.expectedError);
            CHECK(c.quiesces == 1);
            CHECK(t.storage.completions == 1);
            CHECK(!t.prepared);
            CHECK(t.pipe == nullptr);
            CHECK(rtw_transfer_drained(&c.ledger_));
            CHECK(c.references == 1);
            // A duplicate callback cannot charge the same failed transfer twice.
            c.recordCompletion(&t, completion.result, completion.actual, t.generation);
            c.drainCompletions(false);
            CHECK(c.txErrors_ == 1);
            CHECK(c.txUSBTransferErrors_ == 1);
            CHECK(c.statsValue_.outputErrors == (haveStats ? 1U : 0U));
            CHECK(t.storage.completions == 1);
            CHECK(c.references == 1);
        }
    }
}
static void aborted_tx_does_not_count_as_a_usb_error() {
    RTWUSBController c;
    CHECK(c.submit(c.RXSlots, c.rxPipe_, 12) == 0);
    c.finish(c.RXSlots, 0, kIOReturnAborted);
    c.drainCompletions(true);
    CHECK(c.txPackets_ == 0);
    CHECK(c.txErrors_ == 0);
    CHECK(c.txUSBTransferErrors_ == 0);
    CHECK(c.statsValue_.outputErrors == 0);
    CHECK(c.usbError_ == kIOReturnSuccess);
    CHECK(c.quiesces == 0);
    CHECK(c.transfers_[c.RXSlots].storage.completions == 1);
    CHECK(rtw_transfer_drained(&c.ledger_));
    CHECK(c.references == 1);
}
static void stopped_tx_cleanup_preserves_error_counters() {
    RTWUSBController c;
    CHECK(c.submit(c.RXSlots, c.rxPipe_, 12) == 0);
    c.running_ = false;
    c.stopping_ = true;
    rtw_transfer_stop(&c.ledger_);
    c.finish(c.RXSlots, 0, kIOReturnIOError);
    c.drainCompletions(false);
    CHECK(c.txPackets_ == 0);
    CHECK(c.txErrors_ == 0);
    CHECK(c.txUSBTransferErrors_ == 0);
    CHECK(c.statsValue_.outputErrors == 0);
    CHECK(c.usbError_ == kIOReturnSuccess);
    CHECK(c.quiesces == 0);
    CHECK(c.transfers_[c.RXSlots].storage.completions == 1);
    CHECK(rtw_transfer_drained(&c.ledger_));
    CHECK(c.references == 1);
}
int main() {
    successful_tx_preserves_error_counters_with_optional_stats();
    failed_tx_updates_counters_once_with_optional_stats();
    aborted_tx_does_not_count_as_a_usb_error();
    stopped_tx_cleanup_preserves_error_counters();
    recycled_slot_waits_for_older_reads();
    delayed_oldest_blocks_rx_only();
    pending_tx_does_not_block_rx();
    stopped_cleanup_does_not_wait_for_order();
    duplicate_and_stale_callbacks_are_ignored();
    drain_budget_rearms_ready_oldest();
    failed_rx_is_reclaimed_before_quiesce();
    std::printf("RX completion order: %u checks passed\n", checks);
}
