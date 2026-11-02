// SPDX-License-Identifier: GPL-2.0-only
#include "kext_resource_host.hpp"
#include <iostream>

static unsigned failures=0;
#define CHECK(x) do {if(!(x)){std::cerr<<__LINE__<<": " #x "\n";++failures;}}while(0)

// Releasing a controller's WorkLoop lease before its optional coex timer
// cleanup leaves that timer owned by the sibling's surviving WorkLoop.
static void detachedControllerLeavesSiblingEventsOnly() {
    auto*loop=new IOWorkLoop;
    auto*sibling=new IOEventSource;
    loop->addEventSource(sibling);
    for(unsigned cycle=0;cycle<10000;++cycle) {
        RTWUSBController c;loop->retain();c.loop_=loop;
        c.gate_=new IOCommandGate;c.completionEvent_=new IOInterruptEventSource;
        c.scanTimer_=new IOTimerEventSource;c.calibTimer_=new IOTimerEventSource;
        c.watchdogTimer_=new IOTimerEventSource;c.r88CoexTimer_=new IOTimerEventSource;
        for(auto*event:{c.gate_,c.completionEvent_,c.scanTimer_,c.calibTimer_,c.watchdogTimer_,c.r88CoexTimer_})
            loop->addEventSource(event);
        c.r88CoexTimer_->scheduled=true;
        c.r88Storage_=calloc(1,c.R88StorageBytes);c.r88BoardKnown_=true;
        c.releasePlatform();
        if(loop->events.size()!=1) {
            CHECK(loop->events.size()==1);
            // Keep the expected RED run free of unrelated host leak reports.
            auto leaked=loop->events;
            for(auto*event:leaked)if(event!=sibling)loop->removeEventSource(event);
            break;
        }
        CHECK(loop->events.front()==sibling&&sibling->enabled);
        CHECK(!c.r88CoexTimer_&&!c.r88Storage_&&!c.r88BoardKnown_&&!c.loop_);
        c.releasePlatform();
        CHECK(loop->events.size()==1&&resourceLiveEvents==1);
    }
    loop->removeEventSource(sibling);sibling->release();loop->release();
    CHECK(resourceLiveEvents==0);
}

static void transferBuffersAreBoundedAndReusedForEveryFamily() {
    const rtw_wifi_family families[]={RTW_WIFI_RTL8192CU,RTW_WIFI_RTL8188EU,RTW_WIFI_RTL8192EU,
        RTW_WIFI_RTL8814AU,RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU,RTW_WIFI_RTL8192DU,
        RTW_WIFI_RTL8192FU,RTW_WIFI_RTL8710BU,RTW_WIFI_RTL8723BU,
        RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU};
    for(auto family:families) {
        RTWUSBController c;c.family_=family;resourceAllocationCalls=0;
        CHECK(c.allocateTransfers()==0);
        CHECK(resourceLiveBuffers==16);
        CHECK(resourceBufferBytes==(family==RTW_WIFI_RTL8814AU?183296U:117760U));
        for(unsigned i=0;i<100;++i)CHECK(c.allocateTransfers()==0);
        CHECK(resourceAllocationCalls==16);
        CHECK(rtw_transfer_begin(&c.ledger_,0));c.transfers_[0].prepared=true;
        c.freeTransfers();CHECK(resourceLiveBuffers==16);
        CHECK(rtw_transfer_complete(&c.ledger_,0));CHECK(rtw_transfer_take(&c.ledger_,0));
        c.freeTransfers();CHECK(resourceLiveBuffers==0&&resourceBufferBytes==0);
        c.releasePlatform();
    }
}

static void partialAllocationFailuresReleaseEveryOwnedBuffer() {
    for(unsigned failure=1;failure<=16;++failure) {
        RTWUSBController c;resourceAllocationCalls=0;resourceFailAllocationAt=failure;
        CHECK(c.allocateTransfers()==ENOMEM);
        CHECK(resourceLiveBuffers==0&&resourceBufferBytes==0);
        for(const auto&transfer:c.transfers_)CHECK(!transfer.memory);
        resourceFailAllocationAt=0;
        CHECK(c.allocateTransfers()==0);
        c.releasePlatform();CHECK(resourceLiveBuffers==0&&resourceBufferBytes==0);
    }
}

static void pendingCallbacksKeepResourcesUntilDrained() {
    RTWUSBController c;auto*loop=new IOWorkLoop;loop->retain();c.loop_=loop;
    c.r88CoexTimer_=new IOTimerEventSource;loop->addEventSource(c.r88CoexTimer_);
    CHECK(c.allocateTransfers()==0);c.retainedForPending_=true;
    const auto releases=resourceRuntimeReleases;
    c.releasePlatform();
    CHECK(c.loop_==loop&&loop->events.size()==1&&resourceLiveBuffers==16);
    CHECK(resourceRuntimeReleases==releases);
    c.retainedForPending_=false;c.releasePlatform();
    CHECK(loop->events.empty()&&resourceLiveBuffers==0);
    // Also clean the pre-fix detached timer in the RED run.
    auto leaked=loop->events;for(auto*event:leaked)loop->removeEventSource(event);
    loop->release();
}

int main() {
    detachedControllerLeavesSiblingEventsOnly();
    transferBuffersAreBoundedAndReusedForEveryFamily();
    partialAllocationFailuresReleaseEveryOwnedBuffer();
    pendingCallbacksKeepResourcesUntilDrained();
    if(failures)return 1;
    std::cout<<"kext resources: actual teardown, 10000 sibling detach cycles, 13 family transfer bounds, allocation rollback and pending callback ownership passed\n";
}
