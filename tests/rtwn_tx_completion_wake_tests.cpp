#include "rtwn_rx_order_host.hpp"
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); std::exit(1); } } while (0)
static void completion_services_after_reclaim_and_inner_drain() {
    RTWUSBController c, other;
    for(unsigned i=c.RXSlots;i<RTW_TRANSFER_SLOTS;++i)
        CHECK(c.submit(i,c.rxPipe_,12)==0);
    c.drainCompletions(true); CHECK(c.outputWakes==0);
    c.finish(c.RXSlots,0); c.drainCompletions(true);
    CHECK(c.txPackets_==1 && c.outputWakes==1);
    CHECK(c.core_.sc_ic.ic_if.starts==1 && other.outputWakes==0);
    CHECK(c.transfers_[c.RXSlots].storage.completions==1);
}
static void failure_or_cleanup_never_services_stalled_output() {
    for(unsigned reason=0;reason<4;++reason){
        RTWUSBController c; CHECK(c.submit(c.RXSlots,c.rxPipe_,12)==0);
        c.finish(c.RXSlots,0,reason==0?kIOReturnIOError:kIOReturnSuccess);
        if(reason==1)c.running_=false;
        if(reason==2)c.stopping_=true;
        c.drainCompletions(reason!=3);
        CHECK(c.outputWakes==0);
    }
}
int main(){
    completion_services_after_reclaim_and_inner_drain();
    failure_or_cleanup_never_services_stalled_output();
    std::printf("TX completion wake: %u checks passed\n",checks);
}
