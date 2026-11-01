// SPDX-License-Identifier: GPL-2.0-only
// Exercise the real AppKit menu update, which can override setEnabled:.
#define main rtwui_application_main
#include "../Main.mm"
#undef main
#include <cassert>
#include <objc/runtime.h>
static WiFiApp* modalTarget;
static unsigned modalConnections;
@interface RecordingWiFiApp : WiFiApp
@property(nonatomic,copy) NSString* shownDetail;
@end
@implementation RecordingWiFiApp
- (void)connectSSID:(NSData*)ssid {(void)ssid;++modalConnections;}
- (void)showMessage:(NSString*)title detail:(NSString*)detail {(void)title;self.shownDetail=detail;}
@end
@interface FakeLoginItems : NSObject <RTWLoginItemManaging>
@property(nonatomic,assign) RTWLoginItemState state;
@property(nonatomic,assign) BOOL rejectChanges;
@property(nonatomic,assign) NSUInteger settingsOpens;
@end
@implementation FakeLoginItems
- (BOOL)setEnabled:(BOOL)enabled error:(NSError**)error {
    if(self.rejectChanges) {
        if(error)*error=[NSError errorWithDomain:NSCocoaErrorDomain code:1 userInfo:nil];
        return NO;
    }
    self.state=enabled?RTWLoginItemStateEnabled:RTWLoginItemStateNotRegistered;
    return YES;
}
- (BOOL)repairRegistrationAndReturnError:(NSError**)error {
    return [self setEnabled:YES error:error];
}
- (void)openSystemSettings {self.settingsOpens+=1;}
@end
@interface NoModalWiFiApp : WiFiApp
@property(nonatomic,assign) NSUInteger alerts;
@end
@implementation NoModalWiFiApp
- (void)showMessage:(NSString*)title detail:(NSString*)detail {
    (void)title;(void)detail;self.alerts+=1;
}
@end
@interface LifecycleWiFiApp : WiFiApp
@property(nonatomic,assign) NSUInteger polls;
@property(nonatomic,assign) NSUInteger credentialReloads;
@end
@implementation LifecycleWiFiApp
- (void)poll:(id)sender {(void)sender;self.polls+=1;}
- (void)reloadSavedNetworks {self.credentialReloads+=1;}
@end
// Keep the real signal memory, link state, image cache and status button. Only
// the clock/timer are controlled; no driver, settings or credentials are used.
@interface SignalWiFiApp : WiFiApp
@property(nonatomic,assign) double testNow;
- (void)prepareSignalTest;
- (void)observeRSSI:(int)rssi at:(double)now held:(BOOL)held;
- (void)delayStatusAt:(double)now;
- (void)disconnectAt:(double)now;
- (void)advanceTo:(double)now;
- (NSStatusBarButton*)signalButton;
- (void)finishSignalTest;
@end
@implementation SignalWiFiApp
- (void)refreshPollingTimer {}
- (double)uptime {return self.testNow;}
- (void)prepareSignalTest {
    _item=[NSStatusBar.systemStatusBar statusItemWithLength:24];
    _displayedIconKind=-1;_selection.select(3003);
    _authorizedSession=YES;_selectedPresent=YES;_hasStatus=YES;
    _status.version=1;_status.flags=15|RTW_CONTROL_SIGNAL_TRACKED;
    _status.state=4;_status.channel=6;_status.ssidLength=3;
    std::memcpy(_status.ssid,"Lab",3);
}
- (void)observeRSSI:(int)rssi at:(double)now held:(BOOL)held {
    self.testNow=now;_lastStatusSuccess=now;_statusReadDelayed=NO;
    _status.rssi=rssi;
    if(held)_status.flags|=RTW_CONTROL_SIGNAL_HELD;
    else _status.flags&=~RTW_CONTROL_SIGNAL_HELD;
    _signal.observe(_status,now);[self updateAppearance];
}
- (void)delayStatusAt:(double)now {
    self.testNow=now;_statusReadDelayed=YES;[self updateAppearance];
}
- (void)disconnectAt:(double)now {
    self.testNow=now;_statusReadDelayed=NO;
    _status.flags&=~RTW_CONTROL_AUTHORIZED;_status.rssi=0;
    _signal.observe(_status,now);[self updateAppearance];
}
- (void)advanceTo:(double)now {self.testNow=now;[self updateAppearance];}
- (NSStatusBarButton*)signalButton {return _item.button;}
- (void)finishSignalTest {
    [NSStatusBar.systemStatusBar removeStatusItem:_item];_item=nil;
}
@end
static NSData* renderedStatusButton(NSStatusBarButton* button) {
    NSBitmapImageRep* bitmap=[button bitmapImageRepForCachingDisplayInRect:button.bounds];
    assert(bitmap);
    [button cacheDisplayInRect:button.bounds toBitmapImageRep:bitmap];
    return [NSData dataWithBytes:bitmap.bitmapData length:bitmap.bytesPerRow*bitmap.pixelsHigh];
}
static void testSignalAppearance() {
    SignalWiFiApp* app=[SignalWiFiApp new];[app prepareSignalTest];
    [app observeRSSI:-42 at:0 held:NO];
    NSStatusBarButton* button=[app signalButton];
    NSImage* strong=button.image;NSData* strongPixels=renderedStatusButton(button);
    [app observeRSSI:-64 at:2 held:NO];
    assert(button.image==strong); // one low poll is rejected by the median
    [app observeRSSI:-64 at:4 held:NO];
    NSImage* medium=button.image;NSData* mediumPixels=renderedStatusButton(button);
    [app observeRSSI:-90 at:6 held:NO];[app observeRSSI:-90 at:8 held:NO];
    NSImage* weak=button.image;NSData* weakPixels=renderedStatusButton(button);
    assert(strong!=medium&&medium!=weak&&strong!=weak);
    assert(![strongPixels isEqual:mediumPixels]&&![mediumPixels isEqual:weakPixels]&&
           ![strongPixels isEqual:weakPixels]); // variable symbols must render differently
    [app observeRSSI:-42 at:10 held:NO];[app observeRSSI:-42 at:12 held:NO];
    assert(button.image==strong); // returning to a level reuses its cached image
    assert([button.toolTip containsString:@"-42 dBm raw"]);
    assert([button.toolTip containsString:@"-42 dBm filtered"]);
    assert([button.toolTip containsString:@"3/3 bars"]);
    assert([button.accessibilityLabel isEqualToString:button.toolTip]);
    [app observeRSSI:-44 at:14 held:NO];
    assert(button.image==strong&&[button.toolTip containsString:@"-44 dBm raw"]);
    assert([button.toolTip containsString:@"-42 dBm filtered"]);
    assert([button.accessibilityLabel isEqualToString:button.toolTip]);
    [app observeRSSI:-44 at:16 held:YES];
    assert([button.toolTip containsString:@"-44 dBm last valid"]);
    assert(![button.toolTip containsString:@"dBm raw"]);
    [app delayStatusAt:18];
    assert(button.image==strong&&[button.toolTip containsString:@"last valid"]);
    [app delayStatusAt:23];
    NSImage* unknown=button.image;NSData* unknownPixels=renderedStatusButton(button);
    assert(unknown!=strong&&unknown!=medium&&unknown!=weak);
    assert(![unknownPixels isEqual:strongPixels]&&![unknownPixels isEqual:mediumPixels]&&
           ![unknownPixels isEqual:weakPixels]);
    assert(![button.toolTip containsString:@"dBm"]);
    assert([button.accessibilityLabel isEqualToString:button.toolTip]);
    [app observeRSSI:0 at:24 held:NO];
    assert(button.image==unknown&&[button.toolTip containsString:@"signal unavailable"]);
    assert(![button.toolTip containsString:@"0 dBm"]);
    [app observeRSSI:-42 at:26 held:NO];[app advanceTo:37];
    assert(button.image==unknown&&[button.toolTip containsString:@"signal unavailable"]);
    assert(![button.toolTip containsString:@"dBm"]);
    [app disconnectAt:38];
    assert(button.image!=unknown&&button.image!=strong);
    assert(![renderedStatusButton(button) isEqual:unknownPixels]);
    assert(![button.toolTip containsString:@"dBm"]);
    assert(button.image.isTemplate&&button.image.accessibilityDescription.length);
    [app finishSignalTest];
    puts("AppKit signal: distinct rasters, cached transitions, precise RSSI, held/unknown/expiry/disconnected passed");
}
// Model the driver boundary; menu construction, polling, eligibility and
// deduplication remain the production implementation. No real service/USB.
@interface BandDriverConnection : RTWDriverConnection
@property(nonatomic,assign) BOOL fiveGHz;
@property(nonatomic,assign) BOOL highGHz;
@property(nonatomic,assign) BOOL sharedSSID;
@property(nonatomic,assign) NSUInteger scanReads;
@property(nonatomic,assign) unsigned channelMapMode;
@end
@implementation BandDriverConnection
- (uint32_t)exchangeRegistryID:(uint64_t)registryID selector:(uint32_t)selector
                        input:(const void*)input length:(size_t)length
                       output:(void*)output capacity:(size_t)capacity {
    assert(capacity==rtwui::outputSize(selector));
    if(selector==rtwui::listSelector) {
        assert(!registryID&&!input&&!length);
        auto* list=static_cast<rtwui::DeviceList*>(output);*list={};
        list->version=rtwui::ipcVersion;list->count=1;list->devices[0].registryID=3003;
        return 0;
    }
    assert(registryID==3003&&rtw_control_validate_request(selector,input,length)==RTW_CONTROL_VALID);
    if(selector==RTW_CONTROL_CHANNEL_MAP) {
        if(!self.channelMapMode)return static_cast<uint32_t>(kIOReturnUnsupported);
        auto* map=static_cast<RTWControlChannelMap*>(output);*map={};map->version=1;
        const unsigned au[]={100,104,108,112,116,132,136,140},ru[]={132,136,140};
        if(self.channelMapMode==1)for(unsigned ch:au)map->channels[ch/8]|=1U<<(ch%8);
        else for(unsigned ch:ru)map->channels[ch/8]|=1U<<(ch%8);
        if(self.channelMapMode==3)map->reserved[0]=1;
        return 0;
    }
    if(selector==RTW_CONTROL_STATUS) {
        auto* status=static_cast<RTWControlStatus*>(output);*status={};status->version=1;
        status->flags=RTW_CONTROL_DEVICE|RTW_CONTROL_RADIO|
            (self.fiveGHz?RTW_CONTROL_BAND_5GHZ:0)|(self.highGHz?RTW_CONTROL_BAND_5GHZ_HIGH:0);return 0;
    }
    assert(selector==RTW_CONTROL_SCAN_RESULTS);self.scanReads+=1;
    auto* scan=static_cast<RTWControlScanResults*>(output);*scan={};scan->version=1;scan->count=3;
    const char* names[]={"FiveLab","HighLab","TwoLab"};
    const unsigned channels[]={self.channelMapMode?100U:36U,self.channelMapMode?120U:149U,self.channelMapMode?132U:6U};
    for(unsigned i=0;i<3;++i) {
        const char*name=self.sharedSSID?"SharedLab":names[i];
        auto& row=scan->records[i];row.ssidLength=std::strlen(name);
        std::memcpy(row.ssid,name,row.ssidLength);row.channel=channels[i];
        const int strengths[]={-70,-30,-90};
        row.rssi=self.sharedSSID?strengths[i]:-40;row.securityFlags=7;
    }
    return 0;
}
@end
static NSMenuItem* networkItem(NSMenu* menu,const char* ssid) {
    NSData* wanted=[NSData dataWithBytes:ssid length:std::strlen(ssid)];
    for(NSMenuItem* item in menu.itemArray)
        if(item.action==@selector(selectNetwork:)&&
            [item.representedObject isKindOfClass:NSDictionary.class]&&
            [item.representedObject[@"ssid"] isEqual:wanted])return item;
    return nil;
}
static void completeBandPoll(WiFiApp* app,bool scan=true) {
    [app setValue:@(scan) forKey:@"forceNetworkCache"];[app poll:nil];
    NSDate* deadline=[NSDate dateWithTimeIntervalSinceNow:5.0];
    while([[app valueForKey:@"busy"] boolValue]&&deadline.timeIntervalSinceNow>0)
        [NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
    assert(![[app valueForKey:@"busy"] boolValue]);[app rebuildMenu];
}
@interface MapReconnectWiFiApp : RecordingWiFiApp
@property(nonatomic,assign) NSUInteger automaticRequests;
- (void)prepareAutomaticSSID:(NSString*)ssid;
@end
@implementation MapReconnectWiFiApp
- (void)prepareAutomaticSSID:(NSString*)ssid {
    _preferredSSID=[ssid dataUsingEncoding:NSUTF8StringEncoding];_savedNetworks=@[_preferredSSID];
    _savedNetworksLoaded=YES;_autoJoinEnabled=YES;_autoJoin.paused=false;
    _lastLinkedAt=-1;_lastAutoScanAt=[self uptime];
}
- (void)loadPasswordForSSID:(NSData*)ssid connect:(BOOL)connect automatic:(BOOL)automatic {
    assert(ssid.length&&connect&&automatic);self.automaticRequests++;
}
@end
@interface RefreshDriverConnection : RTWDriverConnection
@property(nonatomic,assign) BOOL connected;
@property(nonatomic,assign) BOOL scanning;
@property(nonatomic,assign) NSUInteger scanPollsRemaining;
@property(nonatomic,assign) NSUInteger scanRequests;
@property(nonatomic,assign) NSUInteger resultReads;
@property(nonatomic,assign) NSUInteger leaves;
@property(nonatomic,assign) uint32_t scanError;
@end
@implementation RefreshDriverConnection
- (uint32_t)exchangeRegistryID:(uint64_t)registryID selector:(uint32_t)selector
                        input:(const void*)input length:(size_t)length
                       output:(void*)output capacity:(size_t)capacity {
    assert(capacity==rtwui::outputSize(selector));
    if(selector==rtwui::listSelector) {
        assert(!registryID&&!input&&!length);
        auto* list=static_cast<rtwui::DeviceList*>(output);*list={};
        list->version=rtwui::ipcVersion;list->count=2;
        list->devices[0].registryID=3003;list->devices[1].registryID=4004;return 0;
    }
    assert((registryID==3003||registryID==4004)&&
        rtw_control_validate_request(selector,input,length)==RTW_CONTROL_VALID);
    if(selector==RTW_CONTROL_CHANNEL_MAP)return static_cast<uint32_t>(kIOReturnUnsupported);
    if(selector==RTW_CONTROL_SCAN) {
        ++self.scanRequests;
        if(self.scanError)return self.scanError;
        self.scanPollsRemaining=1;
    } else if(selector==RTW_CONTROL_LEAVE) {
        ++self.leaves;self.connected=NO;self.scanPollsRemaining=0;
    } else if(selector==RTW_CONTROL_STATUS) {
        self.scanning=self.scanPollsRemaining>0;
        if(self.scanning)--self.scanPollsRemaining;
        auto* status=static_cast<RTWControlStatus*>(output);*status={};status->version=1;
        status->flags=RTW_CONTROL_DEVICE|RTW_CONTROL_RADIO|
            (self.scanning?RTW_CONTROL_SCANNING:self.connected?
                RTW_CONTROL_RUNNING|RTW_CONTROL_AUTHORIZED:0);
        status->state=self.scanning?1:self.connected?4:0;
        if(self.connected&&!self.scanning) {
            status->channel=6;status->rssi=-42;status->ssidLength=3;
            std::memcpy(status->ssid,"Lab",3);
        }
        return 0;
    } else if(selector==RTW_CONTROL_SCAN_RESULTS) {
        ++self.resultReads;
        auto* scan=static_cast<RTWControlScanResults*>(output);*scan={};scan->version=1;scan->count=1;
        auto& row=scan->records[0];
        const char* name=self.scanRequests&&!self.scanning?"FreshLab":"CachedLab";
        row.ssidLength=std::strlen(name);std::memcpy(row.ssid,name,row.ssidLength);
        row.channel=6;row.rssi=-48;row.securityFlags=7;return 0;
    } else assert(false&&"Manual refresh must not send JOIN or other commands");
    auto* reply=static_cast<RTWControlReply*>(output);*reply={};reply->version=1;return 0;
}
@end
@interface RefreshWiFiApp : WiFiApp
- (void)prepareReconnectTest;
- (BOOL)reconnectPaused;
@end
@implementation RefreshWiFiApp
- (void)prepareReconnectTest {
    _autoJoinEnabled=YES;_autoJoin.paused=false;_savedNetworksLoaded=YES;
    _preferredSSID=[@"Lab" dataUsingEncoding:NSUTF8StringEncoding];_savedNetworks=@[_preferredSSID];
}
- (BOOL)reconnectPaused {return _autoJoin.paused;}
- (id<RTWCredentialStoring>)credentials {
    assert(false&&"Refreshing networks must not access the Keychain");return nil;
}
@end
static void drainControlWork(WiFiApp* app) {
    NSDate* deadline=[NSDate dateWithTimeIntervalSinceNow:5.0];
    while(([[app valueForKey:@"busy"] boolValue]||
           [[app valueForKey:@"commandPending"] boolValue])&&deadline.timeIntervalSinceNow>0)
        [NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
    assert(![[app valueForKey:@"busy"] boolValue]&&
           ![[app valueForKey:@"commandPending"] boolValue]);
}
static RefreshWiFiApp* refreshTestApp(RefreshDriverConnection* driver,NSUserDefaults* preferences) {
    RefreshWiFiApp* app=[RefreshWiFiApp new];[app prepareReconnectTest];
    [app setValue:preferences forKey:@"preferences"];
    [app setValue:menuForControl() forKey:@"menu"];
    [app setValue:@YES forKey:@"authorizedSession"];
    rtwui::DeviceList list{};list.version=rtwui::ipcVersion;list.count=2;
    list.devices[0].registryID=3003;list.devices[1].registryID=4004;
    [app applyAdapters:list];[app useAdapter:3003];
    [app setValue:driver forKey:@"driver"];
    [app setValue:serialWorkQueue("org.research.TahoeUSBWiFi.test.refresh") forKey:@"queue"];
    [app poll:nil];drainControlWork(app);[app rebuildMenu];return app;
}
static void testManualNetworkRefresh() {
    NSString* suite=[@"org.research.TahoeUSBWiFi.test.refresh." stringByAppendingString:NSUUID.UUID.UUIDString];
    NSUserDefaults* preferences=[[NSUserDefaults alloc]initWithSuiteName:suite];
    NSData* savedSSID=[@"Lab" dataUsingEncoding:NSUTF8StringEncoding];
    [preferences setBool:YES forKey:@"RTWAutoJoinEnabled"];
    [preferences setBool:NO forKey:@"RTWAutoJoinPaused"];
    [preferences setObject:savedSSID forKey:@"RTWLastConnectedSSID"];
    NSDictionary* originalPreferences=[preferences persistentDomainForName:suite];
    RefreshDriverConnection* driver=[RefreshDriverConnection new];driver.connected=YES;
    RefreshWiFiApp* app=refreshTestApp(driver,preferences);
    NSMenu* menu=[app valueForKey:@"menu"];
    assert(networkItem(menu,"CachedLab")&&driver.scanRequests==0);
    const auto generation=[app adapterGeneration];
    // A timer poll already queued when the user clicks must neither consume
    // the request nor mistake its older connected status for scan completion.
    dispatch_queue_t controlQueue=[app valueForKey:@"queue"];
    dispatch_semaphore_t pollGate=dispatch_semaphore_create(0);
    dispatch_async(controlQueue,^{assert(!dispatch_semaphore_wait(pollGate,dispatch_time(DISPATCH_TIME_NOW,5LL*NSEC_PER_SEC)));});
    [app poll:nil];[app refresh:nil];[app refresh:nil];
    dispatch_semaphore_signal(pollGate);drainControlWork(app);
    assert(driver.scanRequests==1); // connected manual action must request an RF scan
    assert([app adapterGeneration]==generation&&[app selectedAdapterID]==3003);
    [app rebuildMenu];
    assert([menu itemWithTitle:@"Refreshing networks…"]&&![[menu itemWithTitle:@"Refreshing networks…"] isEnabled]);
    assert([[app valueForKey:@"notice"] containsString:@"Refreshing networks"]);
    const auto duringScanReads=driver.resultReads;
    [app refresh:nil];drainControlWork(app);assert(driver.scanRequests==1);
    [app poll:nil];drainControlWork(app);[app rebuildMenu];
    assert(driver.resultReads>duringScanReads&&networkItem(menu,"FreshLab"));
    assert([menu itemWithTitle:@"Refresh networks"].enabled);
    assert(![app reconnectPaused]&&driver.leaves==0);
    assert([[preferences persistentDomainForName:suite] isEqual:originalPreferences]);
    [app poll:nil];drainControlWork(app);assert(driver.scanRequests==1); // background reads stay read-only
    [app setValue:@YES forKey:@"joining"];[app refresh:nil];drainControlWork(app);
    assert(driver.scanRequests==1);[app setValue:@NO forKey:@"joining"];
    [app disconnect:nil];drainControlWork(app);
    assert(driver.leaves==1&&[app reconnectPaused]&&[preferences boolForKey:@"RTWAutoJoinPaused"]);
    [app refresh:nil];drainControlWork(app);[app poll:nil];drainControlWork(app);
    assert(driver.scanRequests==2&&driver.leaves==1&&[app reconnectPaused]);
    assert([preferences boolForKey:@"RTWAutoJoinEnabled"]&&
        [[preferences dataForKey:@"RTWLastConnectedSSID"] isEqual:savedSSID]);
    // A command queued for a previous adapter generation must not reach USB.
    RefreshDriverConnection* switchedDriver=[RefreshDriverConnection new];switchedDriver.connected=YES;
    RefreshWiFiApp* switched=refreshTestApp(switchedDriver,preferences);
    dispatch_queue_t queue=[switched valueForKey:@"queue"];
    dispatch_semaphore_t gate=dispatch_semaphore_create(0);
    dispatch_async(queue,^{assert(!dispatch_semaphore_wait(gate,dispatch_time(DISPATCH_TIME_NOW,5LL*NSEC_PER_SEC)));});
    [switched refresh:nil];[switched useAdapter:4004];dispatch_semaphore_signal(gate);
    drainControlWork(switched);[switched rebuildMenu];
    assert(switchedDriver.scanRequests==0&&[switched selectedAdapterID]==4004);
    assert([[switched valueForKey:@"menu"] itemWithTitle:@"Refresh networks"].enabled);
    // A rejected scan must release the refresh action for a retry.
    driver.scanError=uint32_t(kIOReturnBusy);[app refresh:nil];drainControlWork(app);[app rebuildMenu];
    assert([menu itemWithTitle:@"Refresh networks"].enabled);
    assert([[app valueForKey:@"notice"] containsString:@"error"]);
    [preferences removePersistentDomainForName:suite];
    puts("AppKit manual scan: connected refresh, completion results, duplicate/join/hotplug guards, preferences and disconnect semantics passed");
}
static NSModalResponse switchingSSIDModal(id alert,SEL) {
    NSTextField* field=(NSTextField*)[alert accessoryView];field.stringValue=@"HiddenLab";
    [modalTarget useAdapter:2002];return NSAlertFirstButtonReturn;
}
int main() {
    @autoreleasepool {
        [NSApplication sharedApplication];
        testSignalAppearance();
        testManualNetworkRefresh();
        LifecycleWiFiApp* life=[LifecycleWiFiApp new];
        assert([[life valueForKey:@"instanceLock"] intValue]==-1);
        [life workspaceWillSleep:nil];[life workspaceBecameInactive:nil];
        [life workspaceBecameActive:nil];
        assert([[life valueForKey:@"sleeping"] boolValue] && life.polls==0);
        [life workspaceDidWake:nil];
        assert(![[life valueForKey:@"sleeping"] boolValue] && life.polls==1);
        [life workspaceBecameInactive:nil];[life workspaceDidWake:nil];
        assert([[life valueForKey:@"inactiveSession"] boolValue] && life.polls==1);
        [life workspaceBecameActive:nil];assert(life.polls==2 && life.credentialReloads==2);
        NSString* longTitle=[@"Adapter " stringByPaddingToLength:240 withString:@"name " startingAtIndex:0];
        assert(boundedMenuTitle(longTitle).length<=90);
        assert([boundedMenuTitle(@"Line\nOne") rangeOfCharacterFromSet:NSCharacterSet.newlineCharacterSet].location==NSNotFound);
        NSMenu* menu=menuForControl();
        WiFiApp* app=[WiFiApp new];[app setValue:menu forKey:@"menu"];
        NSMenuItem* scan=[app add:@"Refresh networks" action:@selector(refresh:) enabled:NO];
        [menu update];assert(!scan.enabled);
        NSMenuItem* unsupported=[app add:@"Unsupported network" action:@selector(selectNetwork:) enabled:NO];
        [menu update];assert(!unsupported.enabled);
        NSMenuItem* enabled=[app add:@"Authorize access" action:@selector(authorize:) enabled:YES];
        [menu update];assert(enabled.enabled);
        [app setValue:@YES forKey:@"authorizedSession"];
        rtwui::DeviceList adapters{};adapters.version=2;adapters.count=2;
        adapters.devices[0].registryID=1001;strcpy(adapters.devices[0].family,"RTL8192EU");
        adapters.devices[1].registryID=2002;strcpy(adapters.devices[1].family,"RTL8814AU");
        [app applyAdapters:adapters];assert([app selectedAdapterID]==0);
        [app useAdapter:1001];const auto first=[app adapterGeneration];
        [app setValue:@YES forKey:@"hasStatus"];[app setValue:@YES forKey:@"joining"];
        [app useAdapter:2002];assert([app selectedAdapterID]==2002&&[app adapterGeneration]>first);
        assert(![[app valueForKey:@"hasStatus"] boolValue]&&![[app valueForKey:@"joining"] boolValue]);
        [app rebuildMenu];NSMenuItem* chooser=[menu itemWithTitle:@"USB adapter"];
        assert(chooser.submenu.numberOfItems==2);
        assert([chooser.submenu itemAtIndex:1].state==NSControlStateValueOn);
        const auto beforeUnplug=[app adapterGeneration];adapters.count=1;
        [app applyAdapters:adapters];assert([app selectedAdapterID]==2002&&[app adapterGeneration]>beforeUnplug);
        assert(![[app valueForKey:@"selectedPresent"] boolValue]);
        [app applyAdapters:adapters];assert([app selectedAdapterID]==2002);
        assert(![[app valueForKey:@"selectedPresent"] boolValue]);
        // Reappearance of the same selected Registry ID must restore a valid
        // generation; otherwise every subsequent STATUS poll is discarded.
        const auto interrupted=[app adapterGeneration];adapters.count=2;
        [app applyAdapters:adapters];
        assert([app selectedAdapterID]==2002&&[app adapterGeneration]>interrupted);
        assert([[app valueForKey:@"selectedPresent"] boolValue]);
        // Repeated polls must not switch to a different remaining adapter.
        [app useAdapter:1001];assert([app selectedAdapterID]==1001);
        NSMenuItem* stale=[[NSMenuItem alloc]initWithTitle:@"Old network" action:@selector(selectNetwork:) keyEquivalent:@""];
        stale.tag=1;stale.representedObject=@{@"ssid":[NSData dataWithBytes:"Lab" length:3],@"registryID":@2002,@"generation":@(first)};
        [app selectNetwork:stale];assert(![[app valueForKey:@"joining"] boolValue]);
        adapters.count=1;
        WiFiApp* single=[WiFiApp new];[single applyAdapters:adapters];assert([single selectedAdapterID]==1001);
        adapters.devices[0].registryID=3003;[single applyAdapters:adapters];assert([single selectedAdapterID]==3003);
        RecordingWiFiApp* dialog=[RecordingWiFiApp new];modalTarget=dialog;
        [dialog setValue:menuForControl() forKey:@"menu"];[dialog setValue:@YES forKey:@"authorizedSession"];
        adapters.count=2;adapters.devices[0].registryID=1001;adapters.devices[1].registryID=2002;
        [dialog applyAdapters:adapters];[dialog useAdapter:1001];
        Method method=class_getInstanceMethod(NSAlert.class,@selector(runModal));
#pragma clang diagnostic push
#if __has_warning("-Wcast-function-type-mismatch")
#pragma clang diagnostic ignored "-Wcast-function-type-mismatch"
#endif
#if __has_warning("-Wcast-function-type-strict")
#pragma clang diagnostic ignored "-Wcast-function-type-strict"
#endif
        IMP original=method_setImplementation(method,reinterpret_cast<IMP>(switchingSSIDModal));
#pragma clang diagnostic pop
        [dialog otherNetwork:nil];method_setImplementation(method,original);modalTarget=nil;
        assert([dialog selectedAdapterID]==2002&&modalConnections==0);
        // Login option always reflects the system backend, never UserDefaults.
        NSString* suite=[@"org.research.TahoeUSBWiFi.test." stringByAppendingString:NSUUID.UUID.UUIDString];
        NSUserDefaults* preferences=[[NSUserDefaults alloc]initWithSuiteName:suite];
        NoModalWiFiApp* loginApp=[NoModalWiFiApp new];
        [loginApp setValue:preferences forKey:@"preferences"];
        NSMenu* loginMenu=menuForControl();[loginApp setValue:loginMenu forKey:@"menu"];
        FakeLoginItems* loginBackend=[FakeLoginItems new];
        loginBackend.state=RTWLoginItemStateNotRegistered;
        [loginApp setValue:loginBackend forKey:@"loginItems"];
        [loginApp rebuildMenu];
        NSMenuItem* toggle=[loginMenu itemWithTitle:@"Open at login"];
        assert(toggle && toggle.enabled && toggle.state==NSControlStateValueOff);
        [loginApp toggleLoginAtStartup:nil];
        assert(loginBackend.state==RTWLoginItemStateEnabled);
        // Updating the app state alone must NOT reconstruct an open menu.
        assert([loginMenu itemWithTitle:@"Open at login"]==toggle);
        [loginApp rebuildMenu];
        toggle=[loginMenu itemWithTitle:@"Open at login"];
        assert(toggle.state==NSControlStateValueOn);
        // A menu action must update its current checkmark without rebuilding
        // the tracking menu and only after macOS confirms the new state.
        [loginApp setValue:@YES forKey:@"menuOpen"];
        [loginApp toggleLoginAtStartup:toggle];
        assert(toggle.state==NSControlStateValueOff);
        assert([loginMenu itemWithTitle:@"Open at login"]==toggle);
        [loginApp setValue:@NO forKey:@"menuOpen"];
        [loginApp toggleLoginAtStartup:toggle];
        assert(toggle.state==NSControlStateValueOn);
        loginBackend.state=RTWLoginItemStateRequiresApproval;[loginApp rebuildMenu];
        toggle=[loginMenu itemWithTitle:@"Open at login"];
        assert(toggle.state==NSControlStateValueMixed);
        assert([loginMenu itemWithTitle:@"Allow startup in System Settings…"]);
        [loginApp openLoginSettings:nil];assert(loginBackend.settingsOpens==1);
        [loginApp toggleLoginAtStartup:nil];
        assert(loginBackend.state==RTWLoginItemStateRequiresApproval && loginBackend.settingsOpens==2);
        [loginApp disableLoginAtStartup:nil];
        assert(loginBackend.state==RTWLoginItemStateNotRegistered);
        // External updates must be reflected at next menu rebuild.
        loginBackend.state=RTWLoginItemStateEnabled;[loginApp rebuildMenu];
        assert([loginMenu itemWithTitle:@"Open at login"].state==NSControlStateValueOn);
        loginBackend.state=RTWLoginItemStateNotRegistered;
        loginBackend.rejectChanges=YES;[loginApp toggleLoginAtStartup:nil];
        [loginApp rebuildMenu];
        assert(loginBackend.state==RTWLoginItemStateNotRegistered && loginApp.alerts==1);
        assert([loginMenu itemWithTitle:@"Open at login"].state==NSControlStateValueOff);
        loginBackend.state=RTWLoginItemStateNotFound;[loginApp rebuildMenu];
        assert([loginMenu itemWithTitle:@"Open at login"].enabled);
        loginBackend.rejectChanges=NO;
        [loginApp toggleLoginAtStartup:nil];
        assert(loginBackend.state==RTWLoginItemStateEnabled);
        [loginApp rebuildMenu];
        assert([loginMenu itemWithTitle:@"Save a network password…"].enabled);
        assert([loginMenu itemWithTitle:@"Saved passwords…"].enabled);
        [loginApp setValue:@YES forKey:@"credentialBusy"];[loginApp rebuildMenu];
        assert(![loginMenu itemWithTitle:@"Save a network password…"].enabled);
        assert(![loginMenu itemWithTitle:@"Saved passwords…"].enabled);
        [preferences removePersistentDomainForName:suite];
        RecordingWiFiApp* bands=[RecordingWiFiApp new];NSMenu* bandMenu=menuForControl();
        [bands setValue:bandMenu forKey:@"menu"];
        [bands setValue:@YES forKey:@"authorizedSession"];
        [bands setValue:@YES forKey:@"savedNetworksLoaded"];
        adapters.count=1;adapters.devices[0].registryID=3003;[bands applyAdapters:adapters];
        BandDriverConnection* bandDriver=[BandDriverConnection new];
        [bands setValue:bandDriver forKey:@"driver"];
        [bands setValue:serialWorkQueue("org.research.TahoeUSBWiFi.test.bands") forKey:@"queue"];
        completeBandPoll(bands);
        assert(!networkItem(bandMenu,"FiveLab").enabled&&networkItem(bandMenu,"TwoLab").enabled);
        bandDriver.fiveGHz=YES;completeBandPoll(bands);
        NSMenuItem* fiveItem=networkItem(bandMenu,"FiveLab");
        assert(fiveItem.enabled&&fiveItem.tag==1);
        assert([fiveItem.title containsString:@"5 GHz"]&&[fiveItem.title containsString:@"36"]);
        assert([fiveItem.toolTip containsString:@"5180 MHz"]);
        assert(!networkItem(bandMenu,"HighLab").enabled&&networkItem(bandMenu,"TwoLab").enabled);
        bandDriver.fiveGHz=NO;bandDriver.highGHz=YES;completeBandPoll(bands);
        NSMenuItem* highItem=networkItem(bandMenu,"HighLab");
        assert(highItem.enabled&&!networkItem(bandMenu,"FiveLab").enabled);
        assert([highItem.title containsString:@"149"]&&[highItem.toolTip containsString:@"5745 MHz"]);
        [bands about:nil];
        assert([bands.shownDetail containsString:@"Selected driver channels: 2.4 GHz (channels 1–11) or 5 GHz (149/153/157/161/165)."]);
        bandDriver.highGHz=NO;bandDriver.fiveGHz=YES;completeBandPoll(bands);
        const auto beforeSelection=modalConnections;
        [bands selectNetwork:fiveItem];assert(modalConnections==beforeSelection+1);
        // A capability change or a switch to a 2.4-GHz controller cannot keep
        // an old 5-GHz entry enabled from the previous cached scan.
        // A tracking menu cannot be rebuilt; its original action must also
        // recheck the current driver's channel permission before connecting.
        [bands setValue:@YES forKey:@"menuOpen"];
        bandDriver.fiveGHz=NO;completeBandPoll(bands);
        [bands selectNetwork:fiveItem];assert(modalConnections==beforeSelection+1);
        [bands setValue:@NO forKey:@"menuOpen"];[bands rebuildMenu];
        // A status-only bit8 change must reselect the cached eligible BSS.
        RecordingWiFiApp* shared=[RecordingWiFiApp new];NSMenu* sharedMenu=menuForControl();
        [shared setValue:sharedMenu forKey:@"menu"];[shared setValue:@YES forKey:@"authorizedSession"];
        [shared setValue:@YES forKey:@"savedNetworksLoaded"];[shared applyAdapters:adapters];
        BandDriverConnection* sharedDriver=[BandDriverConnection new];sharedDriver.sharedSSID=YES;sharedDriver.fiveGHz=YES;
        [shared setValue:sharedDriver forKey:@"driver"];
        [shared setValue:serialWorkQueue("org.research.TahoeUSBWiFi.test.upper-band") forKey:@"queue"];
        completeBandPoll(shared);
        assert([networkItem(sharedMenu,"SharedLab").representedObject[@"channel"] unsignedIntValue]==36);
        const auto scans=sharedDriver.scanReads;sharedDriver.highGHz=YES;completeBandPoll(shared,false);
        assert(sharedDriver.scanReads==scans);
        assert([networkItem(sharedMenu,"SharedLab").representedObject[@"channel"] unsignedIntValue]==149);
        NSMenuItem* staleHigh=networkItem(sharedMenu,"SharedLab");const auto attempts=modalConnections;
        [shared setValue:@YES forKey:@"menuOpen"];sharedDriver.highGHz=NO;completeBandPoll(shared,false);
        [shared selectNetwork:staleHigh];assert(modalConnections==attempts);

        // Actual Main poll applies the exact map even while full-group flags
        // remain absent. A map-only change also invalidates an open menu action
        // and preferred/sole-network automatic reconnect eligibility.
        MapReconnectWiFiApp* mapped=[MapReconnectWiFiApp new];NSMenu* mappedMenu=menuForControl();
        [mapped setValue:mappedMenu forKey:@"menu"];[mapped setValue:@YES forKey:@"authorizedSession"];
        [mapped setValue:@YES forKey:@"savedNetworksLoaded"];[mapped applyAdapters:adapters];
        BandDriverConnection* mappedDriver=[BandDriverConnection new];mappedDriver.channelMapMode=1;
        [mapped setValue:mappedDriver forKey:@"driver"];
        [mapped setValue:serialWorkQueue("org.research.TahoeUSBWiFi.test.exact-map") forKey:@"queue"];
        completeBandPoll(mapped);
        NSMenuItem* allowedAU=networkItem(mappedMenu,"FiveLab");
        assert(allowedAU.enabled&&!networkItem(mappedMenu,"HighLab").enabled&&networkItem(mappedMenu,"TwoLab").enabled);
        [mapped about:nil];assert([mapped.shownDetail containsString:@"100/104/108/112/116/132/136/140"]);
        [mapped prepareAutomaticSSID:@"FiveLab"];[mapped considerAutoJoin];assert(mapped.automaticRequests==1);
        mappedDriver.channelMapMode=2;completeBandPoll(mapped,false);
        assert(!networkItem(mappedMenu,"FiveLab").enabled&&networkItem(mappedMenu,"TwoLab").enabled);
        assert(mapped.automaticRequests==1);
        const auto mapAttempts=modalConnections;[mapped selectNetwork:allowedAU];assert(modalConnections==mapAttempts);
        [mapped about:nil];assert([mapped.shownDetail containsString:@"132/136/140"]);
        mappedDriver.channelMapMode=3;completeBandPoll(mapped,false);
        assert(!networkItem(mappedMenu,"FiveLab").enabled&&!networkItem(mappedMenu,"TwoLab").enabled);

        assert(!networkItem(bandMenu,"FiveLab").enabled&&networkItem(bandMenu,"TwoLab").enabled);
        puts("AppKit menu/adapter selection/login item: passed");
    }
}
