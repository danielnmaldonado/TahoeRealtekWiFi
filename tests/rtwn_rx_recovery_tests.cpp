#include "platform.hpp"
static unsigned checks=0;
#define CHECK(x) do {++checks;if(!(x)){std::fprintf(stderr,"RX recovery line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void prime(RTWUSBController&c){for(unsigned i=0;i<c.RXSlots;++i)CHECK(c.submit(i,c.rxPipe_,rtw_usb_rx_capacity(c.family_))==0);}
static void pump(RTWUSBController&c){unsigned n=0;while(c.event.pending&&n++<16){c.event.pending=false;RTWUSBController::completionAction(&c,&c.event,1);}CHECK(!c.event.pending);}
static void cleanup(RTWUSBController&c){c.rx.deferAbort=false;c.abortTransfers();CHECK(c.references==1);CHECK(rtw_transfer_drained(&c.ledger_));for(auto&m:c.memory_)CHECK(m.prepares==m.completes);}
static void completeCurrent(RTWUSBController&c,IOReturn result,uint32_t length){
    for(unsigned i=0;i<c.rx.requests.size();++i)if(!c.rx.requests[i].done){c.rx.finish(i,result,length);return;}
    CHECK(false);
}
static void integratedFamiliesRecoverAndUnsupportedFamiliesStayFatal(){
    std::vector<rtw_wifi_family> families{RTW_WIFI_RTL8192EU};
    for(size_t i=0;i<rtw_backend_family_count();++i){const auto*b=rtw_backend_family_at(i);
        if((b->capabilities&RTW_BACKEND_CAP_KEXT_STATION)&&b->family!=RTW_WIFI_RTL8192EU)families.push_back(b->family);
    }
    CHECK(families.size()==13);
    for(auto family:families){RTWUSBController c;c.family_=family;prime(c);
        CHECK(c.submit(4,&c.tx,100)==0);
        c.rx.finish(2,kIOReturnNotResponding,0);c.tx.finish(0,kIOReturnSuccess,100);pump(c);
        CHECK(c.usbError_==kIOReturnSuccess);
        CHECK(c.running_&&c.rx.clearCalls==1&&c.rx.ioCalls==8&&c.ledger_.pending==15);
        CHECK(c.rxRecovery_.attempts==1&&c.rxRecovery_.completed==1&&c.rxRecovery_.consecutive==1);
        CHECK(c.txPackets_==1&&c.tx.ioCalls==1&&c.txErrors_==0);
        c.rx.requests[4].memory->bytes[0]=1;c.rx.requests[5].memory->bytes[0]=2;
        c.rx.finish(5,kIOReturnSuccess,24);pump(c);CHECK(c.receivedCalls==0);
        c.rx.finish(4,kIOReturnSuccess,24);pump(c);
        CHECK((c.received==std::vector<unsigned>{1,2}));CHECK(c.rxRecovery_.consecutive==0);
        CHECK(c.txPackets_==1&&c.tx.ioCalls==1);
        cleanup(c);
        RTWUSBController d;d.family_=family;prime(d);d.rx.deferAbort=true;
        d.rx.finish(2,kIOReturnNotResponding,0);pump(d);CHECK(d.rxRecovery_.pending);
        CHECK(d.submit(4,&d.tx,100)==0);d.tx.finish(0,kIOReturnSuccess,100);pump(d);
        CHECK(d.txPackets_==1&&d.tx.ioCalls==1&&d.rx.ioCalls==4);
        d.rx.deferAbort=false;d.abortTransfers();
        CHECK(!d.rxRecovery_.pending&&d.rxRecovery_.consecutive==1&&d.rxRecovery_.attempts==1&&d.usbError_==0);
        cleanup(d);
    }
    for(auto family:{RTW_WIFI_UNKNOWN,RTW_WIFI_RTL8814BU}){RTWUSBController c;c.family_=family;prime(c);
        c.rx.finish(0,kIOReturnNotResponding,0);pump(c);
        CHECK(c.usbError_==kIOReturnNotResponding&&c.rx.clearCalls==0&&c.rxRecovery_.attempts==0);
        cleanup(c);
    }
}
static void recoveryKeepsRadioAndDrainsBeforeRefill(){
    RTWUSBController c;prime(c);
    c.rx.onClear=[&]{CHECK(!c.memory_[0].prepared);CHECK(c.memory_[0].completes==1);CHECK(c.rx.ioCalls==4);};
    c.rx.onIO=[&]{if(c.rx.ioCalls==5){for(unsigned i=0;i<4;++i){CHECK(c.rx.requests[i].done);CHECK(c.memory_[i].completes==1);}}};
    c.rx.finish(0,kIOReturnNotResponding,0);pump(c);
    CHECK(c.usbError_==kIOReturnSuccess);CHECK(c.running_&&c.quiesces==0);
    CHECK(c.rx.clearCalls==1&&c.rx.withRequest);CHECK(c.receivedCalls==0);
    CHECK(c.rx.ioCalls==8&&c.references==5);CHECK(c.ledger_.pending==15);
    CHECK(c.rxRecovery_.attempts==1&&c.rxRecovery_.completed==1&&!c.rxRecovery_.pending);
    CHECK(c.rxRecovery_.errors==1&&c.rxErrors_==1&&c.rxRecovery_.consecutive==1);
    cleanup(c);
}
static void waitsForEveryOldCallbackWithoutDeliveringOldBytes(){
    RTWUSBController c;prime(c);c.rx.deferAbort=true;
    c.rx.finish(0,kIOReturnNotResponding,0);pump(c);
    CHECK(c.rx.clearCalls==1&&c.rx.ioCalls==4&&c.rxRecovery_.pending);
    CHECK(c.submit(0,c.rxPipe_,c.RXCapacity)!=0);
    c.rx.finish(3,kIOReturnSuccess,24);c.drainCompletions(true);
    CHECK(c.receivedCalls==0&&c.rx.ioCalls==4);
    c.rx.finish(2,kIOReturnAborted,0);c.drainCompletions(true);
    CHECK(c.rx.ioCalls==4);
    c.rx.finish(1,kIOReturnAborted,0);pump(c);
    CHECK(c.rx.ioCalls==8&&c.rxRecovery_.completed==1&&c.rxErrors_==1);
    CHECK(c.receivedCalls==0&&c.references==5);cleanup(c);
}
static void transactionErrorBehindOldestHoleStartsRecovery(){
    RTWUSBController c;prime(c);c.rx.deferAbort=true;
    c.rx.finish(2,kIOReturnNotResponding,0);c.drainCompletions(true);
    CHECK(c.rx.clearCalls==1&&c.rxRecovery_.pending);
    CHECK(c.rx.ioCalls==4&&c.receivedCalls==0&&c.usbError_==0);
    cleanup(c);
}
static void lateCallbacksCannotCompleteNewGenerations(){
    RTWUSBController c;prime(c);c.rx.finish(0,kIOReturnNotResponding,0);pump(c);
    c.rx.duplicate(0,kIOReturnNotResponding,0);c.rx.duplicate(2,kIOReturnSuccess,24);pump(c);
    CHECK(c.rxRecovery_.attempts==1&&c.rxRecovery_.errors==1&&c.rx.ioCalls==8);
    CHECK(c.references==5&&c.receivedCalls==0&&c.ledger_.completed==0);cleanup(c);
}
static void newGenerationRetainsSubmissionOrder(){
    RTWUSBController c;prime(c);c.rx.finish(0,kIOReturnNotResponding,0);pump(c);
    c.rx.requests[4].memory->bytes[0]=1;c.rx.requests[5].memory->bytes[0]=2;
    c.rx.finish(5,kIOReturnSuccess,24);c.drainCompletions(true);CHECK(c.receivedCalls==0);
    c.rx.finish(4,kIOReturnSuccess,24);c.drainCompletions(true);
    CHECK((c.received==std::vector<unsigned>{1,2}));CHECK(c.rxRecovery_.consecutive==0);cleanup(c);
}
static void retryBudgetIsFiniteAndUsefulReceiveResetsIt(){
    RTWUSBController c;prime(c);
    for(unsigned n=1;n<=3;++n){completeCurrent(c,kIOReturnNotResponding,0);pump(c);CHECK(c.rxRecovery_.attempts==n&&c.rxRecovery_.consecutive==n);CHECK(c.usbError_==0);}
    completeCurrent(c,kIOReturnNotResponding,0);pump(c);
    CHECK(c.usbError_==kIOReturnNotResponding&&c.quiesces==1&&c.rx.clearCalls==3);
    CHECK(c.rxRecovery_.exhausted==1&&c.rxRecovery_.failures==1&&c.rxRecovery_.errors==4);cleanup(c);
    RTWUSBController d;prime(d);completeCurrent(d,kIOReturnNotResponding,0);pump(d);
    d.usefulReceive=false;completeCurrent(d,kIOReturnSuccess,24);pump(d);CHECK(d.rxRecovery_.consecutive==1);
    d.usefulReceive=true;completeCurrent(d,kIOReturnSuccess,24);pump(d);CHECK(d.rxRecovery_.consecutive==0);
    cleanup(d);
}
static void watchdogExpiresDelayedAbortAtTwoSeconds(){
    RTWUSBController::clockMS=100;RTWUSBController c;prime(c);c.rx.deferAbort=true;
    c.rx.finish(0,kIOReturnNotResponding,0);pump(c);
    RTWUSBController::clockMS=2099;RTWUSBController::watchdogAction(&c,nullptr);
    CHECK(c.usbError_==0&&c.rxRecovery_.pending&&c.rx.clearCalls==1&&c.timer.lastDelay==100);
    RTWUSBController::clockMS=2100;RTWUSBController::watchdogAction(&c,nullptr);
    CHECK(c.usbError_==kIOReturnNotResponding&&!c.rxRecovery_.pending);
    CHECK(c.rxRecovery_.failures==1&&c.quiesces==1&&c.rx.ioCalls==4);cleanup(c);
}
static void clearAndRefillFailuresPreserveOriginalError(){
    RTWUSBController c;prime(c);c.rx.clearError=kIOReturnIOError;
    c.rx.finish(0,kIOReturnNotResponding,0);pump(c);
    CHECK(c.usbError_==kIOReturnNotResponding&&c.rxRecovery_.failures==1&&c.quiesces==1);cleanup(c);
    for(unsigned call=5;call<=8;++call){RTWUSBController d;prime(d);d.rx.failIOCall=call;
        d.rx.finish(0,kIOReturnNotResponding,0);pump(d);
        CHECK(d.usbError_==kIOReturnNotResponding&&d.rxRecovery_.failures==1&&d.quiesces==1);cleanup(d);
    }
    RTWUSBController::clockMS=100;RTWUSBController e;prime(e);
    e.rx.onClear=[] {RTWUSBController::clockMS=2100;};
    e.rx.finish(0,kIOReturnNotResponding,0);pump(e);
    CHECK(e.usbError_==kIOReturnNotResponding&&e.rxRecovery_.failures==1&&e.rx.ioCalls==4);cleanup(e);
}
static void fatalAndInactivePathsNeverRecover(){
    for(unsigned reason=0;reason<6;++reason){RTWUSBController c;prime(c);
        if(reason==0)c.family_=RTW_WIFI_UNKNOWN;
        if(reason==1)c.sleeping_=true;
        if(reason==2)c.present_=false;
        if(reason==3)c.stopping_=true;
        if(reason==4)c.usbError_=kIOReturnNoDevice;
        if(reason==5)c.closing_=true;
        c.rx.finish(0,kIOReturnNotResponding,0);pump(c);CHECK(c.rx.clearCalls==0);cleanup(c);
    }
    RTWUSBController c;prime(c);CHECK(c.submit(4,&c.tx,100)==0);
    c.rx.finish(0,kIOReturnNotResponding,0);c.tx.finish(0,kIOReturnIOError,0);pump(c);
    CHECK(c.rx.clearCalls==0&&c.usbError_==kIOReturnIOError&&c.txErrors_==1);cleanup(c);
    RTWUSBController d;prime(d);d.rx.finish(0,kIOReturnNotResponding,24);pump(d);
    CHECK(d.rx.clearCalls==0&&d.usbError_==kIOReturnNotResponding&&d.receivedCalls==0);cleanup(d);
}
static void diagnosticsPublishOnlyScalarRecoveryProgress(){
    RTWUSBController c;prime(c);c.rx.finish(0,kIOReturnNotResponding,0);pump(c);
    c.publishRXRecoveryDiagnostics();
    CHECK(c.diagnostics["RTWDiagRXRecoveryErrors"]==1);
    CHECK(c.diagnostics["RTWDiagRXRecoveryAttempts"]==1);
    CHECK(c.diagnostics["RTWDiagRXRecoveryCompleted"]==1);
    CHECK(c.diagnostics["RTWDiagRXRecoveryFailures"]==0);
    CHECK(c.diagnostics["RTWDiagRXRecoveryExhausted"]==0);
    CHECK(c.diagnostics["RTWDiagRXRecoveryPending"]==0);
    CHECK(c.diagnostics["RTWDiagRXRecoveryConsecutive"]==1);
    CHECK(c.diagnostics["RTWDiagRXRecoveryEndpoint"]==0x81);
    CHECK(c.diagnostics["RTWDiagRXRecoveryOriginalError"]==(uint32_t)kIOReturnNotResponding);
    CHECK(c.diagnostics["RTWDiagRXRecoveryLastError"]==(uint32_t)kIOReturnNotResponding);
    cleanup(c);
}
static void recoveryDoesNotHideOtherRXErrors(){
    for(bool partial:{false,true}){
        RTWUSBController c;prime(c);
        c.rx.finish(0,kIOReturnNotResponding,0);
        c.rx.finish(1,partial?kIOReturnNotResponding:kIOReturnIOError,partial?24:0);
        pump(c);
        CHECK(c.usbError_==(partial?kIOReturnNotResponding:kIOReturnIOError));
        CHECK(c.quiesces==1&&c.rx.clearCalls==0&&c.receivedCalls==0);
        cleanup(c);
    }
}
static void slowRefillCannotEscapeDeadline(){
    RTWUSBController::clockMS=100;RTWUSBController c;prime(c);
    c.rx.onIO=[&]{if(c.rx.ioCalls==5)RTWUSBController::clockMS=2100;};
    c.rx.finish(0,kIOReturnNotResponding,0);pump(c);
    CHECK(c.usbError_==kIOReturnNotResponding&&c.quiesces==1);
    CHECK(c.rxRecovery_.completed==0&&c.rxRecovery_.failures==1);
    cleanup(c);
}
static void stopCancelsRecoveryWithoutResettingBudget(){
    RTWUSBController c;prime(c);c.rx.deferAbort=true;
    c.rx.finish(0,kIOReturnNotResponding,0);pump(c);CHECK(c.rxRecovery_.pending);
    c.rx.deferAbort=false;c.abortTransfers();CHECK(!c.rxRecovery_.pending&&c.references==1);
    CHECK(c.rxRecovery_.consecutive==1&&c.rxRecovery_.attempts==1&&c.usbError_==0);cleanup(c);
}
static void transientClearRetriesAreTimedAndKeepOneRecoveryBudget(){
    RTWUSBController::clockMS=100;RTWUSBController c;prime(c);
    c.rx.clearResults={kIOReturnNotResponding,kIOReturnNotResponding,kIOReturnSuccess};
    c.rx.finish(0,kIOReturnNotResponding,0);pump(c);
    CHECK(c.running_&&c.usbError_==0&&c.rxRecovery_.pending&&c.rx.clearCalls==1);
    CHECK(c.rx.ioCalls==4&&c.receivedCalls==0);
    c.drainCompletions(true);RTWUSBController::clockMS=199;RTWUSBController::watchdogAction(&c,nullptr);pump(c);
    CHECK(c.rx.clearCalls==1&&c.rx.ioCalls==4);
    RTWUSBController::clockMS=200;RTWUSBController::watchdogAction(&c,nullptr);pump(c);
    CHECK(c.rx.clearCalls==2&&c.rxRecovery_.pending&&c.usbError_==0);
    RTWUSBController::clockMS=299;c.drainCompletions(true);CHECK(c.rx.clearCalls==2);
    RTWUSBController::clockMS=300;RTWUSBController::watchdogAction(&c,nullptr);pump(c);
    CHECK(c.running_&&!c.rxRecovery_.pending&&c.usbError_==0&&c.ledger_.pending==15);
    CHECK(c.rxRecovery_.attempts==1&&c.rxRecovery_.consecutive==1&&c.rxRecovery_.completed==1);
    c.publishRXRecoveryDiagnostics();
    CHECK(c.diagnostics["RTWDiagRXRecoveryClearAttempts"]==3);
    CHECK(c.diagnostics["RTWDiagRXRecoveryClearFailures"]==2);
    CHECK(c.diagnostics["RTWDiagRXRecoveryTransportRetries"]==2);
    CHECK(c.diagnostics["RTWDiagRXRecoveryClearResult"]==(uint32_t)kIOReturnSuccess);
    CHECK(c.diagnostics["RTWDiagRXRecoveryStage"]==5);
    cleanup(c);
}
static void transientClearExhaustionAndDeadlineRemainFinite(){
    RTWUSBController::clockMS=100;RTWUSBController c;prime(c);c.rx.clearError=kIOReturnNotResponding;
    c.rx.finish(0,kIOReturnNotResponding,0);pump(c);
    for(unsigned ms:{200U,300U}){RTWUSBController::clockMS=ms;RTWUSBController::watchdogAction(&c,nullptr);pump(c);}
    CHECK(!c.running_&&c.usbError_==kIOReturnNotResponding&&c.rx.clearCalls==3&&c.rx.ioCalls==4);
    CHECK(c.rxRecovery_.failures==1&&c.rxRecovery_.attempts==1&&c.rxRecovery_.completed==0);
    c.publishRXRecoveryDiagnostics();CHECK(c.diagnostics["RTWDiagRXRecoveryTransportExhausted"]==1);
    CHECK(c.diagnostics["RTWDiagRXRecoveryFailureStage"]==1);cleanup(c);
    RTWUSBController::clockMS=100;RTWUSBController d;prime(d);d.rx.clearError=kIOReturnNotResponding;
    d.rx.onClear=[] {RTWUSBController::clockMS=2050;};
    d.rx.finish(0,kIOReturnNotResponding,0);pump(d);CHECK(d.rxRecovery_.pending);
    RTWUSBController::clockMS=2100;RTWUSBController::watchdogAction(&d,nullptr);pump(d);
    CHECK(!d.running_&&d.rx.clearCalls==1&&d.rxRecovery_.lastError==kIOReturnTimeout);
    d.publishRXRecoveryDiagnostics();CHECK(d.diagnostics["RTWDiagRXRecoveryFailureStage"]==2);cleanup(d);
}
static void partialRefillIsClearedAndDrainedBeforeRetry(){
    RTWUSBController::clockMS=100;RTWUSBController c;prime(c);
    c.rx.ioResults[6]=kIOReturnNotResponding;
    c.rx.finish(0,kIOReturnNotResponding,0);pump(c);
    CHECK(c.running_&&c.usbError_==0&&c.rxRecovery_.pending&&c.rx.ioCalls==6);
    CHECK(c.rxRecovery_.completed==0&&c.rx.clearCalls==1&&c.receivedCalls==0);
    CHECK(c.ledger_.pending==1&&c.references==2);
    c.rx.requests[4].memory->bytes[0]=77;c.rx.finish(4,kIOReturnSuccess,24);pump(c);
    CHECK(c.receivedCalls==0&&c.rx.ioCalls==6&&c.ledger_.pending==0);
    RTWUSBController::clockMS=200;RTWUSBController::watchdogAction(&c,nullptr);pump(c);
    CHECK(c.rx.clearCalls==2&&c.rx.ioCalls==10&&c.rxRecovery_.completed==1&&c.ledger_.pending==15);
    CHECK(c.rxRecovery_.attempts==1&&c.rxRecovery_.consecutive==1&&c.receivedCalls==0);
    c.publishRXRecoveryDiagnostics();CHECK(c.diagnostics["RTWDiagRXRecoveryRefillFailures"]==1);
    CHECK(c.diagnostics["RTWDiagRXRecoveryRefillResult"]==(uint32_t)kIOReturnSuccess);cleanup(c);
    RTWUSBController::clockMS=100;RTWUSBController d;prime(d);d.rx.ioResults[7]=kIOReturnNotResponding;
    d.rx.finish(0,kIOReturnNotResponding,0);pump(d);CHECK(d.rxRecovery_.pending&&d.ledger_.pending==3);
    d.rx.deferAbort=true;RTWUSBController::clockMS=200;RTWUSBController::watchdogAction(&d,nullptr);pump(d);
    CHECK(d.rx.clearCalls==2&&d.rx.ioCalls==7&&d.rxRecovery_.pending);
    d.publishRXRecoveryDiagnostics();CHECK(d.diagnostics["RTWDiagRXRecoveryStage"]==3);
    d.rx.finish(5,kIOReturnAborted,0);pump(d);CHECK(d.rx.ioCalls==7);
    d.rx.finish(4,kIOReturnAborted,0);pump(d);
    CHECK(d.rx.ioCalls==11&&d.rxRecovery_.completed==1&&d.receivedCalls==0);cleanup(d);
}
static void repeatedPartialRefillFailureExhaustsClearAttempts(){
    RTWUSBController::clockMS=100;RTWUSBController c;prime(c);
    for(unsigned call:{6U,8U,10U})c.rx.ioResults[call]=kIOReturnNotResponding;
    c.rx.finish(0,kIOReturnNotResponding,0);pump(c);
    for(unsigned ms:{200U,300U}){RTWUSBController::clockMS=ms;RTWUSBController::watchdogAction(&c,nullptr);pump(c);}
    CHECK(!c.running_&&c.usbError_==kIOReturnNotResponding&&c.rx.clearCalls==3);
    CHECK(c.rx.ioCalls==10&&c.rxRecovery_.completed==0&&c.rxRecovery_.failures==1&&c.receivedCalls==0);
    c.publishRXRecoveryDiagnostics();CHECK(c.diagnostics["RTWDiagRXRecoveryFailureStage"]==4);
    CHECK(c.diagnostics["RTWDiagRXRecoveryRefillSlot"]==1);
    CHECK(c.diagnostics["RTWDiagRXRecoveryRefillResult"]==(uint32_t)kIOReturnNotResponding);
    cleanup(c);
}
static void haltedPipeDuringOrdinaryRearmStillRecovers(){
    RTWUSBController::clockMS=100;RTWUSBController c;prime(c);bool injected=false;
    c.rx.onIO=[&]{if(!injected&&c.rx.ioCalls==5){injected=true;c.rx.finish(1,kIOReturnNotResponding,0);}};
    c.rx.finish(0,kIOReturnSuccess,0);pump(c);
    CHECK(c.running_&&c.usbError_==0&&c.rxRecovery_.attempts==1&&c.rxRecovery_.completed==1);
    CHECK(c.rx.clearCalls==1&&c.rx.ioCalls==9&&c.ledger_.pending==15&&c.receivedCalls==0);cleanup(c);
    RTWUSBController d;prime(d);d.memory_[0].prepareError=kIOReturnNotResponding;
    d.rx.finish(0,kIOReturnSuccess,0);pump(d);
    CHECK(!d.running_&&d.usbError_==kIOReturnNotResponding&&d.rx.clearCalls==0&&d.rxRecovery_.attempts==0);cleanup(d);
}
static void cancelAndUnplugNeverPerformQueuedRetry(){
    for(bool unplug:{false,true}){
        RTWUSBController::clockMS=100;RTWUSBController c;prime(c);c.rx.clearError=kIOReturnNotResponding;
        c.rx.finish(0,kIOReturnNotResponding,0);pump(c);CHECK(c.rxRecovery_.pending);
        if(unplug){c.present_=false;c.closing_=true;}
        c.abortTransfers();RTWUSBController::clockMS=500;RTWUSBController::watchdogAction(&c,nullptr);pump(c);
        CHECK(c.rx.clearCalls==1&&c.rx.ioCalls==4&&c.references==1&&!c.rxRecovery_.pending);
        c.publishRXRecoveryDiagnostics();CHECK(c.diagnostics["RTWDiagRXRecoveryStage"]==7);cleanup(c);
    }
}
static void drainFailureKeepsItsActualErrorAndStage(){
    RTWUSBController::clockMS=100;RTWUSBController c;prime(c);c.rx.deferAbort=true;
    c.rx.finish(0,kIOReturnNotResponding,0);pump(c);CHECK(c.rxRecovery_.pending);
    c.rx.finish(1,kIOReturnIOError,0);pump(c);
    CHECK(!c.running_&&c.usbError_==kIOReturnIOError&&c.rxRecovery_.failures==1);
    c.publishRXRecoveryDiagnostics();CHECK(c.diagnostics["RTWDiagRXRecoveryFailureStage"]==3);
    CHECK(c.diagnostics["RTWDiagRXRecoveryStage"]==6);cleanup(c);
}
static void usefulReceiveBeforeSecondClearFailureResetsOnlyEpochBudget(){
    RTWUSBController::clockMS=100;RTWUSBController c;prime(c);
    c.rx.finish(0,kIOReturnNotResponding,0);pump(c);CHECK(c.rxRecovery_.completed==1);
    completeCurrent(c,kIOReturnSuccess,24);pump(c);CHECK(c.rxRecovery_.consecutive==0);
    c.rx.clearResults={kIOReturnSuccess,kIOReturnNotResponding,kIOReturnSuccess};
    completeCurrent(c,kIOReturnNotResponding,0);pump(c);
    CHECK(c.running_&&c.rxRecovery_.pending&&c.rxRecovery_.attempts==2&&c.rxRecovery_.consecutive==1);
    RTWUSBController::clockMS=200;RTWUSBController::watchdogAction(&c,nullptr);pump(c);
    CHECK(c.running_&&!c.rxRecovery_.pending&&c.rxRecovery_.completed==2&&c.rxRecovery_.failures==0);
    c.publishRXRecoveryDiagnostics();CHECK(c.diagnostics["RTWDiagRXRecoveryClearAttempts"]==3);
    CHECK(c.diagnostics["RTWDiagRXRecoveryClearAttempt"]==2);cleanup(c);
}
static void hardwareTimersWaitDuringRecoveryAndResumeAfterIt(){
    for(auto family:{RTW_WIFI_RTL8192EU,RTW_WIFI_RTL8814AU,RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821CU,
                     RTW_WIFI_RTL8192DU,RTW_WIFI_RTL8192FU,RTW_WIFI_RTL8710BU,RTW_WIFI_RTL8723BU}){
        RTWUSBController c;c.family_=family;c.rxRecovery_.pending=true;
        RTWUSBController::scanAction(&c,nullptr);RTWUSBController::calibAction(&c,nullptr);
        CHECK(c.core_.timerHardwareCalls==0&&c.acCore_.timerHardwareCalls==0&&c.familyTimerCalls==0);
        CHECK(c.timer.lastDelay==100&&c.timer.scheduled==2&&c.running_);
        c.rxRecovery_.pending=false;RTWUSBController::scanAction(&c,nullptr);RTWUSBController::calibAction(&c,nullptr);
        CHECK(c.core_.timerHardwareCalls+c.acCore_.timerHardwareCalls+c.familyTimerCalls==2);
        cleanup(c);
    }
}
static void exhaustedOrdinaryRearmDoesNotDeliverLaterOldPayload(){
    RTWUSBController::clockMS=100;RTWUSBController c;prime(c);
    c.rxRecovery_.consecutive=3;c.rx.ioResults[5]=kIOReturnNotResponding;
    c.rx.finish(0,kIOReturnSuccess,0);c.rx.finish(1,kIOReturnSuccess,24);pump(c);
    CHECK(!c.running_&&c.usbError_==kIOReturnNotResponding&&c.rxRecovery_.exhausted==1);
    CHECK(c.receivedCalls==0&&c.rxRecovery_.consecutive==3);cleanup(c);
}
int main(int argc,char**argv){
    if(argc==2){
        const std::pair<const char*,void(*)()> cases[]{
            {"transient-clear",transientClearRetriesAreTimedAndKeepOneRecoveryBudget},
            {"clear-exhaustion-deadline",transientClearExhaustionAndDeadlineRemainFinite},
            {"partial-refill",partialRefillIsClearedAndDrainedBeforeRetry},
            {"refill-exhaustion",repeatedPartialRefillFailureExhaustsClearAttempts},
            {"ordinary-halted-rearm",haltedPipeDuringOrdinaryRearmStillRecovers},
            {"cancel-unplug",cancelAndUnplugNeverPerformQueuedRetry},
            {"drain-failure",drainFailureKeepsItsActualErrorAndStage},
            {"second-clear-failure",usefulReceiveBeforeSecondClearFailureResetsOnlyEpochBudget},
            {"hardware-timers",hardwareTimersWaitDuringRecoveryAndResumeAfterIt},
            {"ordinary-exhaustion-payload",exhaustedOrdinaryRearmDoesNotDeliverLaterOldPayload}
        };
        for(const auto&test:cases)if(!std::strcmp(argv[1],test.first)){test.second();std::printf("Controller RX recovery %s: %u checks passed\n",test.first,checks);return 0;}
        CHECK(false);
    }
    transientClearRetriesAreTimedAndKeepOneRecoveryBudget();transientClearExhaustionAndDeadlineRemainFinite();
    partialRefillIsClearedAndDrainedBeforeRetry();repeatedPartialRefillFailureExhaustsClearAttempts();
    haltedPipeDuringOrdinaryRearmStillRecovers();cancelAndUnplugNeverPerformQueuedRetry();
    drainFailureKeepsItsActualErrorAndStage();usefulReceiveBeforeSecondClearFailureResetsOnlyEpochBudget();
    hardwareTimersWaitDuringRecoveryAndResumeAfterIt();
    exhaustedOrdinaryRearmDoesNotDeliverLaterOldPayload();
    integratedFamiliesRecoverAndUnsupportedFamiliesStayFatal();
    recoveryKeepsRadioAndDrainsBeforeRefill();waitsForEveryOldCallbackWithoutDeliveringOldBytes();transactionErrorBehindOldestHoleStartsRecovery();
    lateCallbacksCannotCompleteNewGenerations();newGenerationRetainsSubmissionOrder();retryBudgetIsFiniteAndUsefulReceiveResetsIt();
    watchdogExpiresDelayedAbortAtTwoSeconds();clearAndRefillFailuresPreserveOriginalError();fatalAndInactivePathsNeverRecover();recoveryDoesNotHideOtherRXErrors();slowRefillCannotEscapeDeadline();stopCancelsRecoveryWithoutResettingBudget();diagnosticsPublishOnlyScalarRecoveryProgress();
    std::printf("Controller RX recovery: %u checks passed\n",checks);
}
