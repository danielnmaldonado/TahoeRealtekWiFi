/* SPDX-License-Identifier: GPL-2.0-only */
#include "platform.hpp"
#include "rtw/RTL8814AUDataPath.h"
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"RX capacity line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
#include "rtw/RTL8192CDataPath.h"
#include "rtw/RTL8192DUDataPath.h"
#include "rtw/RTL8192FUDataPath.h"
#include "rtw/RTL8710BUDataPath.h"
#include "rtw/RTL8723BUDataPath.h"
#include "rtw/RTL8812AUDataPath.h"
#include "rtw/RTW88RX.h"
// The native typed codecs use the same named status ordering as USBPackets.
static rtw_packet_error typedStatus(unsigned status){
    switch(status){case 0:return RTW_PACKETS_OK;case 1:return RTW_PACKETS_DONE;
    case 2:return RTW_PACKETS_BAD_ARGUMENT;case 3:return RTW_PACKETS_TRUNCATED;
    case 4:return RTW_PACKETS_MALFORMED;case 5:return RTW_PACKETS_TOO_MANY;
    default:return RTW_PACKETS_BAD_ARGUMENT;}
}
rtw_packet_error RTWUSBController::decodeUSBFrame(rtw_rx_iterator*i,rtw_8192eu_rx_frame*out,uint8_t*)const{
    switch(family_){
    case RTW_WIFI_RTL8192EU:return rtw_8192eu_rx_next(i,out);
    case RTW_WIFI_RTL8723BU:return rtw_8723bu_rx_next(i,out);
    case RTW_WIFI_RTL8192CU:case RTW_WIFI_RTL8188EU:{rtw_92c_rx_frame f{};return rtw_92c_rx_next(i,family_==RTW_WIFI_RTL8188EU,&f);}
    case RTW_WIFI_RTL8192DU:{
        if(i->cursor==i->length)return RTW_PACKETS_DONE;
        if(i->cursor||i->frames)return RTW_PACKETS_MALFORMED;
        rtw_92du_rx_frame f{};const auto e=rtw_92du_parse_rx(i->buffer,i->length,&f);
        if(e==RTW_PACKETS_OK){i->cursor=i->length;++i->frames;}return e;
    }
    case RTW_WIFI_RTL8192FU:{rtw_r92fu_rx_iterator t{i->buffer,i->length,i->cursor,static_cast<unsigned>(i->frames)};rtw_r92fu_rx_frame f{};
        auto e=rtw_r92fu_rx_next(&t,2,&f);i->cursor=t.cursor;i->frames=t.frames;return typedStatus(e);}
    case RTW_WIFI_RTL8710BU:{rtw_r10bu_rx_iterator t{i->buffer,i->length,i->cursor,static_cast<unsigned>(i->frames)};rtw_r10bu_rx_frame f{};
        auto e=rtw_r10bu_rx_next(&t,1,&f);i->cursor=t.cursor;i->frames=t.frames;return typedStatus(e);}
    case RTW_WIFI_RTL8812AU:case RTW_WIFI_RTL8821AU:{rtw_r12au_rx_iterator t{i->buffer,i->length,i->cursor,static_cast<unsigned>(i->frames)};rtw_r12au_rx_frame f{};
        auto e=rtw_r12au_rx_next(&t,family_==RTW_WIFI_RTL8821AU?1:2,&f);i->cursor=t.cursor;i->frames=t.frames;return typedStatus(e);}
    case RTW_WIFI_RTL8821CU:case RTW_WIFI_RTL8822BU:case RTW_WIFI_RTL8822CU:{
        const rtw88_rx_phy_config config{family_,static_cast<uint8_t>(family_==RTW_WIFI_RTL8821CU?1:2),0,0,0,0};
        rtw_8814au_rx_frame f{};return rtw88_rx_next(&config,i,&f);}
    default:{rtw_8814au_rx_frame f{};return rtw_8814au_rx_next(i,&f);}
    }
}
static bool largeReadFamily(rtw_wifi_family f){
    return f==RTW_WIFI_RTL8192EU||f==RTW_WIFI_RTL8814AU||f==RTW_WIFI_RTL8812AU||f==RTW_WIFI_RTL8821AU||
        f==RTW_WIFI_RTL8821CU||f==RTW_WIFI_RTL8822BU||f==RTW_WIFI_RTL8822CU;
}

