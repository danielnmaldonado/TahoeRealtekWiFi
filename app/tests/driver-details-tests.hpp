// SPDX-License-Identifier: GPL-2.0-only
// Restoring a synchronous details alert, duplicate registry reads or a stale
// cross-adapter report must fail this production-Main regression.
static dispatch_semaphore_t detailsEntered,detailsGate;
static std::atomic<unsigned> detailsReads{0};
static std::atomic<bool> detailsReadOnMain{false};
NSString* menuDriverDiagnosticsReport(uint64_t registryID) {
    assert(registryID==3003);detailsReads.fetch_add(1);
    detailsReadOnMain.store(NSThread.isMainThread);
    dispatch_semaphore_signal(detailsEntered);
    assert(!dispatch_semaphore_wait(detailsGate,dispatch_time(DISPATCH_TIME_NOW,5LL*NSEC_PER_SEC)));
    return @"Driver build: 64\nRX errors: 12\nValidated BlockAck filtered: 89\n";
}
@interface DriverDetailsApp : WiFiApp
@property(nonatomic,assign) unsigned shownReports;
@property(nonatomic,assign) unsigned shownMessages;
@property(nonatomic,copy) NSString* report;
@property(nonatomic,copy) NSString* reportExplanation;
- (void)prepare;
@end
@implementation DriverDetailsApp
- (void)updateAppearance {}
- (void)poll:(id)sender {(void)sender;}
- (void)prepare {
    _selection.select(3003);_adapterEpoch.store(_selection.generation);
    _authorizedSession=_selectedPresent=_hasStatus=YES;
    rtwui::DeviceRecord first{},second{};first.registryID=3003;second.registryID=4004;_adapters={first,second};
    _status.version=1;_status.state=4;
    _status.flags=RTW_CONTROL_DEVICE|RTW_CONTROL_RADIO|RTW_CONTROL_RUNNING|RTW_CONTROL_AUTHORIZED;
    _interface=@"enFixture · waiting for DHCP";
}
- (void)showReport:(NSString*)report title:(NSString*)title explanation:(NSString*)explanation {
    assert(NSThread.isMainThread&&[title isEqualToString:@"USB Wi-Fi diagnostics"]);
    ++self.shownReports;self.report=report;self.reportExplanation=explanation;
}
- (void)showMessage:(NSString*)title detail:(NSString*)detail {
    (void)title;(void)detail;++self.shownMessages;
}
@end
static void testDriverDetailsReport() {
    for(unsigned mode=0;mode<5;++mode) {
        DriverDetailsApp* app=[DriverDetailsApp new];[app prepare];
        detailsEntered=dispatch_semaphore_create(0);detailsGate=dispatch_semaphore_create(0);
        detailsReads.store(0);detailsReadOnMain.store(false);
        [app details:nil];[app details:nil];
        assert(!dispatch_semaphore_wait(detailsEntered,dispatch_time(DISPATCH_TIME_NOW,NSEC_PER_SEC))&&
            "Driver details must enqueue one background registry snapshot");
        assert(detailsReads.load()==1&&!detailsReadOnMain.load()&&!app.shownMessages);
        if(mode==1)[app useAdapter:4004];
        if(mode==2)[app workspaceWillSleep:nil];
        if(mode==3)[app workspaceBecameInactive:nil];
        if(mode==4)[app applicationWillTerminate:[NSNotification notificationWithName:@"TestTermination" object:nil]];
        dispatch_semaphore_signal(detailsGate);
        NSDate* deadline=[NSDate dateWithTimeIntervalSinceNow:5];
        while([[app valueForKey:@"driverReportBusy"] boolValue]&&deadline.timeIntervalSinceNow>0)
            [NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]];
        assert(![[app valueForKey:@"driverReportBusy"] boolValue]);
        assert(app.shownReports==(mode==0?1U:0U)&&detailsReads.load()==1);
        if(!mode) {
            assert([app.report containsString:@"Driver build: 64"]);
            assert([app.report containsString:@"Connected via WPA2"]);
            assert([app.report containsString:@"enFixture"]);
            assert([app.report containsString:@"net80211 state: 4"]);
            assert([app.reportExplanation containsString:@"cumulative"]);
            assert([app.reportExplanation containsString:@"BlockAck"]);
        }
    }
    puts("Actual Main driver details: one off-main read, copyable context and adapter/sleep/session/termination fences passed");
}
