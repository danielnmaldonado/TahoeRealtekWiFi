// SPDX-License-Identifier: GPL-2.0-only
#include "ReconnectPolicy.hpp"
#include <cassert>
#include <cstdio>
#include <limits>
int main() {
    unsigned checks=0;
    auto check=[&](bool value){++checks;assert(value);};
    rtwui::RetryGate retry;
    check(retry.due(0));check(!retry.due(-1));
    check(!retry.due(std::numeric_limits<double>::quiet_NaN()));
    retry.fail(0);check(!retry.due(3.9));check(retry.due(4));
    retry.fail(4);check(!retry.due(11.9));check(retry.due(12));
    for(unsigned i=0;i<1000;++i) {
        retry.fail(i+100);
        check(retry.nextAt-retry.lastAt<=60);
        check(retry.failures<=6);
    }
    check(retry.due(0));retry.reset();check(retry.due(0));
    rtwui::AutoJoinPolicy policy;
    // Exhaust every input combination; autojoin must never override a manual
    // leave, stale status, sleep, another join or an interactive credential job.
    for(unsigned mask=0;mask<256;++mask) {
        bool available=mask&1,fresh=mask&2,connected=mask&4,joining=mask&8;
        bool busy=mask&16,sleeping=mask&32,paused=mask&64,exhausted=mask&128;
        policy.paused=paused;policy.retry.reset();
        if(exhausted)for(int i=0;i<3;++i)policy.retry.fail(0);
        check(policy.mayAttempt(100,available,fresh,connected,joining,busy,sleeping)==
              (available&&fresh&&!connected&&!joining&&!busy&&!sleeping&&!paused&&!exhausted));
    }
    policy.paused=true;policy.reset();
    check(!policy.mayAttempt(100,true,true,false,false,false,false));
    policy.paused=false;
    check(policy.mayAttempt(100,true,true,false,false,false,false));
    for(unsigned i=0;i<3;++i)policy.retry.fail(10.0*(i+1));
    check(!policy.mayAttempt(329.9,true,true,false,false,false,false));
    check(policy.retry.failures==3);
    check(policy.mayAttempt(330,true,true,false,false,false,false));
    check(policy.retry.failures==0);
    for(unsigned i=0;i<3;++i)policy.retry.fail(400);
    policy.paused=true;
    check(!policy.mayAttempt(1000,true,true,false,false,false,false));
    check(policy.retry.failures==3);
    policy.paused=false;
    check(!policy.mayAttempt(1000,true,true,false,false,false,true));
    check(policy.retry.failures==3);
    check(!policy.mayAttempt(1000,true,true,false,false,true,false));
    check(policy.retry.failures==3);
    check(policy.mayAttempt(1000,true,true,false,false,false,false));
    check(policy.retry.failures==0);
    for(unsigned i=0;i<3;++i)policy.retry.fail(1000);
    check(policy.mayAttempt(0,true,true,false,false,false,false));
    // Deferred Keychain results must respect the *current* auto-reconnect
    // choice. A manual password request must not be canceled by that choice.
    for(unsigned mask=0;mask<8;++mask) {
        const bool automatic=(mask&1)!=0, enabled=(mask&2)!=0, paused=(mask&4)!=0;
        check(rtwui::credentialJoinStillAllowed(automatic,enabled,paused)==
              (!automatic||(enabled&&!paused)));
    }
    for(unsigned previous=0;previous<5;++previous)
        for(unsigned current=0;current<5;++current)
            for(bool present:{false,true})
                check(rtwui::recoverSoleAdapter(present,previous,current)==
                    (!present&&current==1&&previous<=1));
    printf("reconnect-policy: %u checks passed\n",checks);
}
