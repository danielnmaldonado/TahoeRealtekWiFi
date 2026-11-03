// SPDX-License-Identifier: GPL-2.0-only
#pragma once
// Platform boundaries only: production methods and Sleep/Wake arms are extracted.
// Superclass order follows Apple's IONetworkController.cpp; PMstop/PMinit follow
// IOServicePM.cpp. We do not assume an initial off callback caused a device fault.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
using IOReturn=int;
using OSObject=struct IOService;
using thread_call_t=void*;
constexpr IOReturn kIOReturnSuccess=0,kIOReturnNotReady=-1,kIOReturnBadArgument=-2,
 kIOReturnError=-3,kIOReturnUnsupported=-4,kIOReturnNotOpen=-5,
 kIOReturnNoMemory=-6,kIOReturnNoResources=-7,kIOPMAckImplied=0;
constexpr unsigned long kIOPMPowerOn=2,kIOPMDeviceUsable=0x8000;
constexpr int THREAD_CALL_PRIORITY_KERNEL=0,kIOMediumEthernetAuto=0,kIONetworkLinkValid=1;
struct IOPMPowerState {
 unsigned long version,capabilityFlags,outputPowerCharacter,inputPowerRequirement,
 staticPower,stateOrder,powerToAttain,timeToAttain,settleUpTime,timeToLower,
 settleDownTime,powerDomainBudget;
};
struct RTWUSBController;
struct IOService {
 virtual ~IOService()=default;
 unsigned joins=0,registrations=0,requests=0,initCalls=0,initEffects=0,
 stopCalls=0,stopEffects=0,serviceRegistrations=0;
 bool initialized=false,pmStopped=false,powerProperty=false,registered=false;
 IOReturn registrationError=0,requestError=0;
 unsigned long requestedState=99;
 RTWUSBController*controlling=nullptr;
 std::vector<IOPMPowerState>states;
 std::vector<std::string>events;
 virtual IOReturn setPowerState(unsigned long,IOService*){return 0;}
 virtual IOReturn registerWithPolicyMaker(IOService*){return kIOReturnUnsupported;}
 void PMinit(){++initCalls;if(!initialized){initialized=true;powerProperty=true;++initEffects;events.push_back("PMinit");}}
 void PMstop(){++stopCalls;if(initialized&&!pmStopped){pmStopped=true;powerProperty=false;++stopEffects;events.push_back("PMstop");}}
 void joinPMtree(IOService*child){++joins;child->events.push_back("join");}
 IOReturn registerPowerDriver(IOService*,IOPMPowerState*,unsigned long);
 IOReturn changePowerStateTo(unsigned long);
 void registerService(){++serviceRegistrations;events.push_back("registerService");}
};
struct IONetworkInterface:IOService{void release(){}};
struct IOEthernetInterface:IONetworkInterface{};
struct IOUSBHostInterface:IOService{bool open(IOService*){return true;}};
struct IOWorkLoop{unsigned references=1;int addEventSource(void*){return 0;}void release(){assert(references>1);--references;}};
struct IOInterruptEventSource{static IOInterruptEventSource*interruptEventSource(IOService*,void(*)(void)){static IOInterruptEventSource e;return &e;}void enable(){}};
struct IOTimerEventSource{static IOTimerEventSource*timerEventSource(IOService*,void(*)(void)){static IOTimerEventSource e;return &e;}};
struct IOCommandGate{
 RTWUSBController*owner=nullptr;unsigned references=1;
 static IOCommandGate*commandGate(IOService*);
 IOReturn runAction(IOReturn(*)(OSObject*,void*,void*,void*,void*),void*);
 void release(){assert(references>1);--references;}
};
struct OSDictionary{static OSDictionary*withCapacity(int){static OSDictionary d;return &d;}void release(){}};
struct IONetworkMedium{static IONetworkMedium*medium(int,int){static IONetworkMedium m;return &m;}static bool addMedium(OSDictionary*,IONetworkMedium*){return true;}void release(){}};
struct MockNetworkController:IOService{
 bool attachError=false,wrongInterface=false,callBeforeInterface=false,attachmentGateHeld=false;
 IOEthernetInterface interface;
 IONetworkInterface wrongType;
 bool start(IOService*provider){PMinit();provider->joinPMtree(this);return true;}
 void stop(IOService*){PMstop();events.push_back("superStop");}
 bool attachInterface(IONetworkInterface**,bool);
 void detachInterface(IONetworkInterface*,bool){events.push_back("detach");}
 bool publishMediumDictionary(OSDictionary*){return true;}
 bool setSelectedMedium(IONetworkMedium*){return true;}
 void setLinkStatus(int){}
};
inline void*IOLockAlloc(){return reinterpret_cast<void*>(1);}
inline void*IOSimpleLockAlloc(){return reinterpret_cast<void*>(1);}
inline thread_call_t thread_call_allocate_with_options(void(*)(void),void*,int,int){return reinterpret_cast<void*>(1);}
inline int systq=1;
inline bool taskq_is_initialized(int){return true;}
inline void rtw_transfer_reset(int*){}
inline IOReturn RTWNet80211Acquire(IOWorkLoop**loop){static IOWorkLoop l;*loop=&l;return 0;}
template<class T>inline T*hostCast(IOService*p){return dynamic_cast<T*>(p);}
#define OSDynamicCast(type,value) hostCast<type>(value)
struct RTWUSBController:MockNetworkController{
 enum GateOperation{Initialize=1,Shutdown,Enable,Disable,Output,Control,Terminated,Sleep,Wake,TeardownReady};
 bool runtimeLease_=false,usbOpened_=false,present_=false,powerManaged_=false,
 retainedForPending_=false,sleeping_=false,attached_=false;
 bool platformReleased=false,teardownReady=false;
 unsigned quiesces=0,callbacksBeforeNetif=0;
 IOWorkLoop*loop_=nullptr;
 void*lifecycleLock_=nullptr,*completionLock_=nullptr;
 thread_call_t cleanupCall_=nullptr;
 IOUSBHostInterface*usb_=nullptr;
 IOCommandGate*gate_=nullptr;
 IOCommandGate hostGate;
 IOInterruptEventSource*completionEvent_=nullptr;
 IOTimerEventSource*scanTimer_=nullptr,*calibTimer_=nullptr,*watchdogTimer_=nullptr;
 IOEthernetInterface*netif_=nullptr;
 int ledger_=0;
 IOReturn lastStartResult=0;
 std::string lastStage;
 #include "power_declarations.hpp"
 bool start(IOService*);
 void stop(IOService*);
 IOReturn setPowerState(unsigned long,IOService*)override;
 // Immutable registry metadata has no power/USB effect in this boundary fixture.
 void setProperty(const char*,unsigned long long,unsigned){}
 void reportStartStage(const char*stage,IOReturn result=0){lastStage=stage;lastStartResult=result;events.push_back(stage);}
 static void deferredCleanup(){}
 static void completionAction(){}
 static void scanAction(){}
 static void calibAction(){}
 static void watchdogAction(){}
 static IOReturn gated(OSObject*object,void*operation,void*,void*,void*){
  auto*s=static_cast<RTWUSBController*>(object);
  switch(reinterpret_cast<uintptr_t>(operation)){
  case Initialize:s->attached_=true;return 0;
  case Shutdown:s->attached_=false;return 0;
  #include "power_gate_cases.inc"
  default:return kIOReturnBadArgument;
  }
 }
 IOCommandGate*copyGate(IOWorkLoop**lease){*lease=nullptr;if(!gate_)return nullptr;++gate_->references;++loop_->references;*lease=loop_;return gate_;}
 void quiesce(bool){++quiesces;if(!netif_)++callbacksBeforeNetif;}
 void releasePlatform(){
  // PM must inhibit callbacks before releasing their gate/platform resources.
  assert(!registered||pmStopped);
  platformReleased=true;gate_=nullptr;events.push_back("release");
 }
 void markTeardownReady(){teardownReady=true;}
};
inline IOReturn IOService::registerPowerDriver(IOService*driver,IOPMPowerState*table,unsigned long count){
 ++registrations;events.push_back("registerPowerDriver");
 if(registrationError)return registrationError;
 controlling=dynamic_cast<RTWUSBController*>(driver);assert(controlling);
 states.assign(table,table+count);registered=true;return 0;
}
inline IOReturn IOService::changePowerStateTo(unsigned long state){++requests;requestedState=state;events.push_back("requestPower");return requestError;}
inline IOCommandGate*IOCommandGate::commandGate(IOService*object){auto*c=dynamic_cast<RTWUSBController*>(object);assert(c);c->hostGate.owner=c;return &c->hostGate;}
inline IOReturn IOCommandGate::runAction(IOReturn(*action)(OSObject*,void*,void*,void*,void*),void*operation){return action(owner,operation,nullptr,nullptr,nullptr);}
inline bool MockNetworkController::attachInterface(IONetworkInterface**out,bool){
 events.push_back("prepare");attachmentGateHeld=true;
 if(initialized&&registerWithPolicyMaker(this)!=0){
  // Returning a later error after registering callbacks makes inherited
  // prepare PMstop under the gate: reject that dangerous ordering.
  assert(!registered);
  PMstop();
 }
 attachmentGateHeld=false;
 // Callback timing is legal before attachInterface returns its interface.
 if(callBeforeInterface&&registered&&!pmStopped){setPowerState(0,this);setPowerState(1,this);}
 events.push_back("attach");
 if(attachError)return false;
 *out=wrongInterface?&wrongType:&interface;return true;
}
