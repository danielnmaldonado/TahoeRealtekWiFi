#include "net80211_runtime_host/platform.hpp"
#include "../kext/RTWNet80211Globals.hpp"
#include <sys/_task.h>
#include <cstdio>
#include <thread>
#include <vector>
static unsigned checks;
static void require(bool value,const char *message){++checks;if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
static void increment(void *arg){++*static_cast<unsigned*>(arg);}
static void release_inside_task(void *){RTWNet80211Release();require(RTWNet80211WorkLoop!=nullptr,"globals live inside last task callback");}
int main(){
    require(RTWNet80211Acquire(nullptr)==kIOReturnBadArgument,"null output rejected");
    IOWorkLoop *a=nullptr,*b=nullptr;
    require(RTWNet80211Acquire(&a)==0&&a,"first lease");
    require(RTWNet80211Acquire(&b)==0&&a==b,"two radios share one loop");
    require(RTWNet80211WorkLoop==a&&RTWNet80211CommandGate,"compatibility globals installed");
    require(taskq_is_initialized(systq),"one system queue initialized");
    require(RTWNet80211CommandGate->owner!=a,"compatibility gate has independent owner");
    unsigned calls=0;task first{};task_set(&first,increment,&calls,"radioB");
    RTWNet80211Release();a->release();
    require(RTWNet80211WorkLoop==b&&taskq_is_initialized(systq),"first detach leaves sibling runtime");
    require(task_add(systq,&first)==1,"sibling can queue after first detach");
    require(b->dispatchOne()&&calls==1,"sibling task executes");
    task last{};task_set(&last,release_inside_task,nullptr,"last detach");
    require(task_add(systq,&last)==1,"last detach task queued");
    require(b->dispatchOne(),"last release runs within queue callback");
    require(RTWNet80211WorkLoop==b&&taskq_is_initialized(systq),"cleanup deferred past task callback");
    IOWorkLoop *busy=reinterpret_cast<IOWorkLoop*>(1);
    require(RTWNet80211Acquire(&busy)==kIOReturnBusy&&!busy,"closing runtime rejects resurrection");
    b->release();host_run_thread_calls();
    require(!RTWNet80211WorkLoop&&!RTWNet80211CommandGate,"final drain clears globals");
    require(!taskq_is_initialized(systq),"final drain destroys queue");
    require(host_live_thread_calls==0,"cleanup call frees itself");
    require(host_live_objects==0,"runtime owner gate loop and queue sources all released");
    std::vector<IOWorkLoop*> loops(32);std::vector<std::thread> threads;
    std::atomic<unsigned> failures{0};
    for(unsigned i=0;i<loops.size();i++)threads.emplace_back([&,i]{if(RTWNet80211Acquire(&loops[i]))++failures;});
    for(auto &thread:threads)thread.join();
    require(failures==0,"concurrent acquires succeed");
    for(auto *loop:loops)require(loop==loops[0],"concurrent leases have one shared loop");
    threads.clear();for(auto *loop:loops)threads.emplace_back([loop]{RTWNet80211Release();loop->release();});
    for(auto &thread:threads)thread.join();host_run_thread_calls();
    require(!RTWNet80211WorkLoop&&host_live_objects==0,"concurrent last release drains once");
    host_fail_add_event=true;require(RTWNet80211Acquire(&a)==kIOReturnNoResources&&!a,"allocation failure rolls back");host_fail_add_event=false;
    require(!RTWNet80211WorkLoop&&!RTWNet80211CommandGate&&!taskq_is_initialized(systq),"failed start publishes no dangling globals");
    require(host_live_objects==0&&host_live_thread_calls==0,"failed start releases all resources");
    host_fail_add_at=host_add_event_calls+2;
    require(RTWNet80211Acquire(&a)==kIOReturnNoResources&&!a,"queue timer initialization failure rolls back");host_fail_add_at=0;
    require(!RTWNet80211WorkLoop&&!RTWNet80211CommandGate&&!taskq_is_initialized(systq)&&host_live_objects==0&&host_live_thread_calls==0,"queue allocation failure clears globals and resources");
    for(bool *failure:{&host_fail_workloop,&host_fail_gate,&host_fail_thread_call}){
        *failure=true;require(RTWNet80211Acquire(&a)==kIOReturnNoMemory&&!a,"partial allocation failure rolls back");*failure=false;
        require(!RTWNet80211WorkLoop&&!RTWNet80211CommandGate&&host_live_objects==0&&host_live_thread_calls==0,"partial allocation publishes no resources");
    }
    require(RTWNet80211Acquire(&a)==0,"runtime can initialize again after teardown");
    threads.clear();failures=0;
    for(unsigned i=0;i<32;i++)threads.emplace_back([&]{for(unsigned j=0;j<50;j++){IOWorkLoop *loop=nullptr;if(RTWNet80211Acquire(&loop)||loop!=a)++failures;if(loop){RTWNet80211Release();loop->release();}}});
    for(auto &thread:threads)thread.join();require(failures==0,"mixed concurrent leases retain the anchor runtime");
    RTWNet80211Release();a->release();host_run_thread_calls();
    require(host_live_objects==0,"reinitialized runtime drains");
    std::printf("net80211 shared runtime: %u checks passed\n",checks);
}
