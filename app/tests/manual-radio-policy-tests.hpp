// SPDX-License-Identifier: GPL-2.0-only
// Real Disconnect, polling and manual reconnect paths. Only the USB/Keychain
// boundaries are replaced; both legacy and new status reporters are covered.
@interface ManualRadioDriver : ScreenLifecycleDriver {
@public
    std::map<uint64_t,bool> disabled;
}
@property(nonatomic,assign) BOOL reportsDisabled;
@property(nonatomic,assign) unsigned leaves,joins;
@end
@implementation ManualRadioDriver
- (uint32_t)exchangeRegistryID:(uint64_t)registryID selector:(uint32_t)selector
                        input:(const void*)input length:(size_t)length
                       output:(void*)output capacity:(size_t)capacity {
    if(selector==RTW_CONTROL_LEAVE||selector==RTW_CONTROL_JOIN) {
        assert(rtw_control_validate_request(selector,input,length)==RTW_CONTROL_VALID&&capacity==sizeof(RTWControlReply));
        if(selector==RTW_CONTROL_LEAVE)++self.leaves;else ++self.joins;
        disabled[registryID]=selector==RTW_CONTROL_LEAVE;
        auto& adapter=adapters.at(registryID);
        adapter.connected=adapter.radio=adapter.running=selector==RTW_CONTROL_JOIN;
        adapter.remaining=0;adapter.started=false;
        auto* reply=static_cast<RTWControlReply*>(output);*reply={};reply->version=1;return 0;
    }
    if(selector==RTW_CONTROL_SCAN)disabled[registryID]=false;
    const auto result=[super exchangeRegistryID:registryID selector:selector input:input length:length output:output capacity:capacity];
    if(!result&&selector==RTW_CONTROL_STATUS&&self.reportsDisabled&&disabled[registryID])
        static_cast<RTWControlStatus*>(output)->flags|=RTW_CONTROL_USER_RADIO_DISABLED;
    return result;
}
@end
@interface ManualRadioApp : ScreenLifecycleApp
@end
@implementation ManualRadioApp
- (id<RTWCredentialStoring>)credentials {return [AutomaticAdmissionStore new];}
@end
static void testManualRadioOffSurvivesLifecycle() {
    for(bool reportsDisabled:{false,true}) {
        ManualRadioDriver* driver=[ManualRadioDriver new];driver.reportsDisabled=reportsDisabled;
        ManualRadioApp* app=[ManualRadioApp new];[app prepareScreen:driver paused:NO joining:NO];
        [app setValue:@YES forKey:@"savedNetworksLoaded"];
        [app disconnect:nil];drainControlWork(app);
        assert(driver.leaves==1&&driver->adapters[3003].requests==0&&app.manualPause&&
               "Manual Disconnect must not be undone by background RF refresh");
        for(unsigned i=0;i<3;++i){[app poll:nil];drainControlWork(app);}
        [app workspaceScreensDidSleep:nil];[app workspaceScreensDidWake:nil];drainControlWork(app);
        [app workspaceWillSleep:nil];[app workspaceDidWake:nil];drainControlWork(app);
        assert(driver->adapters[3003].requests==0&&driver.joins==0&&driver->disabled[3003]&&app.manualPause);
        // The selected adapter owns radio intent; a different explicitly
        // selected USB must remain discoverable while this one is off.
        const auto epoch=app.workEpoch;
        NSMenuItem* other=[NSMenuItem new];other.representedObject=@4004;[app selectAdapter:other];drainControlWork(app);
        assert([app selectedAdapterID]==4004&&app.workEpoch!=epoch&&driver->adapters[4004].requests==1);
        [app disconnect:nil];drainControlWork(app);
        assert(driver.leaves==2&&driver->disabled[4004]);
        other.representedObject=@3003;[app selectAdapter:other];drainControlWork(app);
        assert(driver->adapters[3003].requests==0&&driver->disabled[3003]);
        [app refresh:nil];drainControlWork(app);[app poll:nil];drainControlWork(app);
        assert(driver->adapters[3003].requests==1&&!driver->disabled[3003]&&driver.joins==0);
        [app disconnect:nil];drainControlWork(app);assert(driver.leaves==3&&driver->disabled[3003]);
        [app connectSSID:[@"Lab" dataUsingEncoding:NSUTF8StringEncoding]];
        NSDate* deadline=[NSDate dateWithTimeIntervalSinceNow:5];
        while([[app valueForKey:@"credentialBusy"] boolValue]&&deadline.timeIntervalSinceNow>0)
            [NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]];
        drainControlWork(app);
        assert(driver.joins==1&&!driver->disabled[3003]&&!app.manualPause);
        other.representedObject=@4004;[app selectAdapter:other];drainControlWork(app);
        assert(driver->adapters[4004].requests==1&&driver->disabled[4004]);
        [app refresh:nil];drainControlWork(app);assert(driver->adapters[4004].requests==2&&!driver->disabled[4004]);
        [app finishScreenTest];
    }
    ManualRadioDriver* restoredDriver=[ManualRadioDriver new];
    ManualRadioApp* restored=[ManualRadioApp new];[restored prepareScreen:restoredDriver paused:YES joining:NO];
    restoredDriver->adapters[3003].connected=false;
    [restored setValue:@YES forKey:@"savedNetworksLoaded"];[restored poll:nil];drainControlWork(restored);
    assert(!restoredDriver->adapters[3003].requests);
    [restored useAdapter:4004];drainControlWork(restored);assert(restoredDriver->adapters[4004].requests==1);
    [restored finishScreenTest];
    // The kernel's per-adapter manual-off bit also protects a fresh app
    // session whose local autojoin preference has not been paused.
    ManualRadioDriver* driver=[ManualRadioDriver new];driver.reportsDisabled=YES;
    ManualRadioApp* app=[ManualRadioApp new];[app prepareScreen:driver paused:NO joining:NO];
    driver->adapters[3003].connected=false;driver->adapters[3003].radio=false;driver->disabled[3003]=true;
    [app setValue:@YES forKey:@"savedNetworksLoaded"];[app poll:nil];drainControlWork(app);
    assert(!driver->adapters[3003].requests&&!driver.joins&&driver->disabled[3003]);
    [app finishScreenTest];
    puts("Actual Main manual radio off: legacy/new status, repeated poll, screen/system wake and adapter switch preserve intent; manual SCAN/JOIN resume");
}
