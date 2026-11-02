// SPDX-License-Identifier: GPL-2.0-only
// Production Main policy and callbacks; only USB/service and time are replaced.
// No application launch, user defaults, diagnostic logging or Keychain I/O.
#include <map>
struct AutomaticAdapterFixture {
    bool connected=false,radio=false,stall=false,emptyMap=false,running=false,present=true;
    unsigned remaining=0,requests=0,reads=0;
    uint32_t commandError=0,usbError=0,state=0;
    bool started=false,complete=false;
};
@interface AutomaticRefreshDriver : RTWDriverConnection {
@public
    std::map<uint64_t,AutomaticAdapterFixture> adapters;
}
@property(nonatomic,strong) dispatch_semaphore_t scanEntered;
@property(nonatomic,strong) dispatch_semaphore_t scanGate;
@end
@implementation AutomaticRefreshDriver
- (uint32_t)exchangeRegistryID:(uint64_t)registryID selector:(uint32_t)selector
                        input:(const void*)input length:(size_t)length
                       output:(void*)output capacity:(size_t)capacity {
    assert(capacity==rtwui::outputSize(selector));
    if(selector==rtwui::listSelector) {
        assert(!registryID&&!input&&!length);
        auto* list=static_cast<rtwui::DeviceList*>(output);*list={};
        list->version=rtwui::ipcVersion;
        for(const auto& adapter:adapters)list->devices[list->count++].registryID=adapter.first;
        return 0;
    }
    assert(adapters.count(registryID)&&
        rtw_control_validate_request(selector,input,length)==RTW_CONTROL_VALID);
    auto& adapter=adapters.at(registryID);
    if(selector==RTW_CONTROL_CHANNEL_MAP) {
        auto* map=static_cast<RTWControlChannelMap*>(output);*map={};map->version=1;
        if(!adapter.emptyMap)map->channels[6/8]|=1U<<(6%8);
        return 0;
    }
    if(selector==RTW_CONTROL_STATUS) {
        auto* status=static_cast<RTWControlStatus*>(output);*status={};status->version=1;
        status->flags=(adapter.present?RTW_CONTROL_DEVICE:0)|(adapter.radio?RTW_CONTROL_RADIO:0);
        if(adapter.running)status->flags|=RTW_CONTROL_RUNNING;
        status->usbError=adapter.usbError;status->state=adapter.state;
        if(adapter.connected) {
            status->flags|=RTW_CONTROL_RUNNING|RTW_CONTROL_AUTHORIZED;status->state=4;
        } else if(adapter.remaining||adapter.stall) {
            status->flags|=RTW_CONTROL_RADIO|RTW_CONTROL_RUNNING|RTW_CONTROL_SCANNING;
            status->state=1;if(adapter.remaining)--adapter.remaining;
        } else if(adapter.started)adapter.complete=true;
        return 0;
    }
    if(selector==RTW_CONTROL_SCAN_RESULTS) {
        ++adapter.reads;
        auto* results=static_cast<RTWControlScanResults*>(output);*results={};
        results->version=1;results->count=1;
        const std::string name=(adapter.complete?"Fresh":"Old")+std::to_string(registryID);
        auto& row=results->records[0];row.ssidLength=static_cast<uint8_t>(name.size());
        std::memcpy(row.ssid,name.data(),name.size());row.channel=6;row.rssi=-48;row.securityFlags=7;
        return 0;
    }
    assert(selector==RTW_CONTROL_SCAN&&"Automatic refresh must never send JOIN or LEAVE");
    ++adapter.requests;
    if(registryID==3003&&self.scanGate) {
        dispatch_semaphore_signal(self.scanEntered);
        assert(!dispatch_semaphore_wait(self.scanGate,dispatch_time(DISPATCH_TIME_NOW,5LL*NSEC_PER_SEC)));
    }
    if(adapter.commandError)return adapter.commandError;
    adapter.started=true;adapter.complete=false;adapter.remaining=1;
    // enableStation leaves the initialized interface RUNNING after a scan,
    // even when no SSID is authorized and net80211 has returned to idle.
    adapter.radio=true;adapter.running=true;
    auto* reply=static_cast<RTWControlReply*>(output);*reply={};reply->version=1;return 0;
}
@end
@interface AutomaticRefreshApp : WiFiApp
@property(nonatomic,assign) double testNow;
@property(nonatomic,assign) unsigned credentialReloads;
- (void)prepare:(AutomaticRefreshDriver*)driver;
- (BOOL)showsNetwork:(const char*)name;
@end
@implementation AutomaticRefreshApp
- (double)uptime {return self.testNow;}
- (void)updateAppearance {}
- (void)reloadSavedNetworks {++self.credentialReloads;}
- (id<RTWCredentialStoring>)credentials {
    assert(false&&"Automatic refresh must not access credentials");return nil;
}
- (void)prepare:(AutomaticRefreshDriver*)driver {
    self.testNow=100;_driver=driver;_authorizedSession=YES;_savedNetworksLoaded=YES;
    _lastDeviceListAt=_lastNetworkCacheAt=-1;_lastAutoScanAt=-1;
    _menu=menuForControl();
    rtwui::DeviceList list{};list.version=rtwui::ipcVersion;list.count=2;
    list.devices[0].registryID=3003;list.devices[1].registryID=4004;
    [self applyAdapters:list];[self useAdapter:3003];
    _queue=serialWorkQueue("org.research.TahoeUSBWiFi.test.automatic-refresh");
}
- (BOOL)showsNetwork:(const char*)name {
    for(const auto& row:_networks)
        if(row.ssidLength==std::strlen(name)&&!std::memcmp(row.ssid,name,row.ssidLength))return YES;
    return NO;
}
@end
static AutomaticRefreshApp* automaticRefreshApp(AutomaticRefreshDriver* driver) {
    driver->adapters[3003].connected=true;driver->adapters[3003].radio=true;
    driver->adapters[4004]={};
    AutomaticRefreshApp* app=[AutomaticRefreshApp new];[app prepare:driver];return app;
}
static void testAutomaticRefreshSwitch() {
    // Removing the switch-to-idle SCAN must fail this actual-Main regression.
    AutomaticRefreshDriver* driver=[AutomaticRefreshDriver new];
    AutomaticRefreshApp* app=automaticRefreshApp(driver);
    [app poll:nil];drainControlWork(app);
    assert(driver->adapters[3003].requests==0&&[app showsNetwork:"Old3003"]);
    [app useAdapter:4004];drainControlWork(app);
    // No RADIO flag is normal before enableStation(), which SCAN starts.
    assert(driver->adapters[4004].requests==1&&driver->adapters[3003].requests==0);
    assert(![app showsNetwork:"Old3003"]);
    [app poll:nil];drainControlWork(app);
    assert([app showsNetwork:"Fresh4004"]&&![[app valueForKey:@"refreshingNetworks"] boolValue]);
    [app useAdapter:3003];drainControlWork(app);
    assert(driver->adapters[3003].requests==0&&[app showsNetwork:"Old3003"]);
    assert(!app.credentialReloads);
    AutomaticRefreshDriver* runningDriver=[AutomaticRefreshDriver new];
    AutomaticRefreshApp* running=automaticRefreshApp(runningDriver);
    runningDriver->adapters[3003].connected=false;runningDriver->adapters[3003].running=true;
    [running poll:nil];drainControlWork(running);
    assert(runningDriver->adapters[3003].requests==1);
    puts("Actual Main automatic refresh: idle USB switch scans, fresh results replace previous adapter, connected USB stays read-only");
}
static void automaticPoll(AutomaticRefreshApp* app,double now) {
    app.testNow=now;[app poll:nil];drainControlWork(app);
}
static void testAutomaticRefreshFreshness() {
    AutomaticRefreshDriver* driver=[AutomaticRefreshDriver new];
    AutomaticRefreshApp* app=automaticRefreshApp(driver);driver->adapters[3003].connected=false;
    automaticPoll(app,100);automaticPoll(app,100);
    assert(driver->adapters[3003].requests==1&&[app showsNetwork:"Fresh3003"]);
    [app useAdapter:4004];drainControlWork(app);automaticPoll(app,100);
    assert(driver->adapters[4004].requests==1&&[app showsNetwork:"Fresh4004"]);
    // Cache reads and switching away/back must neither age nor restart RF work.
    [app useAdapter:3003];drainControlWork(app);automaticPoll(app,129.99);
    assert(driver->adapters[3003].requests==1&&[app showsNetwork:"Fresh3003"]);
    app.testNow=130;[app menuNeedsUpdate:[app valueForKey:@"menu"]];drainControlWork(app);
    assert(driver->adapters[3003].requests==2&&driver->adapters[4004].requests==1);
    [app refresh:nil];drainControlWork(app);assert(driver->adapters[3003].requests==2);
    automaticPoll(app,130);
    driver->adapters[3003].connected=true;automaticPoll(app,200);
    assert(driver->adapters[3003].requests==2); // stale connected cache stays read-only
    puts("Actual Main refresh freshness: per-ID RF completion, 30-second expiry/menu-open scan, duplicate/manual coalescing passed");
}
static void testAutomaticRefreshFailures() {
    AutomaticRefreshDriver* driver=[AutomaticRefreshDriver new];
    AutomaticRefreshApp* app=automaticRefreshApp(driver);
    driver->adapters[3003].connected=false;driver->adapters[4004].connected=true;
    driver->adapters[3003].commandError=uint32_t(kIOReturnBusy);
    automaticPoll(app,100);assert(driver->adapters[3003].requests==1);
    automaticPoll(app,103.99);assert(driver->adapters[3003].requests==1);
    automaticPoll(app,104);assert(driver->adapters[3003].requests==2);
    automaticPoll(app,111.99);assert(driver->adapters[3003].requests==2);
    automaticPoll(app,112);assert(driver->adapters[3003].requests==3);
    [app useAdapter:4004];drainControlWork(app);[app useAdapter:3003];drainControlWork(app);
    automaticPoll(app,411.99);assert(driver->adapters[3003].requests==3);
    driver->adapters[3003].commandError=0;
    automaticPoll(app,412);assert(driver->adapters[3003].requests==4);
    automaticPoll(app,412);assert([app showsNetwork:"Fresh3003"]);
    assert(![[app valueForKey:@"refreshingNetworks"] boolValue]);
    puts("Actual Main refresh failures: 4/8-second backoff, three-failure budget, 300-second quiet round survives adapter switch and recovers");
}
static void testAutomaticRefreshTimeout() {
    AutomaticRefreshDriver* driver=[AutomaticRefreshDriver new];
    AutomaticRefreshApp* app=automaticRefreshApp(driver);driver->adapters[3003].connected=false;
    automaticPoll(app,100);assert(driver->adapters[3003].requests==1);
    driver->adapters[3003].stall=true;
    automaticPoll(app,129.99);assert([[app valueForKey:@"refreshingNetworks"] boolValue]);
    automaticPoll(app,130);
    assert(![[app valueForKey:@"refreshingNetworks"] boolValue]);
    assert([[app valueForKey:@"notice"] containsString:@"timed out"]);
    automaticPoll(app,134);assert(driver->adapters[3003].requests==1);
    driver->adapters[3003].stall=false;
    automaticPoll(app,134);
    assert(driver->adapters[3003].requests==1&&[app showsNetwork:"Fresh3003"]);
    assert(![[app valueForKey:@"refreshingNetworks"] boolValue]);
    automaticPoll(app,164);assert(driver->adapters[3003].requests==2);
    automaticPoll(app,164);assert([app showsNetwork:"Fresh3003"]);
    // An already running driver scan is observed and completed without SCAN.
    AutomaticRefreshDriver* externalDriver=[AutomaticRefreshDriver new];
    AutomaticRefreshApp* external=automaticRefreshApp(externalDriver);
    externalDriver->adapters[3003].connected=false;
    externalDriver->adapters[3003].started=true;externalDriver->adapters[3003].remaining=1;
    automaticPoll(external,100);automaticPoll(external,100);
    assert(externalDriver->adapters[3003].requests==0&&[external showsNetwork:"Fresh3003"]);
    automaticPoll(external,129.99);assert(externalDriver->adapters[3003].requests==0);
    puts("Actual Main refresh timeout: stuck scan releases after 30 seconds, retries stay bounded, existing driver scan is coalesced");
}
static void testAutomaticRefreshLifecycle() {
    AutomaticRefreshDriver* driver=[AutomaticRefreshDriver new];
    AutomaticRefreshApp* app=automaticRefreshApp(driver);driver->adapters[3003].connected=false;
    automaticPoll(app,100);automaticPoll(app,100);
    [app workspaceWillSleep:nil];[app workspaceDidWake:nil];drainControlWork(app);
    assert(driver->adapters[3003].requests==2&&app.credentialReloads==1);
    automaticPoll(app,100);
    rtwui::DeviceList list{};list.version=rtwui::ipcVersion;list.count=1;
    list.devices[0].registryID=4004;[app applyAdapters:list];
    assert([app selectedAdapterID]==3003&&![[app valueForKey:@"selectedPresent"] boolValue]);
    list.count=2;list.devices[1].registryID=3003;[app applyAdapters:list];drainControlWork(app);
    assert(driver->adapters[3003].requests==3&&[app selectedAdapterID]==3003);
    automaticPoll(app,100);assert([app showsNetwork:"Fresh3003"]);
    puts("Actual Main refresh lifecycle: wake invalidates scan freshness, same-ID hotplug refreshes selected USB without selecting the other one");
}
static void testAutomaticRefreshGuards() {
    for(unsigned mode=0;mode<11;++mode) {
        AutomaticRefreshDriver* driver=[AutomaticRefreshDriver new];
        AutomaticRefreshApp* app=automaticRefreshApp(driver);driver->adapters[3003].connected=false;
        auto& adapter=driver->adapters[3003];
        switch(mode) {
        case 0:adapter.connected=true;break;
        case 1:adapter.present=false;break;
        case 2:adapter.state=2;break;
        case 3:adapter.state=3;break;
        case 4:adapter.state=4;break;
        case 5:adapter.usbError=uint32_t(kIOReturnIOError);break;
        case 6:adapter.emptyMap=true;break;
        case 7:[app setValue:@YES forKey:@"joining"];break;
        case 8:[app setValue:@YES forKey:@"credentialBusy"];break;
        case 9:[app setValue:@YES forKey:@"sleeping"];break;
        case 10:[app setValue:@YES forKey:@"inactiveSession"];break;
        }
        automaticPoll(app,100);assert(adapter.requests==0);
    }
    AutomaticRefreshDriver* disabledDriver=[AutomaticRefreshDriver new];
    AutomaticRefreshApp* disabled=automaticRefreshApp(disabledDriver);
    [disabled setValue:@NO forKey:@"authorizedSession"];
    automaticPoll(disabled,100);
    assert(!disabledDriver->adapters[3003].requests&&!disabledDriver->adapters[3003].reads);
    puts("Actual Main refresh admission: connected/missing/association/USB fault/empty channels/join/credentials/sleep/session/service guards passed");
}
static void waitForAutomaticCommand(AutomaticRefreshApp* app) {
    NSDate* deadline=[NSDate dateWithTimeIntervalSinceNow:5];
    while(![[app valueForKey:@"commandPending"] boolValue]&&deadline.timeIntervalSinceNow>0)
        [NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]];
    assert([[app valueForKey:@"commandPending"] boolValue]);
}
static void testAutomaticRefreshCancellation() {
    // Queue cancellation before any old STATUS/cache/SCAN reaches the boundary.
    AutomaticRefreshDriver* queuedDriver=[AutomaticRefreshDriver new];
    AutomaticRefreshApp* queued=automaticRefreshApp(queuedDriver);queuedDriver->adapters[3003].connected=false;
    dispatch_queue_t queuedQueue=[queued valueForKey:@"queue"];
    dispatch_semaphore_t queuedGate=dispatch_semaphore_create(0);
    dispatch_async(queuedQueue,^{assert(!dispatch_semaphore_wait(queuedGate,dispatch_time(DISPATCH_TIME_NOW,5LL*NSEC_PER_SEC)));});
    [queued poll:nil];[queued useAdapter:4004];dispatch_semaphore_signal(queuedGate);drainControlWork(queued);
    assert(queuedDriver->adapters[3003].requests==0&&queuedDriver->adapters[3003].reads==0);
    assert(queuedDriver->adapters[4004].requests==1&&[queued selectedAdapterID]==4004);
    // A completed STATUS can queue SCAN behind another operation. Invalidate
    // that command's generation before its worker is allowed to execute.
    for(BOOL suspend:{NO,YES}) {
        AutomaticRefreshDriver* driver=[AutomaticRefreshDriver new];
        AutomaticRefreshApp* app=automaticRefreshApp(driver);driver->adapters[3003].connected=false;
        dispatch_queue_t queue=[app valueForKey:@"queue"];
        dispatch_semaphore_t gate=dispatch_semaphore_create(0);
        [app poll:nil];
        dispatch_async(queue,^{assert(!dispatch_semaphore_wait(gate,dispatch_time(DISPATCH_TIME_NOW,5LL*NSEC_PER_SEC)));});
        waitForAutomaticCommand(app);
        if(suspend)[app workspaceWillSleep:nil];else [app useAdapter:4004];
        dispatch_semaphore_signal(gate);dispatch_sync(queue,^{});drainControlWork(app);
        assert(driver->adapters[3003].requests==0);
        if(suspend) {
            assert(!driver->adapters[4004].requests);
            [app workspaceDidWake:nil];drainControlWork(app);
            assert(driver->adapters[3003].requests==1);
        } else assert(driver->adapters[4004].requests==1&&![app showsNetwork:"Old3003"]);
    }
    // Already-entered USB I/O cannot be canceled; its result still must not
    // clear or complete the next adapter's refresh.
    AutomaticRefreshDriver* driver=[AutomaticRefreshDriver new];
    AutomaticRefreshApp* app=automaticRefreshApp(driver);driver->adapters[3003].connected=false;
    driver.scanEntered=dispatch_semaphore_create(0);driver.scanGate=dispatch_semaphore_create(0);
    [app poll:nil];waitForAutomaticCommand(app);
    assert(!dispatch_semaphore_wait(driver.scanEntered,dispatch_time(DISPATCH_TIME_NOW,5LL*NSEC_PER_SEC)));
    [app useAdapter:4004];dispatch_semaphore_signal(driver.scanGate);drainControlWork(app);
    assert(driver->adapters[3003].requests==1&&driver->adapters[4004].requests==1);
    assert([app selectedAdapterID]==4004&&![app showsNetwork:"Old3003"]);
    automaticPoll(app,100);assert([app showsNetwork:"Fresh4004"]);
    puts("Actual Main refresh cancellation: queued polls/SCAN, sleep epoch and in-flight cross-ID callbacks are fenced");
}
static void testAutomaticNetworkRefresh() {
    testAutomaticRefreshSwitch();testAutomaticRefreshFreshness();
    testAutomaticRefreshFailures();testAutomaticRefreshTimeout();
    testAutomaticRefreshLifecycle();testAutomaticRefreshGuards();testAutomaticRefreshCancellation();
}
