// SPDX-License-Identifier: GPL-2.0-only
// Run the real lifecycle handlers and polling path with only USB/Keychain/time
// replaced. Display sleep must never become a user Disconnect or system sleep.
@interface WiFiApp (ScreenLifecycleTestSelectors)
- (void)workspaceScreensDidSleep:(NSNotification*)notification;
- (void)workspaceScreensDidWake:(NSNotification*)notification;
@end
@interface ScreenLifecycleDriver : AutomaticRefreshDriver
@property(nonatomic,assign) unsigned lists,statuses;
@property(nonatomic,strong) dispatch_semaphore_t statusEntered,statusGate;
@property(nonatomic,assign) BOOL blocked;
@end
@implementation ScreenLifecycleDriver
- (uint32_t)exchangeRegistryID:(uint64_t)registryID selector:(uint32_t)selector
                        input:(const void*)input length:(size_t)length
                       output:(void*)output capacity:(size_t)capacity {
    if(selector==rtwui::listSelector)++self.lists;
    if(selector==RTW_CONTROL_STATUS)++self.statuses;
    if(selector==RTW_CONTROL_STATUS&&self.statusGate&&!self.blocked) {
        self.blocked=YES;dispatch_semaphore_signal(self.statusEntered);
        assert(!dispatch_semaphore_wait(self.statusGate,dispatch_time(DISPATCH_TIME_NOW,5LL*NSEC_PER_SEC)));
    }
    return [super exchangeRegistryID:registryID selector:selector input:input length:length output:output capacity:capacity];
}
@end
@interface ScreenLifecycleApp : AutomaticRefreshApp
- (void)prepareScreen:(ScreenLifecycleDriver*)driver paused:(BOOL)paused joining:(BOOL)joining;
- (uint64_t)workEpoch;
- (BOOL)manualPause;
- (void)finishScreenTest;
@end
@implementation ScreenLifecycleApp
- (void)updateAppearance {[self refreshPollingTimer];}
- (void)prepareScreen:(ScreenLifecycleDriver*)driver paused:(BOOL)paused joining:(BOOL)joining {
    _preferences=[[NSUserDefaults alloc]initWithSuiteName:
        [@"org.research.TahoeUSBWiFi.test.screen." stringByAppendingString:NSUUID.UUID.UUIDString]];
    [_preferences setBool:paused forKey:@"RTWAutoJoinPaused"];
    [_preferences setBool:YES forKey:@"RTWAutoJoinEnabled"];
    _connectionPreferencesLoaded=YES;_autoJoinEnabled=YES;_autoJoin.paused=paused;
    driver->adapters[3003].connected=driver->adapters[3003].radio=true;
    driver->adapters[4004]={};[super prepare:driver];
    [self poll:nil];drainControlWork(self);
    _savedNetworksLoaded=NO;_lastSavedReloadAt=-1;
    _joining=joining;_joinAccepted=NO;_joinStartedAt=joining?99:-1;
    _joinSSID=joining?[@"Pending" dataUsingEncoding:NSUTF8StringEncoding]:nil;
    _item=[NSStatusBar.systemStatusBar statusItemWithLength:24];[self refreshPollingTimer];
}
- (uint64_t)workEpoch {return _workEpoch.snapshot();}
- (BOOL)manualPause {return _autoJoin.paused;}
- (void)finishScreenTest {
    [_timer invalidate];_timer=nil;
    if(_item)[NSStatusBar.systemStatusBar removeStatusItem:_item];_item=nil;
}
@end
static void testScreenLifecyclePreservesLink() {
    assert([WiFiApp instancesRespondToSelector:@selector(workspaceScreensDidSleep:)]&&
           [WiFiApp instancesRespondToSelector:@selector(workspaceScreensDidWake:)]&&
           "Display sleep/wake need distinct handlers, without system-sleep suspension");
    for(bool paused:{false,true})for(bool joining:{false,true}) {
        ScreenLifecycleDriver* driver=[ScreenLifecycleDriver new];
        ScreenLifecycleApp* app=[ScreenLifecycleApp new];[app prepareScreen:driver paused:paused joining:joining];
        NSTimer* timer=[app valueForKey:@"timer"];
        const auto epoch=app.workEpoch,generation=[[app valueForKey:@"joinGeneration"] unsignedLongLongValue];
        const auto lists=driver.lists,statuses=driver.statuses,cacheReads=driver->adapters[3003].reads;
        NSDictionary* preferences=[[app valueForKey:@"preferences"] dictionaryRepresentation];
        [app workspaceScreensDidSleep:nil];
        assert(timer.valid&&[app valueForKey:@"timer"]==timer&&app.workEpoch==epoch);
        assert([app selectedAdapterID]==3003);
        assert(![[app valueForKey:@"sleeping"] boolValue]&&![[app valueForKey:@"inactiveSession"] boolValue]);
        assert([[app valueForKey:@"joining"] boolValue]==joining&&app.manualPause==paused);
        assert(driver.lists==lists&&driver.statuses==statuses&&driver->adapters[3003].reads==cacheReads);
        [app workspaceScreensDidWake:nil];drainControlWork(app);
        assert(timer.valid&&[app valueForKey:@"timer"]==timer&&app.workEpoch==epoch);
        assert([app selectedAdapterID]==3003);
        assert([[app valueForKey:@"joinGeneration"] unsignedLongLongValue]==generation&&
               [[app valueForKey:@"joining"] boolValue]==joining&&app.manualPause==paused);
        assert(driver->adapters[3003].connected&&driver->adapters[3003].requests==0);
        assert(driver.lists==lists+1&&driver.statuses==statuses+1&&driver->adapters[3003].reads==cacheReads+1);
        assert(!app.credentialReloads&&[[[app valueForKey:@"preferences"] dictionaryRepresentation] isEqual:preferences]);
        [app finishScreenTest];
    }
    // Even an idle adapter must get only a snapshot because of screen wake;
    // the next ordinary timer poll retains its existing automatic policy.
    ScreenLifecycleDriver* driver=[ScreenLifecycleDriver new];
    ScreenLifecycleApp* app=[ScreenLifecycleApp new];[app prepareScreen:driver paused:NO joining:NO];
    driver->adapters[3003].connected=false;
    [app workspaceScreensDidWake:nil];drainControlWork(app);
    assert(!driver->adapters[3003].requests&&!app.credentialReloads);
    [app poll:nil];drainControlWork(app);assert(driver->adapters[3003].requests==1);
    [app finishScreenTest];
    // A wake during an entered STATUS preserves that work and requests one
    // follow-up snapshot, rather than discarding the refresh or canceling I/O.
    driver=[ScreenLifecycleDriver new];app=[ScreenLifecycleApp new];
    [app prepareScreen:driver paused:YES joining:YES];
    [app setValue:@YES forKey:@"savedNetworksLoaded"];
    driver.statusEntered=dispatch_semaphore_create(0);driver.statusGate=dispatch_semaphore_create(0);
    const auto statuses=driver.statuses,lists=driver.lists,reads=driver->adapters[3003].reads;
    const auto epoch=app.workEpoch;
    [app poll:nil];
    assert(!dispatch_semaphore_wait(driver.statusEntered,dispatch_time(DISPATCH_TIME_NOW,5LL*NSEC_PER_SEC)));
    [app setValue:@NO forKey:@"savedNetworksLoaded"];[app workspaceScreensDidWake:nil];
    assert(app.workEpoch==epoch&&[[app valueForKey:@"busy"] boolValue]);
    dispatch_semaphore_signal(driver.statusGate);drainControlWork(app);
    assert(driver.statuses==statuses+2&&driver.lists==lists+1&&driver->adapters[3003].reads==reads+1);
    assert(!driver->adapters[3003].requests&&!app.credentialReloads&&app.manualPause);
    [app workspaceWillSleep:nil];const auto sleepingEpoch=app.workEpoch;
    [app workspaceScreensDidWake:nil];assert(app.workEpoch==sleepingEpoch&&driver.statuses==statuses+2);
    assert(![app valueForKey:@"timer"]&&[[app valueForKey:@"sleeping"] boolValue]);
    [app finishScreenTest];
    puts("Actual Main screen sleep/wake: live link, join intent, epoch, timer and manual pause preserved; wake refresh is read-only");
}
static void testScreenRefreshYieldsToSuspension() {
    for(unsigned mode=0;mode<3;++mode) {
        ScreenLifecycleDriver* driver=[ScreenLifecycleDriver new];
        ScreenLifecycleApp* app=[ScreenLifecycleApp new];[app prepareScreen:driver paused:NO joining:NO];
        [app setValue:@YES forKey:@"savedNetworksLoaded"];
        driver.statusEntered=dispatch_semaphore_create(0);driver.statusGate=dispatch_semaphore_create(0);
        const auto statuses=driver.statuses,reads=driver->adapters[3003].reads;
        const auto epoch=app.workEpoch;
        [app poll:nil];
        assert(!dispatch_semaphore_wait(driver.statusEntered,dispatch_time(DISPATCH_TIME_NOW,5LL*NSEC_PER_SEC)));
        [app workspaceScreensDidWake:nil];
        if(mode==0)[app workspaceWillSleep:nil];
        if(mode==1)[app workspaceBecameInactive:nil];
        if(mode==2)[app applicationWillTerminate:[NSNotification notificationWithName:@"TestTermination" object:nil]];
        assert(app.workEpoch!=epoch&&![[app valueForKey:@"screenRefreshPending"] boolValue]);
        driver->adapters[3003].connected=false;
        dispatch_semaphore_signal(driver.statusGate);dispatch_sync([app valueForKey:@"queue"],^{});
        drainControlWork(app);
        assert(driver.statuses==statuses+1&&driver->adapters[3003].reads==reads&&!driver->adapters[3003].requests);
        if(mode<2) {
            if(mode==0)[app workspaceDidWake:nil];else [app workspaceBecameActive:nil];
            drainControlWork(app);
            assert(driver->adapters[3003].requests==1&&app.credentialReloads==1&&!app.manualPause);
        }
        [app finishScreenTest];
    }
    puts("Actual Main queued screen refresh: real sleep, session switch and termination cancel it; normal resume policy survives");
}
static void testLifecycleDiagnosticReasons() {
    NSString* path=[NSHomeDirectory() stringByAppendingPathComponent:@"Library/Logs/Tahoe-WiFi-USB/control.jsonl"];
    NSString* before=[NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil]?:@"";
    LifecycleWiFiApp* app=[LifecycleWiFiApp new];
    [app setValue:[[NSUserDefaults alloc]initWithSuiteName:
        [@"org.research.TahoeUSBWiFi.test.lifecycle." stringByAppendingString:NSUUID.UUID.UUIDString]] forKey:@"preferences"];
    [app useAdapter:4294968492ULL];
    [app workspaceWillSleep:nil];[app workspaceDidWake:nil];
    [app workspaceBecameInactive:nil];[app workspaceBecameActive:nil];
    [app workspaceScreensDidSleep:nil];[app workspaceScreensDidWake:nil];
    [app disconnect:nil];
    [app applicationWillTerminate:[NSNotification notificationWithName:@"TestTermination" object:nil]];
    NSString* after=[NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil]?:@"";
    assert([after hasPrefix:before]);
    NSArray* reasons=@[@"system_sleep",@"system_wake",@"session_inactive",@"session_active",
        @"screen_sleep",@"screen_wake",@"manual_disconnect",@"app_termination"];
    NSArray* lines=[[after substringFromIndex:before.length] componentsSeparatedByString:@"\n"];
    assert(lines.count==reasons.count+1&&"Every lifecycle action must append one bounded metadata record");
    for(NSUInteger i=0;i<reasons.count;++i) {
        NSData* json=[lines[i] dataUsingEncoding:NSUTF8StringEncoding];
        NSDictionary* record=[NSJSONSerialization JSONObjectWithData:json options:0 error:nil];
        assert(record.count==6&&[record[@"event"] isEqual:@"lifecycle"]&&[record[@"reason"] isEqual:reasons[i]]);
        assert([record[@"pid"] intValue]==getpid()&&[record[@"registry_id"] unsignedLongLongValue]==4294968492ULL);
        assert([record[@"timestamp"] length]>20&&[record[@"monotonic_ms"] doubleValue]>0&&json.length<512);
    }
    puts("Actual Main lifecycle diagnostics: eight enum reasons, timestamps/PID/selected ID, bounded payload without credentials");
}
