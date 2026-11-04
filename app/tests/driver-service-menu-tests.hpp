// SPDX-License-Identifier: GPL-2.0-only
// Real menu and connection flow; only ServiceManagement and IPC are replaced.
@interface ServiceMenuBackend : RTWDriverServiceManager
@property(nonatomic,assign) RTWLoginItemState fixtureState;
@property(nonatomic,assign) unsigned enables,disables;
@end
@implementation ServiceMenuBackend
- (RTWLoginItemState)state {return self.fixtureState;}
- (BOOL)enableAndReturnError:(NSError**)error {(void)error;++self.enables;return YES;}
- (BOOL)disableAndReturnError:(NSError**)error {(void)error;++self.disables;return YES;}
@end
@interface ServiceMenuDriver : RTWDriverConnection
@property(nonatomic,assign) unsigned lists;
@end
@implementation ServiceMenuDriver
- (uint32_t)exchangeRegistryID:(uint64_t)registryID selector:(uint32_t)selector
    input:(const void*)input length:(size_t)length output:(void*)output capacity:(size_t)capacity {
    (void)registryID;(void)input;(void)length;(void)output;(void)capacity;
    assert(selector==rtwui::listSelector);++self.lists;return rtwui::transportError;
}
@end
@interface ServiceMenuApp : WiFiApp
- (void)prepareService:(ServiceMenuBackend*)backend driver:(ServiceMenuDriver*)driver;
@end
@implementation ServiceMenuApp
- (void)updateAppearance {}
- (void)prepareService:(ServiceMenuBackend*)backend driver:(ServiceMenuDriver*)driver {
    _preferences=[[NSUserDefaults alloc]initWithSuiteName:
        [@"org.research.TahoeUSBWiFi.test.service-menu." stringByAppendingString:NSUUID.UUID.UUIDString]];
    _connectionPreferencesLoaded=YES;_savedNetworksLoaded=YES;
    _serviceManager=backend;_driver=driver;_menu=menuForControl();
    _queue=serialWorkQueue("org.research.TahoeUSBWiFi.test.service-menu");
}
@end
static void drainServiceConnection(WiFiApp* app) {
    NSDate* deadline=[NSDate dateWithTimeIntervalSinceNow:5];
    while([[app valueForKey:@"authorizing"] boolValue]&&deadline.timeIntervalSinceNow>0)
        [NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]];
    assert(![[app valueForKey:@"authorizing"] boolValue]);
}
static void testDriverServiceMenuStates() {
    for(auto state:{RTWLoginItemStateEnabled,RTWLoginItemStateRequiresApproval,
                    RTWLoginItemStateNotRegistered,RTWLoginItemStateNotFound}) {
        ServiceMenuBackend* backend=[ServiceMenuBackend new];backend.fixtureState=state;
        ServiceMenuDriver* driver=[ServiceMenuDriver new];
        ServiceMenuApp* app=[ServiceMenuApp new];[app prepareService:backend driver:driver];
        [app rebuildMenu];NSMenu* menu=[app valueForKey:@"menu"];
        assert(([menu itemWithTitle:@"Enable driver service…"]!=nil)==
            (state==RTWLoginItemStateNotRegistered||state==RTWLoginItemStateNotFound));
        assert(([menu itemWithTitle:@"Retry driver connection…"]!=nil)==(state==RTWLoginItemStateEnabled));
        assert(([menu itemWithTitle:@"Allow driver service…"]!=nil)==(state==RTWLoginItemStateRequiresApproval));
        assert(([menu itemWithTitle:@"Repair driver service…"]!=nil)==(state==RTWLoginItemStateEnabled));
        assert(backend.enables==0&&backend.disables==0&&driver.lists==0);
        if(state!=RTWLoginItemStateEnabled)continue;
        [app connectDriverAutomatically];drainServiceConnection(app);
        NSString* notice=[app valueForKey:@"notice"];
        assert([notice containsString:@"0xe1000002"]&&[notice containsString:@"enabled"]);
        assert(![notice containsString:@"approval"]);
        [app rebuildMenu];NSMenuItem* retry=[menu itemWithTitle:@"Retry driver connection…"];
        assert(retry.enabled&&retry.action==NSSelectorFromString(@"retryDriverConnection:"));
        assert([NSApp sendAction:retry.action to:app from:retry]);drainServiceConnection(app);
        assert(driver.lists==2&&backend.enables==0&&backend.disables==0);
    }
    puts("Actual Main service menu: enable/approval/connection failure separated; retry preserves registration and reports the error code");
}
