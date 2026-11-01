// SPDX-License-Identifier: GPL-2.0-only
#include "deferred_cleanup_host.hpp"

static void finish(RTWUSBController& c,unsigned slot) {
    assert(rtw_transfer_complete(&c.ledger_,slot));
    assert(rtw_transfer_take(&c.ledger_,slot));
}
int main() {
    {
        RTWUSBController c;
        c.shutdownPlatform();c.releasePlatform();c.markTeardownReady();
        assert(c.references==1&&c.platformReleases==1&&!c.call.pending);
        assert(!c.attached_&&!c.macReady_&&c.core_.detaches==1);
    }
    {
        RTWUSBController c;
        assert(rtw_transfer_begin(&c.ledger_,3));
        c.shutdownPlatform();c.shutdownPlatform();
        assert(c.retainedForPending_&&c.references==2&&c.attached_);
        c.releasePlatform();assert(c.platformReleases==0);
        finish(c,3);
        c.scheduleDeferredCleanup();
        assert(!c.call.pending); // super::stop must have finished before release.
        c.markTeardownReady();c.scheduleDeferredCleanup();c.markTeardownReady();
        assert(c.call.pending&&c.call.enqueues==1);
        c.runWorker();
        assert(c.references==1&&c.platformReleases==1&&!c.retainedForPending_);
        assert(!c.attached_&&c.core_.detaches==1);
        assert(c.gate.references==1&&c.loop.references==1);
    }
    {
        RTWUSBController c;
        assert(rtw_transfer_begin(&c.ledger_,0));
        assert(rtw_transfer_begin(&c.ledger_,5));
        c.shutdownPlatform();c.markTeardownReady();
        assert(!c.call.pending);
        finish(c,5);c.scheduleDeferredCleanup();assert(!c.call.pending);
        finish(c,0);c.scheduleDeferredCleanup();assert(c.call.pending);
        c.runWorker();assert(c.references==1&&c.platformReleases==1);
    }
    {
        RTWUSBController c;
        assert(rtw_transfer_begin(&c.ledger_,1));
        c.shutdownPlatform();c.markTeardownReady();
        for(unsigned n=0;n<1000;++n)c.scheduleDeferredCleanup();
        // A USB callback that never arrives must not permit freeing live DMA.
        assert(!c.call.pending&&c.platformReleases==0&&c.references==2);
        finish(c,1);c.scheduleDeferredCleanup();c.runWorker();
        assert(c.references==1&&c.platformReleases==1);
    }
    for(unsigned n=0;n<10000;++n) {
        RTWUSBController c;
        const unsigned slot=n%RTW_TRANSFER_SLOTS;
        assert(rtw_transfer_begin(&c.ledger_,slot));
        c.shutdownPlatform();
        if(n%2)c.markTeardownReady();
        finish(c,slot);c.scheduleDeferredCleanup();
        if(!(n%2))c.markTeardownReady();
        c.runWorker();c.scheduleDeferredCleanup();
        assert(c.references==1&&c.platformReleases==1&&c.call.enqueues==1);
        assert(c.gate.references==1&&c.loop.references==1);
    }
    puts("deferred-cleanup: actual controller methods, delayed callbacks, one-shot teardown and 10000 balanced cycles passed");
}
