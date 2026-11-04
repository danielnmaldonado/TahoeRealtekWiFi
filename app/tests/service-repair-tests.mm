// SPDX-License-Identifier: GPL-2.0-only
// Real manager and Main repair code; only ServiceManagement, protected-file
// lookup, driver IPC and presentation are replaced. No real registration runs.
#import "../CodeIdentity.h"
static BOOL fixtureProtected=YES;
static BOOL fixtureProtectedBundle(NSString* path){(void)path;return fixtureProtected;}
#define RTWProtectedBundle fixtureProtectedBundle
#include "../DriverConnection.mm"
#undef RTWProtectedBundle
#define main rtwui_application_main
#include "../Main.mm"
#undef main
#include <cassert>
@interface RTWDriverServiceManager (ExpectedRepairAPI)
- (void)repairWithValidityCheck:(BOOL (^)(void))valid completion:(void (^)(NSError*))completion;
@end
@interface RepairBackend : NSObject
@property(nonatomic,assign) SMAppServiceStatus status,registeredStatus;
@property(nonatomic,assign) unsigned unregisters,registers;
@property(nonatomic,assign) BOOL registerAccepted,forceEnabledOnFailure;
@property(nonatomic,strong) NSError* registrationError;
@property(nonatomic,copy) void (^pending)(NSError*);
@end
@implementation RepairBackend
- (instancetype)init {self=[super init];if(self){_status=SMAppServiceStatusEnabled;_registeredStatus=SMAppServiceStatusEnabled;_registerAccepted=YES;}return self;}
- (void)unregisterWithCompletionHandler:(void (^)(NSError*))handler {++_unregisters;_pending=handler;}
- (BOOL)registerAndReturnError:(NSError**)error {++_registers;if(error)*error=_registrationError;_status=_registerAccepted?_registeredStatus:(_forceEnabledOnFailure?SMAppServiceStatusEnabled:_status);return _registerAccepted;}
- (void)finish:(NSError*)error {assert(_pending);if(!error)_status=SMAppServiceStatusNotRegistered;void (^reply)(NSError*)=_pending;_pending=nil;reply(error);}
@end
@interface RepairManager : RTWDriverServiceManager
@property(nonatomic,strong) RepairBackend* backend;
@end
@implementation RepairManager
- (SMAppService*)service {return (SMAppService*)self.backend;}
@end
template<class Predicate> static void drainUntil(Predicate finished) {
 NSDate* deadline=[NSDate dateWithTimeIntervalSinceNow:5];
 while(!finished()&&deadline.timeIntervalSinceNow>0)[NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]];
 assert(finished());
}
static RepairManager* manager(){RepairManager* m=[RepairManager new];m.backend=[RepairBackend new];return m;}
static void managerCases(){
 // A removed launchd job does not mean its SM registration was removed.
 // Backend status intentionally remains Enabled until async unregister completes.
 RepairManager* m=manager();__block BOOL completed=NO;__block NSError* result=nil;
 [m repairWithValidityCheck:^{return YES;} completion:^(NSError* e){assert(NSThread.isMainThread);result=e;completed=YES;}];
 drainUntil(^{return m.backend.unregisters==1;});assert(!completed&&!m.backend.registers);
 [m.backend finish:nil];assert(!m.backend.registers);drainUntil(^{return completed;});
 assert(!result&&m.backend.registers==1&&m.state==RTWLoginItemStateEnabled);
 for(SMAppServiceStatus state:{SMAppServiceStatusNotRegistered,SMAppServiceStatusRequiresApproval,SMAppServiceStatusNotFound}){
  m=manager();m.backend.status=state;completed=NO;result=nil;
  [m repairWithValidityCheck:^{return YES;} completion:^(NSError* e){assert(NSThread.isMainThread);result=e;completed=YES;}];
  drainUntil(^{return completed;});assert(result&&!m.backend.unregisters&&!m.backend.registers&&m.backend.status==state);
 }
 m=manager();completed=NO;result=nil;fixtureProtected=NO;
 [m repairWithValidityCheck:^{return YES;} completion:^(NSError* e){result=e;completed=YES;}];drainUntil(^{return completed;});
 assert(result&&!m.backend.unregisters&&!m.backend.registers);fixtureProtected=YES;
 for(unsigned point=0;point<3;++point){
  m=manager();completed=NO;result=nil;__block BOOL current=point!=0;
  [m repairWithValidityCheck:^{return current;} completion:^(NSError* e){assert(NSThread.isMainThread);result=e;completed=YES;}];
  if(point){drainUntil(^{return m.backend.unregisters==1;});if(point==1)current=NO;else fixtureProtected=NO;[m.backend finish:nil];}
  drainUntil(^{return completed;});assert(result&&!m.backend.registers);fixtureProtected=YES;
 }
 m=manager();completed=NO;NSError* failure=[NSError errorWithDomain:NSPOSIXErrorDomain code:EIO userInfo:nil];
 [m repairWithValidityCheck:^{return YES;} completion:^(NSError* e){result=e;completed=YES;}];drainUntil(^{return m.backend.unregisters==1;});
 [m.backend finish:failure];drainUntil(^{return completed;});assert(result==failure&&!m.backend.registers);
 m=manager();m.backend.registerAccepted=NO;m.backend.forceEnabledOnFailure=YES;m.backend.registrationError=failure;completed=NO;result=nil;
 [m repairWithValidityCheck:^{return YES;} completion:^(NSError* e){result=e;completed=YES;}];drainUntil(^{return m.backend.unregisters==1;});
 [m.backend finish:nil];drainUntil(^{return completed;});assert(result==failure&&m.backend.registers==1);
 m=manager();m.backend.registeredStatus=SMAppServiceStatusRequiresApproval;completed=NO;result=nil;
 [m repairWithValidityCheck:^{return YES;} completion:^(NSError* e){result=e;completed=YES;}];drainUntil(^{return m.backend.unregisters==1;});
 [m.backend finish:nil];drainUntil(^{return completed;});assert(!result&&m.state==RTWLoginItemStateRequiresApproval);
 if(@available(macOS 15.0,*))for(unsigned variant=0;variant<3;++variant){
  m=manager();completed=NO;result=nil;
  [m repairWithValidityCheck:^{return YES;} completion:^(NSError* e){result=e;completed=YES;}];drainUntil(^{return m.backend.unregisters==1;});
  NSError* missing=[NSError errorWithDomain:variant==2?NSPOSIXErrorDomain:SMAppServiceErrorDomain code:kSMErrorJobNotFound userInfo:nil];
  if(variant!=1)m.backend.status=SMAppServiceStatusNotRegistered;
  [m.backend finish:missing];drainUntil(^{return completed;});
  assert((variant==0)==(!result&&m.backend.registers==1));if(variant)assert(result&&!m.backend.registers);
 }
 puts("Real service manager: retained registration, async ordering, main callbacks, disabled/pending choices, file/cancellation fences and errors passed");
}
@interface RepairDriver : RTWDriverConnection
@property(nonatomic,assign) unsigned closes;
@end
@implementation RepairDriver
- (void)close {++_closes;}
@end
@interface WiFiApp (ExpectedRepairFlow)
- (void)repairDriverService:(id)sender;
@end
@interface RepairApp : WiFiApp
@property(nonatomic,assign) unsigned connects,authorizes,messages;
@property(nonatomic,assign) BOOL holdConnect;
- (void)prepare:(RepairManager*)m;
- (void)attemptRealAuthorize;
@end
@implementation RepairApp
- (void)prepare:(RepairManager*)m {
 _serviceManager=m;_driver=[RepairDriver new];_queue=serialWorkQueue("org.research.TahoeUSBWiFi.test.repair");
 NSString* suite=[@"org.research.TahoeUSBWiFi.test.repair." stringByAppendingString:NSUUID.UUID.UUIDString];
 _preferences=[[NSUserDefaults alloc]initWithSuiteName:suite];[_preferences setBool:YES forKey:@"RTWStartupChoiceMade"];[_preferences setBool:YES forKey:@"RTWServiceSetupComplete"];
 _connectionPreferencesLoaded=_savedNetworksLoaded=YES;
}
- (void)refreshPollingTimer {}
- (void)updateAppearance {}
- (void)showMessage:(NSString*)title detail:(NSString*)detail {(void)title;(void)detail;++_messages;}
- (void)connectDriverAutomatically {++_connects;if(_holdConnect)_authorizing=YES;}
- (void)authorize:(id)sender {(void)sender;++_authorizes;}
- (void)attemptRealAuthorize {[super authorize:nil];}
@end
static void appCases(){
 RepairManager* m=manager();RepairApp* app=[RepairApp new];[app prepare:m];
 [app configureInstallation];drainUntil(^{return m.backend.unregisters==1;});
 assert([[app valueForKey:@"authorizing"] boolValue]&&!app.authorizes);
 [app repairDriverService:nil];assert(m.backend.unregisters==1);
 [m.backend finish:nil];drainUntil(^{return ![[app valueForKey:@"authorizing"] boolValue];});assert(m.backend.registers==1&&app.connects==1&&!app.messages);
 // SM may publish NotRegistered before unregister's completion. Canceling
 // ordinary work must not expose registration until that completion arrives.
 m=manager();app=[RepairApp new];[app prepare:m];[app repairDriverService:nil];drainUntil(^{return m.backend.unregisters==1;});
 m.backend.status=SMAppServiceStatusNotRegistered;[app suspendActivity];[app attemptRealAuthorize];
 assert(!m.backend.registers&&"Enable must wait for the outstanding SM unregister completion");
 [m.backend finish:nil];
 NSDate* settleEarly=[NSDate dateWithTimeIntervalSinceNow:0.05];while(settleEarly.timeIntervalSinceNow>0)[NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]];
 for(SMAppServiceStatus state:{SMAppServiceStatusNotRegistered,SMAppServiceStatusRequiresApproval}){
  m=manager();m.backend.status=state;app=[RepairApp new];[app prepare:m];[app configureInstallation];
  assert(!m.backend.unregisters&&!m.backend.registers&&!app.authorizes);
 }
 m=manager();m.backend.status=SMAppServiceStatusNotRegistered;app=[RepairApp new];[app prepare:m];
 [[app valueForKey:@"preferences"] setBool:YES forKey:@"RTWResumeServiceAfterInstall"];[app configureInstallation];assert(app.authorizes==1&&!m.backend.unregisters);
 for(unsigned action=0;action<2;++action){
  m=manager();app=[RepairApp new];[app prepare:m];[app repairDriverService:nil];drainUntil(^{return m.backend.unregisters==1;});
  if(action==0)[app workspaceWillSleep:nil];else [app applicationWillTerminate:[NSNotification notificationWithName:NSApplicationWillTerminateNotification object:NSApp]];
  [m.backend finish:nil];
  NSDate* until=[NSDate dateWithTimeIntervalSinceNow:0.05];while(until.timeIntervalSinceNow>0)[NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]];
  assert(!m.backend.registers&&!app.connects);
 }
 // Exercise the actual bounded deadline. Its later callback must not register,
 // and a completed repair's deadline must not cancel a subsequent XPC attempt.
 RepairManager* timedManager=manager();RepairApp* timedApp=[RepairApp new];[timedApp prepare:timedManager];
 [timedApp repairDriverService:nil];drainUntil(^{return timedManager.backend.unregisters==1;});
 timedManager.backend.status=SMAppServiceStatusNotRegistered;
 RepairManager* linkedManager=manager();RepairApp* linkedApp=[RepairApp new];[linkedApp prepare:linkedManager];linkedApp.holdConnect=YES;
 [linkedApp repairDriverService:nil];drainUntil(^{return linkedManager.backend.unregisters==1;});
 [linkedManager.backend finish:nil];drainUntil(^{return linkedApp.connects==1;});
 NSDate* deadline=[NSDate dateWithTimeIntervalSinceNow:15.2];
 while(deadline.timeIntervalSinceNow>0)[NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
 assert(![[timedApp valueForKey:@"authorizing"] boolValue]&&!timedManager.backend.registers);
 [timedApp attemptRealAuthorize];assert(!timedManager.backend.registers);
 assert([[linkedApp valueForKey:@"authorizing"] boolValue]&&linkedManager.backend.registers==1);
 [timedManager.backend finish:nil];
 NSDate* settle=[NSDate dateWithTimeIntervalSinceNow:0.05];while(settle.timeIntervalSinceNow>0)[NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]];
 assert(!timedManager.backend.registers&&!timedApp.connects);
 puts("Real Main service repair: complete-install heals Enabled, preserves disabled/pending, single flight and sleep/termination cancel before re-registration");
}
int main(){@autoreleasepool{
 [NSApplication sharedApplication];
 if(![RTWDriverServiceManager instancesRespondToSelector:@selector(repairWithValidityCheck:completion:)]){fprintf(stderr,"FAIL: service manager has no async unregister/re-register repair\n");return 1;}
 managerCases();appCases();
}}
