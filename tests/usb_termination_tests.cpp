// SPDX-License-Identifier: GPL-2.0-only
#include "usb_termination_host.hpp"
#include <cstdio>
#include <thread>
#include <vector>

static void assertBalanced(const RTWUSBController &driver,const IOUSBHostInterface &usb){
    assert(driver.references==1);
    assert(driver.gate.references==1);
    assert(driver.loop.references==1);
    assert(usb.references==1);
}
int main(){
    for(auto kind:{kIOMessageServiceIsTerminated,kIOMessageServiceIsRequestingClose})for(bool synchronous:{false,true}){
        RTWUSBController c;IOUSBHostInterface primary;IOService extra;c.connect(&primary);primary.synchronousStop=synchronous;c.duExtra_=&extra;
        assert(c.message(kind,&extra,nullptr)==kIOReturnSuccess);
        assert(c.quiesces==1&&primary.closeCalls==1&&c.duExtraCloses==1&&!c.duExtra_);
        c.message(kind,&extra,nullptr);assert(c.duExtraCloses==1);
        c.releasePlatform();assertBalanced(c,primary);
    }

    for(UInt32 kind:{kIOMessageServiceIsTerminated,kIOMessageServiceIsRequestingClose}){
        RTWUSBController controller;IOUSBHostInterface usb;
        controller.connect(&usb);
        assert(controller.message(kind,&usb,nullptr)==kIOReturnSuccess);
        assert(usb.closeCalls==1&&controller.quiesces==1);
        assert(controller.stopped&&controller.platformReleased);
        assert(controller.superMessages==1);
        controller.message(kind,&usb,nullptr);
        assert(usb.closeCalls==1&&controller.superMessages==2);
        assertBalanced(controller,usb);
    }
    {
        RTWUSBController controller;IOUSBHostInterface usb,other;
        controller.connect(&usb);
        assert(controller.message(kIOMessageServiceIsTerminated,&other,nullptr)==kIOReturnSuccess);
        assert(controller.message(999,&usb,nullptr)==kIOReturnSuccess);
        assert(controller.superMessages==2&&controller.quiesces==0&&usb.closeCalls==0);
        controller.message(kIOMessageServiceIsTerminated,&usb,nullptr);
        assert(usb.closeCalls==1&&controller.stopped);
        assertBalanced(controller,usb);
    }
    {
        // Failed / already torn-down workloop must not close from message.
        RTWUSBController controller;IOUSBHostInterface usb;
        controller.connect(&usb);controller.gateAvailable=false;usb.synchronousStop=false;
        controller.message(kIOMessageServiceIsTerminated,&usb,nullptr);
        assert(usb.closeCalls==0&&controller.quiesces==0);
        controller.releasePlatform();
        assert(usb.closeCalls==1&&!controller.usb_);
        controller.closeUSBProvider();
        assert(usb.closeCalls==1);assertBalanced(controller,usb);
    }
    {
        RTWUSBController controller;IOUSBHostInterface usb;
        controller.connect(&usb);controller.gate.failNext=true;usb.synchronousStop=false;
        controller.message(kIOMessageServiceIsTerminated,&usb,nullptr);
        assert(usb.closeCalls==0&&controller.quiesces==0);
        controller.releasePlatform();
        assert(usb.closeCalls==1);assertBalanced(controller,usb);
    }
    {
        // Race two provider-close claimants: only one may call close().
        RTWUSBController controller;IOUSBHostInterface usb;
        controller.connect(&usb);usb.synchronousStop=false;
        std::vector<std::thread> workers;
        for(unsigned n=0;n<12;++n)workers.emplace_back([&]{
            for(unsigned i=0;i<1000;++i)controller.closeUSBProvider();
        });
        for(auto &worker:workers)worker.join();
        assert(usb.closeCalls==1&&!controller.usbOpened_);
        controller.releasePlatform();assertBalanced(controller,usb);
    }
    for(unsigned n=0;n<10000;++n){
        RTWUSBController controller;IOUSBHostInterface usb;
        controller.connect(&usb);
        const UInt32 type=n%2?kIOMessageServiceIsRequestingClose:kIOMessageServiceIsTerminated;
        assert(controller.message(type,&usb,nullptr)==kIOReturnSuccess);
        assert(usb.closeCalls==1&&controller.platformReleased&&controller.stopped);
        assertBalanced(controller,usb);
    }
    puts("usb-termination: extracted message/close methods; reentrant stop, fail-safe, concurrency and 10000 lifecycles passed");
}
