/* SPDX-License-Identifier: GPL-2.0-only */
#include "platform.hpp"
#include "rtw/RTL8814AUDataPath.h"
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"RX capacity line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
rtw_packet_error RTWUSBController::decodeUSBFrame(rtw_rx_iterator*i,rtw_8192eu_rx_frame*,uint8_t*)const{
    rtw_8814au_rx_frame f{};return rtw_8814au_rx_next(i,&f);
}
static void emptyBuffers(RTWUSBController&c){for(auto&t:c.transfers_)t.memory=nullptr;}
static void pump(RTWUSBController&c){unsigned n=0;while(c.event.pending&&n++<16){c.event.pending=false;RTWUSBController::completionAction(&c,&c.event,1);}CHECK(!c.event.pending);}
static void cleanup(RTWUSBController&c){c.abortTransfers();c.freeTransfers();CHECK(c.references==1);CHECK(rtw_transfer_drained(&c.ledger_));CHECK(IOBufferMemoryDescriptor::liveAllocations==0);}
static void familyAllocationAndInitialReads(){
    for(unsigned f=RTW_WIFI_UNKNOWN;f<=RTW_WIFI_RTL8822CU;++f){
        RTWUSBController c;c.family_=static_cast<rtw_wifi_family>(f);emptyBuffers(c);
        const size_t expected=f==RTW_WIFI_RTL8814AU?32768U:16384U;
        CHECK(c.allocateTransfers()==0);
        for(unsigned i=0;i<RTW_TRANSFER_SLOTS;++i)CHECK(c.transfers_[i].memory->getCapacity()==(i<c.RXSlots?expected:4352U));
        CHECK(RTWUSBController::busInitialize(&c)==0);
        CHECK(c.rx.requests.size()==4&&c.references==5&&c.ledger_.pending==15);
        for(const auto&r:c.rx.requests){CHECK(r.length==expected);CHECK(r.memory->getCapacity()>=r.length);}
        CHECK(RTWUSBController::busInitialize(&c)==EBUSY);
        c.freeTransfers();CHECK(c.rx.requests.size()==4&&c.references==5&&c.ledger_.pending==15);
        CHECK(IOBufferMemoryDescriptor::liveAllocations==16);
        CHECK(c.timer.lastDelay==1000);cleanup(c);
    }
}
static void allocationAndInitialReadFailuresRollBack(){
    for(unsigned fail=1;fail<=RTW_TRANSFER_SLOTS;++fail){
        RTWUSBController c;emptyBuffers(c);IOBufferMemoryDescriptor::allocationCalls=0;IOBufferMemoryDescriptor::failAllocation=fail;
        CHECK(c.allocateTransfers()==ENOMEM);CHECK(c.references==1&&c.ledger_.pending==0);
        for(const auto&t:c.transfers_)CHECK(!t.memory);
        CHECK(IOBufferMemoryDescriptor::liveAllocations==0);
    }
    IOBufferMemoryDescriptor::failAllocation=0;
    for(unsigned fail=1;fail<=4;++fail){
        RTWUSBController c;emptyBuffers(c);CHECK(c.allocateTransfers()==0);c.rx.failIOCall=fail;
        CHECK(RTWUSBController::busInitialize(&c)==EIO);CHECK(!c.running_);cleanup(c);
    }
    RTWUSBController c;emptyBuffers(c);
    c.transfers_[0].memory=IOBufferMemoryDescriptor::withCapacity(16384,kIODirectionIn);
    CHECK(c.submit(0,c.rxPipe_,32768)==EINVAL);
    CHECK(c.rx.ioCalls==0&&c.references==1&&c.ledger_.pending==0);
    CHECK(c.allocateTransfers()!=0);cleanup(c);
}
static void rearmAndRecoveryKeepFamilyCapacity(){
    for(auto f:{RTW_WIFI_RTL8814AU,RTW_WIFI_RTL8192EU,RTW_WIFI_RTL8812AU}){
        RTWUSBController c;c.family_=f;emptyBuffers(c);CHECK(c.allocateTransfers()==0);CHECK(RTWUSBController::busInitialize(&c)==0);
        const uint32_t expected=f==RTW_WIFI_RTL8814AU?32768U:16384U;
        c.rx.finish(0,kIOReturnSuccess,0);pump(c);CHECK(c.rx.requests.back().length==expected);
        c.rx.finish(1,kIOReturnNotResponding,0);pump(c);
        CHECK(c.rxRecovery_.completed==1&&c.rxRecovery_.attempts==1&&c.rxRecovery_.consecutive==1);
        CHECK(c.running_&&c.usbError_==0&&c.rx.requests.size()==9);
        for(const auto&r:c.rx.requests){CHECK(r.length==expected);CHECK(r.memory->getCapacity()>=r.length);}
        cleanup(c);
    }
}
static void put32(uint8_t*p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=static_cast<uint8_t>(v>>(i*8));}
static std::vector<uint8_t> fixture(){
    // 12*1600+1000=20200 bytes, below the vendor's20KiB DMA threshold.
    std::vector<uint8_t>b(20200);
    for(unsigned i=0;i<13;++i){auto*p=b.data()+i*1600;const unsigned n=i==12?944U:1544U;
        put32(p,n|(4U<<16)|(1U<<26));put32(p+12,27);std::memset(p+56,0xff,n);p[56]=0x88;p[57]=0x42;}
    return b;
}
static void fullAUReadPreservesFramesAndLegacySplitReproducesPair(){
    const auto b=fixture();RTWUSBController c;emptyBuffers(c);CHECK(c.allocateTransfers()==0);CHECK(RTWUSBController::busInitialize(&c)==0);
    CHECK(c.rx.requests[0].length>=b.size());
    std::memcpy(c.rx.requests[0].memory->getBytesNoCopy(),b.data(),b.size());
    c.rx.finish(0,kIOReturnSuccess,static_cast<uint32_t>(b.size()));pump(c);
    CHECK(c.rxDiag_.aggregateErrors==0);
    CHECK(c.rxDiag_.lastReadBytes==20200&&c.rxDiag_.maxReadBytes==20200&&c.rxDiag_.lastAggregateErrorReadBytes==0);
    rtw_rx_iterator it{};rtw_rx_iterator_init(&it,b.data(),b.size());rtw_8814au_rx_frame f{};
    while(rtw_8814au_rx_next(&it,&f)==RTW_PACKETS_OK){}CHECK(it.frames==13);
    RTWUSBController split;split.receive(b.data(),16384);split.receive(b.data()+16384,b.size()-16384);
    CHECK(split.rxDiag_.aggregateTruncated==1&&split.rxDiag_.aggregateMalformed==1);
    CHECK(split.rxDiag_.partialPrefixBytes==16000&&split.rxDiag_.partialPrefixFrames==10);
    CHECK(split.rxDiag_.lastAggregateErrorOffset==0&&split.rxDiag_.lastAggregateErrorFrames==0);
    CHECK(split.rxDiag_.lastReadBytes==3816&&split.rxDiag_.maxReadBytes==16384&&split.rxDiag_.lastAggregateErrorReadBytes==3816);
    split.receive(b.data(),1600);
    CHECK(split.rxDiag_.lastReadBytes==1600&&split.rxDiag_.maxReadBytes==16384&&split.rxDiag_.lastAggregateErrorReadBytes==3816);
    cleanup(c);
}
int main(){familyAllocationAndInitialReads();allocationAndInitialReadFailuresRollBack();rearmAndRecoveryKeepFamilyCapacity();fullAUReadPreservesFramesAndLegacySplitReproducesPair();std::printf("Controller RX capacity: %u checks passed\n",checks);}