static void emptyBuffers(RTWUSBController&c){for(auto&t:c.transfers_)t.memory=nullptr;}
static void pump(RTWUSBController&c){unsigned n=0;while(c.event.pending&&n++<16){c.event.pending=false;RTWUSBController::completionAction(&c,&c.event,1);}CHECK(!c.event.pending);}
static void cleanup(RTWUSBController&c){c.abortTransfers();c.freeTransfers();CHECK(c.references==1);CHECK(rtw_transfer_drained(&c.ledger_));CHECK(IOBufferMemoryDescriptor::liveAllocations==0);}
static void familyAllocationAndInitialReads(){
    for(unsigned f=RTW_WIFI_UNKNOWN;f<=RTW_WIFI_RTL8822CU;++f){
        RTWUSBController c;c.family_=static_cast<rtw_wifi_family>(f);emptyBuffers(c);
        const size_t expected=largeReadFamily(static_cast<rtw_wifi_family>(f))?32768U:16384U;
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
static void initialAUTransactionErrorRollsBackBeforeExplicitRetry(){
    // A rejected initial read must not be mistaken for a fatal USB fault or
    // a complete RX epoch. The next explicit initialization owns the retry.
    for(unsigned reject=1;reject<=4;++reject){
        RTWUSBController c;emptyBuffers(c);c.radioReady_=false;
        CHECK(c.allocateTransfers()==0);c.rx.ioResults[reject]=kIOReturnNotResponding;
        CHECK(RTWUSBController::busInitialize(&c)==EAGAIN);
        CHECK(!c.running_&&!c.radioReady_&&!c.rateTransportFresh_&&c.usbError_==0);
        CHECK(rtw_transfer_drained(&c.ledger_)&&c.references==1);
        CHECK(c.rx.clearCalls==1&&c.rx.withRequest);
        CHECK(c.rxRecovery_.initialErrors==1&&c.rxRecovery_.initialClearAttempts==1);
        CHECK(c.rxRecovery_.initialConsecutive==1&&c.rxRecovery_.initialOriginalError==kIOReturnNotResponding);
        CHECK(c.rxRecovery_.initialClearResult==0&&c.rxRecovery_.attempts==0&&c.rxRecovery_.failures==0);
        for(const auto&t:c.transfers_)CHECK(!t.prepared);
        CHECK(RTWUSBController::busInitialize(&c)==0);
        CHECK(c.running_&&!c.radioReady_&&c.ledger_.pending==15&&c.references==5);
        CHECK(c.rxRecovery_.initialConsecutive==0&&c.rxRecovery_.initialErrors==1&&c.rxRecovery_.initialClearAttempts==1);
        cleanup(c);
    }
}
static void persistentInitialAUTransactionErrorsHaveABoundedClearBudget(){
    RTWUSBController c;emptyBuffers(c);CHECK(c.allocateTransfers()==0);
    c.rx.ioError=kIOReturnNotResponding;
    for(unsigned attempt=1;attempt<=3;++attempt){
        CHECK(RTWUSBController::busInitialize(&c)==EAGAIN);
        CHECK(!c.running_&&c.usbError_==0&&c.rx.clearCalls==attempt);
        CHECK(rtw_transfer_drained(&c.ledger_)&&c.references==1);
    }
    CHECK(RTWUSBController::busInitialize(&c)==EIO);
    CHECK(!c.running_&&c.usbError_==kIOReturnNotResponding&&c.rx.clearCalls==3);
    CHECK(c.rxRecovery_.initialErrors==4&&c.rxRecovery_.initialClearAttempts==3);
    CHECK(c.rxRecovery_.initialFailures==1&&c.rxRecovery_.initialExhausted==1&&c.rxRecovery_.initialConsecutive==3);
    const auto calls=c.rx.ioCalls;
    CHECK(RTWUSBController::busInitialize(&c)==ENXIO&&c.rx.ioCalls==calls);
    cleanup(c);
}
static void initialAUClearFailurePreservesItsActualFault(){
    for(auto failure:{kIOReturnNotResponding,kIOReturnIOError,kIOReturnTimeout}){
        RTWUSBController c;emptyBuffers(c);CHECK(c.allocateTransfers()==0);
        c.rx.ioResults[3]=kIOReturnNotResponding;c.rx.clearError=failure;
        CHECK(RTWUSBController::busInitialize(&c)==EIO);
        CHECK(!c.running_&&c.usbError_==failure&&c.rx.clearCalls==1);
        CHECK(c.rxRecovery_.initialOriginalError==kIOReturnNotResponding&&c.rxRecovery_.initialClearResult==failure);
        CHECK(c.rxRecovery_.initialErrors==1&&c.rxRecovery_.initialFailures==1&&c.rxRecovery_.initialExhausted==0);
        CHECK(rtw_transfer_drained(&c.ledger_)&&c.references==1);cleanup(c);
    }
}
static void initialAUOldReadsMustDrainBeforeClearOrReuse(){
    RTWUSBController c;emptyBuffers(c);CHECK(c.allocateTransfers()==0);
    c.rx.ioResults[3]=kIOReturnNotResponding;c.rx.deferAbort=true;
    CHECK(RTWUSBController::busInitialize(&c)==EBUSY);
    CHECK(!c.running_&&c.usbError_==0&&c.rx.clearCalls==0&&c.ledger_.pending==3&&c.references==3);
    CHECK(c.rxRecovery_.initialErrors==1&&c.rxRecovery_.initialClearAttempts==0);
    const auto calls=c.rx.ioCalls;
    CHECK(RTWUSBController::busInitialize(&c)==EBUSY&&c.rx.ioCalls==calls);
    c.closing_=true;c.present_=false;
    CHECK(RTWUSBController::busInitialize(&c)==ENXIO&&c.rx.ioCalls==calls&&c.rx.clearCalls==0);
    c.rx.finish(0,kIOReturnAborted,0);c.rx.finish(1,kIOReturnAborted,0);pump(c);
    cleanup(c);
}
static void deferredInitialAUDrainPreservesFatalCompletions(){
    for(unsigned fatalSlot:{0U,1U})for(bool abortedFirst:{false,true})for(bool process:{false,true}){
        RTWUSBController c;emptyBuffers(c);c.radioReady_=false;CHECK(c.allocateTransfers()==0);
        c.rx.ioResults[3]=kIOReturnNotResponding;c.rx.deferAbort=true;
        CHECK(RTWUSBController::busInitialize(&c)==EBUSY);
        CHECK(c.rxRecovery_.initialDrainMask==3&&c.rxRecovery_.initialDrainError==0);
        const unsigned abortedSlot=1U-fatalSlot;
        auto drain=[&]{if(process)pump(c);else c.drainCompletions(false);};
        if(abortedFirst){c.rx.finish(abortedSlot,kIOReturnAborted,0);drain();CHECK(c.usbError_==0);}
        c.rx.finish(fatalSlot,kIOReturnIOError,0);drain();
        CHECK(c.usbError_==kIOReturnIOError&&c.rxRecovery_.initialFailures==1);
        CHECK(c.rxRecovery_.initialDrainError==kIOReturnIOError);
        CHECK(!c.running_&&!c.radioReady_&&c.rx.clearCalls==0);
        if(!abortedFirst){c.rx.finish(abortedSlot,kIOReturnAborted,0);drain();}
        CHECK(rtw_transfer_drained(&c.ledger_)&&c.references==1);
        CHECK(c.rxRecovery_.initialDrainMask==0);
        const auto calls=c.rx.ioCalls;
        CHECK(RTWUSBController::busInitialize(&c)==ENXIO&&c.rx.ioCalls==calls&&c.rx.clearCalls==0);
        cleanup(c);
    }
}
static void deferredInitialAUDrainKeepsTheFirstFaultAcrossCallbackHoles(){
    RTWUSBController c;emptyBuffers(c);c.radioReady_=false;CHECK(c.allocateTransfers()==0);
    c.rx.ioResults[4]=kIOReturnNotResponding;c.rx.deferAbort=true;
    CHECK(RTWUSBController::busInitialize(&c)==EBUSY);
    c.rx.finish(2,kIOReturnTimeout,0);pump(c);
    CHECK(c.usbError_==kIOReturnTimeout&&c.rxRecovery_.initialDrainError==kIOReturnTimeout);
    CHECK(c.rxRecovery_.initialFailures==1&&c.rx.clearCalls==0&&!c.running_&&!c.radioReady_);
    c.rx.finish(0,kIOReturnIOError,0);pump(c);
    c.rx.finish(1,kIOReturnNotReady,0);pump(c);
    CHECK(c.usbError_==kIOReturnTimeout&&c.rxRecovery_.initialDrainError==kIOReturnTimeout);
    CHECK(c.rxRecovery_.initialFailures==1&&c.rxRecovery_.initialDrainMask==0);
    CHECK(rtw_transfer_drained(&c.ledger_)&&c.references==1);
    CHECK(RTWUSBController::busInitialize(&c)==ENXIO&&c.rx.clearCalls==0);cleanup(c);
}
static void deferredInitialAUDrainValidatesLateLengthsAndStatusesDuringCancellation(){
    struct Completion{IOReturn result;uint32_t actual;IOReturn error;};
    const Completion completions[]={{kIOReturnSuccess,32769,kIOReturnUnderrun},
        {kIOReturnNotResponding,24,kIOReturnNotResponding},
        {kIOReturnAborted,32769,kIOReturnUnderrun},{-100,0,-100}};
    for(const auto&completion:completions)for(bool detach:{false,true})for(bool process:{false,true}){
        RTWUSBController c;emptyBuffers(c);CHECK(c.allocateTransfers()==0);
        c.rx.ioResults[3]=kIOReturnNotResponding;c.rx.deferAbort=true;
        CHECK(RTWUSBController::busInitialize(&c)==EBUSY);
        if(detach){c.closing_=true;c.present_=false;}else c.sleeping_=true;
        c.rx.finish(1,completion.result,completion.actual);
        if(process)pump(c);else c.drainCompletions(false);
        CHECK(c.usbError_==completion.error&&c.rxRecovery_.initialDrainError==completion.error);
        CHECK(c.rxRecovery_.initialFailures==1&&!c.running_&&!c.radioReady_&&c.rx.clearCalls==0);
        c.rx.finish(0,kIOReturnAborted,0);pump(c);
        CHECK(c.rxRecovery_.initialDrainMask==0&&rtw_transfer_drained(&c.ledger_)&&c.references==1);
        const auto calls=c.rx.ioCalls;
        CHECK(RTWUSBController::busInitialize(&c)==ENXIO&&c.rx.ioCalls==calls&&c.rx.clearCalls==0);
        cleanup(c);
    }
}
static void deferredInitialAUDrainDoesNotAttributeStaleCallbacksToFreshReads(){
    RTWUSBController c;emptyBuffers(c);CHECK(c.allocateTransfers()==0);
    c.rx.ioResults[3]=kIOReturnNotResponding;c.rx.deferAbort=true;
    CHECK(RTWUSBController::busInitialize(&c)==EBUSY);
    const auto oldGeneration=c.transfers_[0].generation;
    c.rx.finish(0,kIOReturnAborted,0);pump(c);
    c.rx.duplicate(0,kIOReturnIOError,0);pump(c);
    CHECK(c.usbError_==0&&c.rxRecovery_.initialDrainMask==2&&c.rxRecovery_.initialFailures==0);
    c.rx.finish(1,kIOReturnAborted,0);pump(c);
    CHECK(c.rxRecovery_.initialDrainMask==0&&c.rxRecovery_.initialDrainError==0&&c.references==1);
    c.rx.deferAbort=false;
    CHECK(RTWUSBController::busInitialize(&c)==EAGAIN&&c.rx.clearCalls==1);
    CHECK(RTWUSBController::busInitialize(&c)==0&&c.ledger_.pending==15);
    c.recordCompletion(&c.transfers_[0],kIOReturnIOError,0,oldGeneration);pump(c);
    CHECK(c.usbError_==0&&c.rxRecovery_.initialDrainError==0&&c.rxRecovery_.initialFailures==0);
    CHECK(c.running_&&c.ledger_.pending==15&&c.references==5);cleanup(c);
}
static void initialAUCompletionErrorCannotCertifyTheRXSet(){
    for(auto result:{kIOReturnNotResponding,kIOReturnIOError}){
        RTWUSBController c;emptyBuffers(c);c.radioReady_=false;
        CHECK(c.allocateTransfers()==0);
        c.rx.onAccepted=[&]{if(c.rx.ioCalls==4)c.rx.finish(3,result,0);};
        CHECK(RTWUSBController::busInitialize(&c)==(result==kIOReturnNotResponding?EAGAIN:EIO));
        CHECK(!c.running_&&!c.radioReady_&&rtw_transfer_drained(&c.ledger_)&&c.references==1);
        CHECK(c.rx.clearCalls==(result==kIOReturnNotResponding?1U:0U));
        CHECK(c.usbError_==(result==kIOReturnNotResponding?0:result));cleanup(c);
    }
    // The halted pipe's secondary rejection must not hide the first read's
    // distinct fatal callback when both are recorded in the same startup.
    RTWUSBController fatal;emptyBuffers(fatal);CHECK(fatal.allocateTransfers()==0);
    fatal.rx.onIO=[&]{if(fatal.rx.ioCalls==3)fatal.rx.finish(0,kIOReturnIOError,0);};
    CHECK(RTWUSBController::busInitialize(&fatal)==EIO);
    CHECK(fatal.usbError_==kIOReturnIOError&&fatal.rx.clearCalls==0);
    cleanup(fatal);
    RTWUSBController draining;emptyBuffers(draining);CHECK(draining.allocateTransfers()==0);
    draining.rx.ioResults[3]=kIOReturnNotResponding;
    draining.rx.onAbort=[&]{if(draining.rx.abortCalls==1)draining.rx.finish(0,kIOReturnIOError,0);};
    CHECK(RTWUSBController::busInitialize(&draining)==EIO);
    CHECK(draining.usbError_==kIOReturnIOError&&draining.rx.clearCalls==0);
    cleanup(draining);
}
static void fullInitialAUEpochReplenishesOnlyItsClearBudget(){
    RTWUSBController c;emptyBuffers(c);CHECK(c.allocateTransfers()==0);c.rx.ioError=kIOReturnNotResponding;
    for(unsigned attempt=0;attempt<3;++attempt)CHECK(RTWUSBController::busInitialize(&c)==EAGAIN);
    c.rx.ioError=0;CHECK(RTWUSBController::busInitialize(&c)==0);
    CHECK(c.rxRecovery_.initialConsecutive==0&&c.rxRecovery_.initialErrors==3&&c.rxRecovery_.initialClearAttempts==3);
    c.abortTransfers();c.rx.ioError=kIOReturnNotResponding;
    CHECK(RTWUSBController::busInitialize(&c)==EAGAIN);
    CHECK(c.rxRecovery_.initialConsecutive==1&&c.rxRecovery_.initialClearAttempts==4&&c.rxRecovery_.initialErrors==4);
    cleanup(c);
    RTWUSBController fresh;
    CHECK(fresh.rxRecovery_.initialConsecutive==0&&fresh.rxRecovery_.initialErrors==0&&fresh.rxRecovery_.initialClearAttempts==0);
}
static void initialAUClearCannotCompleteStartupAcrossCancellation(){
    for(bool detach:{false,true}){
        RTWUSBController c;emptyBuffers(c);CHECK(c.allocateTransfers()==0);c.rx.ioResults[2]=kIOReturnNotResponding;
        c.rx.onClear=[&]{if(detach){c.closing_=true;c.present_=false;}else c.sleeping_=true;};
        CHECK(RTWUSBController::busInitialize(&c)==ENXIO);
        CHECK(!c.running_&&!c.radioReady_&&!c.rateTransportFresh_&&c.usbError_==0);
        CHECK(c.rx.clearCalls==1&&rtw_transfer_drained(&c.ledger_)&&c.references==1);
        const auto calls=c.rx.ioCalls;
        CHECK(RTWUSBController::busInitialize(&c)==ENXIO&&c.rx.ioCalls==calls);cleanup(c);
    }
}
static void initialFailureDoesNotDisturbAnotherAdapter(){
    RTWUSBController au,eu;emptyBuffers(au);emptyBuffers(eu);eu.family_=RTW_WIFI_RTL8192EU;
    CHECK(au.allocateTransfers()==0&&eu.allocateTransfers()==0);
    CHECK(RTWUSBController::busInitialize(&eu)==0);
    au.rx.ioError=kIOReturnNotResponding;
    CHECK(RTWUSBController::busInitialize(&au)==EAGAIN);
    CHECK(eu.running_&&eu.ledger_.pending==15&&eu.references==5&&eu.rx.clearCalls==0);
    au.sleeping_=true;const auto calls=au.rx.ioCalls;
    CHECK(RTWUSBController::busInitialize(&au)==ENXIO&&au.rx.ioCalls==calls);
    CHECK(eu.running_&&eu.ledger_.pending==15&&eu.references==5);
    au.abortTransfers();au.freeTransfers();eu.abortTransfers();eu.freeTransfers();
    CHECK(au.references==1&&eu.references==1&&IOBufferMemoryDescriptor::liveAllocations==0);
    RTWUSBController legacy;emptyBuffers(legacy);legacy.family_=RTW_WIFI_RTL8192EU;
    CHECK(legacy.allocateTransfers()==0);legacy.rx.ioResults[1]=kIOReturnNotResponding;
    CHECK(RTWUSBController::busInitialize(&legacy)==EAGAIN&&legacy.rx.clearCalls==1);cleanup(legacy);
}
static void allStationFamiliesRecoverInitialRXWithTheirOwnCapacity(){
    unsigned stationFamilies=0;
    for(unsigned family=RTW_WIFI_UNKNOWN;family<=RTW_WIFI_RTL8822CU;++family){
        const auto kind=static_cast<rtw_wifi_family>(family);
        const auto*backend=rtw_backend_for_family(kind);
        if(!backend||!(backend->capabilities&RTW_BACKEND_CAP_KEXT_STATION))continue;
        ++stationFamilies;
        for(unsigned rejected=1;rejected<=4;++rejected){
            RTWUSBController c;c.family_=kind;c.radioReady_=false;emptyBuffers(c);
            CHECK(c.allocateTransfers()==0);c.rx.ioResults[rejected]=kIOReturnNotResponding;
            CHECK(RTWUSBController::busInitialize(&c)==EAGAIN);
            CHECK(!c.running_&&!c.radioReady_&&!c.rateTransportFresh_&&c.usbError_==0);
            CHECK(c.rx.clearCalls==1&&rtw_transfer_drained(&c.ledger_)&&c.references==1);
            CHECK(c.rxRecovery_.initialOriginalError==kIOReturnNotResponding);
            CHECK(RTWUSBController::busInitialize(&c)==0&&c.ledger_.pending==15);
            CHECK(c.rxRecovery_.initialConsecutive==0);
            const auto capacity=rtw_usb_rx_capacity(kind);
            for(const auto&read:c.rx.requests)CHECK(read.length==capacity);
            cleanup(c);
        }
    }
    CHECK(stationFamilies==13);
}
static void allStationFamiliesRetainDeferredInitialFaultsAndBoundRetries(){
    for(unsigned family=RTW_WIFI_UNKNOWN;family<=RTW_WIFI_RTL8822CU;++family){
        const auto kind=static_cast<rtw_wifi_family>(family);
        const auto*backend=rtw_backend_for_family(kind);
        if(!backend||!(backend->capabilities&RTW_BACKEND_CAP_KEXT_STATION))continue;
        {
            RTWUSBController c;c.family_=kind;emptyBuffers(c);CHECK(c.allocateTransfers()==0);
            c.rx.ioResults[3]=kIOReturnNotResponding;c.rx.deferAbort=true;
            CHECK(RTWUSBController::busInitialize(&c)==EBUSY);
            CHECK(c.rxRecovery_.initialDrainMask==3&&c.rx.clearCalls==0);
            c.sleeping_=true;c.rx.finish(0,kIOReturnIOError,0);pump(c);
            CHECK(c.usbError_==kIOReturnIOError&&c.rxRecovery_.initialDrainError==kIOReturnIOError);
            c.rx.finish(1,kIOReturnAborted,0);pump(c);
            CHECK(c.rxRecovery_.initialDrainMask==0&&c.rxRecovery_.initialFailures==1);
            c.sleeping_=false;const auto calls=c.rx.ioCalls;
            CHECK(RTWUSBController::busInitialize(&c)==ENXIO&&c.rx.ioCalls==calls&&c.rx.clearCalls==0);
            cleanup(c);
        }
        {
            RTWUSBController c;c.family_=kind;emptyBuffers(c);CHECK(c.allocateTransfers()==0);
            c.rx.ioError=kIOReturnNotResponding;
            for(unsigned attempt=0;attempt<3;++attempt)CHECK(RTWUSBController::busInitialize(&c)==EAGAIN);
            CHECK(RTWUSBController::busInitialize(&c)==EIO);
            CHECK(c.rx.clearCalls==3&&c.rxRecovery_.initialExhausted==1&&c.usbError_==kIOReturnNotResponding);
            CHECK(!c.running_&&!c.rateTransportFresh_);cleanup(c);
        }
        for(bool oversize:{false,true}){
            RTWUSBController c;c.family_=kind;c.radioReady_=false;emptyBuffers(c);CHECK(c.allocateTransfers()==0);
            c.rx.onAccepted=[&]{if(c.rx.ioCalls==4)c.rx.finish(3,oversize?kIOReturnSuccess:kIOReturnNotResponding,
                oversize?rtw_usb_rx_capacity(kind)+1:0);};
            CHECK(RTWUSBController::busInitialize(&c)==(oversize?EIO:EAGAIN));
            CHECK(!c.running_&&!c.radioReady_&&!c.rateTransportFresh_);
            CHECK(c.usbError_==(oversize?kIOReturnUnderrun:0)&&c.rx.clearCalls==(oversize?0U:1U));
            cleanup(c);
        }
        {
            RTWUSBController c;c.family_=kind;emptyBuffers(c);CHECK(c.allocateTransfers()==0);
            c.rx.ioResults[2]=kIOReturnNotResponding;c.rx.clearError=kIOReturnTimeout;
            CHECK(RTWUSBController::busInitialize(&c)==EIO);
            CHECK(c.usbError_==kIOReturnTimeout&&c.rxRecovery_.initialOriginalError==kIOReturnNotResponding);
            CHECK(c.rxRecovery_.initialClearResult==kIOReturnTimeout);cleanup(c);
        }
        {
            RTWUSBController c;c.family_=kind;emptyBuffers(c);CHECK(c.allocateTransfers()==0);c.closing_=true;
            CHECK(RTWUSBController::busInitialize(&c)==ENXIO&&c.rx.ioCalls==0);cleanup(c);
        }
    }
    for(auto kind:{RTW_WIFI_UNKNOWN,RTW_WIFI_RTL8814BU}){
        RTWUSBController c;c.family_=kind;emptyBuffers(c);CHECK(c.allocateTransfers()==0);
        c.rx.ioError=kIOReturnNotResponding;CHECK(RTWUSBController::busInitialize(&c)==EIO);
        CHECK(c.rx.clearCalls==0&&c.rxRecovery_.initialErrors==0&&!c.running_);cleanup(c);
    }
}
static void ordinaryDeferredStopRetainsFatalCompletionsForEveryStation(){
    for(unsigned family=RTW_WIFI_UNKNOWN;family<=RTW_WIFI_RTL8822CU;++family){
        const auto kind=static_cast<rtw_wifi_family>(family);const auto*b=rtw_backend_for_family(kind);
        if(!b||!(b->capabilities&RTW_BACKEND_CAP_KEXT_STATION))continue;
        RTWUSBController c;c.family_=kind;emptyBuffers(c);CHECK(c.allocateTransfers()==0);
        CHECK(RTWUSBController::busInitialize(&c)==0);c.core_.core_error=ETIMEDOUT;
        c.rx.deferAbort=true;c.abortTransfers();
        CHECK(!c.running_&&c.ledger_.pending==15&&c.usbError_==0);
        c.rx.finish(0,kIOReturnIOError,0);
        for(unsigned i=1;i<4;++i)c.rx.finish(i,kIOReturnAborted,0);
        pump(c);CHECK(c.usbError_==kIOReturnIOError&&c.core_.core_error==ETIMEDOUT);
        CHECK(rtw_transfer_drained(&c.ledger_)&&!c.radioReady_);
        c.rx.deferAbort=false;cleanup(c);
    }
}
static void lateStopCallbackAfterPreflightIsValidatedBeforeReclaim(){
    for(bool startup:{false,true}){
        RTWUSBController c;emptyBuffers(c);CHECK(c.allocateTransfers()==0);c.rx.deferAbort=true;
        if(startup){c.rx.ioResults[3]=kIOReturnNotResponding;CHECK(RTWUSBController::busInitialize(&c)==EBUSY);}
        else{CHECK(RTWUSBController::busInitialize(&c)==0);c.abortTransfers();}
        c.rx.finish(0,kIOReturnAborted,0);
        hostAfterUnlock=[&]{hostAfterUnlock=nullptr;c.rx.finish(1,kIOReturnIOError,0);};
        c.drainCompletions(false);CHECK(!hostAfterUnlock);
        CHECK(c.usbError_==kIOReturnIOError&&!c.running_);
        if(startup)CHECK(c.rxRecovery_.initialDrainError==kIOReturnIOError);
        else CHECK(c.rxRecovery_.stopDrainError==kIOReturnIOError&&c.rxRecovery_.stopDrainFailures==1);
        for(unsigned i=2;i<c.rx.requests.size();++i)c.rx.finish(i,kIOReturnAborted,0);
        pump(c);CHECK(c.rxRecovery_.stopDrainMask==0&&c.rxRecovery_.initialDrainMask==0);
        c.rx.deferAbort=false;cleanup(c);
    }
}
static void ordinaryStopPreservesFirstFaultAndDoesNotAdoptStaleCallbacks(){
    for(bool process:{false,true}){
        RTWUSBController c;emptyBuffers(c);CHECK(c.allocateTransfers()==0);CHECK(RTWUSBController::busInitialize(&c)==0);
        c.rx.deferAbort=true;c.abortTransfers();c.sleeping_=true;
        c.rx.finish(2,kIOReturnTimeout,0);c.drainCompletions(process);
        CHECK(c.usbError_==kIOReturnTimeout&&c.rxRecovery_.stopDrainError==kIOReturnTimeout);
        c.rx.finish(0,kIOReturnIOError,0);c.drainCompletions(process);
        c.rx.finish(1,kIOReturnAborted,0);c.rx.finish(3,kIOReturnAborted,0);pump(c);
        CHECK(c.usbError_==kIOReturnTimeout&&c.rxRecovery_.stopDrainError==kIOReturnTimeout);
        CHECK(c.rxRecovery_.stopDrainFailures==2&&c.rxRecovery_.stopDrainMask==0);
        c.rx.deferAbort=false;cleanup(c);
    }
    {
        RTWUSBController c;emptyBuffers(c);CHECK(c.allocateTransfers()==0);CHECK(RTWUSBController::busInitialize(&c)==0);
        const auto previous=c.transfers_[0].generation;c.abortTransfers();
        CHECK(c.rxRecovery_.stopDrainMask==0&&c.rxRecovery_.stopDrainError==0);
        CHECK(RTWUSBController::busInitialize(&c)==0);
        c.recordCompletion(&c.transfers_[0],kIOReturnIOError,0,previous);pump(c);
        CHECK(c.usbError_==0&&c.rxRecovery_.stopDrainMask==0&&c.rxRecovery_.stopDrainError==0);
        CHECK(c.running_&&c.ledger_.pending==15&&c.references==5);cleanup(c);
    }
    for(bool shortWrite:{false,true}){
        RTWUSBController c;emptyBuffers(c);CHECK(c.allocateTransfers()==0);CHECK(RTWUSBController::busInitialize(&c)==0);
        CHECK(c.submit(c.RXSlots,c.txPipes_[0],64)==0);c.rx.deferAbort=c.tx.deferAbort=true;
        c.abortTransfers();c.closing_=true;c.tx.finish(0,shortWrite?kIOReturnSuccess:kIOReturnIOError,shortWrite?63:0);c.drainCompletions(false);
        const auto expected=shortWrite?kIOReturnUnderrun:kIOReturnIOError;
        CHECK(c.usbError_==expected&&c.rxRecovery_.stopDrainError==expected);
        CHECK(c.txErrors_==1&&c.txUSBTransferErrors_==1);
        for(unsigned i=0;i<4;++i)c.rx.finish(i,kIOReturnAborted,0);pump(c);
        CHECK(c.rxRecovery_.stopDrainMask==0&&rtw_transfer_drained(&c.ledger_));
        c.rx.deferAbort=c.tx.deferAbort=false;cleanup(c);
    }
}
static void rearmAndRecoveryKeepFamilyCapacity(){
    for(unsigned value=RTW_WIFI_UNKNOWN;value<=RTW_WIFI_RTL8822CU;++value){
        const auto f=static_cast<rtw_wifi_family>(value);const auto*backend=rtw_backend_for_family(f);
        if(!backend||!(backend->capabilities&RTW_BACKEND_CAP_KEXT_STATION))continue;
        RTWUSBController c;c.family_=f;emptyBuffers(c);CHECK(c.allocateTransfers()==0);CHECK(RTWUSBController::busInitialize(&c)==0);
        const uint32_t expected=largeReadFamily(static_cast<rtw_wifi_family>(f))?32768U:16384U;
        c.rx.finish(0,kIOReturnSuccess,0);pump(c);CHECK(c.rx.requests.back().length==expected);
        c.rx.finish(1,kIOReturnNotResponding,0);pump(c);
        CHECK(c.rxRecovery_.completed==1&&c.rxRecovery_.attempts==1&&c.rxRecovery_.consecutive==1);
        CHECK(c.running_&&c.usbError_==0&&c.rx.requests.size()==9);
        for(const auto&r:c.rx.requests){CHECK(r.length==expected);CHECK(r.memory->getCapacity()>=r.length);}
        cleanup(c);
    }
}
static void put32(uint8_t*p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=static_cast<uint8_t>(v>>(i*8));}
static void fullEUReadPreservesOpaqueAggregateAbove16KiB(){
    // Opaque length-delimited units only: no 802.11 frame contents or I/O.
    // 14*1728=24192 precedes one complete 4120-byte unit: 28312 total.
    std::vector<uint8_t>b(28312,0x5a);
    for(unsigned i=0;i<15;++i){
        auto*p=b.data()+i*1728;std::memset(p,0,24);put32(p,i==14?4096U:1700U);
    }
    RTWUSBController c;c.family_=RTW_WIFI_RTL8192EU;emptyBuffers(c);
    CHECK(c.allocateTransfers()==0);CHECK(RTWUSBController::busInitialize(&c)==0);
    CHECK(c.rx.requests[0].length>=b.size());
    std::memcpy(c.rx.requests[0].memory->getBytesNoCopy(),b.data(),b.size());
    c.rx.finish(0,kIOReturnSuccess,static_cast<uint32_t>(b.size()));pump(c);
    CHECK(c.rxDiag_.aggregateErrors==0&&c.rxDiag_.lastReadBytes==28312);
    CHECK(c.rxDiag_.partialAggregates==0&&c.rx.requests.back().length>=b.size());
    rtw_rx_iterator it{};rtw_rx_iterator_init(&it,b.data(),b.size());rtw_8192eu_rx_frame f{};
    rtw_packet_error e;while((e=rtw_8192eu_rx_next(&it,&f))==RTW_PACKETS_OK){}
    CHECK(e==RTW_PACKETS_DONE&&it.frames==15&&it.cursor==b.size());
    c.rx.finish(1,kIOReturnNotResponding,0);pump(c);
    CHECK(c.rxRecovery_.completed==1&&c.usbError_==0);
    const unsigned recovered=static_cast<unsigned>(c.rx.requests.size())-c.RXSlots;
    CHECK(c.rx.requests[recovered].length>=b.size());
    std::memcpy(c.rx.requests[recovered].memory->getBytesNoCopy(),b.data(),b.size());
    c.rx.finish(recovered,kIOReturnSuccess,static_cast<uint32_t>(b.size()));pump(c);
    CHECK(c.rxDiag_.aggregates==2&&c.rxDiag_.bytes==56624&&c.rxDiag_.aggregateErrors==0);
    cleanup(c);
}
static void allStationPayloadBoundariesUseTheirActualCodec(){
    unsigned covered=0;
    for(unsigned value=RTW_WIFI_UNKNOWN;value<=RTW_WIFI_RTL8822CU;++value){
        const auto family=static_cast<rtw_wifi_family>(value);const auto*backend=rtw_backend_for_family(family);
        if(!backend||!(backend->capabilities&RTW_BACKEND_CAP_KEXT_STATION))continue;
        ++covered;
        // Seven aggregated backends must retain a complete 28,312-byte read.
        // Remaining backends use one opaque complete 4,120-byte unit.
        const unsigned count=largeReadFamily(family)?15U:1U;
        std::vector<uint8_t>b(count==15?28312:4120,0x5a);
        for(unsigned i=0;i<count;++i){auto*p=b.data()+i*1728;std::memset(p,0,24);put32(p,i+1==count?4096U:1700U);}
        RTWUSBController c;c.family_=family;emptyBuffers(c);CHECK(c.allocateTransfers()==0);
        CHECK(RTWUSBController::busInitialize(&c)==0);CHECK(c.rx.requests[0].length>=b.size());
        std::memcpy(c.rx.requests[0].memory->getBytesNoCopy(),b.data(),b.size());
        c.rx.finish(0,kIOReturnSuccess,static_cast<uint32_t>(b.size()));pump(c);
        CHECK(c.rxDiag_.aggregateErrors==0&&c.rxDiag_.aggregates==1&&c.rxDiag_.bytes==b.size());
        c.rx.finish(1,kIOReturnNotResponding,0);pump(c);CHECK(c.rxRecovery_.completed==1);
        const unsigned slot=static_cast<unsigned>(c.rx.requests.size())-c.RXSlots;
        CHECK(c.rx.requests[slot].length>=b.size());
        std::memcpy(c.rx.requests[slot].memory->getBytesNoCopy(),b.data(),b.size());
        c.rx.finish(slot,kIOReturnSuccess,static_cast<uint32_t>(b.size()));pump(c);
        CHECK(c.rxDiag_.aggregateErrors==0&&c.rxDiag_.aggregates==2&&c.rxDiag_.bytes==2*b.size());
        std::printf("RX_CAPACITY_MATRIX,%u,%zu,%u,%u,%u,%llu\n",value,
            c.transfers_[0].memory->getCapacity(),c.rx.requests[0].length,
            c.rx.requests.back().length,c.rx.requests[slot].length,
            static_cast<unsigned long long>(c.rxDiag_.maxReadBytes));
        cleanup(c);
    }
    CHECK(covered==13);
}
static rtw_packet_error decodeForPreflight(rtw_rx_iterator*it,void*context){
    rtw_8192eu_rx_frame frame{};uint8_t report=0;
    return static_cast<RTWUSBController*>(context)->decodeUSBFrame(it,&frame,&report);
}
static void allStationFinalWirePayloadBounds(){
    unsigned covered=0;
    for(unsigned value=RTW_WIFI_UNKNOWN;value<=RTW_WIFI_RTL8822CU;++value){
        const auto family=static_cast<rtw_wifi_family>(value);const auto*backend=rtw_backend_for_family(family);
        if(!backend||!(backend->capabilities&RTW_BACKEND_CAP_KEXT_STATION))continue;
        ++covered;RTWUSBController c;c.family_=family;
        const size_t alignment=family==RTW_WIFI_RTL8192CU||family==RTW_WIFI_RTL8188EU?128:8;
        for(size_t payload=24;payload<32;++payload){
            const size_t used=24+payload,padded=(used+alignment-1)&~(alignment-1);
            std::vector<uint8_t>b(padded,0x5a);std::memset(b.data(),0,24);put32(b.data(),static_cast<uint32_t>(payload));
            for(size_t length=used;length<=padded;++length){
                rtw_rx_preflight_result result{};
                // DU uses one unaggregated unit and permits zero suffix only.
                if(family==RTW_WIFI_RTL8192DU)std::memset(b.data()+used,0,padded-used);
                CHECK(rtw_rx_preflight_transfer(b.data(),length,decodeForPreflight,&c,&result)==RTW_PACKETS_DONE);
                CHECK(result.valid_frames==1&&result.valid_bytes==length);
            }
            // Test the descriptor's full wire length, including all FCS bytes
            // on families that later strip them. Missing1..4 bytes still fail.
            for(size_t missing=1;missing<=4;++missing){rtw_rx_preflight_result result{};
                CHECK(rtw_rx_preflight_transfer(b.data(),used-missing,decodeForPreflight,&c,&result)!=RTW_PACKETS_DONE);
                CHECK(result.valid_frames==0&&result.valid_bytes==0);
            }
            b.resize(padded+1);b[padded]=0x5a;rtw_rx_preflight_result result{};
            CHECK(rtw_rx_preflight_transfer(b.data(),b.size(),decodeForPreflight,&c,&result)!=RTW_PACKETS_DONE);
        }
    }
    CHECK(covered==13);
}
static void aggregatedAUShortUnitsBeyond64(){
    // A healthy bounded opaque USB aggregate is not capped at64 MPDUs by
    // pinned rtw88/usb.c; decode every complete unit within the actual read.
    std::vector<uint8_t>b(65*56,0x5a);
    for(unsigned i=0;i<65;++i){auto*p=b.data()+i*56;std::memset(p,0,24);put32(p,32);}
    for(auto family:{RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU}){
        RTWUSBController c;c.family_=family;rtw_rx_preflight_result prefix{};
        CHECK(rtw_rx_preflight_transfer(b.data(),b.size(),decodeForPreflight,&c,&prefix)==RTW_PACKETS_DONE);
        CHECK(prefix.valid_frames==65&&prefix.valid_bytes==3640);
    }
}
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
int main(){aggregatedAUShortUnitsBeyond64();allStationFinalWirePayloadBounds();allStationPayloadBoundariesUseTheirActualCodec();fullEUReadPreservesOpaqueAggregateAbove16KiB();lateStopCallbackAfterPreflightIsValidatedBeforeReclaim();ordinaryStopPreservesFirstFaultAndDoesNotAdoptStaleCallbacks();ordinaryDeferredStopRetainsFatalCompletionsForEveryStation();allStationFamiliesRetainDeferredInitialFaultsAndBoundRetries();allStationFamiliesRecoverInitialRXWithTheirOwnCapacity();familyAllocationAndInitialReads();allocationAndInitialReadFailuresRollBack();initialAUTransactionErrorRollsBackBeforeExplicitRetry();persistentInitialAUTransactionErrorsHaveABoundedClearBudget();initialAUClearFailurePreservesItsActualFault();initialAUOldReadsMustDrainBeforeClearOrReuse();deferredInitialAUDrainPreservesFatalCompletions();deferredInitialAUDrainKeepsTheFirstFaultAcrossCallbackHoles();deferredInitialAUDrainValidatesLateLengthsAndStatusesDuringCancellation();deferredInitialAUDrainDoesNotAttributeStaleCallbacksToFreshReads();initialAUCompletionErrorCannotCertifyTheRXSet();fullInitialAUEpochReplenishesOnlyItsClearBudget();initialAUClearCannotCompleteStartupAcrossCancellation();initialFailureDoesNotDisturbAnotherAdapter();rearmAndRecoveryKeepFamilyCapacity();fullAUReadPreservesFramesAndLegacySplitReproducesPair();std::printf("Controller RX capacity: %u checks passed\n",checks);}
