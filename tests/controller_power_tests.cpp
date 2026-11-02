// SPDX-License-Identifier: GPL-2.0-only
#include "controller_power_host.hpp"
#include <algorithm>
static size_t event(const RTWUSBController&c,const char*name){
 auto i=std::find(c.events.begin(),c.events.end(),name);assert(i!=c.events.end());return static_cast<size_t>(i-c.events.begin());
}
int main(){
 // RED target: late registration inherits Unsupported and PMstop in prepare.
 for(unsigned adapter=0;adapter<2;++adapter){
  RTWUSBController c;IOUSBHostInterface usb;c.callBeforeInterface=true;
  assert(c.start(&usb));
  assert(c.powerProperty&&!c.pmStopped&&c.powerManaged_);
  assert(c.initEffects==1&&c.initCalls==1&&usb.joins==1);
  assert(c.registrations==1&&c.requests==1&&c.requestedState==1);
  assert(event(c,"PMinit")<event(c,"prepare"));
  assert(event(c,"prepare")<event(c,"registerPowerDriver"));
  assert(event(c,"requestPower")<event(c,"attach"));
  assert(c.states.size()==2&&c.states[0].version==1&&c.states[0].capabilityFlags==0);
  assert(c.states[1].version==1&&c.states[1].capabilityFlags==(2|0x8000));
  assert(c.states[1].inputPowerRequirement==2&&c.states[1].outputPowerCharacter==2);
  assert(c.callbacksBeforeNetif==1&&!c.sleeping_);
  assert(c.hostGate.references==1&&c.loop_->references==1);
  assert(c.registerWithPolicyMaker(&c)==0&&c.registrations==1&&c.requests==1);
  c.stop(&usb);
  assert(!c.powerManaged_&&c.stopEffects==1&&c.platformReleased&&c.teardownReady);
  assert(event(c,"PMstop")<event(c,"release"));
 }
 {
  RTWUSBController c;IOUSBHostInterface usb;c.registrationError=-19;
  assert(!c.start(&usb));assert(c.registrations==1&&c.requests==0);
  assert(c.pmStopped&&!c.powerManaged_&&c.platformReleased);
  assert(c.lastStage=="power-register"&&c.lastStartResult==-19);
  assert(c.registerWithPolicyMaker(&c)==-19&&c.registrations==1);
 }
 {
  RTWUSBController c;IOUSBHostInterface usb;c.requestError=-20;
  assert(!c.start(&usb));assert(c.registrations==1&&c.requests==1);
  assert(c.pmStopped&&!c.powerManaged_&&c.platformReleased);
  assert(c.lastStage=="power-register"&&c.lastStartResult==-20);
  assert(event(c,"PMstop")<event(c,"release"));
 }
 {
  RTWUSBController c;IOUSBHostInterface usb;c.attachError=true;
  assert(!c.start(&usb));assert(c.registrations==1&&c.requests==1);
  assert(c.pmStopped&&!c.powerManaged_&&c.platformReleased);
  assert(event(c,"PMstop")<event(c,"release"));
 }
 {
  RTWUSBController c;IOUSBHostInterface usb;c.wrongInterface=true;
  assert(!c.start(&usb));assert(c.registrations==1&&c.requests==1);
  assert(c.pmStopped&&!c.powerManaged_&&c.platformReleased);
  assert(c.lastStage=="ethernet-type"&&c.lastStartResult==kIOReturnUnsupported);
  assert(event(c,"PMstop")<event(c,"detach"));
  assert(event(c,"PMstop")<event(c,"release"));
 }
 {
  RTWUSBController c;
  assert(c.registerWithPolicyMaker(nullptr)==kIOReturnBadArgument);
  assert(c.registrations==0&&c.requests==0&&!c.powerManaged_);
 }
 puts("controller-power: actual start/policy/setPowerState/stop passed; order, initial on, 2 adapters, callback timing, errors and teardown");
}
