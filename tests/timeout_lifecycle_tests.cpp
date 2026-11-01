// SPDX-License-Identifier: GPL-2.0-only
#include "platform.hpp"
#include <sys/CTimeout.hpp>
#include <cstdio>
static unsigned fired=0;
static void count(void*){++fired;}
static void freeSelf(void* value){
    auto** timeout=static_cast<CTimeout**>(value);++fired;
    assert(CTimeout::timeout_free(nullptr,timeout,nullptr,nullptr,nullptr)==0);
}
static void rearm(void* value){
    auto** timeout=static_cast<CTimeout**>(value);++fired;
    int delay=1;
    assert(CTimeout::timeout_add_msec(nullptr,timeout,(*timeout)->tm->getWorkLoop(),&delay,nullptr)==0);
}
int main(){
    IOWorkLoop* loop=new IOWorkLoop;CTimeout* timer=nullptr;
    int delay=10;
    assert(CTimeout::timeout_set(nullptr,nullptr,(void*)count,nullptr,nullptr)==kIOReturnBadArgument);
    failInit=true;
    assert(CTimeout::timeout_set(nullptr,&timer,(void*)count,nullptr,nullptr)==kIOReturnNoMemory);
    assert(!timer&&liveObjects==1);failInit=false;
    assert(CTimeout::timeout_set(nullptr,&timer,(void*)count,nullptr,nullptr)==0);
    assert(timer&&!timer->tm&&!timer->isPending);
    failTimer=true;
    assert(CTimeout::timeout_add_msec(nullptr,&timer,loop,&delay,nullptr)==kIOReturnNoMemory);
    assert(liveObjects==2);failTimer=false;failAdd=true;
    assert(CTimeout::timeout_add_msec(nullptr,&timer,loop,&delay,nullptr)==kIOReturnNoResources);
    assert(!timer->tm&&liveObjects==2);failAdd=false;failArm=true;
    assert(CTimeout::timeout_add_msec(nullptr,&timer,loop,&delay,nullptr)==kIOReturnNoResources);
    assert(!timer->isPending);failArm=false;
    assert(CTimeout::timeout_add_msec(nullptr,&timer,loop,&delay,nullptr)==0);
    assert(timer->isPending);loop->fire();assert(fired==1&&!timer->isPending);
    assert(CTimeout::timeout_set(nullptr,&timer,(void*)rearm,&timer,nullptr)==0);
    assert(CTimeout::timeout_add_msec(nullptr,&timer,loop,&delay,nullptr)==0);
    loop->fire();assert(fired==2&&timer->isPending);
    assert(CTimeout::timeout_set(nullptr,&timer,(void*)freeSelf,&timer,nullptr)==0);
    assert(!timer->isPending&&!timer->tm->pending);
    assert(CTimeout::timeout_add_msec(nullptr,&timer,loop,&delay,nullptr)==0);
    loop->fire();assert(fired==3&&!timer&&loop->timers.empty()&&liveObjects==1);
    for(unsigned i=0;i<10000;++i){
        assert(CTimeout::timeout_set(nullptr,&timer,(void*)count,nullptr,nullptr)==0);
        assert(CTimeout::timeout_add_msec(nullptr,&timer,loop,&delay,nullptr)==0);
        assert(CTimeout::timeout_del(nullptr,&timer,nullptr,nullptr,nullptr)==0);
        assert(!timer->isPending&&!timer->tm->pending);
        assert(CTimeout::timeout_free(nullptr,&timer,nullptr,nullptr,nullptr)==0);
        assert(!timer&&liveObjects==1&&loop->timers.empty());
    }
    assert(CTimeout::timeout_free(nullptr,&timer,nullptr,nullptr,nullptr)==0);
    loop->release();assert(liveObjects==0);
    puts("timeout-lifecycle: real source, self-free, rearm, fault injection and 10000 balanced cycles passed");
}
