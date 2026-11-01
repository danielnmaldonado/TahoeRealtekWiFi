// SPDX-License-Identifier: GPL-2.0-only
#import "../LoginItem.h"
#include <cstdio>
#include <cstdlib>
static unsigned checks=0;
static void check(bool condition,const char* message) {
    ++checks;if(!condition){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
@interface TestLoginBackend : NSObject <RTWLoginItemBackend>
@property(nonatomic) RTWLoginItemState state;
@property(nonatomic) RTWLoginItemState registeredState;
@property(nonatomic) BOOL preparationFails;
@property(nonatomic) BOOL registrationFails;
@property(nonatomic) BOOL removalFails;
@property(nonatomic) NSUInteger preparations;
@property(nonatomic) NSUInteger registrations;
@property(nonatomic) NSUInteger removals;
@property(nonatomic) NSUInteger settingsOpens;
@end
@implementation TestLoginBackend
- (instancetype)init {
    self=[super init];if(self){_state=RTWLoginItemStateNotRegistered;_registeredState=RTWLoginItemStateEnabled;}return self;
}
- (BOOL)prepareBundleAndReturnError:(NSError**)error {
    ++_preparations;
    if(_preparationFails&&error)*error=[NSError errorWithDomain:@"test.prepare" code:1 userInfo:nil];
    return !_preparationFails;
}
- (BOOL)registerAndReturnError:(NSError**)error {
    ++_registrations;
    if(_registrationFails) {
        if(error)*error=[NSError errorWithDomain:@"test.register" code:2 userInfo:nil];return NO;
    }
    _state=_registeredState;return YES;
}
- (BOOL)unregisterAndReturnError:(NSError**)error {
    ++_removals;
    if(_removalFails) {
        if(error)*error=[NSError errorWithDomain:@"test.remove" code:3 userInfo:nil];return NO;
    }
    _state=RTWLoginItemStateNotRegistered;return YES;
}
- (void)openSystemSettings {++_settingsOpens;}
@end
int main() {
    @autoreleasepool {
        TestLoginBackend* backend=[TestLoginBackend new];
        RTWLoginItemManager* manager=[[RTWLoginItemManager alloc]initWithBackend:backend];
        NSError* error=nil;
        check([manager setEnabled:YES error:&error],"register unregistered");
        check(backend.preparations==1&&backend.registrations==1,"prepare before register");
        check(manager.state==RTWLoginItemStateEnabled&&!error,"confirmed enabled");
        check([manager setEnabled:YES error:&error]&&backend.registrations==1,"idempotent enable");
        check([manager setEnabled:NO error:&error]&&backend.removals==1,"disable");
        check([manager setEnabled:NO error:&error]&&backend.removals==1,"idempotent disable");
        backend.state=RTWLoginItemStateNotFound;
        check([manager setEnabled:YES error:&error],"not-found can register");
        check(backend.registrations==2&&manager.state==RTWLoginItemStateEnabled,"not-found recovered");
        backend.state=RTWLoginItemStateNotRegistered;backend.registeredState=RTWLoginItemStateRequiresApproval;
        check([manager setEnabled:YES error:&error],"pending registered");
        check(manager.state==RTWLoginItemStateRequiresApproval,"pending not called enabled");
        const auto beforePending=backend.registrations;
        check([manager repairRegistrationAndReturnError:&error],"pending repair opens settings");
        check(backend.settingsOpens==1&&backend.registrations==beforePending,"pending no reregister");
        check([manager setEnabled:NO error:&error],"pending can cancel explicitly");
        backend.preparationFails=YES;
        check(![manager setEnabled:YES error:&error]&&error,"reject unstable/uninstalled path");
        check(backend.registrations==beforePending,"no register after preparation failure");
        backend.state=RTWLoginItemStateEnabled;
        check([manager setEnabled:NO error:&error],"unregister after app moved away");
        backend.preparationFails=NO;backend.registrationFails=YES;
        check(![manager setEnabled:YES error:&error],"registration error propagated");
        check([error.domain isEqual:@"test.register"],"preserve system error domain");
        backend.registrationFails=NO;backend.registeredState=RTWLoginItemStateNotFound;
        check(![manager setEnabled:YES error:&error]&&error,"true return without confirmed status not success");
        backend.state=RTWLoginItemStateEnabled;backend.registeredState=RTWLoginItemStateEnabled;
        const auto removed=backend.removals,registered=backend.registrations;
        check([manager repairRegistrationAndReturnError:&error],"repair stale enabled registration");
        check(backend.removals==removed+1&&backend.registrations==registered+1,"repair unregister then register");
        backend.removalFails=YES;
        check(![manager repairRegistrationAndReturnError:&error],"repair stops on removal failure");
        check(backend.registrations==registered+1,"no duplicate registration after removal failure");
        check(![manager setEnabled:NO error:&error],"disable failure visible");
        backend.state=RTWLoginItemStateNotRegistered;
        check(manager.state==RTWLoginItemStateNotRegistered,"external disable reflected");
        std::printf("login-item backend: %u checks passed (mock, no login changes)\n",checks);
    }
}
